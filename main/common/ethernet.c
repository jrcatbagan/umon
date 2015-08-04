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
 * ethernet.c:
 *
 * This code supports most of the generic ethernet/IP/ARP/UDP stuff.
 *
 * Original author:     Ed Sutter (ed.sutter@alcatel-lucent.com)
 *
 */
#include "config.h"
#include "endian.h"
#include "stddefs.h"
#include "genlib.h"

#if INCLUDE_ETHERNET
#include "cpuio.h"
#include "ether.h"
#include "monflags.h"
#include "cli.h"
#include "timer.h"

void ShowEthernetStats(void);

#if INCLUDE_MONCMD
void    executeMONCMD(void);
void    processMONCMD(struct ether_header *,ushort);
int     SendIPMonChar(uchar,int);
char    IPMonCmdLine[CMDLINESIZE];
int     IPMonCmdVerbose;
int     IPMonCmdActive;         /* Set if MONCMD is in progress. */
#endif

#if INCLUDE_DHCPBOOT
#define dhcpStateCheck()    dhcpStateCheck()
#define dhcpDisable()       dhcpDisable()
#define ShowDhcpStats()     ShowDhcpStats()
#else
#define dhcpStateCheck()
#define dhcpDisable()
#define ShowDhcpStats()
#endif

#if INCLUDE_TFTP
#define tftpStateCheck()    tftpStateCheck()
#define tftpInit()          tftpInit()
#define ShowTftpStats()     ShowTftpStats()
#else
#define tftpStateCheck()
#define tftpInit()
#define ShowTftpStats()
#endif


#if INCLUDE_ETHERVERBOSE
int EtherVerbose;           /* Verbosity flag (see ether.h). */
#endif

char *Etheradd, *IPadd;     /* Pointers to ascii addresses */
uchar BinIpAddr[4];         /* Space for binary IP address */
uchar BinEnetAddr[6];       /* Space for binary MAC address */
int EtherPollingOff;        /* Non-zero if ethernet polling is off. */
int EtherIsActive;          /* Non-zero if ethernet is up. */
int EtherIPERRCnt;          /* Number of IP errors detected. */
int EtherUDPERRCnt;         /* Number of UDP errors detected. */
int EtherXFRAMECnt;         /* Number of packets transmitted. */
int EtherRFRAMECnt;         /* Number of packets received. */
int EtherPollNesting;       /* Incremented when pollethernet() is called. */
int MaxEtherPollNesting;    /* High-warter mark of EtherPollNesting. */
ushort  UniqueIpId;
ulong IPMonCmdHdrBuf[(sizeof(struct ether_header) + sizeof(struct ip) + sizeof(struct Udphdr) + 128)/(sizeof(ulong))];
struct  ether_header *IPMonCmdHdr;

/* AppPktPtr & AppPktLen:
 * These two values are used to allow the monitor's ethernet driver
 * to easily (not necessarily most efficiently) hook up to an application
 * that needs to be able to send and/or receive ethernet packets.
 * Refer to discussion above monRecvEnetPkt().
 */
char *AppPktPtr;
int AppPktLen;

/* Ports used by the monitor have defaults, but can be redefined using
 * shell variables:
 */
ushort  MoncmdPort;         /* shell var: MCMDPORT */
ushort  GdbPort;            /* shell var: GDBPORT */
ushort  DhcpClientPort;     /* shell var: DCLIPORT */
ushort  DhcpServerPort;     /* shell var: DSRVPORT */
ushort  TftpPort;           /* shell var: TFTPPORT */
ushort  TftpSrcPort;        /* shell var: TFTPPORT */

uchar BroadcastAddr[] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
uchar AllZeroAddr[]   = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

#if INCLUDE_ETHERVERBOSE
struct  pinfo {
    int pnum;
    char    *pname;
} protocols[] = {
    { IP_IP,        "IP" },
    { IP_ICMP,      "ICMP" },
    { IP_IGMP,      "IGMP" },
    { IP_GGP,       "GGP" },
    { IP_TCP,       "TCP" },
    { IP_PUP,       "PUP" },
    { IP_UDP,       "UDP" },
    { 0,0 },
};

struct enet_verbosity {
    char    letter;
    ulong   flags;
} enet_verbose_tbl[] = {
    { '0',  0 },
    { 'a',  SHOW_ARP | SHOW_BROADCAST },
    { 'c',  SHOW_BADCSUM },
    { 'C',  SHOW_BADCSUM | SHOW_BADCSUMV },
    { 'd',  SHOW_DHCP },
#if INCLUDE_GDB
    { 'g',  SHOW_GDB },
#endif
    { 'i',  SHOW_INCOMING },
    { 'I',  SHOW_INCOMING | SHOW_BROADCAST },
    { 'o',  SHOW_OUTGOING },
    { 'p',  SHOW_PHY },
    { 't',  SHOW_TFTP_STATE },
    { 'x',  SHOW_HEX },
    { 'X',  SHOW_HEX | SHOW_ASCII },
    { 0,0 }
};

int
SetEthernetVerbosity(char *letters)
{
    ulong   verbose = 0;
    struct  enet_verbosity *evp = enet_verbose_tbl;

    while(*letters) {
        evp = enet_verbose_tbl;
        while(evp->letter) {
            if(*letters == evp->letter) {
                verbose |= evp->flags;
                break;
            }
            evp++;
        }
        if(evp->letter == 0) {
            printf("Invalid verbosity: '%c'\n",*letters);
            return(CMD_PARAM_ERROR);
        }

        letters++;
    }
    EtherVerbose = verbose;
    return(CMD_SUCCESS);
}
#endif

char *EtherHelp[] = {
    "Ethernet interface",
    "-[d:pt:v:V] {cmd} [cmd args]",
#if INCLUDE_VERBOSEHELP
    "Options...",
    " -d {1|0}     driver debug mode (1=on)",
    " -p {1|0}     promiscuous mode (1=on)",
    " -t           self-test ethernet interface",
#if INCLUDE_ETHERVERBOSE
    " -v {flgs}    enable specific verbosity...",
    "    0: turn off verbosity",
    "    a: enable ARP trace",
    "    c: print csum errmsg",
    "    C: dump csum errpkt",
    "    d: enable DHCP trace",
#if INCLUDE_GDB
    "    g: enable GDB trace",
#endif
    "    i: incoming packets (minus broadcast)",
    "    I: incoming packets (including broadcast)",
    "    o: outgoing packets",
    "    p: phy r/w accesses",
    "    t: enable TFTP trace",
    "    x: enable hex dump (requires i,I or o)",
    "    X: same as 'x' plus ascii",
    " -V        full verbosity (same as -v Iodtx)",
#endif
    "",
    "Commands...",
    "   {on | off | mac | stat | {print I|i|O pkt len} | {psnd addr len}}",
#endif
    0
};


int
Ether(int argc,char *argv[])
{
    int opt;

    /* Automatically turn polling on if this command is issued.
     */
    EtherPollingOff = 0;

    while((opt=getopt(argc,argv,"d:p:s:tv:V")) != -1) {
        switch(opt) {
        case 'p':
            if(*optarg == '1') {
                enablePromiscuousReception();
            } else {
                disablePromiscuousReception();
            }
            return(CMD_SUCCESS);
        case 't':
            if(EtherIsActive == 0) {
                printf("Selftest requires active driver to run\n");
                return(CMD_FAILURE);
            }
            enselftest(1);
            return(CMD_SUCCESS);
#if INCLUDE_ETHERVERBOSE
        case 'd':
            if(*optarg == '1') {
                EtherVerbose |= SHOW_DRIVER_DEBUG;
            } else {
                EtherVerbose &= ~SHOW_DRIVER_DEBUG;
            }
            return(CMD_SUCCESS);
        case 'V':
            EtherVerbose = SHOW_ALL;
            return(CMD_SUCCESS);
        case 'v':
            return(SetEthernetVerbosity(optarg));
#endif
        default:
            return(CMD_PARAM_ERROR);
        }
    }

    if(argc <= optind) {
        return(CMD_SUCCESS);
    }

    if(!strcmp(argv[optind],"off")) {
        enreset();
        EtherIsActive = 0;
        return(CMD_SUCCESS);
    }
#if INCLUDE_ETHERVERBOSE
    else if(!strcmp(argv[optind],"print")) {
        if(argc == optind+4) {
            int len, mode;
            ulong overbose;
            struct ether_header *pkt;

            overbose = EtherVerbose;
            switch(argv[optind+1][0]) {
            case 'O':
                mode = ETHER_OUTGOING;
                EtherVerbose = SHOW_ALL;
                break;
            case 'I':
                mode = ETHER_INCOMING;
                EtherVerbose = SHOW_ALL;
                break;
            case 'i':
                mode = ETHER_INCOMING;
                EtherVerbose = (SHOW_ALL & ~SHOW_BROADCAST);
                break;
            default:
                return(CMD_PARAM_ERROR);
            }
            pkt = (struct ether_header *)strtol(argv[optind+2],0,0);
            len = (int)strtol(argv[optind+3],0,0);
            printPkt(pkt,len,mode);
            EtherVerbose = overbose;
            return(CMD_SUCCESS);
        } else {
            return(CMD_PARAM_ERROR);
        }
    }
#endif
    else if(!strcmp(argv[optind],"mac")) {
        storeMac(1);
        return(CMD_SUCCESS);
    } else if(!strcmp(argv[optind],"psnd")) {
        ulong addr, len;
        uchar *buf;
        addr = strtol(argv[optind+1],0,0);
        len = strtol(argv[optind+2],0,0);
        buf = getXmitBuffer();
        memcpy((char *)buf,(char *)addr,(int)len);
        sendBuffer(len);
        return(CMD_SUCCESS);
    } else if(!strcmp(argv[optind],"stat")) {
        ShowEthernetStats();
        ShowEtherdevStats();
        ShowDhcpStats();
        ShowTftpStats();
        return(CMD_SUCCESS);
    } else if(strcmp(argv[optind],"on")) {
        return(CMD_PARAM_ERROR);
    }

#if INCLUDE_ETHERVERBOSE
    EthernetStartup(EtherVerbose,0);
#else
    EthernetStartup(0,0);
#endif
    return(CMD_SUCCESS);
}

void
ShowEthernetStats(void)
{
    printf("Ethernet interface currently %sabled.\n",
           EtherIsActive ? "en" : "dis");
    printf("Transmitted frames:      %d\n",EtherXFRAMECnt);
    printf("Received frames:         %d\n",EtherRFRAMECnt);
    printf("IP hdr cksum errors:     %d\n",EtherIPERRCnt);
    printf("UDP pkt cksum errors:    %d\n",EtherUDPERRCnt);
    printf("Max pollethernet nest:   %d\n",MaxEtherPollNesting);
}

/* DisableEthernet():
 * Shut down the interface, and return the state of the
 * interface prior to forcing the shut down.
 */
int
DisableEthernet(void)
{
    int eia;

    eia = EtherIsActive;
    EtherIsActive = 0;
#if INCLUDE_MONCMD
    IPMonCmdActive = 0;
#endif
    DisableEtherdev();
    return(eia);
}

/* EthernetWaitforLinkup():
 * If the ENET_LINK_IS_UP() macro is defined, then use it to poll
 * the just-initialized ethernet link so that we don't return from
 * this point until the port is active.
 * Return:
 *   0  if no macro is defined or poll is aborted
 *   1  if link is up
 *   -1 if link is not up
 */
int
EthernetWaitforLinkup(void)
{
    int linkup = 0;

#ifdef ENET_LINK_IS_UP
#ifndef LINKUP_TICK_MAX
#define LINKUP_TICK_MAX 15
#endif
    extern int ENET_LINK_IS_UP(void);

    int tick;
    struct elapsed_tmr tmr;

    if(getenv("ETHERNET_NOWAIT")) {
        return(0);
    }

    for(tick=0; tick<LINKUP_TICK_MAX; tick++) {
        startElapsedTimer(&tmr,500);
        while(1) {
            if(gotachar()) {
                goto done;
            }

            if(ENET_LINK_IS_UP()) {
                linkup = 1;
                goto done;
            }
            if(msecElapsed(&tmr)) {
                if(tick == 0) {
                    tick++;
                    printf("Wait for enet link up (hit-a-key to abort) ");
                } else {
                    putchar('.');
                }
                break;
            }
        }
    }
    if(tick == LINKUP_TICK_MAX) {
        linkup = -1;
        printf(" give up.");
        DisableEthernet();
    }

done:
    putchar('\n');
#endif

    return(linkup);
}

int
EthernetStartup(int verbose, int justreset)
{
    /* Initialize the retransmission delay calculator: */
    RetransmitDelay(DELAY_INIT_DHCP);

    EtherIPERRCnt = 0;
    EtherXFRAMECnt = 0;
    EtherRFRAMECnt = 0;
    EtherUDPERRCnt = 0;
#if INCLUDE_MONCMD
    IPMonCmdActive = 0;
#endif
    EtherPollNesting = 0;
    MaxEtherPollNesting = 0;
    DHCPState = DHCPSTATE_NOTUSED;
#if INCLUDE_ETHERVERBOSE
    if(getenv("ETHERNET_DEBUG")) {
        EtherVerbose |= SHOW_DRIVER_DEBUG;
    } else {
        EtherVerbose = 0;
    }
#endif

    /* Setup all the IP addresses used by the monitor... */
    if(getAddresses() == -1) {
        return(-1);
    }

    /* Call device specific startup code: */
    if(EtherdevStartup(verbose) < 0) {
        return(-1);
    }

    /* Initialize some TFTP state... */
    tftpInit();

#if INCLUDE_DHCPBOOT
    /* If EthernetStartup is called as a result of anything other than a
     * target reset, don't startup any DHCP/BOOTP transaction...
     */
    if(!justreset) {
        dhcpDisable();
    }
#endif
    EtherIsActive = 1;
    EtherPollingOff = 0;

    /* Wait for link up state before continuing...
     */
    EthernetWaitforLinkup();

    /* Issue a gratuitous ARP to let other devices on the net know we're
     * here, and also to make sure some other device isn't already using
     * the IP address that this target is using...
     */
    sendGratuitousArp();

    return(0);
}

/* pollethernet():
 *  Called at a few critical points in the monitor code to poll the
 *  ethernet device and keep track of the state of DHCP and TFTP.
 */
int
pollethernet(void)
{
    int pcnt;

    if((!EtherIsActive) || EtherPollingOff || (EtherPollNesting > 4)) {
        return(0);
    }

    EtherPollNesting++;
    if(EtherPollNesting > MaxEtherPollNesting) {
        MaxEtherPollNesting = EtherPollNesting;
    }

    pcnt = polletherdev();
#if INCLUDE_MONCMD
    if(IPMonCmdLine[0] != 0) {
        executeMONCMD();
    }
#endif

    dhcpStateCheck();
    tftpStateCheck();

    EtherPollNesting--;
    return(pcnt);
}

/* getAddresses():
 * Try getting ether/ip addresses from environment.
 * If not there, try getting them from some target-specific interface.
 * If not there, then get them from raw flash.
 * If not there, just use the hard-coded default.
 * Also, load all port numbers from shell variables, else default.
 *
 * Discussion regarding etheraddr[]...
 * The purpose of this array is to provide a point in flash that is
 * initialized to 0xff by the code (see reset.s).  This then allows some
 * other mechanism (storeMAC() or bed of nails, etc..) to program this
 * location to some non-0xff value.  This allows the base monitor image to
 * be common, but then be modified by other code or external hardware.
 */

int
getAddresses(void)
{
    char    *gdbPort, *dcliPort, *dsrvPort, *tftpPort;

    /* Set up port numbers: */
    gdbPort  = getenv("GDBPORT");
    dcliPort = getenv("DCLIPORT");
    dsrvPort = getenv("DSRVPORT");
    tftpPort = getenv("TFTPPORT");

#if INCLUDE_MONCMD
    {
        char *mcmdPort = getenv("MCMDPORT");
        if(mcmdPort) {
            MoncmdPort = (ushort)strtol(mcmdPort,0,0);
        } else {
            MoncmdPort = IPPORT_MONCMD;
        }
    }
#endif

    if(gdbPort) {
        GdbPort = (ushort)strtol(gdbPort,0,0);
    } else {
        GdbPort = IPPORT_GDB;
    }
    if(dcliPort) {
        DhcpClientPort = (ushort)strtol(dcliPort,0,0);
    } else {
        DhcpClientPort = IPPORT_DHCP_CLIENT;
    }
    if(dsrvPort) {
        DhcpServerPort = (ushort)strtol(dsrvPort,0,0);
    } else {
        DhcpServerPort = IPPORT_DHCP_SERVER;
    }
    if(tftpPort) {
        TftpPort = (ushort)strtol(tftpPort,0,0);
    } else {
        TftpPort = IPPORT_TFTP;    /* 69	*/
    }
    TftpSrcPort = IPPORT_TFTPSRC;           /* 8888 */

    /* Retrieve MAC address and store in shell variable ETHERADD...
     * First see if the shell variable is already loaded.
     * If not see if some target-specific interface has it.
     * If not see if the the string is stored in raw flash (usually this
     *   storage is initialized in reset.s of the target-specific code).
     * Finally, as a last resort, use the default set up in config.h.
     */
    if(!(Etheradd = getenv("ETHERADD"))) {
        if(!(Etheradd = extGetEtherAdd())) {
#if INCLUDE_FLASH
            if(etheraddr[0] != 0xff) {
                Etheradd = (char *)etheraddr;
            } else
#endif
                Etheradd = DEFAULT_ETHERADD;
        }
        setenv("ETHERADD",Etheradd);
    }

    /* Apply the same logic as above to the IP address... */
    if(!(IPadd = getenv("IPADD"))) {
        if(!(IPadd = extGetIpAdd())) {
            IPadd = DEFAULT_IPADD;
        }
        setenv("IPADD",IPadd);
    }

    /* Convert addresses to binary:
     */
    if(EtherToBin(Etheradd,BinEnetAddr) < 0) {
        return(-1);
    }

    /* If the ethernet address is 0:0:0:0:0:0, then we
     * return an error here so that the interface is not
     * brought up.
     */
    if(memcmp((char *)BinEnetAddr, (char *)AllZeroAddr,6) == 0) {
        static int firsttime;

        if(firsttime == 0) {
            printf("\nNULL MAC address, network interface disabled.\n");
            firsttime = 1;
        }
        return(-1);
    }

#if INCLUDE_DHCPBOOT
    if(DhcpIPCheck(IPadd) == -1) {
        return(-1);
    }
#else
    if(IpToBin(IPadd,BinIpAddr) < 0) {
        return(-1);
    }
#endif
    /* Initialize a unique number based on MAC: */
    UniqueIpId = xcrc16(BinEnetAddr,6);

    return(0);
}

/* processPACKET():
 *  This is the top level of the message processing after a complete
 *  packet has been received over ethernet.  It's all just a lot of
 *  parsing to determine whether the message is for this board's IP
 *  address (broadcast reception may be enabled), and the type of
 *  incoming protocol.  Once that is determined, the packet is either
 *  processed (TFTP, DHCP, ARP, ICMP-ECHO, etc...) or discarded.
 */
void
processPACKET(struct ether_header *ehdr, ushort size)
{
    int     i, udpdone;
    ushort  *datap, udpport;
    ulong   csum;
    struct ip *ihdr;
    struct Udphdr *uhdr;

    WATCHDOG_MACRO;

    /* If source MAC address is this board, then assume the MAC is in
     * full-duplex mode and we received our own outgoing broadcast
     * message (i.e. ignore it)...
     */
    if(!memcmp((char *)&(ehdr->ether_shost),(char *)BinEnetAddr,6)) {
        return;
    }

    printPkt(ehdr,size,ETHER_INCOMING);

    /* AppPktPtr is used by monRecvEnetPkt() so that an application can
     * use the monitor's ethernet driver.  For more info, refer to notes
     * above the monRecvEnetPkt() function.
     */
    if(AppPktPtr) {
        memcpy(AppPktPtr,(char *)ehdr,size > AppPktLen ? AppPktLen : size);
        AppPktPtr = 0;
        AppPktLen = size;
        return;
    }

    EtherRFRAMECnt++;

    if(ehdr->ether_type == ecs(ETHERTYPE_ARP)) {
        processARP(ehdr,size);
        return;
    } else if(ehdr->ether_type == ecs(ETHERTYPE_REVARP)) {
        processRARP(ehdr,size);
        return;
    } else if(ehdr->ether_type != ecs(ETHERTYPE_IP)) {
        return;
    }

    ihdr = (struct ip *)(ehdr + 1);

    /* If not version # 4, return now... */
    if(getIP_V(ihdr->ip_vhl) != 4) {
        return;
    }

#if INCLUDE_RARPIPASSIGN
    /* If destination MAC address matches ours, and our IP address is
     * 0.0.0.0, and this is an ICMP request, then this may be a reverse
     * ARP, so we allow this packet through...
     * Note that this logic is only applicable if DHCP is NOT running.
     */
    if(DHCPState == DHCPSTATE_NOTUSED) {
        if(!memcmp((char *)&(ehdr->ether_dhost),(char *)BinEnetAddr,6) &&
                (BinIpAddr[0] == 0) && (BinIpAddr[1] == 0) &&
                (BinIpAddr[2] == 0) && (BinIpAddr[3] == 0) &&
                (ihdr->ip_p == IP_ICMP)) {
            goto skipIPAddrFilter;
        }
    }
#endif

    /* IP address filtering:
     * At this point, the only packets accepted are those destined for this
     * board's IP address or broadcast to the subnet, plus DHCP, if active,
     */
    if(memcmp((char *)&(ihdr->ip_dst),(char *)BinIpAddr,4)) {
        long net_mask, sub_net_addr;

#if INCLUDE_DNS
        if(memcmp((char *)&(ihdr->ip_dst),(char *)mDNSIp,4)) {
#endif
            GetBinNetMask((uchar *) &net_mask);
            sub_net_addr = ihdr->ip_dst.s_addr & ~net_mask; /* x.x.x.255 */
            uhdr = (struct Udphdr *)(ihdr+1);

            if(ihdr->ip_p != IP_UDP
                    || ecs(uhdr->uh_dport) != MoncmdPort
                    || sub_net_addr != ~net_mask) {
#if INCLUDE_DHCPBOOT
                if(DHCPState == DHCPSTATE_NOTUSED) {
                    return;
                }
                if(ihdr->ip_p != IP_UDP) {
                    return;
                }
                uhdr = (struct Udphdr *)(ihdr+1);
                if(uhdr->uh_dport != ecs(DhcpClientPort)) {
                    return;
                }
#else
                return;
#endif
            }
#if INCLUDE_DNS
        }
#endif
    }

#if INCLUDE_RARPIPASSIGN
skipIPAddrFilter:
#endif

    /* Verify incoming IP header checksum...
     * Refer to section 3.2 of TCP/IP Illustrated, Vol 1 for details.
     */
    csum = 0;
    datap = (ushort *) ihdr;
    for(i=0; i<(sizeof(struct ip)/sizeof(ushort)); i++,datap++) {
        csum += *datap;
    }
    csum = (csum & 0xffff) + (csum >> 16);
    if(csum != 0xffff) {
        EtherIPERRCnt++;
#if INCLUDE_ETHERVERBOSE
        if(EtherVerbose & SHOW_BADCSUM) {
            printf("IP csum error: 0x%04x != 0xffff\n",(ushort)csum);
            if(EtherVerbose & SHOW_BADCSUMV) {
                int overbose = EtherVerbose;

                EtherVerbose = SHOW_ALL;
                printPkt(ehdr,size,ETHER_INCOMING);
                EtherVerbose = overbose;
            }
        }
#endif
        return;
    }

#if INCLUDE_ICMP
    if(ihdr->ip_p == IP_ICMP) {
        processICMP(ehdr,size);
        return;
    } else
#endif
        if(ihdr->ip_p == IP_TCP) {
            processTCP(ehdr,size);
            return;
        } else if(ihdr->ip_p != IP_UDP) {

#if INCLUDE_ICMP
            SendICMPUnreachable(ehdr,ICMP_UNREACHABLE_PROTOCOL);
#endif
#if INCLUDE_ETHERVERBOSE
            {
                int j;

                if(!(EtherVerbose & SHOW_INCOMING)) {
                    return;
                }
                for(j=0; protocols[j].pname; j++) {
                    if(ihdr->ip_p == protocols[j].pnum) {
                        printf("%s not supported\n",
                               protocols[j].pname);
                        return;
                    }
                }
            }
#endif
            printf("<%02x> protocol unrecognized\n", ihdr->ip_p);
            return;
        }

    uhdr = (struct Udphdr *)(ihdr+1);

    /* If non-zero, verify incoming UDP packet checksum...
     * Refer to section 11.3 of TCP/IP Illustrated, Vol 1 for details.
     */
    if(uhdr->uh_sum) {
        int     len;
        struct  UdpPseudohdr    pseudohdr;

        memcpy((char *)&pseudohdr.ip_src.s_addr,(char *)&ihdr->ip_src.s_addr,4);
        memcpy((char *)&pseudohdr.ip_dst.s_addr,(char *)&ihdr->ip_dst.s_addr,4);
        pseudohdr.zero = 0;
        pseudohdr.proto = ihdr->ip_p;
        pseudohdr.ulen = uhdr->uh_ulen;

        csum = 0;
        datap = (ushort *) &pseudohdr;
        for(i=0; i<(sizeof(struct UdpPseudohdr)/sizeof(ushort)); i++) {
            csum += *datap++;
        }

        /* If length is odd, pad and add one. */
        len = ecs(uhdr->uh_ulen);
        if(len & 1) {
            uchar   *ucp;
            ucp = (uchar *)uhdr;
            ucp[len] = 0;
            len++;
        }
        len >>= 1;

        datap = (ushort *) uhdr;
        for(i=0; i<len; i++) {
            csum += *datap++;
        }
        csum = (csum & 0xffff) + (csum >> 16);
        if(csum != 0xffff) {
            EtherUDPERRCnt++;
#if INCLUDE_ETHERVERBOSE
            if(EtherVerbose & SHOW_BADCSUM) {
                printf("UDP csum error: 0x%04x != 0xffff\n",(ushort)csum);
                if(EtherVerbose & SHOW_BADCSUMV) {
                    int overbose = EtherVerbose;

                    EtherVerbose = SHOW_ALL;
                    printPkt(ehdr,size,ETHER_INCOMING);
                    printf("pseudohdr.ip_src: 0x%08lx\n",
                           pseudohdr.ip_src.s_addr);
                    printf("pseudohdr.ip_dst: 0x%08lx\n",
                           pseudohdr.ip_dst.s_addr);
                    printf("pseudohdr.zero: 0x%02x\n", pseudohdr.zero);
                    printf("pseudohdr.proto: 0x%02x\n", pseudohdr.proto);
                    printf("pseudohdr.ulen: 0x%04x\n", pseudohdr.ulen);
                    EtherVerbose = overbose;
                }
            }
#endif
            return;
        }
    }
    udpport = ecs(uhdr->uh_dport);
    udpdone = 0;

#if INCLUDE_MONCMD
    if(!udpdone && (udpport == MoncmdPort)) {
        processMONCMD(ehdr,size);
        udpdone = 1;
    }
#endif
#if INCLUDE_DHCPBOOT
    if(!udpdone && (udpport == DhcpClientPort)) {
        processDHCP(ehdr,size);
        udpdone = 1;
    }
#endif
#if INCLUDE_TFTP
    if(!udpdone && ((udpport == TftpPort) || (udpport == TftpSrcPort))) {
        processTFTP(ehdr,size);
        udpdone = 1;
    }
#endif
#if INCLUDE_DNS
    if(!udpdone && (udpport == DnsPort)) {
        processDNS(ehdr,size);
        udpdone = 1;
    }
    if(!udpdone && (ecl(ihdr->ip_dst.s_addr) == DNSMCAST_IP) &&
            (udpport == DNSMCAST_PORT)) {
        processMCASTDNS(ehdr,size);
        udpdone = 1;
    }
#endif
#if INCLUDE_GDB
    if(!udpdone && (udpport == GdbPort)) {
        processGDB(ehdr,size);
        udpdone = 1;
    }
#endif
    if(!udpdone) {
#if INCLUDE_ETHERVERBOSE
        if(EtherVerbose & SHOW_INCOMING) {
            uchar *cp;
            cp = (uchar *)&(ihdr->ip_src);
            printf("  Unexpected IP pkt from %d.%d.%d.%d ",
                   cp[0],cp[1],cp[2],cp[3]);
            printf("(sport=0x%x,dport=0x%x)\n",
                   ecs(uhdr->uh_sport),ecs(uhdr->uh_dport));
        }
#endif
#if INCLUDE_ICMP
        SendICMPUnreachable(ehdr,ICMP_UNREACHABLE_PORT);
#endif
    }
}

#if INCLUDE_MONCMD
#define MONCMD_SRCIP_VARNAME    "MONCMD_SRCIP"
#define MONCMD_SRCPORT_VARNAME  "MONCMD_SRCPORT"

/* processMONCMD():
 * This function is called as a result of receiving a packet on port
 * 777.  It will process the incoming packet as if it was an ASCII
 * command destined for MicroMonitor's CLI.
 *
 * As of Aug 16, 2004, support for NETCAT is also in this function.
 * Differentiating between netcat and moncmd is done by looking
 * for the terminating newline character which is only present with
 * netcat.  For example:
 * The following host command line:  <netcat -u 135.222.140.72 777>
 * puts the netcat user into an interactive mode with uMon so
 * normal uMon commands can be issued through netcat (until interrupted).
 *
 * As of Aug 5, 2005, another change has been made to uMon's MONCMD
 * server as a result of a bug reported by Leon Pollack.  The change
 * breaks up the processing of the incoming moncmd request into two
 * parts processMONCMD() and executeMONCMD() (refer to CVS log for
 * more details):
 *
 * 1. processMONCMD():
 *    Retrieve the incoming message from the ethernet interface and
 *    store the message in a local buffer to be processed later.
 * 2. executeMONCMD():
 *    After the ethernet packet has been properly dequeued, then
 *    process the remote command appropriately.
 */

void
processMONCMD(struct ether_header *ehdr,ushort size)
{
    int     verbose = 0, doitnow = 0;
    struct  ip *ihdr;
    struct  Udphdr *uhdr;
    char    *moncmd;
    uchar   *src;

    if(size > sizeof(IPMonCmdHdrBuf)) {
        return;
    }

    ihdr = (struct ip *)(ehdr + 1);
    uhdr = (struct Udphdr *)(ihdr + 1);
    moncmd = (char *)(uhdr + 1);
    memcpy((char *)IPMonCmdHdrBuf,(char *)ehdr,size);
    IPMonCmdHdr = (struct ether_header *)&IPMonCmdHdrBuf;
    src = (uchar *)&ihdr->ip_src;

    /* Keep track of who sent the most recent moncmd request:
     */
    shell_sprintf(MONCMD_SRCIP_VARNAME,"%d.%d.%d.%d",
                  src[0],src[1],src[2],src[3]);
    shell_sprintf(MONCMD_SRCPORT_VARNAME,"%d",ecs(uhdr->uh_sport));

    if(!MFLAGS_NOMONCMDPRN()) {
        printf("MONCMD (from %s): ",getenv(MONCMD_SRCIP_VARNAME));
        puts(moncmd);
        verbose = 1;
    }

    if(strlen(moncmd) >= (sizeof(IPMonCmdLine) - 2)) {
        printf("MONCMD (from %s): too long\n",getenv(MONCMD_SRCIP_VARNAME));
        return;
    }

    /* A leading '.' tells the moncmd server to execute the command now,
     * not after the pollethernet queue has been emptied...
     */
    if(*moncmd == '.') {
        moncmd++;
        doitnow = 1;
    }

    strcpy(IPMonCmdLine+1,moncmd);
    IPMonCmdLine[0] = '+';
    IPMonCmdVerbose = verbose;

    if(doitnow) {
        executeMONCMD();
    }
}

void
executeMONCMD(void)
{
    char    *ncnl;          /* netcat newline */
    char    *moncmd;

    /* Clear the initial '+' character so that this function is
     * never executed multiple times because of calls to
     * polletherdev() during the MONCMD transaction.
     */
    IPMonCmdLine[0] = 0;

    /* If the first character of the incoming command is an '@', then
     * the response is not sent back to the client...
     */
    moncmd = IPMonCmdLine + 1;
    if(*moncmd == '@') {
        IPMonCmdActive = 0;
        moncmd++;
    } else {
        IPMonCmdActive = 1;
    }

    /* Added to support netcat...
     */
    ncnl = strchr(moncmd,0x0a);
    if(ncnl) {
        *ncnl = 0;
    }

    docommand(moncmd,IPMonCmdVerbose);

    if(ncnl) {
        writeprompt();
    }

    if(IPMonCmdActive) {
        SendIPMonChar(0,1);
        IPMonCmdActive = 0;
    }

    stkchk("Post-sendIPmonchar");
    if(!ncnl) {
        writeprompt();
    }

    IPMonCmdLine[0] = 0;
}

int
SendIPMonChar(uchar c, int done)
{
    static  int idx;
    static  char linebuf[128];
    int len, hdrlen;
    struct ether_header *te;
    struct ip *ti, *ri;
    struct Udphdr *tu, *ru;

    if(!IPMonCmdActive) {
        return(0);
    }

    /* Check for overflow and if detected, reset the buffer pointer...
     */
    if(idx >= sizeof(linebuf)) {
        idx = 0;
    }

    linebuf[idx++] = c;

    if((idx < sizeof(linebuf)) && (!done) && (c != '\n')) {
        return(0);
    }

    /* Once inside the meat of this function, clear the IPMonCmdActive flag
     * to avoid recursion if an error message is to be printed by some
     * called by this function...
     */
    IPMonCmdActive = 0;

    hdrlen = sizeof(struct ip) + sizeof(struct Udphdr);
    len = idx + hdrlen ;

    te = EtherCopy(IPMonCmdHdr);

    ti = (struct ip *)(te + 1);
    ri = (struct ip *)(IPMonCmdHdr + 1);
    ti->ip_vhl = ri->ip_vhl;
    ti->ip_tos = ri->ip_tos;
    ti->ip_len = ecs(len);
    ti->ip_id = ipId();
    ti->ip_off = ri->ip_off;
    ti->ip_ttl = UDP_TTL;
    ti->ip_p = IP_UDP;
    memcpy((char *)&(ti->ip_src.s_addr),(char *)BinIpAddr,
           sizeof(struct in_addr));
    memcpy((char *)&(ti->ip_dst.s_addr),(char *)&(ri->ip_src.s_addr),
           sizeof(struct in_addr));

    tu = (struct Udphdr *)(ti + 1);
    ru = (struct Udphdr *)(ri + 1);
    tu->uh_sport = ru->uh_dport;
    tu->uh_dport = ru->uh_sport;
    tu->uh_ulen = ecs((ushort)(sizeof(struct Udphdr) + idx));
    memcpy((char *)(tu+1),linebuf,idx);

    ipChksum(ti);       /* Compute checksum of ip hdr */
    udpChksum(ti);      /* Compute UDP checksum */

    sendBuffer(MONRESPSIZE);
    idx = 0;
    IPMonCmdActive = 1;
    return(1);
}
#endif

#if INCLUDE_ETHERVERBOSE
/*
 *  printPkt(ehdr,len)
 */
void
printPkt(struct ether_header *ehdr, int len, int direction)
{
    struct  arphdr *arpp;
    char    *dir;

    /* Filter based on verbosity level... */
    switch(direction) {
    case ETHER_INCOMING:
        if(!(EtherVerbose & SHOW_INCOMING)) {
            return;
        }
        dir = "INCOMING";
        break;
    case ETHER_OUTGOING:
        if(!(EtherVerbose & SHOW_OUTGOING)) {
            return;
        }
        dir = "OUTGOING";
        break;
    default:
        printf("printPkt() direction error\n");
        dir = "???";
        break;
    }

    /* If direction is incoming and SHOW_BROADCAST is not set, then */
    /* return here if the destination host is broadcast. */
    if((direction == ETHER_INCOMING) &&
            (!(EtherVerbose & SHOW_BROADCAST)) &&
            (!memcmp((char *)ehdr->ether_dhost.ether_addr_octet,(char *)BroadcastAddr,6))) {
        return;
    }

    printf("\n%s PACKET (%d bytes):\n",dir,len);
    if(EtherVerbose & SHOW_HEX) {
        printMem((uchar *)ehdr,len,EtherVerbose & SHOW_ASCII);
    }
    printf("  Destination Host = %02x:%02x:%02x:%02x:%02x:%02x\n",
           ehdr->ether_dhost.ether_addr_octet[0],
           ehdr->ether_dhost.ether_addr_octet[1],
           ehdr->ether_dhost.ether_addr_octet[2],
           ehdr->ether_dhost.ether_addr_octet[3],
           ehdr->ether_dhost.ether_addr_octet[4],
           ehdr->ether_dhost.ether_addr_octet[5]);

    printf("  Source Host =      %02x:%02x:%02x:%02x:%02x:%02x\n",
           ehdr->ether_shost.ether_addr_octet[0],
           ehdr->ether_shost.ether_addr_octet[1],
           ehdr->ether_shost.ether_addr_octet[2],
           ehdr->ether_shost.ether_addr_octet[3],
           ehdr->ether_shost.ether_addr_octet[4],
           ehdr->ether_shost.ether_addr_octet[5]);


    switch(ehdr->ether_type) {
    case ecs(ETHERTYPE_IP):
        printIp((struct ip *)(ehdr+1));
        break;
    case ecs(ETHERTYPE_PUP):
        printf("  Type = PUP\n");
        break;
    case ecs(ETHERTYPE_ARP):
        arpp = (struct arphdr *)(ehdr+1);
        printf("  Type = ARP %s from IP %d.%d.%d.%d to %d.%d.%d.%d)\n",
               arpp->operation == ecs(ARP_RESPONSE) ? "RESPONSE" : "REQUEST",
               arpp->senderia[0],arpp->senderia[1],
               arpp->senderia[2],arpp->senderia[3],
               arpp->targetia[0],arpp->targetia[1],
               arpp->targetia[2],arpp->targetia[3]);
        break;
    case ecs(ETHERTYPE_REVARP):
        printf("  Type = REVARP\n");
        break;
    default:
        printf("  Type = 0x%04x ???\n", ehdr->ether_type);
        break;
    }
}

void
AppPrintPkt(char *buf, int size, int incoming)
{
    int overbose, mode;

    overbose = EtherVerbose;
    if(incoming) {
        EtherVerbose = SHOW_ALL & ~SHOW_OUTGOING;
        mode = ETHER_INCOMING;
    } else {
        EtherVerbose = SHOW_ALL & ~SHOW_INCOMING;
        mode = ETHER_OUTGOING;
    }
    printPkt((struct ether_header *)buf,size,mode);
    EtherVerbose = overbose;
}

/*
 *  printIp(p)
 */
int
printIp(struct ip *ihdr)
{
    ushort  ipoff;
    struct ip *icpy;
    int     i, ipsize;
    char    *fragmented;
    char    buf[16], buf1[16], *payload;
    ulong   tmp[sizeof(struct ip)/2];

    ipsize = ((ihdr->ip_vhl & 0x0f) << 2);

    /* Copy data to aligned memory space so printf doesn't crash. */
    memcpy((char *)tmp,(char *)ihdr,sizeof(struct ip));
    icpy = (struct ip *)tmp;
    printf("  IP:  vhl/tos  len    id     offset  ttl/proto  csum\n");
    printf("       x%02x%02x    x%04x  x%04x  x%04x   x%02x%02x      x%04x\n",
           icpy->ip_vhl,icpy->ip_tos, ecs(icpy->ip_len),
           ecs(icpy->ip_id), ecs(icpy->ip_off),
           icpy->ip_ttl, icpy->ip_p, ecs(icpy->ip_sum));

    printf("       src/dest: %s / %s\n",
           IpToString(icpy->ip_src.s_addr,buf),
           IpToString(icpy->ip_dst.s_addr,buf1));

    /* Check for options...
     */
    if(ipsize > sizeof(struct ip)) {
        printf("       IP Options: x");
        for(i=sizeof(struct ip); i<ipsize; i++) {
            printf("%02x",((char *)ihdr)[i]);
        }
        printf("\n");
    }

    payload = (char *)ihdr;
    payload += ipsize;
    ipoff = ecs(icpy->ip_off);

    if(ipoff & 0x3fff) {
        if(ipoff == IP_MOREFRAGS) {
            fragmented = " (first fragment)";
        } else if((ipoff & IP_MOREFRAGS) == 0) {
            fragmented = " (last fragment)";
        } else {
            fragmented = " (fragment)";
        }
    } else {
        fragmented = "";
    }

    /* Only print the header info if this is not a fragment or if it
     * is the first fragment...
     */
    if((ipoff == 0) || (ipoff == IP_MOREFRAGS) || (ipoff == IP_DONTFRAG)) {
        if(icpy->ip_p == IP_UDP) {
            printUdp((struct Udphdr *)payload,fragmented);
            return(0);
        } else if(icpy->ip_p == IP_IGMP) {
            printIgmp((struct Igmphdr *)payload,fragmented);
            return(0);
        }
    }

    for(i=0; protocols[i].pname; i++) {
        if(icpy->ip_p == protocols[i].pnum) {
            printf("  Protocol: %s%s\n",protocols[i].pname,fragmented);
            return(0);
        }
    }
    printf("  <%02x>: unknown IP protocol%s\n", icpy->ip_p,fragmented);
    return (1);
}

/*
 *  printUdp(p)
 */
int
printUdp(struct Udphdr *p,char *fragmented)
{
    ushort  dport, sport;

    dport = ecs(p->uh_dport);
    sport = ecs(p->uh_sport);

#if INCLUDE_DHCPBOOT
    if((dport == DhcpServerPort) || (dport == DhcpClientPort)) {
        printDhcp(p);
        return(0);
    }
#endif
    printf("  UDP: sport dport ulen sum%s\n",fragmented);
    printf("       %4d  %4d  %4d %4d\n",
           sport, dport, ecs(p->uh_ulen),ecs(p->uh_sum));
    return(0);
}

/*
 *  printIgmp(p)
 */
int
printIgmp(struct Igmphdr *p,char *fragmented)
{
    uchar   buf[16];

    printf(" IGMP: type mrt  csum  group%s\n",fragmented);
    printf("       x%02x  x%02x  x%04x %s\n",
           p->type, p->mrt, p->csum, IpToString(p->group,(char *)buf));
    return(0);
}
#endif

/* IpToString():
 *  Incoming ascii pointer is assumed to be pointing to at least 16
 *  characters of available space.  Conversion from long to ascii is done
 *  and string is terminated with NULL.  The ascii pointer is returned.
 */
char *
IpToString(ulong ipadd,char *ascii)
{
    uchar   *cp;

    cp = (uchar *)&ipadd;
    sprintf(ascii,"%d.%d.%d.%d",
            (int)cp[0],(int)cp[1],(int)cp[2],(int)cp[3]);
    return(ascii);
}

char *
EtherToString(uchar *etheradd,char *ascii)
{
    sprintf(ascii,"%02x:%02x:%02x:%02x:%02x:%02x",(int)etheradd[0],
            (int)etheradd[1],(int)etheradd[2],(int)etheradd[3],
            (int)etheradd[4],(int)etheradd[5]);
    return(ascii);
}

/* ipChksum():
 *  Compute the checksum of the incoming IP header.  The size of
 *  the header is variable because of the possibility of there
 *  being options; hence, the size of the header is taken from
 *  the ip_vhl field instead of assuming sizeof(struct ip).
 *  The incoming pointer to an IP header is directly populated with
 *  the result.
 */
void
ipChksum(struct ip *ihdr)
{
    register int    i, stot;
    register ushort *sp;
    register long   csum;

    csum = 0;
    ihdr->ip_sum = 0;
    stot = (((ihdr->ip_vhl & 0x0f) << 2) / (int)sizeof(ushort));
    sp = (ushort *) ihdr;
    for(i=0; i<stot; i++,sp++) {
        csum += *sp;
        if(csum & 0x80000000) {
            csum = (csum & 0xffff) + (csum >> 16);
        }
    }
    while(csum >> 16) {
        csum = (csum & 0xffff) + (csum >> 16);
    }
    ihdr->ip_sum = ~csum;
}

/*  udpChksum():
 *  Compute the checksum of the UDP packet.
 *  The incoming pointer is to an ip header, the udp header after that ip
 *  header is directly populated with the result.
 *  Got part of this code out of Steven's TCP/IP Illustrated Volume 2.
 */
void
udpChksum(struct ip *ihdr)
{
    register int    i;
    register ushort *datap;
    register long   sum;
    int     len;
    struct  Udphdr *uhdr;
    struct  UdpPseudohdr    pseudohdr;

    uhdr = (struct Udphdr *)(ihdr+1);
    uhdr->uh_sum = 0;

    /* Note that optionally, the checksum can be forced to zero here,
     * so a return could replace the remaining code.
     * That would be kinda dangerous, but it is an option according to
     * the spec.
     */

    /* Start with the checksum of the pseudo header:
     * Note that we have to use memcpy because we don't know if the incoming
     * stream is aligned properly.
     */
    memcpy((char *)&pseudohdr.ip_src.s_addr,(char *)&ihdr->ip_src.s_addr,4);
    memcpy((char *)&pseudohdr.ip_dst.s_addr,(char *)&ihdr->ip_dst.s_addr,4);
    pseudohdr.zero = 0;
    pseudohdr.proto = ihdr->ip_p;
    pseudohdr.ulen = uhdr->uh_ulen;

    /* Get checksum of pseudo header: */
    sum = 0;
    datap = (ushort *) &pseudohdr;
    for(i=0; i<(sizeof(struct UdpPseudohdr)/sizeof(ushort)); i++) {
        sum += *datap++;
        if(sum & 0x80000000) {
            sum = (sum & 0xffff) + (sum >> 16);
        }
    }

    len = ecs(uhdr->uh_ulen);
    datap = (ushort *) uhdr;

    /* If length is odd, pad with zero and add 1... */
    if(len & 1) {
        uchar   *ucp;
        ucp = (uchar *)uhdr;
        ucp[len] = 0;
        len++;
    }

    while(len) {
        sum += *datap++;
        if(sum & 0x80000000) {
            sum = (sum & 0xffff) + (sum >> 16);
        }
        len -= 2;
    }

    while(sum >> 16) {
        sum = (sum & 0xffff) + (sum >> 16);
    }

    uhdr->uh_sum = ~sum;
}

struct  ether_header *
EtherCopy(struct ether_header *re)
{
    struct  ether_header *te;

    te = (struct ether_header *) getXmitBuffer();
    memcpy((char *)&(te->ether_shost),(char *)BinEnetAddr,6);
    memcpy((char *)&(te->ether_dhost),(char *)&(re->ether_shost),6);
    te->ether_type = re->ether_type;
    return(te);
}

ushort
ipId(void)
{
    return(++UniqueIpId);
}


/* getTuneup():
 *  The DHCP, TFTP and ARP timeout & retry mechanism can be tuned based on
 *  the content of the shell variables DHCPRETRYTUNE, TFTPRETRYTUNE and
 *  ARPRETRYTUNE respectively.
 *  Return 0 if variable is not found, -1 if there is a detected error in
 *  the content of the shell variable; else 1 indicating that the three
 *  parameters have been loaded from the content of the shell variable.
 */
int
getTuneup(char *varname,int *rexmitdelay,int *giveupcount,int *rexmitdelaymax)
{
    char *vp, *colon1, *colon2;

    vp = getenv(varname);
    if(!vp) {
        return(0);
    }
    colon1 = strchr(vp,':');
    if(colon1) {
        colon2 = strchr(colon1+1,':');
        if(colon2) {
            *rexmitdelay = (int)strtol(vp,0,0);
            *giveupcount = (int)strtol(colon1+1,0,0);
            *rexmitdelaymax = (int)strtol(colon2+1,0,0);
            return(1);
        }
    }
    printf("Syntax error in %s\n",varname);
    return(-1);
}

/* RetransmitDelay():
 *  This function provides a common point for retransmission delay
 *  calculation.  It is an implementation "similar" to the recommendation
 *  made in the RFC 2131 (DHCP) section 4.1 paragraph #8...
 *
 *  The delay before the first retransmission is 4 seconds randomized by
 *  the value of a uniform random number chosen from the range -1 to +2.
 *  The delay before the next retransmission is 8 seconds randomized by
 *  the same number as previous randomization.  Each subsequent retransmission
 *  delay is doubled up to a maximum of 66 seconds.  Once a delay of 66
 *  seconds is reached, return that value for 6 subsequent delay
 *  requests, then return RETRANSMISSION_TIMEOUT (-1) indicating that the
 *  requestor should give up.
 *
 *  The value of randomdelta will be 2, 1, 0 or -1 depending on the target's
 *  IP address.
 *
 *  The return values will be...
 *  4+randomdelta, 8+randomdelta, 16+randomdelta, etc... up to 64+randomdelta.
 *  Then after returning 64+randomdelta 6 times, return RETRANSMISSION_TIMEOUT.
 *
 *  NOTE: if DELAY_RETURN is the opcode, then RETRANSMISSION_TIMEOUT is
 *        never returned, once the max is reached, it is always the value
 *        returned;
 *        if DELAY_OR_TIMEOUT_RETURN is the opcode, then once maxoutcount
 *        reaches 6, RETRANSMISSION_TIMEOUT is returned.
 *
 *  NOTE1: this function supports the ability to modify the above-discussed
 *         parameters.  Start with DELAY_INIT_DHCP to set up the parameters
 *         discussed above; start with DELAY_INIT_XXXX for others.
 */
int
RetransmitDelay(int opcode)
{
    static int randomdelta;     /* Used to slightly randomize the delay.
                                 * Taken from the 2 least-significant-bits
                                 * of the IP address (range = -1 to 2).
                                 */
    static int rexmitdelay;     /* Doubled each time DELAY_INCREMENT
                                 * is called until it is greater than the
                                 * value stored in rexmitdelaymax.
                                 */
    static int rexmitdelaymax;  /* See rexmitdelay. */
    static int maxoutcount;     /* Number of times the returned delay has
                                 * reached its max.
                                 */
    static int giveupcount;     /* Once maxoutcount reaches this value, we
                                 * give up and return TIMEOUT.
                                 */
    int     rexmitstate;

    rexmitstate = RETRANSMISSION_ACTIVE;
    switch(opcode) {
    case DELAY_INIT_DHCP:
        if(getTuneup("DHCPRETRYTUNE",&rexmitdelay,
                     &giveupcount,&rexmitdelaymax) <= 0) {
            rexmitdelay = 4;
            giveupcount = 6;
            rexmitdelaymax = 64;
        }
        maxoutcount = 0;
        randomdelta = (int)(BinIpAddr[3] & 3) - 1;
        break;
    case DELAY_INIT_TFTP:
        if(getTuneup("TFTPRETRYTUNE",&rexmitdelay,
                     &giveupcount,&rexmitdelaymax) <= 0) {
            rexmitdelay = 2;
            giveupcount = 4;
            rexmitdelaymax = 8;
        }
        maxoutcount = 0;
        randomdelta = (int)(BinIpAddr[3] & 3) - 1;
        break;
    case DELAY_INIT_ARP:
        if(getTuneup("ARPRETRYTUNE",&rexmitdelay,
                     &giveupcount,&rexmitdelaymax) <= 0) {
            rexmitdelay = 1;
            giveupcount = 0;
            rexmitdelaymax = 4;
        }
        maxoutcount = 0;
        randomdelta = 0;
        break;
    case DELAY_INCREMENT:
        if(rexmitdelay < rexmitdelaymax) {
            rexmitdelay <<= 1;    /* double it. */
        } else {
            maxoutcount++;
        }

        if(maxoutcount > giveupcount) {
            rexmitstate = RETRANSMISSION_TIMEOUT;
        }
        break;
    case DELAY_OR_TIMEOUT_RETURN:
        if(maxoutcount > giveupcount) {
            rexmitstate = RETRANSMISSION_TIMEOUT;
        }
        break;
    case DELAY_RETURN:
        break;
    default:
        printf("\007TimeoutAlgorithm error 0x%x.\n",opcode);
        rexmitstate = RETRANSMISSION_TIMEOUT;
        break;
    }
    if(rexmitstate == RETRANSMISSION_TIMEOUT) {
        return(RETRANSMISSION_TIMEOUT);
    } else {
        return(rexmitdelay+randomdelta);
    }
}

/* monSendEnetPkt() & monRecvEnetPkt():
 * These two functions allow the monitor to provide a primitive
 * connect to the ethernet interface for an application (using
 * mon_sendenetpkt() and mon_recvenetpkt()).
 *
 * Note: These are obviously not very sophisticated; however, they
 * provide an immediate mechanism for LWIP to access the raw driver.
 * without the application even being aware of the type of ethernet
 * device.
 */
int
monSendEnetPkt(char *pkt, int pktlen)
{
    /* If pkt is zero and pktlen is either 0 or -1, we use this as a
     * mechanism that changes uMon's ethernet polling state.  This is
     * necessary so that an application can use the monitor's API along
     * with the monitor's hook to ethernet, but without having other
     * hooks in uMon do any pollethernet() calls.
     */
    if(pkt == 0) {
        if(pktlen == 0) {
            EtherPollingOff = 1;
            return(0);
        }
        if(pktlen == -1) {
            EtherPollingOff = 0;
            return(0);
        }
    }

    /* Copy the incoming packet into a packet that has been allocated
     * by the monitor's packet allocator for this ethernet device:
     */
    memcpy((char *)getXmitBuffer(),(char *)pkt,pktlen);
    sendBuffer(pktlen);
    return(pktlen);
}

int
monRecvEnetPkt(char *pkt, int pktlen)
{
    int pcnt, len;

    /* Prior to calling polletherdev(), this function sets up the globals
     * AppPktPtr and AppPktLen so that if a packet is recived, and
     * polletherdev() calls processPACKET(), the data will simply be
     * copied to the requested buffer space instead of being processed
     * by the monitor's packet handler (see the top of processPACKET()
     * for the use of AppPktPtr & AppPktLen).
     *
     * NOTE: this assumes that the target-specific function polletherdev()
     * will only process one packet per call.  If it can process more than
     * one per call, then packets will be lost here.
     */
    AppPktPtr = pkt;
    AppPktLen = pktlen;
    pcnt = polletherdev();
    if(pcnt == 0) {
        len = 0;
    } else {
        if(pcnt > 1) {
            len = -AppPktLen;
        } else {
            len = AppPktLen;
        }
    }
    AppPktPtr = 0;
    return(len);
}

#ifndef USE_ALT_STOREMAC

/* storeMac():
 * This function can be called in system startup (after flash drivers
 * are initialized) to give the user the opportunity to program a
 * MAC address into the "etheraddr" block of flash in monitor space.
 * This is only done if that space is erased (0xff), so only on the
 * first pass will it be called.
 * It is useful for monitors that want to exist without a monrc file,
 * but should still have a MAC address.
 * As of uMon_1.0, this function can also be called by the ether command.
 */
void
storeMac(int verbose)
{
#if INCLUDE_FLASH
    int snum, yes;
    char macascii[24], prefill[24], buf[6];
    uchar *realetheraddr;
#ifdef TO_FLASH_ADDR
    /* The address the program is linked to is not necessarily the
     * physical address where flash operations can be performed on.
     * A typical use could be that the program is linked to run in
     * a cacheable region but writing to the flash can only be done
     * in an uncached region.
     * "config.h" is a good place to define the TO_FLASH_ADDR macro.
     */
    realetheraddr = (uchar *)TO_FLASH_ADDR(etheraddr);
#else
    realetheraddr = etheraddr;
#endif

    if(realetheraddr[0] != 0xff) {
        if(verbose) {
            printf("MAC store abort: etheraddr[] contains data\n");
        }
        return;
    }

    /* If etheraddr[] is writeable (RAM), then assume that this
     * function is being called as part of a RAM based temporary
     * monitor; hence, no need to do it...
     */
    realetheraddr[0] = 0;
    monDelay(100);
    if(realetheraddr[0] == 0) {
        realetheraddr[0] = 0xff;
        if(verbose) {
            printf("MAC store abort: etheraddr[] in RAM\n");
        }
        return;
    }

    /* Use whatever is in config.h as the default MAC address,
     * then allow the user to override it with getline_p()...
     */
    strcpy(prefill,DEFAULT_ETHERADD);

#if INCLUDE_ETHERVERBOSE
    printf("\nMAC address must be configured.\n");
    printf("The system's MAC address is a 6-digit, colon-delimited string\n");
    printf("(for example: 12:34:56:78:9a:bc).  It must be unique for all\n");
    printf("ethernet controllers present on a given subnet.  MAC addresses\n");
    printf("are often allocated by a product vendor to prevent duplication,\n");
    printf("and are frequently documented on decals or other materials\n");
    printf("provided with the product.  The following prompt allows you\n");
    printf("to specify the MAC address for this device.\n");
    if(strlen(prefill) != 0) {
        printf("Use backspace if the printed default needs modification...\n");
    }
    printf("\n");
#endif

    do {
        printf("Enter MAC address (xx:xx:xx:xx:xx:xx):\n");
        if(getline_p(macascii,sizeof(macascii),0,prefill) == 0) {
            return;
        }
    } while(EtherToBin((char *)macascii,(uchar *)buf) == -1);

    printf("Configuring '%s' as MAC address, ok?",macascii);
    yes =  askuser(" (y or n)");

    putchar('\n');
    if(!yes) {
        return;
    }

    if(addrtosector((uchar *)realetheraddr,&snum,0,0) < 0) {
        return;
    }

    sprintf(buf,"%d",snum);
    sectorProtect(buf,0);
    AppFlashWrite((uchar *)realetheraddr,(uchar *)macascii,strlen(macascii)+1);
    sectorProtect(buf,1);

    printf("MAC address burned in at 0x%lx\n",(long)realetheraddr);
#else
    printf("Error: MAC storage requires flash driver\n");
#endif
}
#endif

#endif  /* INCLUDE_ETHERNET */

/* EtherToBin():
 *  Convert ascii MAC address string to binary.  Note that this is outside
 *  the #if INCLUDE_ETHERNET because it is used by password.c.  This correctly
 *  implies that if there is no ethernet interface, then we need a different
 *  solution for the password backdoor!.
 */
int
EtherToBin(char *ascii,uchar *binary)
{
    int i, digit;
    char    *acpy, *acpy1;

    acpy = ascii;
    for(i=0; i<6; i++) {
        digit = (int)strtol(acpy,&acpy1,16);
        if(((i != 5) && (*acpy1++ != ':')) ||
                ((i == 5) && (*acpy1 != 0)) ||
                ((acpy1 - acpy) < 2) ||
                (digit < 0) || (digit > 255)) {
            printf("Misformed ethernet addr at: 0x%lx\n",(long)ascii);
            return(-1);
        }
        acpy = acpy1;
        binary[i] = (uchar)digit;
    }
    return(0);
}

int
IpToBin(char *ascii,uchar *binary)
{
    int i, digit;
    char    *acpy;

    acpy = ascii;
    for(i=0; i<4; i++) {
        digit = (int)strtol(acpy,&acpy,10);
        if(((i != 3) && (*acpy++ != '.')) ||
                ((i == 3) && (*acpy != 0)) ||
                (digit < 0) || (digit > 255)) {
            printf("Misformed IP addr at 0x%lx\n",(long)ascii);
            return(-1);
        }
        binary[i] = (uchar)digit;
    }
    return(0);
}

