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
 * ether.h:
 *
 * This file is a single "contains-all" header file to support different
 * components of ethernet/IP along with the peer and upper layer protocols
 * like ICMP, UDP, TFTP, DHCP etc...
 *
 * Original author:     Ed Sutter (ed.sutter@alcatel-lucent.com)
 *
 */

#ifndef _ETHER_H_
#define _ETHER_H_

#define __PACKED__	__attribute__ ((packed))

#ifndef INCLUDE_MONCMD
#define INCLUDE_MONCMD INCLUDE_ETHERNET
#endif

#ifndef INCLUDE_ETHERVERBOSE
#define INCLUDE_ETHERVERBOSE INCLUDE_ETHERNET
#endif

#ifndef INCLUDE_RARPIPASSIGN
#define INCLUDE_RARPIPASSIGN INCLUDE_ETHERNET
#endif

/************************************************************************
 *
 * Default MAC & IP addresses...
 * 
 */

#ifndef DEFAULT_ETHERADD
#define	DEFAULT_ETHERADD "00:00:00:00:00:00"
#endif

#ifndef DEFAULT_IPADD
#define	DEFAULT_IPADD "0.0.0.0"
#endif

/************************************************************************
 *
 * Retransmission delay stuff...
 * 
 */

#define DELAY_INIT_ARP			1
#define DELAY_INIT_DHCP			2
#define DELAY_INIT_TFTP			3
#define DELAY_INCREMENT			4
#define DELAY_RETURN			5
#define DELAY_OR_TIMEOUT_RETURN	6
#define RETRANSMISSION_TIMEOUT	-1
#define RETRANSMISSION_ACTIVE	1

/************************************************************************
 *
 * Ethernet stuff...
 * 
 */
   
struct ether_addr {
	unsigned char  ether_addr_octet[6];
} __PACKED__ ;
 
struct  ether_header {
	struct  ether_addr ether_dhost;
	struct  ether_addr ether_shost;
	unsigned short ether_type;
} __PACKED__ ;
#define ETHERSIZE	sizeof(struct ether_header)
 
#define ETHERTYPE_PUP           0x0200          /* PUP protocol */
#define ETHERTYPE_IP            0x0800          /* IP protocol */
#define ETHERTYPE_ARP           0x0806          /* Addr resolution protocol */
#define ETHERTYPE_REVARP        0x8035          /* Reverse ARP */

#define ETHER_MINPKT			64

/************************************************************************
 *
 * ARP/RARP stuff...
 *
 */

#define SIZEOFARPCACHE	8

#define ARPSIZE (sizeof(struct ether_header) + sizeof(struct arphdr))

struct arphdr {
	unsigned short	hardware;		/* 1 for ethernet */
	unsigned short	protocol;
	unsigned char	hlen;
	unsigned char	plen;
	unsigned short	operation;	
	unsigned char	senderha[6];
	unsigned char	senderia[4];
	unsigned char	targetha[6];
	unsigned char	targetia[4];
} __PACKED__ ;

/* ARP/RARP operations:. */
#define ARP_REQUEST		1
#define ARP_RESPONSE	2
#define RARP_REQUEST	3
#define RARP_RESPONSE	4

/************************************************************************
 *
 * IP stuff...
 *
 */

struct in_addr {
	unsigned long s_addr;
} __PACKED__ ;

#define getIP_V(x)		((x)>>4) 
#define getIP_HL(x)		((x)&0xf)
#define IP_DONTFRAG		0x4000			/* dont fragment flag */
#define IP_MOREFRAGS	0x2000			/* more fragments flag */
#define IP_VER			0x4
#define IP_HDR_LEN		(sizeof(struct ip)>>2)
#define IP_HDR_VER_LEN	((IP_VER<<4)|IP_HDR_LEN)
#define IP_HLEN(ihdr)	((ihdr->ip_vhl&0x0f)<<2)

struct ip {
	unsigned char  ip_vhl;		/* version & header length */
	unsigned char  ip_tos;		/* type of service */
	short  ip_len;				/* total length */
	unsigned short ip_id;		/* identification */
   	short  ip_off;				/* fragment offset field */
	unsigned char  ip_ttl;		/* time to live */
	unsigned char  ip_p;		/* protocol */
	unsigned short ip_sum;		/* checksum */
	struct in_addr ip_src;		/* source address */
	struct in_addr ip_dst;		/* dest address */
} __PACKED__ ;
#define IPSIZE	sizeof(struct ip)

/* ip protocols */
#define IP_IP   0
#define IP_ICMP 1
#define IP_IGMP 2
#define IP_GGP  3
#define IP_TCP  6
#define IP_PUP  12
#define IP_UDP  17

#define IP1(a) ((a & 0xff000000) >> 24)
#define IP2(a) ((a & 0xff0000) >> 16)
#define IP3(a) ((a & 0xff00) >> 8)
#define IP4(a) (a & 0xff)

#define IP2LONG(a,b,c,d) ((a << 24) | (b << 16) | (c << 8) | d)

/************************************************************************
 *
 * ICMP stuff...
 * 
 */

#define ICMP_UNREACHABLESIZE (sizeof(struct ether_header) + \
	sizeof(struct ip) + sizeof(struct icmp_unreachable_hdr) + datalen)
#define ICMP_TIMERQSTSIZE (sizeof(struct ether_header) + \
	sizeof(struct ip) + sizeof(struct icmp_time_hdr))
#define ICMP_ECHORQSTSIZE (sizeof(struct ether_header) + \
	sizeof(struct ip) + sizeof(struct icmp_echo_hdr))

#define INVALID_TIMESTAMP 0xffffffff
#define NONSTANDARD_TIMESTAMP 0x80000000

/* ICMP protocol header.
 */
struct icmp_hdr {
	unsigned char	type;		/* type of message */
	unsigned char	code;		/* type subcode */
	unsigned short	cksum;		/* ones complement cksum of struct */
} __PACKED__ ;

/* ICMP time request.
 */
struct icmp_time_hdr {
	unsigned char	type;		/* type of message */
	unsigned char	code;		/* type subcode */
	unsigned short	cksum;		/* ones complement cksum of struct */
	unsigned short	id;			/* identifier */
	unsigned short	seq;		/* sequence number */
	unsigned long	orig;		/* originate timestamp */
	unsigned long	recv;		/* receive timestamp */
	unsigned long	xmit;		/* transmit timestamp */
} __PACKED__ ;

/* ICMP echo reply.
 */
struct icmp_echo_hdr {
	unsigned char	type;		/* type of message */
	unsigned char	code;		/* type subcode */
	unsigned short	cksum;		/* ones complement cksum of struct */
	unsigned short	id;			/* identifier */
	unsigned short	seq;		/* sequence number */
} __PACKED__ ;

struct icmp_unreachable_hdr {
	unsigned char	type;		/* type of message */
	unsigned char	code;		/* type subcode */
	unsigned short	cksum;		/* ones complement cksum of struct */
	unsigned short unused1;		/* Is alway zero */
	unsigned short unused2;		/* Is alway zero */
} __PACKED__ ;

/* ICMP types
 */
#define ICMP_ECHOREPLY			0
#define ICMP_DESTUNREACHABLE	3
#define ICMP_SOURCEQUENCH		4
#define ICMP_REDIRECT			5
#define ICMP_ECHOREQUEST		8
#define ICMP_TIMEEXCEEDED		11
#define ICMP_PARAMPROBLEM		12
#define ICMP_TIMEREQUEST		13
#define ICMP_TIMEREPLY			14
#define ICMP_INFOREQUEST		15
#define ICMP_INFOREPLY			16
#define ICMP_ADDRMASKREQUEST	17
#define ICMP_ADDRMASKREPLY		18

/* A few unreachable codes.
 */
#define ICMP_UNREACHABLE_PROTOCOL			2
#define ICMP_UNREACHABLE_PORT			    3
#define ICMP_DEST_UNREACHABLE_MAX_CODE 		12

/************************************************************************
 *
 * UDP stuff...
 *
 */

/* UDP protocol header.
 */
struct Udphdr {
	unsigned short	uh_sport;		/* source port */
	unsigned short	uh_dport;		/* destination port */
	unsigned short	uh_ulen;		/* udp length */
	unsigned short	uh_sum;			/* udp checksum */
} __PACKED__ ;
#define UDPSIZE	sizeof(struct Udphdr)

/* UDP pseudo header.
 */
struct UdpPseudohdr {
	struct in_addr ip_src;		/* source address */
	struct in_addr ip_dst;		/* dest address */
	unsigned char	zero;		/* fixed to zero */
	unsigned char	proto;		/* protocol */
	unsigned short	ulen;		/* udp length */
} __PACKED__ ;

#define UDP_OPEN	2
#define UDP_DATA	3
#define UDP_ACK		4
#define UDP_TTL		0x3c

/************************************************************************
 *
 * IGMP stuff...
 *
 */

/* IGMP protocol header.
 */
struct Igmphdr {
	unsigned char	type;		/* IGMP message type */
	unsigned char	mrt;		/* Max response time */
	unsigned short	csum;		/* Checksum of IGMP header */
	unsigned long	group;		/* Multicast group ip */
} __PACKED__ ;

#define IGMPSIZE	sizeof(struct Igmphdr)

#define	IGMPTYPE_QUERY		0x11
#define IGMPTYPE_REPORTV1	0x12
#define IGMPTYPE_REPORTV2	0x16
#define IGMPTYPE_JOIN		IGMPTYPE_REPORTV2
#define IGMPTYPE_LEAVE		0x17

#define ALL_HOSTS				0xe0000001		/* 224.0.0.1 */
#define ALL_MULTICAST_ROUTERS	0xe0000002		/* 224.0.0.2 */

#define IGMP_JOINRQSTSIZE (sizeof(struct ether_header) + \
	sizeof(struct ip) + sizeof(long) + sizeof(struct Igmphdr))

/************************************************************************
 *
 * TFTP stuff...
 *
 */

/* TFTP state values:
 */
#define TFTPOFF			0
#define TFTPIDLE		1
#define TFTPACTIVE		2
#define TFTPERROR		3
#define TFTPSENTRRQ		4
#define TFTPTIMEOUT		5
#define TFTPHOSTERROR	6
#define TFTPSENTWRQ		7

/* TFTP opcode superset (see Stevens pg 466)
 */
#define TFTP_RRQ		1
#define TFTP_WRQ		2
#define TFTP_DAT		3
#define TFTP_ACK		4
#define TFTP_ERR		5
#define TFTP_OACK		6	/* RFC2347: Option Acknowledgement opcode. */

/* TFTP is UDP port 69... (see Stevens pg 206)
 */
#define IPPORT_TFTP			69
#define IPPORT_TFTPSRC		8888

#define TFTP_NETASCII_WRQ	1
#define TFTP_NETASCII_RRQ	2
#define TFTP_OCTET_WRQ		3
#define TFTP_OCTET_RRQ		4
#define TFTP_DATAMAX		512
#define TFTP_PKTOVERHEAD	(ETHERSIZE + IPSIZE + UDPSIZE)
#define TFTPACKSIZE			(TFTP_PKTOVERHEAD + 4)

/************************************************************************
 *
 * DHCP stuff...
 * See RFCs 2131 & 2132 for more details.
 * Note that the values used for dhcpstate enum are based on the state
 * diagram in 3rd Edition Comer pg 375.	 Plus a few more so that I can
 * used the same enum for DHCP and BOOTP.
 *
 */

#define BOOTPSIZE (sizeof(struct ether_header) + sizeof(struct ip) + \
	sizeof(struct Udphdr) + sizeof(struct bootphdr))

#define DHCPSIZE (sizeof(struct ether_header) + sizeof(struct ip) + \
	sizeof(struct Udphdr) + sizeof(struct dhcphdr))

#define IPPORT_DHCP_SERVER	67
#define IPPORT_DHCP_CLIENT	68

#define DHCPBOOTP_REQUEST	1
#define DHCPBOOTP_REPLY		2

#define USE_NULL			0
#define USE_DHCP			1
#define USE_BOOTP			2

#define DHCP_VERBOSE	(SHOW_DHCP|SHOW_INCOMING|SHOW_OUTGOING|SHOW_BROADCAST)

/* DHCP Message types:
 */
#define DHCPDISCOVER		1
#define DHCPOFFER			2
#define DHCPREQUEST			3
#define DHCPDECLINE			4
#define DHCPACK				5
#define DHCPNACK			6
#define DHCPRELEASE			7
#define DHCPINFORM			8
#define DHCPFORCERENEW		9
#define DHCPLEASEQUERY		10
#define DHCPLEASEUNASSIGNED	11
#define DHCPLEASEUNKNOWN	12
#define DHCPLEASEACTIVE		13
#define DHCPUNKNOWN		99

/* DHCPState (short) values: (upper bit set = bootp)
 */
#define	DHCPSTATE_INITIALIZE		0x0001
#define	DHCPSTATE_INITDELAY			0x0002
#define	DHCPSTATE_SELECT			0x0003
#define	DHCPSTATE_REQUEST			0x0004
#define	DHCPSTATE_BOUND				0x0005
#define	DHCPSTATE_RENEW				0x0006
#define	DHCPSTATE_REBIND			0x0007
#define	DHCPSTATE_NOTUSED			0x0008
#define	DHCPSTATE_RESTART			0x0009
#define BOOTP_MODE					0x8000
#define	BOOTPSTATE_INITIALIZE		0x8001
#define	BOOTPSTATE_INITDELAY		0x8002
#define	BOOTPSTATE_REQUEST			0x8003
#define	BOOTPSTATE_RESTART			0x8004
#define	BOOTPSTATE_COMPLETE			0x8005

/* DHCP Options
 */
#define DHCPOPT_SUBNETMASK			1
#define DHCPOPT_ROUTER				3
#define DHCPOPT_HOSTNAME			12
#define DHCPOPT_ROOTPATH			17
#define DHCPOPT_BROADCASTADDRESS	28
#define DHCPOPT_VENDORSPECIFICINFO	43
#define DHCPOPT_REQUESTEDIP			50
#define DHCPOPT_LEASETIME			51
#define DHCPOPT_MESSAGETYPE			53
#define DHCPOPT_SERVERID			54
#define DHCPOPT_PARMRQSTLIST		55
#define DHCPOPT_CLASSID				60
#define DHCPOPT_CLIENTID			61
#define DHCPOPT_NISDOMAINNAME		64
#define DHCPOPT_NISSERVER			65

#define STANDARD_MAGIC_COOKIE		0x63825363		/* 99.130.83.99 */

struct dhcphdr {
	unsigned char	op;
	unsigned char	htype;
	unsigned char	hlen;
	unsigned char	hops;
	unsigned long	transaction_id;
	unsigned short	seconds;
	unsigned short	flags;
	unsigned long	client_ip;
	unsigned long	your_ip;
	unsigned long	server_ip;
	unsigned long	router_ip;
	unsigned char	client_macaddr[16];
	unsigned char	server_hostname[64];
	unsigned char	bootfile[128];
	unsigned long	magic_cookie;
	/* Dhcp options would start here */
} __PACKED__ ;

struct bootphdr {
	unsigned char	op;
	unsigned char	htype;
	unsigned char	hlen;
	unsigned char	hops;
	unsigned long	transaction_id;
	unsigned short	seconds;
	unsigned short	unused;
	unsigned long	client_ip;
	unsigned long	your_ip;
	unsigned long	server_ip;
	unsigned long	router_ip;
	unsigned char	client_macaddr[16];
	unsigned char	server_hostname[64];
	unsigned char	bootfile[128];
	unsigned char	vsa[64];
} __PACKED__ ;

unsigned char *DhcpGetOption(unsigned char, unsigned char *);

/************************************************************************
 * 
 * DNS stuff...
 *
 * The area of the dnshdr structure beginning with the question array
 * is actually a variable sized area for four sections:
 *  question, answer, authority and additional information.
 * For the monitor's implementation of DNS, it is simplified to need
 * only the question section because we are only using this to do
 * a standard DNS query (give a domain name to a server and wait for
 * a response that has the IP address).
 * So, for the sake of building a fixed size structure, we allocate a
 * 64 byte question section and assume that no single domain name request
 * will exceed that length.
 * Note: first byte of domain name is the size.
 *
 * For MulticastDNS information I used...
 *   http://files.multicastdns.org/draft-cheshire-dnsext-multicastdns.txt
 */
#define MAX_DOMAIN_NAME_SIZE	63

struct dnshdr {
	unsigned short	id;
	unsigned short	param;
	unsigned short	num_questions;
	unsigned short	num_answers;
	unsigned short	num_authority;	
	unsigned short	num_additional;
	unsigned char	question[MAX_DOMAIN_NAME_SIZE+1];
} __PACKED__ ;

#define IPPORT_DNS			53
#define TYPE_A				1
#define TYPE_CNAME			5
#define CLASS_IN			1

/* DNS Error codes:
 */
#define DNSERR_NULL				0
#define DNSERR_COMPLETE			1
#define DNSERR_NOSRVR			2
#define DNSERR_SOCKETFAIL		3
#define DNSERR_FORMATERR		4
#define DNSERR_SRVRFAILURE		5
#define DNSERR_NAMENOEXIST		6
#define DNSERR_BADRESPTYPE		7
#define DNSERR_BADQSTNCOUNT		8
#define DNSERR_BADANSWRCOUNT	9
#define DNSERR_NOTARESPONSE		10
#define DNSERR_BADRESPID		11

#define DNSMCAST_IP				IP2LONG(224,0,0,251)
#define DNSMCAST_PORT			5353

/* DNS Buffer sizes:
 */
#define MAX_CACHED_HOSTNAMES	32
#define MAX_HOSTNAME_SIZE		255

#define DNS_RETRY_MAX		5
#define DNS_PKTBUF_SIZE		512

struct dnserr {
	int	errno;
	char *errstr;
};

struct dnscache {
	int	idx;
	unsigned long addr;
	char name[MAX_HOSTNAME_SIZE+1];
};

extern const char mDNSIp[];

/************************************************************************
 *
 * TCP stuff...
 *
 */

struct	tcphdr {
	unsigned short	sport;				/* Source port */
	unsigned short	dport;				/* Source port */
	unsigned long	seqno;				/* Sequence number */
	unsigned long	ackno;				/* Acknowledgment number */
	unsigned short	flags;				/* Flags (lower 6 bits) */
	unsigned short	windowsize;			/* Window size */
	unsigned short	tcpcsum;			/* TCP checksum */
	unsigned short	urgentptr;			/* Urgent pointer */
/*	options (if any) & data (if any) follow */
} __PACKED__ ;

/* Masks for flags (made up of flag bits, reserved bits & header length):
 */
#define TCP_FLAGMASK	0x003F
#define TCP_RSVDMASK	0x0FC0
#define TCP_HDRLENMASK	0xF000
#define TCP_HLEN(tp)	(((tp)->flags & TCP_HDRLENMASK) >> 10)

/* Masks for flag bits within flags member:
 */
#define TCP_FINISH			0x0001
#define TCP_SYNC			0x0002
#define TCP_RESET			0x0004
#define TCP_PUSH			0x0008
#define TCP_ACK				0x0010
#define TCP_URGENT			0x0020


/************************************************************************
 *
 * Miscellaneous...
 *
 */

/* Port that is used to issue monitor commands through ethernet.
 */
#define MONRESPSIZE (sizeof(struct ether_header) + sizeof(struct ip) + \
	sizeof(struct Udphdr) + idx)
#define IPPORT_MONCMD		777
#define IPPORT_GDB			1234

/* Verbosity levels used by various ethernet layers:
 */
#define SHOW_INCOMING		0x00000001
#define SHOW_OUTGOING		0x00000002
#define SHOW_HEX			0x00000004
#define SHOW_BROADCAST		0x00000008
#define SHOW_TFTP_TICKER	0x00000010
#define SHOW_DHCP			0x00000020
#define SHOW_ARP			0x00000040
#define SHOW_ASCII			0x00000080
#define SHOW_BADCSUM		0x00000100
#define SHOW_BADCSUMV		0x00000200
#define SHOW_PHY			0x00000400
#define SHOW_GDB			0x00000800
#define SHOW_TFTP_STATE		0x00001000
#define SHOW_DRIVER_DEBUG	0x00002000
#define SHOW_ALL			0xffffffff

#define ENET_DEBUG_ENABLED() (EtherVerbose & SHOW_DRIVER_DEBUG)

#define ETHER_INCOMING	1
#define ETHER_OUTGOING	2

extern	char *Etheradd, *IPadd;
extern  unsigned char etheraddr[], ipaddr[];
extern	unsigned short MoncmdPort, TftpPort, TftpSrcPort;
extern	unsigned short DhcpClientPort, DhcpServerPort, DHCPState;
extern	int EtherVerbose, IPMonCmdActive, EtherIsActive, EtherPollingOff;
extern	unsigned char BinEnetAddr[], BinIpAddr[];
extern	unsigned char AllZeroAddr[], BroadcastAddr[];
extern	int EtherXFRAMECnt, EtherRFRAMECnt, EtherIPERRCnt, EtherUDPERRCnt;

extern	int	getAddresses(void);
extern	int IpToBin(char *,unsigned char *);
extern	int EtherToBin(char *,unsigned char *);
extern  char *extGetIpAdd(void), *extGetEtherAdd(void);
extern	char *IpToString(unsigned long, char *);
extern	char *EtherToString(unsigned char *,char *);
extern	unsigned short ipId(void);
extern	unsigned char *ArpEther(unsigned char *,unsigned char *,int);
extern	unsigned char *getXmitBuffer(void);
extern	unsigned char *EtherFromCache(unsigned char *);
extern	void ipChksum(struct ip *), udpChksum(struct ip *);
extern	void tcpChksum(struct ip *);
#if INCLUDE_ETHERVERBOSE
extern	void printPkt(struct ether_header *,int,int);
#else
#define printPkt(a,b,c)
#endif
extern	void DhcpBootpDone(int, struct dhcphdr *,int);
extern	void DhcpVendorSpecific(struct	dhcphdr *);
extern	int sendBuffer(int);
extern	struct	ether_header *EtherCopy(struct ether_header *);
extern	int ValidDHCPOffer(struct	dhcphdr *);
extern	int buildDhcpHdr(struct dhcphdr *);
extern	int printDhcpVSopt(int,int,char *);
extern	int RetransmitDelay(int);
extern	int tftpGet(unsigned long,char *,char *,char *,char *,char *,char *);
extern	void printDhcp(struct Udphdr *);
extern	int printIp(struct ip*);
extern	int printUdp(struct Udphdr *,char *);
extern	int printIgmp(struct Igmphdr *,char *);
extern	void processPACKET(struct ether_header *,unsigned short);
extern	void processTCP(struct ether_header *,unsigned short);
extern	int processTFTP(struct ether_header *,unsigned short);
extern	int processICMP(struct ether_header *,unsigned short);
extern	char *processARP(struct ether_header *,unsigned short);
extern	int processDHCP(struct ether_header *,unsigned short);
extern	int processRARP(struct ether_header *,unsigned short);
extern	int processGDB(struct ether_header *,unsigned short);
extern	int processDNS(struct ether_header *,unsigned short);
extern	int processMCASTDNS(struct ether_header *,unsigned short);
extern	int SendICMPUnreachable(struct ether_header *,unsigned char);
extern	void enresetmmu(void), enreset(void);
extern	void ShowEtherdevStats(void), ShowTftpStats(void), ShowDhcpStats(void);
extern	void enablePromiscuousReception(void);
extern	void disablePromiscuousReception(void);
extern	int enselftest(int), polletherdev(void), EtherdevStartup(int);
extern	void DisableEtherdev(void);
extern	void tftpStateCheck(void), dhcpStateCheck(void);
extern	int DhcpIPCheck(char *);
extern	void dhcpDisable(void);
extern	void tftpInit(void);
extern	void disableBroadcastReception(void);
extern	void enableBroadcastReception(void);
extern	void sendGratuitousArp(void);

extern unsigned long getHostAddr(char *);
extern int dnsCacheDelName(char *);
extern void dnsCacheInit(void);
extern int dnsCacheAdd(char *, unsigned long);
extern int dnsCacheDelAddr(unsigned long);
extern short DnsPort;

#if INCLUDE_ETHERNET
extern	int pollethernet(void);
extern	int EthernetStartup(int,int);
#else
#define pollethernet()
#define EthernetStartup(a,b)
#endif

#if INCLUDE_MONCMD
extern	int SendIPMonChar(unsigned char,int);
#else
#define SendIPMonChar(a,b)
#endif

extern	int DisableEthernet(void);
extern	void storeMac(int);
extern	void GetBinNetMask(unsigned char *binnetmask);
extern	int monSendEnetPkt(char *pkt, int cnt);
extern	int monRecvEnetPkt(char *pkt, int cnt);
extern	void AppPrintPkt(char *buf, int size, int incoming);

#endif
