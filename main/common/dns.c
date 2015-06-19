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
 * dns.c:
 *
 * This file implements a basic DNS client and some form of a multicast-DNS
 * reponder.  The functionality provides the basic ability to retrieve an IP
 * address from a domain name.
 * Refer to RFC 1035 section 4.1 for the packet format.
 * 
 *
 * Original author:     Ed Sutter (ed.sutter@alcatel-lucent.com)
 *
 */
#include "config.h"
#if INCLUDE_DNS
#include "endian.h"
#include "genlib.h"
#include "stddefs.h"
#include "ether.h"
#include "cli.h"
#include "timer.h"

const char mDNSIp[] = { 224, 0, 0, 251 };
const char mDNSMac[] = { 0x01, 0x00, 0x5e, 0x00, 0x00, 0xfb };

char * dnsErrStr(int errno);

short DnsPort;
static unsigned short dnsId;
static unsigned long dnsIP;
static char dnsErrno, dnsWaiting, dnsCacheInitialized;
static struct dnscache hostnames[MAX_CACHED_HOSTNAMES];

/* ip_to_bin...
 * Essentially identical to IpToBin() in ethernet.c, but without
 * the error message.
 */
int
ip_to_bin(char *ascii,uchar *binary)
{
	int	i, digit;
	char	*acpy;

	acpy = ascii;
	for(i=0;i<4;i++) {
		digit = (int)strtol(acpy,&acpy,10);
		if (((i != 3) && (*acpy++ != '.')) ||
			((i == 3) && (*acpy != 0)) ||
			(digit < 0) || (digit > 255)) {
			return(-1);
		}
		binary[i] = (uchar)digit;
	}
	return(0);
}


/* getHostAddr():
 * This function is a simplified version of gethostbyname().
 * Given a domain name, this function will first query a
 * locally maintained cache then, if not found there, it will
 * issue a DNS query to retrieve the hosts IP address.
 */
unsigned long
getHostAddr(char *hostname)
{
	short	port;
	struct ip *ipp;
	struct Udphdr *udpp;
	struct dnshdr *dnsp;
	struct elapsed_tmr tmr;
	struct ether_header *enetp;
	uchar	binenet[8], *enetaddr;
	unsigned long	srvrip, binip;
	char	*pp, *np, *srvrname, *dot;
	int		i, namelen, pktsize, ip_len;

	/* First check to see if the incoming host name is simply a
	 * decimal-dot-formatted IP address.  If it is, then just
	 * convert it to a 32-bit long and return here...
	 */
	if (ip_to_bin(hostname,(uchar *)&binip) == 0)
		return(binip);
		
	if (!dnsCacheInitialized)
		dnsCacheInit();

	dnsErrno = DNSERR_NULL;

	/* First try to find the hostname in our local cache...
	 */
	for(i=0;i<MAX_CACHED_HOSTNAMES;i++) {
		if (strcmp(hostnames[i].name,hostname) == 0) 
			return(hostnames[i].addr);
	}

	/* If not in the cache, we query the network.  First look
	 * for a DNSSRVR defined in the environment.  If found, use
	 * it; else, use IP broadcast.
	 */
	
	/* See if this is a mDNS request...
	 */
	if (strstr(hostname,".local")) {
		DnsPort = port = DNSMCAST_PORT;
		srvrip = htonl(DNSMCAST_IP);
		memcpy((char *)binenet,(char *)mDNSMac,sizeof(mDNSMac));
		enetaddr = binenet;
	}
	else {
		port = IPPORT_DNS;
		DnsPort = port+1;
		srvrname = getenv("DNSSRVR");
		if (srvrname == (char *)0)
			srvrip = 0xffffffff;
		else {
			if (IpToBin(srvrname,(uchar *)&srvrip) < 0)
				return(0);
		}

		/* Get the ethernet address for the IP:
		 */
		enetaddr = ArpEther((uchar *)&srvrip,binenet,0);
		if (!enetaddr) {
			printf("ARP failed for 0x%x\n",srvrip);
			return(0);
		}
	}

	/* Retrieve an ethernet buffer from the driver and populate the
	 * ethernet level of packet:
	 */
	enetp = (struct ether_header *) getXmitBuffer();
	memcpy((char *)&enetp->ether_shost,(char *)BinEnetAddr,6);
	memcpy((char *)&enetp->ether_dhost,(char *)binenet,6);
	enetp->ether_type = htons(ETHERTYPE_IP);

	/* Move to the IP portion of the packet and populate it
	 * appropriately:
	 */
	ipp = (struct ip *) (enetp + 1);
	ipp->ip_vhl = IP_HDR_VER_LEN;
	ipp->ip_tos = 0;
	ipp->ip_id = ipId();
	ipp->ip_off = 0;
	ipp->ip_ttl = UDP_TTL;
	ipp->ip_p = IP_UDP;
	memcpy((char *)&ipp->ip_src.s_addr,(char *)BinIpAddr,4);
	memcpy((char *)&ipp->ip_dst.s_addr,(char *)&srvrip,4);

	/* Now UDP...
	 */
	udpp = (struct Udphdr *) (ipp + 1);
	udpp->uh_sport = htons(DnsPort);
	udpp->uh_dport = htons(port);

	/* Finally, the DNS data ...
	 */
	dnsp = (struct dnshdr *)(udpp+1);

	/* Build the message.  This query supports a single internet
	 * host-address question.
	 * 
	 * Fixed header...
	 */
	dnsId = ipId(); 
	dnsp->id = htons(dnsId);		/* Unique id */
	dnsp->param = 0;				/* Parameter field */
	dnsp->num_questions = htons(1);	/* # of questions */
	dnsp->num_answers = 0;			/* # of answers */
	dnsp->num_authority = 0;		/* # of authority */
	dnsp->num_additional = 0;		/* # of additional */

	/* The formatted name list..
	 * Each name is preceded by a single-byte length, so for our
	 * query, the list is "LNNNLNNNLNNN0", where...
	 *	'L' is the single-byte length of the name.
	 *	'NN..N' is variable-lenght domain.
	 *	'0' is the length of the next name in the list; hence,
	 *		indicating the end of the list.
	 *
	 * For each '.' (dot)  in the hostname, there is a 
	 * LNNN pair.
	 */
	pp = (char *)dnsp->question;
	np = (char *)hostname;
	namelen = strlen(hostname);
	do {
		dot = strchr(np,'.');
		if (dot) {
			*pp++ = dot-np;
			memcpy(pp,np,dot-np);
			pp += (dot-np);
			np = dot + 1;
		}
		else {
			*pp++ = strlen(np);
			strcpy(pp,np);
		}
	} while(dot);


	/* Since the size of the name can be arbitrary (not aligned),
	 * we must populate the TYPE and CLASS fields one byte at
	 * a time...
	 */
	pp += (strlen(np) + 1);
	*pp++ = 0;
	*pp++ = 1;	/* type = 'A' (host address) */
	*pp++ = 0;
	*pp = 1;	/* class = 'IN' (the internet) */

	/* Send the DNS query:
	 * Total message size is...
	 * FIXED_HDR_SIZE + NAME_SIZE + TYPE_SIZE + CLASS_SIZE + (NAMELEN_SIZE*2)
	 * where...
	 * 	FIXED_HDR_SIZE	= 12
	 *  NAME_SIZE		= strlen(hostname) = namelen
	 *  TYPE_SIZE		= sizeof(short) = 2
	 *  CLASS_SIZE		= sizeof(short) = 2
	 *  NAMELEN_SIZE	= sizeof(char) = 1
	 * 
	 * There are 2 name lengths.  The first one is the size of the host
	 * name we are querying for and the second one is zero (indicating no
	 * more names in the list).
	 * So, the total length of the packet is <namelen + 18>.
	 */

	pktsize = namelen+18;
	ip_len = sizeof(struct ip) + sizeof(struct Udphdr) + pktsize;
	ipp->ip_len = htons(ip_len);
	udpp->uh_ulen = htons((ushort)(ip_len - sizeof(struct ip)));

	ipChksum(ipp);			/* Compute csum of ip hdr */
	udpChksum(ipp);			/* Compute UDP checksum */

//	printf("Sending DNS rqst id=%04x (%d %d)\n",dnsId,pktsize,ip_len);

	dnsWaiting = 1;
	sendBuffer(ETHERSIZE + IPSIZE + UDPSIZE + pktsize);

	/* Wait for 3 seconds for a response...
	 */
	startElapsedTimer(&tmr,3000);
	while(!msecElapsed(&tmr)) {
		if (dnsErrno != DNSERR_NULL)
			break;
		pollethernet();
	}
	if (dnsErrno == DNSERR_COMPLETE) {
		dnsCacheAdd(hostname,dnsIP);
		shell_sprintf("DNSIP","%d.%d.%d.%d",
			IP1(dnsIP),IP2(dnsIP),IP3(dnsIP), IP4(dnsIP));
		return(dnsIP);
	}
	else {
		if (dnsErrno == DNSERR_NULL)
			printf("DNS attempt timeout\n");
		else 
			printf("DNSErr: %s\n",dnsErrStr((int)dnsErrno));
		setenv("DNSIP",0);
	}
	return(0);
}

/* Note two different processDNS/processMCASTDNS functions...
 * processDNS() provides the necessary code to deal with responses
 * that we should expect to get as a result of issuing a dns request.
 * processMCASTDNS() provides the necessary code to deal with requests
 * from other machines querying for our hostname/ip info.
 */

/* processDNS():
 * This function provides the recieving half of the DNS query above.
 */
int
processDNS(struct ether_header *ehdr,ushort size)
{
	int 	i;
	char 	*pp;
	struct	ip *ihdr;
	struct	Udphdr *uhdr;
	struct	dnshdr *dhdr;
	unsigned short	rtype, qtot, atot;

	if (dnsWaiting == 0)
		return(0);

	dnsWaiting = 0;
	ihdr = (struct ip *)(ehdr + 1);
	uhdr = (struct Udphdr *)((char *)ihdr + IP_HLEN(ihdr));
	dhdr = (struct dnshdr *)(uhdr + 1);

//	printf("DNS: (%d)\n",size);
//	printf("  dnsid: %04x\n",htons(dhdr->id));
//	printf("  param: %04x\n",htons(dhdr->param));
//	printf("  quest: %04x\n",htons(dhdr->num_questions));
//	printf("  answe: %04x\n",htons(dhdr->num_answers));
//	printf("  autho: %04x\n",htons(dhdr->num_authority));
//	printf("  addit: %04x\n",htons(dhdr->num_additional));
//	printMem(dhdr->question, size - UDPSIZE - IPSIZE - ETHERSIZE - 12,1);  

	/* Verify the DNS response...
	 */
	if ((htons(dhdr->param) & 0x8000) == 0) {			/* response? */
		dnsErrno = DNSERR_NOTARESPONSE;
		return(0);
	}
	if (htons(dhdr->id) != dnsId) {						/* correct id? */
		dnsErrno = DNSERR_BADRESPID;
		return(0);
	}
	if ((rtype = (htons(dhdr->param) & 0xf)) != 0) {	/* response normal? */
		switch(rtype) {
			case 1:
				dnsErrno = DNSERR_FORMATERR;
				break;
			case 2:
				dnsErrno = DNSERR_SRVRFAILURE;
				break;
			case 3:
				dnsErrno = DNSERR_NAMENOEXIST;
				break;
			default:
				dnsErrno = DNSERR_BADRESPTYPE;
				break;
		}
		return(0);
	}
	qtot = htons(dhdr->num_questions);
	if ((atot = htons(dhdr->num_answers)) < 1) {	/* answer count >= 1? */
		dnsErrno = DNSERR_BADANSWRCOUNT;
		return(0);
	}

	/* At this point we can assume that the received packet format
	 * is ok.  Now we need to parse the packet for the "answer" to 
	 * our query...
	 */

	/* Set 'pp' to point to the start of the question list.
	 * There should only be one question in the response.
	 */
	pp = (char *)&dhdr->question;

	/* Skip over the questions:
	 */
	for(i=0;i<qtot;i++) {
		while(*pp) 			/* while 'L' is nonzero */
			pp += (*pp + 1);
		pp += 5;			/* Account for last 'L' plus TYPE/CLASS */
	}

	/* The 'pp' pointer is now pointing a list of resource records that
	 * correspond to the answer list.  It is from this list that we
	 * must retrieve the information we are looking for...
	 */

	for(i=0;i<atot;i++) {
		unsigned short type, len;

		/* The first portion of the record is the resource domain name
		 * and it may be literal string (a 1-octet count followed by
		 * characters that make up the name) or a pointer to a literal
		 * string.
		 */
		if ((*pp & 0xc0) == 0xc0) {	/* compressed? (section 4.1.4 RFC1035) */
			pp += 2;
		}
		else {
			while(*pp) 				/* while 'L' is nonzero */
				pp += (*pp + 1);
			pp += 1;
		}
		memcpy((char *)&type,pp,2);
		type = htons(type);
		pp += 8;
		memcpy((char *)&len,pp,2);
		len = htons(len);
		if ((type == TYPE_A) && (len == 4)) {
			pp += 2;
			memcpy((char *)&dnsIP,pp,4);
			dnsIP = htonl(dnsIP);
		}
		else 
			pp += (len+2);
	}
	dnsErrno = DNSERR_COMPLETE;
	return(0);
}

/* processMCASTDNS():
 * If we're here, then we've received a request over the Multi-cast
 * DNS address for some host name.  If the hostname in this request
 * matches this board's hostname, then return this board's IP address.
 * If the shell variable HOSTNAME isn't set, then just return immediately.
 */
int
processMCASTDNS(struct ether_header *ehdr,ushort size)
{
	int 	l1, ql = 0;
	struct	ip *ihdr;
	char 	*pp, *myname;
	struct	Udphdr *uhdr;
	struct	dnshdr *dhdr;

	ihdr = (struct ip *)(ehdr + 1);
	uhdr = (struct Udphdr *)(ihdr + 1);
	dhdr = (struct dnshdr *)(uhdr + 1);

	// If this message is DNS response, branch to processDNS()...
	if ((htons(dhdr->param) & 0x8000) == 0x8000) {
		processDNS(ehdr,size);
		return(0);
	}

	if ((myname = getenv("HOSTNAME")) == 0)
		return(0);

	// If this isn't a normal query type, return...
	if ((htons(dhdr->param) & 0xf) != 0)
		return(0);

	// If there isn't exactly 1 question, return...
	if (htons(dhdr->num_questions) != 1)
		return(0);

	/* At this point we can assume that the received packet format
	 * is ok.  Now we need to parse the packet for the "question"...
	 */

	/* Set 'pp' to point to the start of the question list.
	 */
	pp = (char *)dhdr->question;
	l1 = *pp;
	while(*pp) {
		ql += *pp;
		ql++;
		pp += (*pp + 1);
	}

	/* If the name in the question matches our hostname, then send a
	 * reply...
	 *
	 * Referring to top of pg 20 of the document
	 *  http://files.multicastdns.org/draft-cheshire-dnsext-multicastdns.txt...
	 *
	 * Multicast DNS responses MUST be sent to UDP port 5353 on the
	 * 224.0.0.251 multicast address.
	 */
	if ((l1 == strlen(myname)) &&
		(memcmp(myname,(char *)&dhdr->question[1],l1) == 0)) {
		struct ip *ti, *ri;
		struct Udphdr *tu, *ru;
		struct ether_header *te;
		struct	dnshdr *td;
		short	type, class;
		long	ttl;
		struct	elapsed_tmr tmr;

		te = EtherCopy(ehdr);
		ti = (struct ip *) (te + 1);
		ri = (struct ip *) (ehdr + 1);
		ti->ip_vhl = ri->ip_vhl;
		ti->ip_tos = ri->ip_tos;
		ti->ip_id = ri->ip_id;
		ti->ip_off = ri->ip_off;
		ti->ip_ttl = UDP_TTL;
		ti->ip_p = IP_UDP;
		memcpy((char *)&(ti->ip_src.s_addr),(char *)BinIpAddr,
			sizeof(struct in_addr));
		memcpy((char *)&(ti->ip_dst.s_addr),(char *)mDNSIp,
			sizeof(struct in_addr));

		tu = (struct Udphdr *) (ti + 1);
		ru = (struct Udphdr *) (ri + 1);
		tu->uh_sport = ru->uh_dport;
		tu->uh_dport = htons(DNSMCAST_PORT);

		td = (struct dnshdr *)(tu+1);

		/* Flags: */
		td->id = dhdr->id;
		td->param = htons(0x8400);		
		td->num_questions = 0;
		td->num_answers = htons(1);
		td->num_authority = 0;
		td->num_additional = 0;

		/* Answers: */
		pp = (char *)td->question;
		memcpy(pp,(char *)dhdr->question,ql+1);
		pp += (ql+1);

		type = htons(TYPE_A);
		memcpy(pp,(char *)&type,2);
		pp += 2;

		class = htons(CLASS_IN);
		memcpy(pp,(char *)&class,2);
		pp += 2;
		
		ttl = htonl(900);
		memcpy(pp,(char *)&ttl,4);
		pp += 4;

		*pp++ = 0;	/* 2-byte length of address */
		*pp++ = 4;	
		memcpy(pp,(char *)BinIpAddr,4);

		tu->uh_ulen = ecs((UDPSIZE + 27 + ql));
		ti->ip_len = ecs((UDPSIZE + 27 + ql + sizeof(struct ip)));

		ipChksum(ti);		/* Compute checksum of ip hdr */
		udpChksum(ti);		/* Compute UDP checksum */

		/* Delay for some random time between 20-120msec...
		 */
		startElapsedTimer(&tmr,20 + (BinEnetAddr[5] & 0x3f));
		while(!msecElapsed(&tmr));
		
		sendBuffer(ETHERSIZE + IPSIZE + UDPSIZE + 27 + ql);
	}
	return(0);
}

/* DNS Cache utilities:
 */
void
dnsCacheInit(void)
{
	int	i;

	for(i=0;i<MAX_CACHED_HOSTNAMES;i++) {
		hostnames[i].idx = 0;
		hostnames[i].addr = 0;
		hostnames[i].name[0] = 0;
	}
	dnsCacheInitialized = 1;
}

int
dnsCacheDump(void)
{
	int	i, tot;
	struct	dnscache *hnp;

	tot = 0;
	hnp = hostnames;
	for(i=0;i<MAX_CACHED_HOSTNAMES;i++,hnp++) {
		if (hnp->addr) {
			printf("%3d %30s: %d.%d.%d.%d\n",hnp->idx,hnp->name, IP1(hnp->addr),
				IP2(hnp->addr), IP3(hnp->addr), IP4(hnp->addr));
			tot++;
		}
	}
	return(tot);
}

int
dnsCacheAdd(char *name, unsigned long inaddr)
{
	int	i, li, lowest;
	struct	dnscache *hnp;
	static	int idx = 0;

	if (!dnsCacheInitialized)
		dnsCacheInit();

	/* Validate incoming name size:
	 */
	if ((strlen(name) >= MAX_HOSTNAME_SIZE) || (inaddr == 0))
		return(-1);

	lowest = 0x70000000; 

	/* First look for an empty slot...
	 */
	hnp = hostnames;
	li = MAX_CACHED_HOSTNAMES;
	for(i=0;i<MAX_CACHED_HOSTNAMES;i++,hnp++) {
		if (hnp->addr == 0) {
			strcpy(hnp->name,name);
			hnp->addr = inaddr;
			hnp->idx = idx++;
			return(1);
		}
		else {
			if (hnp->idx < lowest) {
				lowest = hnp->idx;
				li = i;
			}
		}
	}

	if (i == MAX_CACHED_HOSTNAMES)
		return(-1);
	
	/* If all slots are filled, use the slot that had the
	 * the lowest idx value (this would be the oldest entry)...
	 */
	hnp = &hostnames[li];
	strcpy(hnp->name,name);
	hnp->addr = inaddr;
	hnp->idx = idx++;
	return(1);
}

int
dnsCacheDelAddr(unsigned long addr)
{
	int	i;

	if (!dnsCacheInitialized) {
		dnsCacheInit();
		return(0);
	}

	for(i=0;i<MAX_CACHED_HOSTNAMES;i++) {
		if (hostnames[i].addr == addr) {
			hostnames[i].name[0] = 0;
			hostnames[i].addr = 0;
			return(1);
		}
	}
	return(0);
}

int
dnsCacheDelName(char *name)
{
	int	i;

	if (!dnsCacheInitialized) {
		dnsCacheInit();
		return(0);
	}

	for(i=0;i<MAX_CACHED_HOSTNAMES;i++) {
		if (strcmp(hostnames[i].name,name) == 0) {
			hostnames[i].name[0] = 0;
			hostnames[i].addr = 0;
			return(1);
		}
	}
	return(0);
}

struct dnserr dnsErrTbl[] = {
	{ DNSERR_NOSRVR,			"no dns server" },
	{ DNSERR_SOCKETFAIL,		"socket fail" },
	{ DNSERR_FORMATERR,			"format error" },
	{ DNSERR_SRVRFAILURE,		"server error" },
	{ DNSERR_NAMENOEXIST,		"no name exists" },
	{ DNSERR_BADRESPTYPE,		"bad dns server response" },
	{ DNSERR_BADQSTNCOUNT,		"bad question count" },
	{ DNSERR_BADANSWRCOUNT,		"bad answer count" },
	{ DNSERR_NOTARESPONSE,		"replay not a dns response" },
	{ DNSERR_BADRESPID,			"bad dns response id" },

	/* Must be last entry in table: */
	{ DNSERR_NULL,				"no error" }
};

char *
dnsErrStr(int errno)
{
	struct	dnserr *dep;

	dep = dnsErrTbl;
	while(dep->errno != DNSERR_NULL) {
		if (dep->errno == errno)
			return(dep->errstr);
		dep++;
	}
	return("invalid dns errno");
}

char *DnsHelp[] = {
	"DNS Client",
	"{hostname} | {cmd args}",
	" Valid cmd values:",
	"  add   {name} {ip}",
	"  cache {dump | init}",
	"  mdns {on | off}",
	"  del   {name}",
	0,
};

int
DnsCmd(int argc, char *argv[])
{
	unsigned long addr;

	if (argc == 2) {
		if ((addr = getHostAddr(argv[1])) != 0) {
			printf("%s: %d.%d.%d.%d\n",argv[1],
				IP1(dnsIP),IP2(dnsIP),IP3(dnsIP), IP4(dnsIP));
		}
	}
	else if (argc == 3) {
		if (strcmp(argv[1],"cache") == 0) {
			if (strcmp(argv[2],"dump") == 0)
				dnsCacheDump();
			else if (strcmp(argv[2],"init") == 0)
				dnsCacheInit();
			else
				return(CMD_PARAM_ERROR);
		}
		else if (strcmp(argv[1],"mdns") == 0) {
			extern void enableMulticastReception(void);
			extern void disableMulticastReception(void);

			if (strcmp(argv[2],"on") == 0)
				enableMulticastReception();
			else if (strcmp(argv[2],"off") == 0)
				disableMulticastReception();
			else
				return(CMD_PARAM_ERROR);
		}
		else if (strcmp(argv[1],"del") == 0) {
			dnsCacheDelName(argv[2]);
		}
		else
			return(CMD_PARAM_ERROR);
	}
	else if (argc == 4) {
		if (strcmp(argv[1],"add") == 0) {
			if (IpToBin(argv[3],(uchar *)&addr) < 0)
				return(CMD_FAILURE);
			dnsCacheAdd(argv[2],addr);
		}
		else
			return(CMD_PARAM_ERROR);
	}
	else
		return(CMD_PARAM_ERROR);

	return(CMD_SUCCESS);
}

#endif
