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
 * syslog.c:
 *
 *  This code supports the monitor's SYSLOC client.
 *
 * Original author:     Ed Sutter (ed.sutter@alcatel-lucent.com)
 *
 */
#include "config.h"
#if INCLUDE_SYSLOG
#include "endian.h"
#include "genlib.h"
#include "ether.h"
#include "stddefs.h"
#include "cli.h"

#define SYSLOG_PORT 514

struct nameval {
    char *name;
    int val;
};

/* Priority codes:
 */
#define LOG_EMERG       0   /* system is unusable */
#define LOG_ALERT       1   /* action must be taken immediately */
#define LOG_CRIT        2   /* critical conditions */
#define LOG_ERR         3   /* error conditions */
#define LOG_WARNING     4   /* warning conditions */
#define LOG_NOTICE      5   /* normal but significant condition */
#define LOG_INFO        6   /* informational */
#define LOG_DEBUG       7   /* debug-level messages */

/* Facility codes:
 */
#define LOG_KERN        (0<<3)  /* kernel messages */
#define LOG_USER        (1<<3)  /* random user-level messages */
#define LOG_MAIL        (2<<3)  /* mail system */
#define LOG_DAEMON      (3<<3)  /* system daemons */
#define LOG_AUTH        (4<<3)  /* security/authorization messages */
#define LOG_SYSLOG      (5<<3)  /* messages generated internally by syslogd */
#define LOG_LPR         (6<<3)  /* line printer subsystem */
#define LOG_NEWS        (7<<3)  /* network news subsystem */
#define LOG_UUCP        (8<<3)  /* UUCP subsystem */
#define LOG_CRON        (9<<3)  /* clock daemon */
#define LOG_AUTHPRIV    (10<<3) /* security/authorization messages (private) */
#define LOG_FTP         (11<<3) /* ftp daemon */
#define LOG_LOCAL0      (16<<3) /* reserved for local use */
#define LOG_LOCAL1      (17<<3) /* reserved for local use */
#define LOG_LOCAL2      (18<<3) /* reserved for local use */
#define LOG_LOCAL3      (19<<3) /* reserved for local use */
#define LOG_LOCAL4      (20<<3) /* reserved for local use */
#define LOG_LOCAL5      (21<<3) /* reserved for local use */
#define LOG_LOCAL6      (22<<3) /* reserved for local use */
#define LOG_LOCAL7      (23<<3) /* reserved for local use */

#define FACILITY_MASK   0x03f8
#define PRIORITY_MASK   0x0007

struct nameval prioritynames[] = {
    { "emerg",      LOG_EMERG },
    { "alert",      LOG_ALERT },
    { "critical",   LOG_CRIT },
    { "error",      LOG_ERR },
    { "warning",    LOG_WARNING },
    { "notice",     LOG_NOTICE },
    { "info",       LOG_INFO },
    { "debug",      LOG_DEBUG },
    { 0,            0 }
};

struct nameval facilitynames[] = {
    { "kernel",     LOG_KERN },
    { "user",       LOG_USER },
    { "mail",       LOG_MAIL },
    { "daemon",     LOG_DAEMON },
    { "authorize",  LOG_AUTH },
    { "syslog",     LOG_SYSLOG },
    { "lpr",        LOG_LPR },
    { "news",       LOG_NEWS },
    { "uucp",       LOG_UUCP },
    { "cron",       LOG_CRON },
    { "authpriv",   LOG_AUTHPRIV },
    { "ftp",        LOG_FTP },
    { "local0",     LOG_LOCAL0 },
    { "local1",     LOG_LOCAL1 },
    { "local2",     LOG_LOCAL2 },
    { "local3",     LOG_LOCAL3 },
    { "local4",     LOG_LOCAL4 },
    { "local5",     LOG_LOCAL5 },
    { "local6",     LOG_LOCAL6 },
    { "local7",     LOG_LOCAL7 },
    { 0,            0 }
};

void
syslogInfo(void)
{
    int tot;
    struct nameval *nvp;

    tot = 0;
    nvp = prioritynames;
    printf("\nValid 'priority' names:\n");
    while(nvp->name) {
        printf("%12s",nvp->name);
        nvp++;
        if(++tot == 4) {
            printf("\n");
            tot = 0;
        }
    }

    tot = 0;
    nvp = facilitynames;
    printf("\nValid 'facility' names:\n");
    while(nvp->name) {
        printf("%12s",nvp->name);
        nvp++;
        if(++tot == 4) {
            printf("\n");
            tot = 0;
        }
    }
}


int
getPriorityValue(char *prio)
{
    struct nameval *nvp;

    if(prio == 0) {
        return(0);
    }

    nvp = prioritynames;
    while(nvp->name) {
        if(!strcmp(nvp->name,prio)) {
            return(nvp->val);
        }
        nvp++;
    }
    return(-1);
}

int
getFacilityValue(char *fac)
{
    struct nameval *nvp;

    if(fac == 0) {
        return(0);
    }

    nvp = facilitynames;
    while(nvp->name) {
        if(!strcmp(nvp->name,fac)) {
            return(nvp->val);
        }
        nvp++;
    }
    return(-1);
}

int
sendSyslog(uchar *syslogsrvr,char *msg, short port, int null)
{
    int msglen;
    uchar *syslogmsg;
    ushort ip_len, sport;
    struct ether_header *enetp;
    struct ip *ipp;
    struct Udphdr *udpp;
    uchar   binip[8], binenet[8], *enetaddr;

    /* msglen is the length of the message, plus optionally the
     * terminating null character (-n option of syslog command)..
     */
    msglen = strlen(msg) + null;

    /* Convert IP address to binary:
     */
    if(IpToBin((char *)syslogsrvr,(unsigned char *)binip) < 0) {
        return(0);
    }

    /* Get the ethernet address for the IP:
     */
    enetaddr = ArpEther(binip,binenet,0);
    if(!enetaddr) {
        printf("ARP failed for %s\n",syslogsrvr);
        return(0);
    }

    /* Retrieve an ethernet buffer from the driver and populate the
     * ethernet level of packet:
     */
    enetp = (struct ether_header *) getXmitBuffer();
    memcpy((char *)&enetp->ether_shost,(char *)BinEnetAddr,6);
    memcpy((char *)&enetp->ether_dhost,(char *)binenet,6);
    enetp->ether_type = ecs(ETHERTYPE_IP);

    /* Move to the IP portion of the packet and populate it
     * appropriately:
     */
    ipp = (struct ip *)(enetp + 1);
    ipp->ip_vhl = IP_HDR_VER_LEN;
    ipp->ip_tos = 0;
    ip_len = sizeof(struct ip) + sizeof(struct Udphdr) + msglen;
    ipp->ip_len = ecs(ip_len);
    ipp->ip_id = ipId();
    ipp->ip_off = 0;
    ipp->ip_ttl = UDP_TTL;
    ipp->ip_p = IP_UDP;
    memcpy((char *)&ipp->ip_src.s_addr,(char *)BinIpAddr,4);
    memcpy((char *)&ipp->ip_dst.s_addr,(char *)binip,4);

    /* Now UDP...
     */
    sport = port+1;
    udpp = (struct Udphdr *)(ipp + 1);
    udpp->uh_sport = ecs(sport);
    udpp->uh_dport = ecs(port);
    udpp->uh_ulen = ecs((ushort)(ip_len - sizeof(struct ip)));

    /* Finally, the SYSLOG data ...
     */
    syslogmsg = (uchar *)(udpp+1);
    strcpy((char *)syslogmsg,(char *)msg);

    ipChksum(ipp);          /* Compute csum of ip hdr */
    udpChksum(ipp);         /* Compute UDP checksum */

    sendBuffer(ETHERSIZE + IPSIZE + UDPSIZE + msglen);
    return(0);
}

char *SyslogHelp[] = {
    "Syslog client",
    "-[f:lP:np:v] {srvr ip} {msg}",
#if INCLUDE_VERBOSEHELP
    "Options:",
    " -f {fac}  specify facility",
    " -l        list facility & priority strings",
    " -n        append null-char to message",
    " -P {##}   override default port 514",
    " -p {prio} specify priority",
    " -v        verbose",
#endif
    0,
};


int
SyslogCmd(int argc, char *argv[])
{
    char *facility, *priority, *msg;
    int opt, verbose, fac_val, prio_val, val, port, null;

    port =  SYSLOG_PORT;
    facility = priority = 0;
    null = fac_val = prio_val = verbose = 0;

    while((opt=getopt(argc,argv,"f:lnP:p:v")) != -1) {
        switch(opt) {
        case 'f':
            facility = optarg;
            break;
        case 'l':
            syslogInfo();
            return(CMD_SUCCESS);
        case 'n':
            null = 1;
            break;
        case 'P':
            port = atoi(optarg);
            break;
        case 'p':
            priority = optarg;
            break;
        case 'v':
            verbose = 1;
            break;
        default:
            return(CMD_PARAM_ERROR);
        }
    }

    if(argc != optind+2) {
        return(CMD_PARAM_ERROR);
    }

    prio_val = getPriorityValue(priority);
    fac_val = getFacilityValue(facility);

    if((prio_val == -1) || (fac_val == -1)) {
        return(CMD_PARAM_ERROR);
    }

    val = (prio_val | fac_val);

    if((priority != 0) || (facility != 0)) {
        msg = malloc(strlen(argv[optind+1])+16);
        if(!msg) {
            return(CMD_FAILURE);
        }
        sprintf(msg,"<%d>%s",val,argv[optind+1]);
    } else {
        msg = argv[optind+1];
    }

    if(verbose) {
        printf("Syslog msg '%s' to %s\n",msg,argv[optind]);
    }

    sendSyslog((unsigned char *)argv[optind],msg,port,null);

    if(val) {
        free(msg);
    }

    return(CMD_SUCCESS);
}

#endif

