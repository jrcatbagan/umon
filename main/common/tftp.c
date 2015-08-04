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
 * tftp.c:
 *
 * This code supports the monitor's TFTP server.
 * TFTP is covered under RFC 783.
 *
 * Original author:     Ed Sutter (ed.sutter@alcatel-lucent.com)
 *
 */
#include "config.h"
#if INCLUDE_TFTP
#include <stdarg.h>
#include "endian.h"
#include "genlib.h"
#include "cpuio.h"
#include "ether.h"
#include "tfs.h"
#include "tfsprivate.h"
#include "monflags.h"
#include "stddefs.h"
#include "flash.h"
#include "cli.h"
#include "timer.h"

/* To reduce uMon footprint size, INCLUDE_TFTPSRVR can be set to 0 in
 * config.h...
 */
#ifndef INCLUDE_TFTPSRVR
#define INCLUDE_TFTPSRVR 1
#endif

#if INCLUDE_ETHERVERBOSE
#define TFTPVERBOSE(a) a
#else
#define TFTPVERBOSE(a)
#endif


#define MODE_NULL       0
#define MODE_NETASCII   1
#define MODE_OCTET      2

void ShowTftpStats(void);
static int SendTFTPData(struct ether_header *,ushort,uchar *,int);
static int SendTFTPErr(struct ether_header *,short,int,char *fmt, ...);
static int SendTFTPAck(struct ether_header *,ushort);
static int SendTFTPRRQ(uchar *,uchar *,char *,char *,uchar *);
static int SendTFTPWRQ(uchar *,uchar *,char *,char *);


static  struct elapsed_tmr tftpTmr;

static ushort   tftpPrevBlock;  /* Keeps the value of the block number of
                                 * the previous TFTP transaction.
                                 */
static int TftpWrqMode;         /* Set to MODE_NETASCII, MODE_OCTET or
                                 * MODE_NULL based on the incoming WRQ
                                 * request
                                 */
static int TftpChopCount;       /* Number of characters chopped from the
                                 * incoming file because of NETASCII
                                 * conversion (remove 0x0d).
                                 */
static short TftpLastPktSize;
static uchar TftpLastPkt[TFTP_PKTOVERHEAD+TFTP_DATAMAX+4];
/* Storage of last packet sent.  This is
 * used if it is determined that the packet
 * most recently sent must be sent again.
 */

static long TftpRetryTimeout;   /* Count used during a TFTP transfer to
                                 * kick off a retransmission of a packet.
                                 */
static ushort TftpRmtPort;      /* Remote port of tftp transfer */
static uchar *TftpAddr;         /* Current destination address used for tftp
                                 * file transfer to/from local memory.
                                 */
static char *TftpLTMptr;        /* List-to-mem pointer used when filename
                                 * for RRQ is ".".
                                 */
static int  TftpCount;          /* Running total number of bytes transferred
                                 * for a particular tftp transaction.
                                 */
static char TftpState;          /* Used to keep track of the current state
                                 * of a tftp transaction.
                                 */
static char TftpTurnedOff;      /* If non-zero, then tftp is disabled. */
static char TftpGetActive;      /* If non-zero, then 'get' is in progress. */
static char TftpPutActive;      /* If non-zero, then 'put' is in progress. */
static char TftpErrString[32];  /* Used to post a tftp error message. */
static char TftpTfsFname[TFSNAMESIZE+64];   /* Store name of WRQ destination
                                             * file (plus flags & info).
                                             */
#if INCLUDE_TFTPSRVR
/* ls_to_mem():
 * Turn the list of files into a string buffer that contains
 * each filename followed by a newline.  Each filename is the "full"
 * name, meaning that it will contain commas to delimit the flags and
 * info fields of the file.
 * Return a pointer to the allocated space.
 */
static char *
ls_to_mem(void)
{
#if INCLUDE_TFS
    int     size;
    TFILE   *tfp;
    char    *base, *bp, *info, *flags, fbuf[16];

    /* First determine how much memory will be needed to store
     * the list of files...  Each filename in the list will consist
     * of the string formatted as "name,flags,info\n"...
     */
    size = 0;
    tfp = (TFILE *)0;
    while((tfp = tfsnext(tfp))) {
        /* Increment size to include filename, 2 commas and a newline...
         */
        size += (strlen(TFS_NAME(tfp)) + 3);

        /* Determine if flags and/or info field is to be present in
         * the name, and add that to the size appropriately...
         */
        flags = tfsflagsbtoa(TFS_FLAGS(tfp),fbuf);
        if(!fbuf[0]) {
            flags = 0;
        }
        if(flags) {
            size += strlen(flags);
        }
        if(TFS_INFO(tfp)) {
            size += strlen(TFS_INFO(tfp));
        }
    }

    if(size == 0) {
        return(0);
    }

    /* Allocate the space.  Add one additional byte to the size to
     * account for the NULL termination at the end of the string...
     */
    base = bp = malloc(size+1);
    if(bp == (char *)0) {
        return(0);
    }

    /* Load the buffer with the list of names with their
     * TFS extensions...
     */
    tfp = (TFILE *)0;
    while((tfp = tfsnext(tfp))) {
        flags = tfsflagsbtoa(TFS_FLAGS(tfp),fbuf);
        if(!flags || !fbuf[0]) {
            flags = "";
        }
        if(TFS_INFO(tfp)) {
            info = TFS_INFO(tfp);
        } else {
            info = "";
        }

        bp += sprintf(bp,"%s,%s,%s\n",TFS_NAME(tfp),flags,info);
    }

    return(base);
#else
    return(0);
#endif
}
#endif

static char *
tftpStringState(int state)
{
    switch(state) {
    case TFTPOFF:
        return("OFF");
    case TFTPIDLE:
        return("IDLE");
    case TFTPACTIVE:
        return("ACTIVE");
    case TFTPERROR:
        return("ERROR");
    case TFTPHOSTERROR:
        return("HOSTERROR");
    case TFTPSENTRRQ:
        return("SENTRRQ");
    case TFTPSENTWRQ:
        return("SENTWRQ");
    case TFTPTIMEOUT:
        return("TIMEOUT");
    default:
        return("???");
    }
}

static int
tftpGotoState(int state)
{
    int ostate;

    ostate = TftpState;
    TftpState = state;
#if INCLUDE_ETHERVERBOSE
    if((EtherVerbose & SHOW_TFTP_STATE) && (state != ostate))
        printf("  TFTP State change %s -> %s\n",
               tftpStringState(ostate), tftpStringState(state));
#endif
    return(ostate);
}

/* tftpGet():
 *  Return size of file if successful; else 0.
 */
int
tftpGet(ulong addr,char *tftpsrvr,char *mode, char *hostfile,char *tfsfile,
        char *tfsflags,char *tfsinfo)
{
    int     done;
    uchar   binip[8], binenet[8], *enetaddr;

    setenv("TFTPGET",0);

    /* Convert IP address to binary: */
    if(IpToBin(tftpsrvr,binip) < 0) {
        return(0);
    }

    /* Get the ethernet address for the IP: */
    /* Give ARP the same verbosity (if any) set up for TFTP: */
    TFTPVERBOSE(if(EtherVerbose & SHOW_TFTP_STATE) EtherVerbose |= SHOW_ARP);
    enetaddr = ArpEther(binip,binenet,0);
    TFTPVERBOSE(EtherVerbose &= ~SHOW_ARP);
    if(!enetaddr) {
        printf("ARP failed for %s\n",tftpsrvr);
        return(0);
    }

    printf("Retrieving %s from %s...\n",hostfile,tftpsrvr);

    /* Send the TFTP RRQ to initiate the transfer. */
    if(SendTFTPRRQ(binip,binenet,hostfile,mode,(uchar *)addr) < 0) {
        printf("RRQ failed\n");
        return(0);
    }

    TftpGetActive = 1;

    /* Wait for TftpState to indicate that the transaction has completed... */
    done = 0;
    while(!done) {
        pollethernet();
        switch(TftpState) {
        case TFTPIDLE:
            printf("Rcvd %d bytes",TftpCount);
            done = 1;
            break;
        case TFTPERROR:
            printf("TFTP Terminating\n");
            done = 2;
            break;
        case TFTPHOSTERROR:
            printf("Host error: %s\n",TftpErrString);
            done = 2;
            break;
        case TFTPTIMEOUT:
            printf("Timing out (%d bytes rcvd)\n",TftpCount);
            done = 2;
            break;
        default:
            break;
        }
    }
    TftpGetActive = 0;

    if(done == 2) {
        tftpInit();
        return(0);
    }

    if(tfsfile) {
        int err, filesize;

        filesize = TftpCount - TftpChopCount;
        printf("\nAdding %s (size=%d) to TFS...",tfsfile,filesize);
        err = tfsadd(tfsfile,tfsinfo,tfsflags,(uchar *)addr,filesize);
        if(err != TFS_OKAY) {
            printf("%s: %s\n",tfsfile,(char *)tfsctrl(TFS_ERRMSG,err,0));
        }
    }
    printf("\n");
    shell_sprintf("TFTPGET","%d",TftpCount);
    return(TftpCount);
}

/* tftpPut():
 * Return size of file if successful; else 0.
 *
 * Simple steps from RFC 1350 for PUT are:
 * 1. Send WRQ to port 69 of remote server
 * 2. Receive ACK 0 from Server RemotePort (not 69)
 * 3. Send data less than 512 bytes to RemotePort which closes it out.
 * 4. Reinit if error (above in GET errors if done == 2)
 *
 * This 'put' capability was contributed by Steve Shireman.
 */

int
tftpPut(char *tftpsrvr,char *mode,char *hostfile,char *tfsfile,
        char *tfsflags,char *tfsinfo)
{
#if INCLUDE_TFS
    int     done, tot;
    TFILE   *tfp;
    uchar   binip[8], binenet[8], *enetaddr;

    setenv("TFTPPUT",0);

    if((tfp = tfsstat(tfsfile)) == (TFILE *)0) {
        return(0);
    }

    /* For 'put', the address increments and the count decrements as
     * the transfer progresses.
     */
    TftpAddr = (uchar *)TFS_BASE(tfp);
    TftpCount = tot = TFS_SIZE(tfp);

    /* Convert IP address to binary: */
    if(IpToBin(tftpsrvr,binip) < 0) {
        return(0);
    }

    /* Get the ethernet address for the IP:
     * (give ARP the same verbosity set up for TFTP)
     */
    TFTPVERBOSE(if(EtherVerbose & SHOW_TFTP_STATE) EtherVerbose |= SHOW_ARP);
    enetaddr = ArpEther(binip,binenet,0);
    TFTPVERBOSE(EtherVerbose &= ~SHOW_ARP);

    if(!enetaddr) {
        printf("ARP failed for %s\n",tftpsrvr);
        return(0);
    }

    printf("Sending %s to %s:%s...\n",tfsfile,tftpsrvr,hostfile);
    TftpPutActive = 1;
    tftpPrevBlock = 0;

    /* Send the TFTP WRQ to initiate the transfer. */
    if(SendTFTPWRQ(binip,binenet,hostfile,mode) < 0) {
        printf("WRQ failed\n");
        return(0);
    }

    /* Wait for TftpState to indicate that the transaction has completed... */
    done = 0;
    while(!done) {
        pollethernet();
        switch(TftpState) {
        case TFTPIDLE:
            printf("Sent %d bytes",tot);
            done = 1;
            break;
        case TFTPERROR:
            printf("TFTP Terminating\n");
            done = 2;
            break;
        case TFTPHOSTERROR:
            printf("Host error: %s\n",TftpErrString);
            done = 2;
            break;
        case TFTPTIMEOUT:
            printf("Timing out (%d bytes sent)\n",TftpCount);
            done = 2;
            break;
        default:
            break;
        }
    }
    TftpPutActive = 0;

    if(done == 2) {
        tftpInit();
        return(0);
    }
    printf("\n");
    shell_sprintf("TFTPPUT","%d",tot);
    return(tot);
#else
    printf("TFTP 'put' requires TFS\n");
    return(0);
#endif
}

/* tftpInit():
 *  Called by the ethenet initialization to initialize state variables.
 */
void
tftpInit()
{
    TftpCount = -1;
    TftpRmtPort = 0;
    TftpTurnedOff = 0;
    tftpGotoState(TFTPIDLE);
    TftpAddr = (uchar *)0;
}

/* storePktAndSend():
 *  The final stage in sending a TFTP packet...
 *  1. Compute IP and UDP checksums;
 *  2. Copy ethernet packet to a buffer so that it can be resent
 *      if necessary (by tftpStateCheck()).
 *  3. Store the size of the packet;
 *  4. Send the packet out the interface.
 *  5. Reset the timeout count and re-transmission delay variables.
 */
static void
storePktAndSend(struct ip *ipp, struct ether_header *epkt,int size)
{
    ipChksum(ipp);                          /* Compute csum of ip hdr */
    udpChksum(ipp);                         /* Compute UDP checksum */
    /* Copy packet to static buffer */
    memcpy((char *)TftpLastPkt,(char *)epkt,size);
    TftpLastPktSize = size;                 /* Copy size to static location */
    sendBuffer(size);                       /* Send buffer out ethernet i*/

    /* Re-initialize the re-transmission delay variables.
     */
    TftpRetryTimeout = RetransmitDelay(DELAY_INIT_TFTP);
    startElapsedTimer(&tftpTmr,TftpRetryTimeout * 1000);
}

/* getTftpSrcPort():
 *  Each time a TFTP RRQ goes out, use a new source port number.
 *  Cycle through a block of 256 port numbers...
 */
static ushort
getTftpSrcPort(void)
{
    if(TftpSrcPort < (IPPORT_TFTPSRC+256)) {
        TftpSrcPort++;
    } else {
        TftpSrcPort = IPPORT_TFTPSRC;
    }
    return(TftpSrcPort);
}

/* tftpStateCheck():
 *  Called by the pollethernet function to support the ability to retry
 *  on a TFTP transmission that appears to have terminated prematurely
 *  due to a lost packet.  If a packet is sent and the response is not
 *  received within about 1-2 seconds, the packet is re-sent.  The retry
 *  will repeat 8 times; then give up and set the TFTP state to idle.
 *
 *  Taken from RFC 1350 section 2 "Overview of the Protocol"...
 *  ... If a packet gets lost in the network, the intended recipient will
 *  timeout and may retransmit his last packet (which may be data or an
 *  acknowledgement), thus causing the sender of the lost packet to retransmit
 *  the lost packet.
 *
 *  Taken from RFC 1123 section 4.2.3.2 "Timeout Algorithms"...
 *  ... a TFTP implementation MUST use an adaptive timeout ...
 */

void
tftpStateCheck(void)
{
    uchar   *buf;
    long    delay;
    ushort  tftp_opcode;
    struct  ip *ihdr;
    struct  Udphdr *uhdr;
    struct  ether_header *ehdr;

    switch(TftpState) {
    case TFTPIDLE:
    case TFTPTIMEOUT:
    case TFTPERROR:
        return;
    default:
        break;
    }

    /* If timeout occurs, re-transmit the packet...
     */
    if(!msecElapsed(&tftpTmr)) {
        return;
    }

    delay = RetransmitDelay(DELAY_INCREMENT);
    if(delay == RETRANSMISSION_TIMEOUT) {
#if INCLUDE_ETHERVERBOSE
        if(EtherVerbose & SHOW_TFTP_STATE) {
            printf("  TFTP_RETRY giving up\n");
        }
#endif
        tftpGotoState(TFTPTIMEOUT);
        enableBroadcastReception();
        return;
    }

    /* Get a transmit buffer and copy the packet that was last sent.
     * Insert a new IP ID, recalculate the checksums and send it again...
     * If the opcode of the packet to be re-transmitted is RRQ, then
     * use a new port number.
     */
    buf = (uchar *)getXmitBuffer();
    memcpy((char *)buf,(char *)TftpLastPkt,TftpLastPktSize);
    ehdr = (struct ether_header *)buf;
    ihdr = (struct ip *)(ehdr + 1);
    uhdr = (struct Udphdr *)(ihdr + 1);
    tftp_opcode = *(ushort *)(uhdr + 1);
    ihdr->ip_id = ipId();
    if(tftp_opcode == ecs(TFTP_RRQ)) {
        uhdr->uh_sport = getTftpSrcPort();
        self_ecs(uhdr->uh_sport);
    }
    ipChksum(ihdr);
    udpChksum(ihdr);
    sendBuffer(TftpLastPktSize);

#if INCLUDE_ETHERVERBOSE
    if(EtherVerbose & SHOW_TFTP_STATE) {
        printf("  TFTP_RETRY (%ld secs)\n",TftpRetryTimeout);
    }
#endif

    TftpRetryTimeout = delay;
    startElapsedTimer(&tftpTmr,TftpRetryTimeout * 1000);
    return;
}

#if INCLUDE_TFTPSRVR
/* tftpStartSrvrFilter():
 * Called when either a TFTP_RRQ or TFTP_WRQ is received indicating that
 * a client wants to start up a transfer.
 * If TftpState is IDLE, ERROR or TIMEOUT, this means it is safe to start
 * up a new transfer through the server; otherwise return 0.
 * Also, if the server has been disabled by "tftp off" at the command line
 * (i.e. TftpTurnedOff == 1) then return 0.
 */
static int
tftpStartSrvrFilter(struct ether_header *ehdr,struct Udphdr *uhdr)
{
    if(TftpTurnedOff) {
        SendTFTPErr(ehdr,0,1,"TFTP srvr off");
        return(0);
    }

    if((TftpState == TFTPIDLE) || (TftpState == TFTPERROR) ||
            (TftpState == TFTPTIMEOUT)) {
        TftpTfsFname[0] = 0;
        TftpRmtPort = ecs(uhdr->uh_sport);
        tftpGotoState(TFTPACTIVE);
        return(1);
    }
    /* If block is zero and the incoming WRQ request is from the same
     * port as was previously recorded, then assume the ACK sent back
     * to the requester was not received, and this is a WRQ that is
     * being re-sent.  That being the case, just send the Ack back.
     */
    else if((tftpPrevBlock == 0) && (TftpRmtPort == uhdr->uh_sport)) {
        SendTFTPAck(ehdr,0);
        return(0);
    } else {
        /* Note: the value of TftpState is not changed (final arg to
         * SendTFTPErr is 0) to TFTPERROR.  This is because
         * we received a RRQ/WRQ request while processing a different
         * TFTP transfer.  We want to send the error response to the
         * sender, but we don't want to stay in an error state because
         * there is another valid TFTP transfer in progress.
         */
        SendTFTPErr(ehdr,0,0,"TFTP srvr busy");
        return(0);
    }
}
#endif

/* tftpTransferComplete():
 * Print a message indicating that the transfer has completed.
 */
static void
tftpTransferComplete(void)
{
#if INCLUDE_ETHERVERBOSE
    if((EtherVerbose & SHOW_TFTP_STATE) || (!MFLAGS_NOTFTPPRN())) {
        printf("TFTP transfer complete.\n");
    }
#endif
}

/* processTFTP():
 *  This function handles the majority of the TFTP requests that a
 *  TFTP server must be able to handle.  There is no real robust
 *  error handling, but it seems to work pretty well.
 *  Refer to Stevens' "UNIX Network Programming" chap 12 for details
 *  on TFTP.
 *  Note: During TFTP, promiscuity, broadcast & multicast reception
 *  are all turned off.  This is done to speed up the file transfer.
 */
int
processTFTP(struct ether_header *ehdr,ushort size)
{
    struct  ip *ihdr;
    struct  Udphdr *uhdr;
    uchar   *data;
    int     count;
    ushort  opcode, block, errcode;
    char    *errstring, *tftpp;
#if INCLUDE_TFTPSRVR
    char    *comma, *env, *filename, *mode;
#endif

    ihdr = (struct ip *)(ehdr + 1);
    uhdr = (struct Udphdr *)((char *)ihdr + IP_HLEN(ihdr));
    tftpp = (char *)(uhdr + 1);
    opcode = *(ushort *)tftpp;

    switch(opcode) {
#if INCLUDE_TFTPSRVR
    case ecs(TFTP_WRQ):
        filename = tftpp+2;
#if INCLUDE_ETHERVERBOSE
        if((EtherVerbose & SHOW_TFTP_STATE) || (!MFLAGS_NOTFTPPRN())) {
            printf("TFTP rcvd WRQ: file <%s>\n", filename);
        }
#endif
        if(!tftpStartSrvrFilter(ehdr,uhdr)) {
            return(0);
        }

        mode = filename;
        while(*mode) {
            mode++;
        }
        mode++;

        /* Destination of WRQ can be an address (0x...), environment
         * variable ($...) or a TFS filename...
         */
        if(filename[0] == '$') {
            env = getenv(&filename[1]);
            if(env) {
                TftpAddr = (uchar *)strtol(env,(char **)0,0);
            } else {
                SendTFTPErr(ehdr,0,1,"Shell var not set");
                return(0);
            }
        } else if((filename[0] == '0') && (filename[1] == 'x')) {
            TftpAddr = (uchar *)strtol(filename,(char **)0,0);
        } else {
            if(MFLAGS_NOTFTPOVW() && tfsstat(filename)) {
                SendTFTPErr(ehdr,6,1,"File exists");
                return(0);
            }
            TftpAddr = (uchar *)getAppRamStart();
            strncpy(TftpTfsFname,filename,sizeof(TftpTfsFname)-1);
            TftpTfsFname[sizeof(TftpTfsFname)-1] = 0;
        }
        TftpCount = -1; /* not used with WRQ, so clear it */

        /* Convert mode to lower case... */
        strtolower(mode);
        if(!strcmp(mode,"netascii")) {
            TftpWrqMode = MODE_NETASCII;
        } else if(!strcmp(mode,"octet")) {
            TftpWrqMode = MODE_OCTET;
        } else {
            SendTFTPErr(ehdr,0,1,"Mode '%s' not supported.",mode);
            TftpWrqMode = MODE_NULL;
            TftpCount = -1;
            return(0);
        }
        block = 0;
        tftpPrevBlock = block;
        TftpChopCount = 0;
        disableBroadcastReception();
        break;
    case ecs(TFTP_RRQ):
        filename = tftpp+2;
#if INCLUDE_ETHERVERBOSE
        if((EtherVerbose & SHOW_TFTP_STATE) || (!MFLAGS_NOTFTPPRN())) {
            printf("TFTP rcvd RRQ: file <%s>\n",filename);
        }
#endif
        if(!tftpStartSrvrFilter(ehdr,uhdr)) {
            return(0);
        }
        mode = filename;
        while(*mode) {
            mode++;
        }
        mode++;
        comma = strchr(filename,',');
        if(!comma) {
            TFILE   *tfp;

            if(!strcmp(filename,".")) {
                TftpLTMptr = (char *)ls_to_mem();
                TftpAddr = (uchar *)TftpLTMptr;
                if(TftpLTMptr) {
                    TftpCount = strlen((char *)TftpAddr);
                } else {
                    TftpCount = 0;
                }
            } else {
                tfp = tfsstat(filename);
                if(!tfp) {
                    SendTFTPErr(ehdr,0,1,"File (%s) not found",filename);
                    TftpCount = -1;
                    return(0);
                }
                TftpAddr = (uchar *)TFS_BASE(tfp);
                TftpCount = TFS_SIZE(tfp);
            }
        } else {
            *comma++ = 0;
            if((filename[0] == '$') && (getenv(&filename[1]))) {
                TftpAddr = (uchar *)strtol(getenv(&filename[1]),(char **)0,0);
            } else {
                TftpAddr = (uchar *)strtol(filename,(char **)0,0);
            }
            TftpCount = strtol(comma,(char **)0,0);
        }
        if(strcmp(mode,"octet")) {
            SendTFTPErr(ehdr,0,1,"Must use binary mode");
            TftpCount = -1;
            return(0);
        }
        block = tftpPrevBlock = 1;
        disableBroadcastReception();
        tftpGotoState(TFTPACTIVE);
        SendTFTPData(ehdr,block,TftpAddr,TftpCount);
        return(0);
#endif
    case ecs(TFTP_DAT):
        block = ecs(*(ushort *)(tftpp+2));
        count = ecs(uhdr->uh_ulen) - (sizeof(struct Udphdr)+4);
#if INCLUDE_ETHERVERBOSE
        if(EtherVerbose & SHOW_TFTP_STATE) {
            printf("  Rcvd TFTP_DAT (%d,blk=%d)\n",count,block);
        } else if(EtherVerbose & SHOW_TFTP_TICKER) {
            ticktock();
        }
#endif

        if(TftpState == TFTPSENTRRQ) {      /* See notes in SendTFTPRRQ() */
            tftpPrevBlock = 0;
            if(block == 1) {
                TftpRmtPort = ecs(uhdr->uh_sport);
                tftpGotoState(TFTPACTIVE);
            } else {
                SendTFTPErr(ehdr,0,1,"invalid block (%d)",block);
                return(0);
            }
        }
        /* Since we don't ACK the final TFTP_DAT from the server until after
         * the file has been written, it is possible that we will receive
         * a re-transmitted TFTP_DAT from the server.  This is ignored by
         * Sending another ACK...
         */
        else if((TftpState == TFTPIDLE) && (block == tftpPrevBlock)) {
            SendTFTPAck(ehdr,block);
#if INCLUDE_ETHERVERBOSE
            if(EtherVerbose & SHOW_TFTP_STATE) {
                printf("  (packet ignored)\n");
            }
#endif
            return(0);
        } else if(TftpState != TFTPACTIVE) {
            SendTFTPErr(ehdr,0,1,"Bad state (%s) for incoming TFTP_DAT",
                        tftpStringState(TftpState));
            return(0);
        }

        if(ecs(uhdr->uh_sport) != TftpRmtPort) {
            SendTFTPErr(ehdr,0,0,"TFTP_DAT porterr: rcvd %d, expected %d",
                        ecs(uhdr->uh_sport),TftpRmtPort);
            return(0);
        }
        if(block == tftpPrevBlock) {    /* If block didn't increment, assume */
            SendTFTPAck(ehdr,block);    /* retry.  Ack it and return here.  */
            return(0);          /* Otherwise, if block != tftpPrevBlock+1, */
        }                       /* return an error, and quit now.   */
        else if((block == 0) && ((tftpPrevBlock & 0xffff) == 0xffff)) {
            /* This case occurs when the block number wraps.
             * It is a legal case where the downloaded data exceeds
             * 32Mg (blockno > 0xffff).
             */
        } else if(block != tftpPrevBlock+1) {
#ifdef DONT_IGNORE_OUT_OF_SEQUENCE_BLOCKS
            SendTFTPErr(ehdr,0,1,"TFTP_DAT blockerr: rcvd %d, expected %d",
                        block,tftpPrevBlock+1);
            TftpCount = -1;
#endif
            return(0);
        }
        TftpCount += count;
        tftpPrevBlock = block;
        data = (uchar *)(tftpp+4);

        /* If count is less than TFTP_DATAMAX, this must be the last
         * packet of the transfer, so clean up state here.
         */
        if(count < TFTP_DATAMAX) {
            enableBroadcastReception();
            tftpGotoState(TFTPIDLE);
        }

        /* Make sure the destination address for the data is not flash
         * or BSS space...
         */
        if(inUmonBssSpace((char *)TftpAddr,(char *)(TftpAddr+count))) {
            SendTFTPErr(ehdr,0,1,"TFTP can't write to uMon BSS space");
            TftpCount = -1;
            return(0);
        }

#if INCLUDE_FLASH
        if(InFlashSpace(TftpAddr,count)) {
            SendTFTPErr(ehdr,0,1,"TFTP can't write directly to flash");
            TftpCount = -1;
            return(0);
        }
#endif
        /* Copy data from enet buffer to TftpAddr location.
         * If netascii mode is active, then this transfer is much
         * slower...
         * If not netascii, then we use s_memcpy() because it does
         * a verification of each byte written and will abort as soon
         * as a failure is detected.
         */
        if(TftpWrqMode == MODE_NETASCII) {
            int tmpcount = count;

            while(tmpcount) {
                if(*data == 0x0d) {
                    data++;
                    tmpcount--;
                    TftpChopCount++;
                    continue;
                }

                *TftpAddr = *data;
                if(*TftpAddr != *data) {
                    SendTFTPErr(ehdr,0,1,"Write error at 0x%lx",
                                (ulong)TftpAddr);
                    TftpCount = -1;
                    return(0);
                }
                TftpAddr++;
                data++;
                tmpcount--;
            }
        } else {
            if(s_memcpy((char *)TftpAddr,(char *)data,count,0,0) != 0) {
                SendTFTPErr(ehdr,0,1,"Write error at 0x%lx",(ulong)TftpAddr);
                TftpCount = -1;
                return(0);
            }
            TftpAddr += count;
        }

        /* Check for transfer complete (count < TFTP_DATAMAX)... */
        if(count < TFTP_DATAMAX) {
            if(TftpTfsFname[0]) {
                char *fcomma, *icomma, *flags, *info;
                int err;

                /* If the transfer is complete and TftpTfsFname[0]
                 * is non-zero, then write the data to the specified
                 * TFS file... Note that a comma in the filename is
                 * used to find the start of (if any) the TFS flags
                 * string.  A second comma, marks the info field.
                 */
                info = (char *)0;
                flags = (char *)0;
                fcomma = strchr(TftpTfsFname,',');
                if(fcomma) {
                    icomma = strchr(fcomma+1,',');
                    if(icomma) {
                        *icomma = 0;
                        info = icomma+1;
                    }
                    if(tfsctrl(TFS_FATOB,(long)(fcomma+1),0) != -1) {
                        *fcomma = 0;
                        flags = fcomma+1;
                    } else {
                        SendTFTPErr(ehdr,0,1,"Invalid flag '%s'",
                                    TftpTfsFname);
                        TftpTfsFname[0] = 0;
                        break;
                    }
                }
#if INCLUDE_ETHERVERBOSE
                if((EtherVerbose & SHOW_TFTP_STATE) || (!MFLAGS_NOTFTPPRN())) {
                    printf("TFTP adding file: '%s' to TFS.\n",TftpTfsFname);
                }
#endif
                err = tfsadd(TftpTfsFname,info,flags,
                             (uchar *)getAppRamStart(),TftpCount+1-TftpChopCount);
                if(err != TFS_OKAY) {
                    SendTFTPErr(ehdr,0,1,"TFS err: %s",
                                (char *)tfsctrl(TFS_ERRMSG,err,0));
                }
                TftpTfsFname[0] = 0;
            } else {
                int cnt;
                char *addr;

                /* If the transfer is complete and no file add is to
                 * be done, then we flush d-cache and invalidate
                 * i-cache across the memory space that was just
                 * copied to.  This is necessary in case the
                 * binary data that was just transferred is code.
                 */
                cnt = TftpCount + 1;
                addr = (char *)TftpAddr - cnt;
                flushDcache(addr,cnt);
                invalidateIcache(addr,cnt);
            }
            if(!TftpGetActive) {
                shell_sprintf("TFTPRCV","%d",TftpCount+1);
            }
            tftpTransferComplete();
        }
        break;
    case ecs(TFTP_ACK):
        block = ecs(*(ushort *)(tftpp+2));
#if INCLUDE_ETHERVERBOSE
        if(EtherVerbose & SHOW_TFTP_STATE) {
            printf("  Rcvd TFTP_ACK (blk#%d)\n",block);
        }
#endif
        if((TftpState != TFTPACTIVE) && (TftpState != TFTPSENTWRQ)) {
            SendTFTPErr(ehdr,0,1,"Bad state (%s) for incoming TFTP_ACK",
                        tftpStringState(TftpState));
            return(0);
        }
        if(block == tftpPrevBlock) {
            if(TftpCount > TFTP_DATAMAX) {
                if(TftpState == TFTPACTIVE) {
                    TftpCount -= TFTP_DATAMAX;
                    TftpAddr += TFTP_DATAMAX;
                }
                SendTFTPData(ehdr,block+1,TftpAddr,TftpCount);
                if(TftpState == TFTPSENTWRQ) {
                    TftpCount -= TFTP_DATAMAX;
                    TftpAddr += TFTP_DATAMAX;
                }
                tftpPrevBlock++;
            } else if(TftpCount == TFTP_DATAMAX) {
                if(TftpState == TFTPACTIVE) {
                    TftpCount = 0;
                }
                SendTFTPData(ehdr,block+1,TftpAddr,TftpCount);
                if(TftpState == TFTPSENTWRQ) {
                    TftpCount = 0;
                }
                tftpPrevBlock++;
            } else {
                if(TftpState == TFTPSENTWRQ) {
                    if(TftpCount == -1) {
                        TftpCount = 0;
                        tftpGotoState(TFTPIDLE);
                        enableBroadcastReception();
                        tftpTransferComplete();
                    } else {
                        SendTFTPData(ehdr,block+1,TftpAddr,TftpCount);
                        TftpAddr += TftpCount;
                        TftpCount = -1;
                        tftpPrevBlock++;
                    }
                } else {
                    TftpAddr += TftpCount;
                    TftpCount = 0;
                    tftpGotoState(TFTPIDLE);
                    if(TftpLTMptr) {
                        free(TftpLTMptr);
                        TftpLTMptr = (char *)0;
                    }
                    enableBroadcastReception();
                    tftpTransferComplete();
                }
            }
        } else if(block == tftpPrevBlock-1) {
            SendTFTPData(ehdr,block+1,TftpAddr,TftpCount);
        } else {
#ifdef DONT_IGNORE_OUT_OF_SEQUENCE_BLOCKS
            SendTFTPErr(ehdr,0,1,"TFTP_ACK blockerr: rcvd %d, expected %d",
                        block,tftpPrevBlock-1);
            TftpCount = -1;
#endif
            return(0);
        }
        return(0);
    case ecs(TFTP_ERR):
        errcode = ecs(*(ushort *)(tftpp+2));
        errstring = tftpp+4;
#if INCLUDE_ETHERVERBOSE
        if(EtherVerbose & SHOW_TFTP_STATE) {
            printf("  Rcvd TFTP_ERR #%d (%s)\n",errcode,errstring);
        }
#endif
        TftpCount = -1;
        tftpGotoState(TFTPHOSTERROR);
        strncpy(TftpErrString,errstring,sizeof(TftpErrString)-1);
        TftpErrString[sizeof(TftpErrString)-1] = 0;
        return(0);
    default:
#if INCLUDE_ETHERVERBOSE
        if(EtherVerbose & SHOW_TFTP_STATE) {
            printf("  Rcvd <%04x> unknown TFTP opcode\n", opcode);
        }
#endif
        TftpCount = -1;
        return(-1);
    }
    SendTFTPAck(ehdr,block);
    return(0);
}

/* SendTFTPRRQ():
 *  Pass the ether and ip address of the TFTP server, along with the
 *  filename and mode to start up a target-initiated TFTP download.
 *  The initial TftpState value is TFTPSENTRRQ, this is done so that incoming
 *  TFTP_DAT packets can be verified...
 *   - If a TFTP_DAT packet is received and TftpState is TFTPSENTRRQ, then
 *     the block number should be 1.  If this is true, then that server's
 *     source port is stored away in TftpRmtPort so that all subsequent
 *     TFTP_DAT packets will be compared to the initial source port.  If no
 *     match, then respond with a TFTP error or ICMP PortUnreachable message.
 *   - If a TFTP_DAT packet is received and TftpState is TFTPSENTRRQ, then
 *     if the block number is not 1, generate a error.
 */
static int
SendTFTPRRQ(uchar *ipadd,uchar *eadd,char *filename,char *mode,uchar *loc)
{
    uchar *tftpdat;
    ushort ip_len;
    struct ether_header *te;
    struct ip *ti;
    struct Udphdr *tu;

    TftpChopCount = 0;
    tftpGotoState(TFTPSENTRRQ);
    TftpAddr = loc;
    TftpCount = 0;

    /* Retrieve an ethernet buffer from the driver and populate the
     * ethernet level of packet:
     */
    te = (struct ether_header *) getXmitBuffer();
    memcpy((char *)&te->ether_shost,(char *)BinEnetAddr,6);
    memcpy((char *)&te->ether_dhost,(char *)eadd,6);
    te->ether_type = ecs(ETHERTYPE_IP);

    /* Move to the IP portion of the packet and populate it appropriately: */
    ti = (struct ip *)(te + 1);
    ti->ip_vhl = IP_HDR_VER_LEN;
    ti->ip_tos = 0;
    ip_len = sizeof(struct ip) +
             sizeof(struct Udphdr) + strlen(filename) + strlen(mode) + 4;
    ti->ip_len = ecs(ip_len);
    ti->ip_id = ipId();
    ti->ip_off = 0;
    ti->ip_ttl = UDP_TTL;
    ti->ip_p = IP_UDP;
    memcpy((char *)&ti->ip_src.s_addr,(char *)BinIpAddr,4);
    memcpy((char *)&ti->ip_dst.s_addr,(char *)ipadd,4);

    /* Now udp... */
    tu = (struct Udphdr *)(ti + 1);
    tu->uh_sport = getTftpSrcPort();
    self_ecs(tu->uh_sport);
    tu->uh_dport = ecs(TftpPort);
    tu->uh_ulen = ecs((ushort)(ip_len - sizeof(struct ip)));

    /* Finally, the TFTP specific stuff... */
    tftpdat = (uchar *)(tu+1);
    *(ushort *)(tftpdat) = ecs(TFTP_RRQ);
    strcpy((char *)tftpdat+2,(char *)filename);
    strcpy((char *)tftpdat+2+strlen((char *)filename)+1,mode);

    if(!strcmp(mode,"netascii")) {
        TftpWrqMode = MODE_NETASCII;
    } else {
        TftpWrqMode = MODE_OCTET;
    }

    storePktAndSend(ti, te,TFTPACKSIZE+strlen(filename)+strlen(mode));

#if INCLUDE_ETHERVERBOSE
    if(EtherVerbose & SHOW_TFTP_STATE) {
        printf("\n  Sent TFTP_RRQ (file=%s)\n",filename);
    }
#endif

    return(0);
}


/* SendTFTPWRQ(): SSADDED
 *  Pass the ether and ip address of the TFTP server, along with the
 *  filename and mode to start up a target-initiated TFTP download.
 *  The initial TftpState value is TFTPSENTRRQ, this is done so that incoming
 *  TFTP_DAT packets can be verified...
 *   - If a TFTP_DAT packet is received and TftpState is TFTPSENTRRQ, then
 *     the block number should be 1.  If this is true, then that server's
 *     source port is stored away in TftpRmtPort so that all subsequent
 *     TFTP_DAT packets will be compared to the initial source port.  If no
 *     match, then respond with a TFTP error or ICMP PortUnreachable message.
 *   - If a TFTP_DAT packet is received and TftpState is TFTPSENTRRQ, then
 *     if the block number is not 1, generate a error.
 */
static int
SendTFTPWRQ(uchar *ipadd,uchar *eadd,char *filename,char *mode)
{
    uchar *tftpdat;
    ushort ip_len;
    struct ether_header *te;
    struct ip *ti;
    struct Udphdr *tu;

    TftpChopCount = 0;

    /* Retrieve an ethernet buffer from the driver and populate the
     * ethernet level of packet:
     */
    te = (struct ether_header *) getXmitBuffer();
    memcpy((char *)&te->ether_shost,(char *)BinEnetAddr,6);
    memcpy((char *)&te->ether_dhost,(char *)eadd,6);
    te->ether_type = ecs(ETHERTYPE_IP);

    /* Move to the IP portion of the packet and populate it appropriately: */
    ti = (struct ip *)(te + 1);
    ti->ip_vhl = IP_HDR_VER_LEN;
    ti->ip_tos = 0;
    ip_len = sizeof(struct ip) +
             sizeof(struct Udphdr) + strlen(filename) + strlen(mode) + 4;
    ti->ip_len = ecs(ip_len);
    ti->ip_id = ipId();
    ti->ip_off = 0;
    ti->ip_ttl = UDP_TTL;
    ti->ip_p = IP_UDP;
    memcpy((char *)&ti->ip_src.s_addr,(char *)BinIpAddr,4);
    memcpy((char *)&ti->ip_dst.s_addr,(char *)ipadd,4);

    /* Now udp... */
    tu = (struct Udphdr *)(ti + 1);
    tu->uh_sport = getTftpSrcPort();
    self_ecs(tu->uh_sport);
    tu->uh_dport = ecs(TftpPort);
    tu->uh_ulen = ecs((ushort)(ip_len - sizeof(struct ip)));

    /* Finally, the TFTP specific stuff... */
    tftpdat = (uchar *)(tu+1);
    *(ushort *)(tftpdat) = ecs(TFTP_WRQ);
    strcpy((char *)tftpdat+2,(char *)filename);
    strcpy((char *)tftpdat+2+strlen((char *)filename)+1,mode);

    if(!strcmp(mode,"netascii")) {
        TftpWrqMode = MODE_NETASCII;
    } else {
        TftpWrqMode = MODE_OCTET;
    }

    storePktAndSend(ti, te,TFTPACKSIZE+strlen(filename)+strlen(mode));

#if INCLUDE_ETHERVERBOSE
    if(EtherVerbose & SHOW_TFTP_STATE) {
        printf("\n  Sent TFTP_WRQ (file=%s)\n",filename);
    }
#endif

    tftpGotoState(TFTPSENTWRQ);
    return(0);
}
//ss END OF ADDED SendTRTPWRQ


/* SendTFTPAck():
 */
static int
SendTFTPAck(struct ether_header *re,ushort block)
{
    uchar *tftpdat;
    ushort ip_len;
    struct ether_header *te;
    struct ip *ti, *ri;
    struct Udphdr *tu, *ru;

    te = EtherCopy(re);

    ti = (struct ip *)(te + 1);
    ri = (struct ip *)(re + 1);
    ti->ip_vhl = ri->ip_vhl;
    ti->ip_tos = ri->ip_tos;
    ip_len = sizeof(struct ip) + sizeof(struct Udphdr) + 4;
    ti->ip_len = ecs(ip_len);
    ti->ip_id = ipId();
    ti->ip_off = ri->ip_off;
    ti->ip_ttl = UDP_TTL;
    ti->ip_p = IP_UDP;
    memcpy((char *)&(ti->ip_src.s_addr),(char *)&(ri->ip_dst.s_addr),
           sizeof(struct in_addr));
    memcpy((char *)&(ti->ip_dst.s_addr),(char *)&(ri->ip_src.s_addr),
           sizeof(struct in_addr));

    tu = (struct Udphdr *)(ti + 1);
    ru = (struct Udphdr *)(ri + 1);
    tu->uh_sport = ru->uh_dport;
    tu->uh_dport = ru->uh_sport;
    tu->uh_ulen = ecs((ushort)(ip_len - sizeof(struct ip)));

    tftpdat = (uchar *)(tu+1);
    *(ushort *)(tftpdat) = ecs(TFTP_ACK);
    *(ushort *)(tftpdat+2) = ecs(block);

    storePktAndSend(ti,te,TFTPACKSIZE);

#if INCLUDE_ETHERVERBOSE
    if(EtherVerbose & SHOW_TFTP_STATE) {
        printf("  Sent TFTP_ACK (blk#%d)\n",block);
    }
#endif
    return(0);
}

/* SendTFTPErr():
 */
static int
SendTFTPErr(struct ether_header *re,short errno,int changestate,char *fmt, ...)
{
    va_list argp;
    short len, tftplen, hdrlen;
    uchar *tftpmsg;
    struct ether_header *te;
    struct ip *ti, *ri;
    struct Udphdr *tu, *ru;
    static char errmsg[128];

    if(changestate) {
        tftpGotoState(TFTPERROR);
    }

    va_start(argp,fmt);
    vsnprintf(errmsg,sizeof(errmsg),fmt,argp);
    va_end(argp);

#if INCLUDE_ETHERVERBOSE
    if((EtherVerbose & SHOW_TFTP_STATE) || (!MFLAGS_NOTFTPPRN())) {
        printf("TFTP err: %s\n",errmsg);
    }
#endif

    tftplen = strlen(errmsg) + 1 + 4;
    hdrlen = sizeof(struct ip) + sizeof(struct Udphdr);
    len = tftplen + hdrlen ;

    te = EtherCopy(re);

    ti = (struct ip *)(te + 1);
    ri = (struct ip *)(re + 1);
    ti->ip_vhl = ri->ip_vhl;
    ti->ip_tos = ri->ip_tos;
    ti->ip_len = ecs(len);
    ti->ip_id = ipId();
    ti->ip_off = ri->ip_off;
    ti->ip_ttl = UDP_TTL;
    ti->ip_p = IP_UDP;
    memcpy((char *)&(ti->ip_src.s_addr),(char *)&(ri->ip_dst.s_addr),
           sizeof(struct in_addr));
    memcpy((char *)&(ti->ip_dst.s_addr),(char *)&(ri->ip_src.s_addr),
           sizeof(struct in_addr));

    tu = (struct Udphdr *)(ti + 1);
    ru = (struct Udphdr *)(ri + 1);
    tu->uh_sport = ru->uh_dport;
    tu->uh_dport = ru->uh_sport;
    tu->uh_ulen = sizeof(struct Udphdr) + tftplen;
    self_ecs(tu->uh_ulen);

    tftpmsg = (uchar *)(tu+1);
    *(ushort *)(tftpmsg) = ecs(TFTP_ERR);
    * (ushort *)(tftpmsg+2) = ecs(errno);
    strcpy((char *)tftpmsg+4,(char *)errmsg);

    storePktAndSend(ti,te,TFTPACKSIZE + strlen(errmsg) + 1);

#if INCLUDE_ETHERVERBOSE
    if(EtherVerbose & SHOW_TFTP_STATE) {
        printf("  Sent TFTP Err#%d (%s) \n",errno,errmsg);
    }
#endif

    return(0);
}

/* SendTFTPData():
 */
static int
SendTFTPData(struct ether_header *re,ushort block,uchar *data,int count)
{
    int len, tftplen, hdrlen;
    uchar *tftpmsg;
    struct ether_header *te;
    struct ip *ti, *ri;
    struct Udphdr *tu, *ru;

    if(count > TFTP_DATAMAX) {
        count = TFTP_DATAMAX;
    }

    tftplen = count + 2 + 2;    /* sizeof (data + opcode + blockno) */
    hdrlen = sizeof(struct ip) + sizeof(struct Udphdr);
    len = tftplen + hdrlen ;

    te = EtherCopy(re);

    ti = (struct ip *)(te + 1);
    ri = (struct ip *)(re + 1);
    ti->ip_vhl = ri->ip_vhl;
    ti->ip_tos = ri->ip_tos;
    ti->ip_len = ecs(len);
    ti->ip_id = ipId();
    ti->ip_off = ri->ip_off;
    ti->ip_ttl = UDP_TTL;
    ti->ip_p = IP_UDP;
    memcpy((char *)&(ti->ip_src.s_addr),(char *)&(ri->ip_dst.s_addr),
           sizeof(struct in_addr));
    memcpy((char *)&(ti->ip_dst.s_addr),(char *)&(ri->ip_src.s_addr),
           sizeof(struct in_addr));

    tu = (struct Udphdr *)(ti + 1);
    ru = (struct Udphdr *)(ri + 1);
    tu->uh_sport = ru->uh_dport;
    tu->uh_dport = ru->uh_sport;
    tu->uh_ulen = sizeof(struct Udphdr) + tftplen;
    self_ecs(tu->uh_ulen);

    tftpmsg = (uchar *)(tu+1);
    *(ushort *)(tftpmsg) = ecs(TFTP_DAT);
    *(ushort *)(tftpmsg+2) = ecs(block);
    memcpy((char *)tftpmsg+4,(char *)data,count);

    len+=sizeof(struct ether_header);

    storePktAndSend(ti,te,len);

#if INCLUDE_ETHERVERBOSE
    if(EtherVerbose & SHOW_TFTP_STATE)
        printf("  Sent TFTP data blk#%d (%d bytes @ 0x%lx) \n",
               block,count,(ulong)data);
#endif
    return(0);
}

#if 0
THIS IS NOT YET TESTED
/* parseLongFilename():
 * Given a full filename that may contain up to three different
 * comma-delimited tokens, this function will parse that string and
 * return the requested tokens.
 *
 * The syntax of fullname can be...
 *  - name
 *  - name,flags
 *  - name,flags,info
 *  - name,,info
 *
 * Return 0 if successful, -1 if failure.
 */
int
parseLongFilename(char *fullname,char *name, char *flags, char *info)
{
    char    *comma, *cp;

    /* Parse the fullname string looking for commas to determine what
     * information is in the string.  Then, based on the presence of
     * the commas, populate the incoming pointers with the appropriate
     * portion of the fullname string.
     * If the pointer is NULL, then just skip over that portion of the
     * algorithm.
     */
    cp = fullname;
    comma = strchr(cp,',');
    if(comma) {
        if(name) {
            while(cp != comma) {
                *name++ = *cp++;
            }
            *name = 0;
        }
        cp++;
        comma = strchr(cp,',');
        if(comma) {
            if(flags) {
                while(cp != comma) {
                    *flags++ = *cp++;
                }
                *flags = 0;
            }
            cp++;
            if(info) {
                strcpy(info,cp);
            }
        } else {
            if(flags) {
                strcpy(flags,cp);
            }
        }
    } else {
        if(name) {
            strcpy(name,fullname);
        }
    }
    return(0);
}
#endif

/* Tftp():
 *  Initiate a tftp transfer at the target (target is client).
 *  Command line:
 *      tftp [options] {IP} {get|put} {file|addr} [len]...
 *      tftp [options] {IP} get file dest_addr
 *      tftp [options] {IP} put addr dest_file len
 *  Currently, only "get" is supported.
 */

char *TftpHelp[] = {
    "Trivial file transfer protocol",
    "-[aF:f:i:vV] [on|off|IP] {get|put filename [addr]} ss",
#if INCLUDE_VERBOSEHELP
    " -a        use netascii mode",
    " -F {file} name of tfs file to copy to",
    " -f {flgs} file flags (see tfs)",
    " -i {info} file info (see tfs)",
    " -v        verbosity = ticker",
    " -V        verbosity = state",
#endif
    0,
};

int
Tftp(int argc,char *argv[])
{
    int     opt, verbose;
    char    *mode, *file, *info, *flags;
    ulong   addr;

    verbose = 0;
    file = (char *)0;
    info = (char *)0;
    flags = (char *)0;
    mode = "octet";
    while((opt=getopt(argc,argv,"aF:f:i:vV")) != -1) {
        switch(opt) {
        case 'a':
            mode = "netascii";
            break;
        case 'f':
            flags = optarg;
            break;
        case 'F':
            file = optarg;
            break;
        case 'i':
            info = optarg;
            break;
        case 'v':
            verbose |= SHOW_TFTP_TICKER;
            break;
        case 'V':
            verbose |= SHOW_TFTP_STATE;
            break;
        default:
            return(CMD_PARAM_ERROR);
        }
    }
    if(argc < (optind+1)) {
        return(CMD_PARAM_ERROR);
    }

    if(argc == optind+1) {
        if(!strcmp(argv[optind],"on")) {
            TftpTurnedOff = 0;
            TftpGetActive = 0;
            TftpPutActive = 0; //ss ADDED
        } else if(!strcmp(argv[optind],"off")) {
            TftpTurnedOff = 1;
            TftpGetActive = 0;
            tftpGotoState(TFTPIDLE);
            TftpPutActive = 0; //ss ADDED
        } else {
            return(CMD_PARAM_ERROR);
        }
        return(CMD_SUCCESS);
    }

    /* If either the info or flags field has been specified, but the */
    /* filename is not specified, error here... */
    if((info || flags) && (!file)) {
        printf("Filename missing\n");
        return(CMD_FAILURE);
    }

    if(!strcmp(argv[optind+1],"get")) {

        if(argc == optind+4) {
            addr = (ulong)strtol(argv[optind+3],0,0);
        } else if(argc == optind+3) {
            addr = getAppRamStart();
        } else {
            return(CMD_PARAM_ERROR);
        }

        TFTPVERBOSE(EtherVerbose |= verbose);
        tftpGet(addr,argv[optind],mode,argv[optind+2],file,flags,info);
        TFTPVERBOSE(EtherVerbose &= ~verbose);
    } else if(!strcmp(argv[optind+1],"put")) {

        if(argc == (optind+3)) {
            file = argv[optind+2];
        } else if(argc == (optind+4)) {
            file = argv[optind+3];
        } else {
            return(CMD_PARAM_ERROR);
        }

        TFTPVERBOSE(EtherVerbose |= verbose);
        tftpPut(argv[optind],mode,file,argv[optind+2],flags,info);
        TFTPVERBOSE(EtherVerbose &= ~verbose);

    } else {
        return(CMD_PARAM_ERROR);
    }

    return(CMD_SUCCESS);
}

void
ShowTftpStats(void)
{
    printf("Current TFTP state: %s\n",tftpStringState(TftpState));
}

#endif
