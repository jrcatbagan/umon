/**************************************************************************
 *
 * Copyright (c) 2013 Alcatel-Lucent
 *
 * Alcatel Lucent licenses this file to You under the Apache License,
 * Version 2.0 (the "License"); you may not use this file except in
 * compliance with the License.  A copy of the License is contained the
 * file LICENSE at the top level of this repository.
 * You may also obtain a copy of the License at:
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 **************************************************************************
 *
 * icmp.c:
 *
 * Support some basic ICMP stuff.
 *
 * Original author:     Ed Sutter (ed.sutter@alcatel-lucent.com)
 *
 */
#include "config.h"

#if INCLUDE_ETHERNET
#include "endian.h"
#include "genlib.h"
#include "ether.h"
#include "stddefs.h"
#include "cli.h"
#include "timer.h"

#if INCLUDE_ICMP

/* To reduce uMon footprint size, INCLUDE_ICMPTIME can be set to 0 in
 * config.h if necessary...
 */
#ifndef INCLUDE_ICMPTIME
#define INCLUDE_ICMPTIME 1
#endif

#define MSECS_PER_HOUR 3600000
#define MSECS_PER_MINUTE 60000
#define MSECS_PER_SECOND 1000

#define ICMP_TIME_ID        0x1111      /* made these up. */
#define ICMP_ECHO_ID        0x2222
#define ICMP_ECHO_DATASIZE  26

#if INCLUDE_ICMPTIME
static ulong    ICMPTimeStamp;
#endif

static int      CheckSrvrResolution;
ushort          ICMPEchoReplyId, ICMPSeqNoRcvd;
int             SendICMPRequest(uchar,uchar *,uchar *,ulong,ushort);

int SendEchoResp(struct ether_header *);

/* Destination unreachable codes: (3rd Edition Comer pg 129) */
char    *IcmpDestUnreachableCode[] = {
    "Network unreachable",
    "Host unreachable",
    "Protocol unreachable",
    "Port unreachable",
    "Fragmentation needed and DF set",
    "Source route failed",
    "???",
};

char *IcmpHelp[] = {
    "ICMP interface",
    "-[c:d:f:rs:t:v:] {operation} [args]",
#if INCLUDE_VERBOSEHELP
    "Options...",
    " -c {#}    echo count",
    " -s {#}    size of icmp-echo packet",
    " -t {#}    timeout (msec) to wait for response (default=2000)",
    " -v {var}  load result in shellvar 'var'",
#if INCLUDE_ICMPTIME
    " -d {#}    delta (hours) relative to GMT",
    " -f {x|d}  hex or decimal output (default is ascii)",
    " -r        check server resolution",
    "Operations...",
    " time {IP}",
    " echo {IP}",
    "Notes...",
    " If '.ns' is appended to time, then time is non-standard.",
    " The 'dfr' options are used by time, options 'cs' are used by echo.",
#endif
#endif
    0,
};

int
Icmp(int argc,char *argv[])
{
    ushort  seqno;
    long    size;
    struct  elapsed_tmr tmr;
    int     opt, count, timeout;
    uchar   binip[8], binenet[8];
    char    *varname, *operation, *ipadd;
#if INCLUDE_ICMPTIME
    uchar   buf[32];
    char    *nonstandard = "", format='a';
    int     timestamp, hour, min, sec, gmt_delta=0;
#endif

    size = 0;
    count = 1;
    timeout = 2000;
    varname = (char *)0;
    CheckSrvrResolution = 0;

    while((opt=getopt(argc,argv,"c:d:f:rs:t:v:")) != -1) {
        switch(opt) {
        case 'c':   /* count used by ping */
            count = strtol(optarg,(char **)0,0);
            break;
#if INCLUDE_ICMPTIME
        case 'd':   /* difference between this time zone and Greenwich */
            gmt_delta = strtol(optarg,(char **)0,0);
            break;
        case 'f':
            format = *optarg;
            break;
        case 'r':
            CheckSrvrResolution = 1;
            break;
#endif
        case 's':
            size = strtol(optarg,(char **)0,0);
            break;
        case 't':
            timeout = strtol(optarg,(char **)0,0);
            break;
        case 'v':
            varname = optarg;
            break;
        default:
            return(CMD_PARAM_ERROR);
        }
    }

    if(argc == optind) {
        return(CMD_PARAM_ERROR);
    }

    operation = argv[optind];
    ipadd = argv[optind+1];

    /* If time or echo, do the common up-front stuff here... */
    if(!strcmp(operation,"time") || !strcmp(operation,"echo")) {
        if(argc != optind + 2) {
            return(CMD_PARAM_ERROR);
        }

        /* Convert IP address to binary: */
        if(IpToBin(ipadd,binip) < 0) {
            return(CMD_SUCCESS);
        }
    }

#if INCLUDE_ICMPTIME
    if(!strcmp(operation, "time")) {

        /* Get the ethernet address for the IP: */
        if(!ArpEther(binip,binenet,0)) {
            printf("ARP failed for %s\n",ipadd);
            return(CMD_FAILURE);
        }

        ICMPTimeStamp = INVALID_TIMESTAMP;
        SendICMPRequest(ICMP_TIMEREQUEST,binip,binenet,0,0);

        /* Limit the amount of time waiting for a response... */
        startElapsedTimer(&tmr,timeout);
        while(msecElapsed(&tmr)) {
            pollethernet();
            if(ICMPTimeStamp != INVALID_TIMESTAMP) {
                break;
            }
        }
        if(ELAPSED_TIMEOUT(&tmr)) {
            printf("No response\n");
            return(CMD_FAILURE);
        }

        if(ICMPTimeStamp & NONSTANDARD_TIMESTAMP) {
            ICMPTimeStamp &= ~NONSTANDARD_TIMESTAMP;
            nonstandard = ".ns";
        }

        if(format == 'a') {
            timestamp = ICMPTimeStamp + (gmt_delta*MSECS_PER_HOUR);
            hour = timestamp/MSECS_PER_HOUR;
            timestamp -= hour*MSECS_PER_HOUR;
            min = timestamp/MSECS_PER_MINUTE;
            timestamp -= min*MSECS_PER_MINUTE;
            sec = timestamp/MSECS_PER_SECOND;
            timestamp -= sec*MSECS_PER_SECOND;
            sprintf((char *)buf,"%02d:%02d:%02d.%03d%s",hour,min,sec,timestamp,
                    nonstandard);
        } else if(format == 'x') {
            sprintf((char *)buf,"0x%lx%s",ICMPTimeStamp,nonstandard);
        } else if(format == 'd') {
            sprintf((char *)buf,"%ld%s",ICMPTimeStamp,nonstandard);
        } else {
            return(CMD_PARAM_ERROR);
        }
        if(varname) {
            setenv(varname,(char *)buf);
        } else if(!CheckSrvrResolution) {
            printf("%s\n",buf);
        }
        CheckSrvrResolution = 0;
    } else
#endif
        if(!strcmp(operation, "echo")) {
            seqno = 0;
            while(count-- > 0) {

                /* Is this a self-ping? */
                if(memcmp((char *)binip,(char *)BinIpAddr,4) == 0) {
                    printf("Yes, I am alive!\n");
                    break;
                }
                /* Get the ethernet address for the IP: */
                if(!ArpEther(binip,binenet,0)) {
                    printf("ARP failed for %s\n",ipadd);
                    if(varname) {
                        setenv(varname,"NOANSWER");
                    }
                    continue;
                }

                ICMPEchoReplyId = 0;
                SendICMPRequest(ICMP_ECHOREQUEST,binip,binenet,size,seqno);

                /* Limit the amount of time waiting for a response... */
                startElapsedTimer(&tmr,timeout);
                while(!msecElapsed(&tmr)) {
                    pollethernet();
                    if(ICMPEchoReplyId == ICMP_ECHO_ID) {
                        break;
                    }
                }

                if(ELAPSED_TIMEOUT(&tmr)) {
                    printf("no answer from %s\n",ipadd);
                    if(varname) {
                        setenv(varname,"NOANSWER");
                    }
                } else {
                    printf("%s is alive",ipadd);
                    if(count) {
                        printf(" icmp_seq=%d",ICMPSeqNoRcvd);
                        startElapsedTimer(&tmr,timeout);
                        while(!msecElapsed(&tmr)) {
                            pollethernet();
                        }
                        seqno++;
                    }
                    if(varname) {
                        setenv(varname,"ALIVE");
                    }
                    printf("\n");
                }
            }
        } else {
            printf("Unrecognized ICMP op: %s\n",operation);
        }
    return(CMD_SUCCESS);
}

int
processICMP(struct ether_header *ehdr,ushort size)
{
    struct  ip *ipp;
    struct  icmp_hdr *icmpp;
    int i;

    ipp = (struct ip *)(ehdr + 1);

    icmpp = (struct icmp_hdr *)((char *)ipp+IP_HLEN(ipp));
    if(icmpp->type == ICMP_ECHOREQUEST) {       /* 3rdEd Comer pg 127 */
#if INCLUDE_RARPIPASSIGN
        /* If we're here and our IP address is 0.0.0.0, then the
         * incoming packet's destination MAC address must have matched
         * ours, so we assume (kinda dangerous) that we should assign
         * this IP address to ourself...
         * This capability allows a board to come up with a MAC address
         * assigned, and IP set to 0.0.0.0, then the host can do
         *  arp -a IPADD MAC
         *  ping IPADD
         * to assing IPADD to this board.
         */
        if(DHCPState == DHCPSTATE_NOTUSED) {
            if((BinIpAddr[0] == 0) && (BinIpAddr[1] == 0) &&
                    (BinIpAddr[2] == 0) && (BinIpAddr[3] == 0)) {
                memcpy((char *)BinIpAddr,(char *)&(ipp->ip_dst),4);
                printf("RARP IP Assignment: %d.%d.%d.%d\n",
                       BinIpAddr[0], BinIpAddr[1],
                       BinIpAddr[2], BinIpAddr[3]);
                shell_sprintf("IPADD","%d.%d.%d.%d",
                              BinIpAddr[0], BinIpAddr[1],
                              BinIpAddr[2], BinIpAddr[3]);
            }
        }
#endif
        SendEchoResp(ehdr);
        return(0);
    } else if(icmpp->type == ICMP_ECHOREPLY) {
        struct icmp_echo_hdr *icmpecho;

        icmpecho = (struct icmp_echo_hdr *)icmpp;
        ICMPEchoReplyId = ecs(icmpecho->id);
        ICMPSeqNoRcvd = ecs(icmpecho->seq);
    } else if(icmpp->type == ICMP_DESTUNREACHABLE) { /* 3rdEd Comer pg 129 */
#if INCLUDE_ETHERVERBOSE
        if(EtherVerbose & SHOW_INCOMING) {
            i = icmpp->code;
            if((i > 5) || (i < 0)) {
                i = 6;
            }
            printf("  ICMP: %s (code=%d)\n",
                   IcmpDestUnreachableCode[i],icmpp->code);
        }
#endif
        return(0);
    }
#if INCLUDE_ICMPTIME
    else if(icmpp->type == ICMP_TIMEREPLY) {
        struct icmp_time_hdr *icmptime;
        ulong orig, recv, xmit;

        icmptime = (struct icmp_time_hdr *)icmpp;
        memcpy((char *)&orig,(char *)&icmptime->orig,4);
        memcpy((char *)&recv,(char *)&icmptime->recv,4);
        memcpy((char *)&xmit,(char *)&icmptime->xmit,4);

        ICMPSeqNoRcvd = icmptime->seq;
#if INCLUDE_ETHERVERBOSE
        if(EtherVerbose & SHOW_INCOMING) {
            printf("    ICMP_TIMEREPLY: orig=0x%lx,recv=0x%lx,xmit=0x%lx\n",
                   orig,recv,xmit);
        }
#endif
        if(CheckSrvrResolution) {
            static int ICMPTimeStampTbl[20];

            if(icmptime->seq < 20) {
                ICMPTimeStampTbl[icmptime->seq] = xmit;
                SendICMPRequest(ICMP_TIMEREQUEST,(uchar *)&ipp->ip_src,
                                (uchar *)&ehdr->ether_shost,0,icmptime->seq+1);
            } else {

                printf("TS00: %d\n",ICMPTimeStampTbl[0]);
                for(i=1; i<20; i++) {
                    printf("TS%02d: %d (dlta=%d)\n",i,
                           ICMPTimeStampTbl[i],
                           ICMPTimeStampTbl[i]-ICMPTimeStampTbl[i-1]);
                }
                ICMPTimeStamp = xmit;
            }
        } else {
            ICMPTimeStamp = xmit;
        }
    }
#endif
    else {
#if INCLUDE_ETHERVERBOSE
        if(EtherVerbose & SHOW_INCOMING) {
            printf("    ICMPTYPE=%d\n",icmpp->type);
        }
#endif
        return(-1);
    }
    return(0);
}

/* SendEchoResp():
 *  Called in response to an ICMP ECHO REQUEST.  Typically used as
 *  a response to a PING.
 */
int
SendEchoResp(struct ether_header *re)
{
    int i, icmp_len, datalen, oddlen;
    ulong   t;
    ushort  *sp, ip_len;
    struct ether_header *te;
    struct ip *ti, *ri;
    struct icmp_echo_hdr *ticmp, *ricmp;

    ri = (struct ip *)(re + 1);
    datalen = ecs(ri->ip_len) - ((ri->ip_vhl & 0x0f) * 4);

    te = EtherCopy(re);
    ti = (struct ip *)(te + 1);
    ti->ip_vhl = ri->ip_vhl;
    ti->ip_tos = ri->ip_tos;
    ti->ip_len = ri->ip_len;
    ti->ip_id = ipId();
    ti->ip_off = 0;
    ti->ip_ttl = UDP_TTL;
    ti->ip_p = IP_ICMP;
    memcpy((char *)&(ti->ip_src.s_addr),(char *)&(ri->ip_dst.s_addr),
           sizeof(struct in_addr));
    memcpy((char *)&(ti->ip_dst.s_addr),(char *)&(ri->ip_src.s_addr),
           sizeof(struct in_addr));

    ticmp = (struct icmp_echo_hdr *)(ti + 1);
    ricmp = (struct icmp_echo_hdr *)(ri + 1);
    ticmp->type = ICMP_ECHOREPLY;
    ticmp->code = 0;
    ticmp->cksum = 0;
    ticmp->id = ricmp->id;
    ticmp->seq = ricmp->seq;
    memcpy((char *)(ticmp+1),(char *)(ricmp+1),datalen-8);

    ip_len = ecs(ti->ip_len);
    if(ip_len & 1) {        /* If size is odd, temporarily bump up ip_len by */
        oddlen = 1;         /* one and add append a null byte for csum. */
        ((char *)ti)[ip_len] = 0;
        ip_len++;
    } else {
        oddlen = 0;
    }
    icmp_len = (ip_len - sizeof(struct ip))/2;

    ipChksum(ti);   /* compute checksum of ip hdr: (3rd Edition Comer pg 100) */

    /* compute checksum of icmp message: (3rd Edition Comer pg 126) */
    ticmp->cksum = 0;
    sp = (ushort *) ticmp;
    for(i=0,t=0; i<icmp_len; i++,sp++) {
        t += ecs(*sp);
    }
    t = (t & 0xffff) + (t >> 16);
    ticmp->cksum = ~t;

    self_ecs(ticmp->cksum);

    if(oddlen) {
        ip_len--;
    }

    sendBuffer(sizeof(struct ether_header) + ip_len);
#if INCLUDE_ETHERVERBOSE
    if(EtherVerbose & SHOW_OUTGOING) {
        printf("  Sent Echo Response\n");
    }
#endif
    return(0);
}

/* SendICMPRequest():
 *  Currently supports ICMP_TIMEREQUEST and ICMP_ECHOREQUEST.
 */
int
SendICMPRequest(uchar type,uchar *binip,uchar *binenet,
                ulong tmpval,ushort seq)
{
    ushort  *sp;
    uchar   *data;
    struct  ip *ti;
    int     i, icmp_len;
    struct  ether_header *te;
    struct  icmp_time_hdr *ticmp;
    ulong   origtime, datasize, csum;

    datasize = ICMP_ECHO_DATASIZE;

    /* Retrieve an ethernet buffer from the driver and populate the */
    /* ethernet level of packet: */
    te = (struct ether_header *) getXmitBuffer();
    memcpy((char *)&te->ether_shost,(char *)BinEnetAddr,6);
    memcpy((char *)&te->ether_dhost,(char *)binenet,6);
    te->ether_type = ecs(ETHERTYPE_IP);

    /* Move to the IP portion of the packet and populate it appropriately: */
    ti = (struct ip *)(te + 1);
    ti->ip_vhl = IP_HDR_VER_LEN;
    ti->ip_tos = 0;
#if INCLUDE_ICMPTIME
    if(type == ICMP_TIMEREQUEST) {
        origtime = tmpval;
        ti->ip_len = sizeof(struct ip) + sizeof(struct icmp_time_hdr);
    } else
#endif
    {
        if(tmpval != 0) {
            datasize = tmpval;
        }
        ti->ip_len = sizeof(struct ip) + sizeof(struct icmp_echo_hdr) +
                     datasize;
    }

    ti->ip_id = ipId();
    ti->ip_off = 0;
    ti->ip_ttl = UDP_TTL;
    ti->ip_p = IP_ICMP;
    memcpy((char *)&ti->ip_src.s_addr,(char *)BinIpAddr,4);
    memcpy((char *)&ti->ip_dst.s_addr,(char *)binip,4);

    icmp_len = (ti->ip_len - sizeof(struct ip))/2;
    self_ecs(ti->ip_len);
    self_ecs(ti->ip_off);
    self_ecs(ti->ip_id);

    ipChksum(ti);       /* Compute csum of ip hdr */

    /* Move to the ICMP portion of the packet and populate it appropriately: */
    ticmp = (struct icmp_time_hdr *)(ti+1);
    ticmp->type = type;
    ticmp->code = 0;
    ticmp->seq = ecs(seq);
#if INCLUDE_ICMPTIME
    if(type == ICMP_TIMEREQUEST) {
        ticmp->id = ICMP_TIME_ID;
        memcpy((char *)&ticmp->orig,(char *)&origtime,4);
        memset((char *)&ticmp->recv,0,4);
        memset((char *)&ticmp->xmit,0,4);
    } else
#endif
    {
        ticmp->id = ICMP_ECHO_ID;

        /* Add 'datasize' bytes of data... */
        data = (uchar *)((struct icmp_echo_hdr *)ticmp+1);
        for(i=0; i<datasize; i++) {
            data[i] = 'a'+i;
        }

    }

    /* compute checksum of icmp message: (3rd Edition Comer pg 126) */
    csum = 0;
    ticmp->cksum = 0;
    sp = (ushort *) ticmp;
    for(i=0; i<icmp_len; i++,sp++) {
        csum += ecs(*sp);
    }
    csum = (csum & 0xffff) + (csum >> 16);
    ticmp->cksum = ~csum;

    self_ecs(ticmp->cksum);

#if INCLUDE_ICMPTIME
    if(type == ICMP_TIMEREQUEST) {
        self_ecl(ticmp->orig);
        self_ecl(ticmp->recv);
        self_ecl(ticmp->xmit);
        sendBuffer(ICMP_TIMERQSTSIZE);
    } else
#endif
        sendBuffer(ICMP_ECHORQSTSIZE+datasize);
    return(0);
}

/* Send an ICMP Unreachable message. All ICMP Unreachable messages
 * include the IP header and 64 bits of the datagram that could not be
 * delivered.
 *
 * 're' is the pointer to the packet that was received in response to which
 * the unreachable message is being sent. 'icmp_code' is the code of the
 * ICMP message.
 */
int
SendICMPUnreachable(struct ether_header *re,uchar icmp_code)
{
    int i, len;
    ushort *sp, r_iphdr_len, ip_len;
    ulong t;
    struct ether_header *te;
    struct ip *ti, *ri;
    struct icmp_unreachable_hdr *ticmp;

    /* If DONTSEND_ICMP_UNREACHABLE is set, then don't send out the
     * message...
     */
    if(getenv("DONTSEND_ICMP_UNREACHABLE")) {
        return(0);
    }

    ri = (struct ip *)(re + 1);

    /* Length of the received IP hdr */
    r_iphdr_len = ((ri->ip_vhl & 0x0f)<<2);

    te = EtherCopy(re);

    ti = (struct ip *)(te + 1);
    ti->ip_vhl = IP_HDR_VER_LEN;
    ti->ip_tos = 0;
    /* Length of the outgoing IP packet = IP header + ICMP header +
     * IP header and 8 bytes of unreachable datagram
     */
    ip_len = (IP_HDR_LEN<<2)+sizeof(struct icmp_unreachable_hdr)+r_iphdr_len+8;
    ti->ip_len = ecs(ip_len);
    ti->ip_id = ipId();
    ti->ip_off = 0;
    ti->ip_ttl = UDP_TTL;
    ti->ip_p = IP_ICMP;

    /* Note that we do memcpy instead of struct copy because the ip
     * header is not not word-aligned. As a result, the source and dest
     * addresses are not word aligned either. The compiler generates word
     * copy instructions for struct copy and this causes address alignement
     * exceptions.
     */
    memcpy((char *)&(ti->ip_src.s_addr),(char *)&(ri->ip_dst.s_addr),
           sizeof(struct in_addr));
    memcpy((char *)&(ti->ip_dst.s_addr),(char *)&(ri->ip_src.s_addr),
           sizeof(struct in_addr));

    ticmp = (struct icmp_unreachable_hdr *)(ti + 1);
    ticmp->type = ICMP_DESTUNREACHABLE;
    ticmp->code = icmp_code;
    ticmp->cksum = 0;
    ticmp->unused1=0;
    ticmp->unused2=0;

    /* Copy the IP header and 8 bytes of the received datagram */
    memcpy((char *)(ticmp+1),(char *)ri,r_iphdr_len+8);

    ipChksum(ti);   /* compute checksum of ip hdr */

    /* compute checksum of icmp message: (see Comer pg 91) */
    len = (ip_len - sizeof(struct ip))/2;
    ticmp->cksum = 0;
    sp = (ushort *) ticmp;
    for(i=0,t=0; i<len; i++,sp++) {
        t += ecs(*sp);
    }
    t = (t & 0xffff) + (t >> 16);
    ticmp->cksum = ~t;
    self_ecs(ticmp->cksum);

    sendBuffer(sizeof(struct ether_header) + ip_len);

#if INCLUDE_ETHERVERBOSE
    if(EtherVerbose & SHOW_OUTGOING)
        printf("  Sent ICMP Dest Unreachable message. Code='%s'.\n",
               IcmpDestUnreachableCode[icmp_code]);
#endif
    return(0);
}

#endif  /* INCLUDE_ICMP */
#endif  /* INCLUDE_ETHERNET */
