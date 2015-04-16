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
 * arp.c:
 * This code supports some basic arp/rarp stuff.
 *
 * Original author:	Ed Sutter (ed.sutter@alcatel-lucent.com)
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

static void	ArpFlush(void);
static void ArpShow(char *);
static void llas(char *);
static int	ArpStore(uchar *,uchar *);
static int	IpIsOnThisNet(uchar *);

static unsigned long probeIP;
static char probeAbort;

/* Constants used by Dynamic IPV4 Local-Link Address setup...
 * (see RFC 3927)
 */
#define PROBE_WAIT			1000		/* (msec) */
#define PROBE_NUM			3
#define PROBE_MIN			1000		/* (msec) */
#define PROBE_MAX			2000		/* (msec) */
#define ANNOUNCE_WAIT		2000		/* (msec) */
#define ANNOUNCE_NUM 		2
#define ANNOUNCE_INTERVAL 	2000		/* (msec) */
#define MAX_CONFLICTS		10
#define RATE_LIMIT_INTERVAL	60000		/* (msec) */
#define DEFEND_INTERVAL 	10000		/* (msec) */

/* arpcache[]:
 *	Used to store the most recent set of IP-to-MAC address correlations.
 */
struct arpcache {
	uchar	ip[4];
	uchar	ether[6];
} ArpCache[SIZEOFARPCACHE];

/* ArpIdx & ArpTot:
 *	Used for keeping track of current arp cache content.
 */
int ArpIdx, ArpTot;

/* ArpStore():
 *	Called with binary ip and ethernet addresses.
 *	It will store that set away in the cache.
 */
int
ArpStore(uchar *ip,uchar *ether)
{
	if (EtherFromCache(ip))
		return(0);
	if (ArpIdx >= SIZEOFARPCACHE)
		ArpIdx=0;
	memcpy((char *)ArpCache[ArpIdx].ip,(char *)ip,4);
	memcpy((char *)ArpCache[ArpIdx].ether,(char *)ether,6);
	ArpIdx++;
	ArpTot++;
	return(0);
}

/* EtherFromCache():
 *	Called with a binary (4-byte) ip address.  If a match is found
 *	in the cache, return a pointer to that ethernet address; else
 *	return NULL.
 */
uchar *
EtherFromCache(uchar *ip)
{
	int	i;

	for(i=0;i<SIZEOFARPCACHE;i++) {
		if (!memcmp((char *)ArpCache[i].ip, (char *)ip,4))
			return(ArpCache[i].ether);
	}
	return(0);
}

void
ArpFlush(void)
{
	int	i;

	for(i=0;i<SIZEOFARPCACHE;i++) {
		memset((char *)ArpCache[i].ip,0,4);
		memset((char *)ArpCache[i].ether,0,6);
	}
	ArpIdx = ArpTot = 0;
}

/* ArpShow():
 *	Dump the content of the current arp cache.
 */
void
ArpShow(char *ip)
{
	struct	arpcache *ap, *end;

	if (ArpTot < SIZEOFARPCACHE)
		end = &ArpCache[ArpTot];
		else
		end = &ArpCache[SIZEOFARPCACHE];

	for (ap=ArpCache;ap<end;ap++) {
		if ((!ip) || (!memcmp((char *)ip, (char *)ap->ip,4))) {
			printf("%02x:%02x:%02x:%02x:%02x:%02x = ",
			    ap->ether[0], ap->ether[1], ap->ether[2], ap->ether[3],
			    ap->ether[4], ap->ether[5]);
			printf("%d.%d.%d.%d\n",
			    ap->ip[0], ap->ip[1], ap->ip[2], ap->ip[3]);
		}
	}
}

/* SendArpResp():
 *	Called in response to an ARP REQUEST.  The incoming ethernet
 *	header pointer points to the memory area that contains the incoming
 *	ethernet packet that this function is called in response to.
 *	In other words, it is used to fill the majority of the response
 *	packet fields.
 */
int
SendArpResp(struct ether_header *re)
{
	struct ether_header *te;
	struct arphdr *ta, *ra;

	te = (struct ether_header *) getXmitBuffer();
	memcpy((char *)&(te->ether_shost), (char *)BinEnetAddr,6);
	memcpy((char *)&(te->ether_dhost),(char *)&(re->ether_shost),6);
	te->ether_type = ecs(ETHERTYPE_ARP);

	ta = (struct arphdr *) (te + 1);
	ra = (struct arphdr *) (re + 1);
	ta->hardware = ra->hardware;
	ta->protocol = ra->protocol;
	ta->hlen = ra->hlen;
	ta->plen = ra->plen;
	ta->operation = ARP_RESPONSE;
	memcpy((char *)ta->senderha, (char *)BinEnetAddr,6);
	memcpy((char *)ta->senderia, (char *)ra->targetia,4);
	memcpy((char *)ta->targetha, (char *)ra->senderha,6);
	memcpy((char *)ta->targetia, (char *)ra->senderia,4);
	self_ecs(ta->hardware);
	self_ecs(ta->protocol);
	self_ecs(ta->operation);
	sendBuffer(ARPSIZE);
	return(0);
}

/* SendArpRequest():
 * If 'probe' is non-zero, then the request is an arp-probe.
 * Refer to RFC3927 for more on that.
 */
static int
SendArpRequest(uchar *ip, int probe)
{
	struct ether_header *te;
	struct arphdr *ta;

	/* Populate the ethernet header: */
	te = (struct ether_header *) getXmitBuffer();
	memcpy((char *)&(te->ether_shost), (char *)BinEnetAddr,6);
	memcpy((char *)&(te->ether_dhost), (char *)BroadcastAddr,6);
	te->ether_type = ecs(ETHERTYPE_ARP);

	/* Populate the arp header: */
	ta = (struct arphdr *) (te + 1);
	ta->hardware = ecs(1);		/* 1 for ethernet */
	ta->protocol = ecs(ETHERTYPE_IP);
	ta->hlen = 6;		/* Length of hdware (ethernet) address */
	ta->plen = 4;		/* Length of protocol (ip) address */
	ta->operation = ecs(ARP_REQUEST);
	memcpy((char *)ta->senderha, (char *)BinEnetAddr,6);
	if (probe) {
		memset((char *)ta->senderia,0,4);
		memset((char *)ta->targetha,0,6);
	}
	else {
		memcpy((char *)ta->senderia, (char *)BinIpAddr,4);
		memcpy((char *)ta->targetha, (char *)BroadcastAddr,6);
	}
	memcpy((char *)ta->targetia, (char *)ip,4);
	sendBuffer(ARPSIZE);
	return(0);
}

/* GetBinNetMask():
 *  Return a subnet mask in binary form.
 *	Since this function needs a netmask, if NETMASK is not set, then
 *	it uses the default based on the upper 3 bits of the IP address
 *	(refer to TCP/IP Illustrated Volume 1 Pg8 for details).
 */
void
GetBinNetMask(uchar *binnetmask)
{
	char	*nm;

	nm = getenv("NETMASK");
	if (!nm) {
		memset((char *)binnetmask,0xff,4);
		if ((BinIpAddr[0] & 0xe0) == 0xc0) {		/* Class C */
			binnetmask[3] = 0;
		}
		else if ((BinIpAddr[0] & 0xc0) == 0x80) {	/* Class B */
			binnetmask[3] = 0;
			binnetmask[2] = 0;
		}
		else if ((BinIpAddr[0] & 0x80) == 0x00) {	/* Class A */
			binnetmask[3] = 0;
			binnetmask[2] = 0;
			binnetmask[1] = 0;
		}
	}
	else
		IpToBin(nm,binnetmask);
}

/* IpIsOnThisNet():
 *	Return 1 if the incoming ip is on this subnet; else 0.
 *	For each bit in the netmask that is set to 1...
 *	If the corresponding bits in the incoming IP and this board's IP
 *	do not match, return 0; else return 1.
 */
int
IpIsOnThisNet(uchar *ip)
{
	int		i;
	uchar	binnetmask[8];

	GetBinNetMask(binnetmask);

	for(i=0;i<4;i++) {
		if ((ip[i] & binnetmask[i]) != (BinIpAddr[i] & binnetmask[i])) {
			return(0);
		}
	}
	return(1);
}

/* Arp():
 *	If no args, just dump the arp cache.
 *	If arg is present, then assume it to be an ip address.  
 *	Check the arp cache for the presence of that ip<->correlation, if
 *	present, print it; else issue and arp request for that IP and wait
 *	for a response.
 */

char *ArpHelp[] = {
	"Address resolution protocol",
	"-[flps:v] [IP]",
#if INCLUDE_VERBOSEHELP
	" -f   flush cache",
	" -l   dynamic config using link-local arp probe",
	" -p   proxy arp",
	" -s{eadr}   store eadr/IP into cache",
#if INCLUDE_ETHERVERBOSE
	" -v   verbose",
#endif
#endif
	0,
};

int
Arp(int argc,char *argv[])
{
	int		opt, proxyarp, llad;
	char	binip[8], binether[8], *storeether;

	llad = proxyarp = 0;
	storeether = (char *)0;
	while ((opt=getopt(argc,argv,"flps:v")) != -1) {
		switch(opt) {
		case 'f':	/* Flush current arp cache */
			ArpFlush();
			break;
		case 'l':			/* Dynamic ip-config using link-local addressing */
			llad = 1;	/* (refer to RFC 3927) */
			break;
		case 's':	/* Store specified IP/MAC combination in arp cache. */
			storeether = optarg;
			break;
		case 'p':	/* Assume gateway will run proxy arp */
			proxyarp = 1;
			break;
#if INCLUDE_ETHERVERBOSE
		case 'v':	/* Enable verbosity for ARP. */
			EtherVerbose |= SHOW_ARP;
			break;
#endif
		default:
			return(CMD_PARAM_ERROR);
		}
	}

	/* If llad flag is set, then we do a dynamic configuration using the
	 * link-local protocol as described in RFC 3927...
	 */
	if (llad) {
		char	buf[32];
		struct	elapsed_tmr tmr;
		int		i, retry, conflicts, psval;

#if INCLUDE_DHCPBOOT
		dhcpDisable();
#endif
		probeAbort = retry = conflicts = 0;

		/* Use MAC address to establish a uniformly selected pseudo random
		 * value between 0 and 1000...
	 	 */
		psval = (BinEnetAddr[5] * 4);

		monDelay(psval);

		if (argc == (optind+1))
			IpToBin((char *)argv[optind],(unsigned char *)&probeIP);
		else
			llas((char *)&probeIP);

nextllas:
		SendArpRequest((uchar *)&probeIP,1);
		startElapsedTimer(&tmr,ANNOUNCE_WAIT);
		while(!msecElapsed(&tmr)) {	
			if (probeAbort) {
				llas((char *)&probeIP);
				probeAbort = retry = 0;
				monDelay(psval + PROBE_MIN);
				goto nextllas;
			}
			if (EtherFromCache((uchar *)&probeIP)) {
				if (++conflicts > MAX_CONFLICTS)
					monDelay(RATE_LIMIT_INTERVAL);
				else
					monDelay(psval + PROBE_MIN);
				if (++retry == PROBE_NUM) {
					llas((char *)&probeIP);
					retry = 0;
				}
				goto nextllas;
			}
			pollethernet();
		}
		/* If we're here, then we found an IP address that is not being
		 * used on the local subnet...
		 */
		setenv("IPADD",IpToString(probeIP,buf));
		setenv("NETMASK","255.255.0.0");
		setenv("GIPADD",0);
#if INCLUDE_ETHERVERBOSE
		EthernetStartup(EtherVerbose,0);
#else
		EthernetStartup(0,0);
#endif
		for(i=0;i<ANNOUNCE_NUM;i++) {
			SendArpRequest(BinIpAddr,0);
			monDelay(ANNOUNCE_INTERVAL);
		}
	}
	else if (argc == (optind+1)) {
		IpToBin((char *)argv[optind],(unsigned char *)binip);
		if (storeether) {
			EtherToBin((char *)storeether, (unsigned char *)binether);
			ArpStore((unsigned char *)binip,(unsigned char *)binether);
		}
		else {
			if (ArpEther((unsigned char *)binip,(unsigned char *)0,proxyarp))
				ArpShow(binip);
		}
	}
	else
		ArpShow(0);
#if INCLUDE_ETHERVERBOSE
	EtherVerbose &= ~ SHOW_ARP;
#endif
	return(CMD_SUCCESS);
}

/* ArpEther():
 *	Retrieve the ethernet address that corresponds to the incoming IP 
 *	address.  First check the local ArpCache[], then if not found issue
 *	an ARP request... If the incoming IP is on this net, then issue the
 *	request for the MAC address of that IP; otherwise, issue the request
 *	for this net's GATEWAY.
 */
uchar *
ArpEther(uchar *binip, uchar *ecpy, int proxyarp)
{
	char	*gip;
	struct	elapsed_tmr tmr;
	uchar	gbinip[8], *Ip, *ep;
	int		timeoutsecs, retry;

	if (!EtherIsActive) {
		printf("Ethernet disabled\n");
		return(0);
	}

	/* First check local cache. If found, return with pointer to MAC. */
	ep = EtherFromCache(binip);
	if (ep) {
		if (ecpy) {
			memcpy((char *)ecpy, (char *)ep,6);
			ep = ecpy;
		}
		return(ep);
	}

	retry = 0;
	RetransmitDelay(DELAY_INIT_ARP);
	while(1) {
		/* If IP is not on this net, then get the GATEWAY IP address. */
		if (!proxyarp && !IpIsOnThisNet(binip)) {
			gip = getenv("GIPADD");
			if (gip) {
				IpToBin(gip,gbinip);
				if (!IpIsOnThisNet(gbinip)) {
					printf("GIPADD/IPADD subnet confusion.\n");
					return(0);
				}
				ep = EtherFromCache(gbinip);
				if (ep) {
					if (ecpy) {
						memcpy((char *)ecpy, (char *)ep,6);
						ep = ecpy;
					}
					return(ep);
				}
				SendArpRequest(gbinip,0);
				Ip = gbinip;
			}
			else {
				SendArpRequest(binip,0);
				Ip = binip;
			}
		}
		else {
			SendArpRequest(binip,0);
			Ip = binip;
		}
		if (retry) {
			printf("  ARP Retry #%d (%d.%d.%d.%d)\n",retry,
				binip[0],binip[1],binip[2],binip[3]);
		}

		/* Now that the request has been issued, wait for a while to see if
		 * the ARP cache is loaded with the result of the request.
		 */
		timeoutsecs = RetransmitDelay(DELAY_OR_TIMEOUT_RETURN);
		if (timeoutsecs == RETRANSMISSION_TIMEOUT)
			break;
		startElapsedTimer(&tmr,timeoutsecs*1000);
		while(!msecElapsed(&tmr)) {
			ep = EtherFromCache(Ip);
			if (ep)
				break;

			pollethernet();
		}
		if (ep) {
			if (ecpy) {
				memcpy((char *)ecpy, (char *)ep,6);
				ep = ecpy;
			}
			return(ep);
		}
		RetransmitDelay(DELAY_INCREMENT);
		retry++;
	}
#if INCLUDE_ETHERVERBOSE
	if ((EtherVerbose & SHOW_ARP) && (retry))
		printf("  ARP giving up\n");
#endif
	return(0);
}

/* processRARP();
 *	Called by the fundamental ethernet driver code to process a RARP
 *	request.
 */
int
processRARP(struct ether_header *ehdr,ushort size)
{
	struct	arphdr *arpp;

	arpp = (struct arphdr *)(ehdr+1);
	self_ecs(arpp->hardware);
	self_ecs(arpp->protocol);
	self_ecs(arpp->operation);

	switch(arpp->operation) {
	case RARP_RESPONSE:
		if (!memcmp((char *)arpp->targetha, (char *)BinEnetAddr,6)) {
#if INCLUDE_ETHERVERBOSE
			if (EtherVerbose & SHOW_ARP) {
				printf("  RARP Response from %d.%d.%d.%d\n",
				    arpp->senderia[0],arpp->senderia[1],
				    arpp->senderia[2],arpp->senderia[3]);
				printf("  MY IP: from %d.%d.%d.%d\n",
				    arpp->targetia[0],arpp->targetia[1],
				    arpp->targetia[2],arpp->targetia[3]);
			}
#endif
			memcpy((char *)BinIpAddr, (char *)arpp->targetia,6);
		}
		break;
	case RARP_REQUEST:
		break;
	default:
		printf("  Invalid RARP operation: 0x%x\n",arpp->operation);
		return(-1);
	}
	return(0);
}

/* processARP();
 *	Called by the fundamental ethernet driver code to process a ARP
 *	request.
 */
char *
processARP(struct ether_header *ehdr,ushort size)
{
	struct	arphdr *arpp;

	arpp = (struct arphdr *)(ehdr+1);
	self_ecs(arpp->hardware);
	self_ecs(arpp->protocol);
	self_ecs(arpp->operation);

	/* If the sender IP address is all zeroes, then assume this is
	 * an arp probe.  If we are currently doing a probe, and the
	 * address we are probing matches the target IP address of this
	 * probe, then set a flag that will abort the current probe.
	 */
	if ((arpp->senderia[0] == 0) && (arpp->senderia[1] == 0) &&
		(arpp->senderia[2] == 0) && (arpp->senderia[3] == 0)) {
			if (memcmp((char *)arpp->targetia,(char *)&probeIP,4) == 0)
				probeAbort = 1;
	}

	switch(arpp->operation) {
	case ARP_REQUEST:
		if (!memcmp((char *)arpp->targetia, (char *)BinIpAddr,4)) {
			ArpStore(arpp->senderia,arpp->senderha);
			SendArpResp(ehdr);
		}
		break;
	case ARP_RESPONSE:
		if (!memcmp((char *)arpp->targetia, (char *)BinIpAddr,4)) {
			if (!memcmp((char *)arpp->targetia,(char *)arpp->senderia,4))
				printf("WARNING: IP %s may be in use on network\n",IPadd);

			ArpStore(arpp->senderia,arpp->senderha);
		}
		break;
	default:
		printf("  Invalid ARP operation: 0x%x\n",arpp->operation);
		printPkt(ehdr,(int)size,ETHER_INCOMING);
		return((char *)0);
	}
	return((char *)(arpp + 1));
}

/* sendGratuitousARP():
 * As defined in RFC 3220...
 *
 *  An ARP packet sent by a node in order to spontaneously
 *  cause other nodes to update an entry in their ARP cache.
 *
 * Issue a "gratuitous ARP" (RFC 3220) for two reasons...
 * - make sure no other host is already using this IP address.
 * - cause other devices on cable to update their arp cache.
 */
void
sendGratuitousArp(void)
{
	SendArpRequest(BinIpAddr,0);
}

/* llas():
 * Link local address selection.
 * Referring to sec 2.1 of RFC3927, select an address using a pseudo-random
 * number generator to assign a number in the range from
 * 169.254.1.0 (0xa9fe0100) thru 169.254.254.255 (0xa9fefeff).
 * To do this, we run a 32-bit CRC on the 6-byte MAC address, then add the
 * least significant 8 bits of that value to the base of the address
 * range.  Each successive time this function is called, then increment
 * by the least significant 4 bits of the LSB of the MAC. 
 */
#define LLAD_BEGIN	0xa9fe0100
#define LLAD_END	0xa9fefeff

static void
llas(char *ipptr)
{
	static char beenhere;
	static unsigned long llad;
	unsigned long pseudorandval, tmp;

	if (beenhere == 0) {
		pseudorandval = crc32(BinEnetAddr,6);
		llad = LLAD_BEGIN + (pseudorandval & 0xff); 
	}
	else {
		pseudorandval = (unsigned long)(BinEnetAddr[5] & 0xf);
		llad += pseudorandval;
		if (llad >= LLAD_END)
			llad = LLAD_BEGIN + pseudorandval + beenhere;
	}
	beenhere++;

	printf("LLAD: %d.%d.%d.%d\n",
		(llad & 0xff000000) >> 24, (llad & 0xff0000) >> 16,
		(llad & 0xff00) >> 8, (llad & 0xff));

	tmp = ecl(llad);

	if (ipptr)
		memcpy(ipptr,(char *)&tmp,4);
}

#endif
