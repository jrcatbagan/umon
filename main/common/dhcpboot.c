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
 * dhcpboot.c:
 *
 * This code implements a subset of the DHCP client protocol.
 * Based on RFC2131 spec, the "automatic allocation" mode, in which DHCP
 * assigns a permanent IP address to a client, is the only mode supported.
 *
 * The idea is that the monitor boots up, and if IPADD is set to DHCP, then
 * DHCP is used to populate shell variables with a server-supplied IP
 * address, NetMask and Gateway IP address. Then, when the application
 * is launched (probably via TFS), it can retrieve the content of those
 * shell variables for use by the application.
 *
 * Sequence of events for this limited implementation of DHCP...
 * Client issues a DHCP_DISCOVER, server responds with a DHCP_OFFER,
 * client issues a DHCP_REQUEST and server responds with a DHCP_ACK.
 * DISCOVER: request by the client to broadcast the fact that it is looking
 * for a DHCP server.
 * OFFER: reply from the server when it receives a DISCOVER request from 
 *	a client.  The offer may contain all the information that the DHCP
 *	client needs to bootup, but this is dependent on the configuration of
 *	the server.
 * REQUEST: request by the client for the server (now known because an OFFER
 *	was received) to send it the information it needs.
 * ACK: reply from the server with the information requested.
 *
 * NOTE: this file contains a generic DHCP client supporting "automatic
 * allocation mode" (infinite lease time).  There are several different
 * application-specific enhancements that can be added and hopefully
 * they have been isolated through the use of the dhcp_00.c file.
 * I've attempted to isolate as much of the non-generic code to
 * the file dhcp_XX.c (where dhcp_00.c is the default code).  If non-default
 * code is necessary, then limit the changes to a new dhcp_XX.c file.  This
 * will allow the code in this file to stay generic; hence, the user of this
 * code will be able to accept monitor upgrades without the need to touch
 * this file.  The makefile must link in some additional dhcp_XX.c file
 * (default is dhcp_00.c).  Bottom line... there should be no need to modify
 * this file for application-specific stuff; if there is, please let me know.
 *
 * NOTE1: the shell variable IPADD can also be set to DHCPV or DHCPv to 
 * enable different levels of verbosity during DHCP transactions... 'V'
 * is full DHCP verbosity and 'v' only prints the DhcpSetEnv() calls.
 *
 * NOTE2: this file supports DHCP and BOOTP.  Most of the function names
 * refer to DHCP even though their functionality is shared by both DHCP 
 * and BOOTP.  This is because I wrote this originally for DHCP, then added
 * the hooks for BOOTP... Bottom line: don't let the names confuse you!
 *
 *
 * Original author:     Ed Sutter (ed.sutter@alcatel-lucent.com)
 *
 */

#include "config.h"
#include "endian.h"
#include "cpuio.h"
#include "ether.h"
#include "tfs.h"
#include "tfsprivate.h"
#include "genlib.h"
#include "stddefs.h"
#include "cli.h"
#include "timer.h"

int	DHCPStartup(short), BOOTPStartup(short);
int DhcpSetEnv(char *,char *);
int SendDHCPDiscover(int,short);
void dhcpDumpVsa(void), printDhcpOptions(uchar *);

unsigned short	DHCPState;

#if INCLUDE_DHCPBOOT

static struct elapsed_tmr dhcpTmr;
static int		DHCPCommandIssued;
static ulong	DHCPTransactionId;

/* Variables used for DHCP Class ID specification: */
static char	*DHCPClassId;
static int	DHCPClassIdSize;

/* Variables used for DHCP Client ID specification: */
static char	DHCPClientId[32];
static int	DHCPClientIdSize, DHCPClientIdType;

/* Variables used for setting up a DHCP Parameter Request List: */
static uchar	DHCPRequestList[32];
static int		DHCPRqstListSize;

/* Variable to keep track of elapsed seconds since DHCP started: */
static short	DHCPElapsedSecs;

char *DhcpHelp[] = {
	"Issue a DHCP discover",
	"-[brvV] [vsa]",
#if INCLUDE_VERBOSEHELP
	"Options...",
	" -b      use bootp",
	" -r      retry",
	" -v|V    verbosity",
#endif
		0,
};

int
Dhcp(int argc,char *argv[])
{
	int	opt, bootp;

	bootp = 0;
	DHCPCommandIssued = 1;
	while ((opt=getopt(argc,argv,"brvV")) != -1) {
		switch(opt) {
		case 'b':
			bootp = 1;
			break;
		case 'r':
			DHCPCommandIssued = 0;
			break;
#if INCLUDE_ETHERVERBOSE
		case 'v':
			EtherVerbose = SHOW_DHCP;
			break;
		case 'V':
			EtherVerbose = DHCP_VERBOSE;
			break;
#endif
		default:
			return(CMD_PARAM_ERROR);
		}
	}

	if (argc == optind+1)  {
		if (!strcmp(argv[optind],"vsa")) {
			dhcpDumpVsa();
			return(CMD_SUCCESS);
		}
		else
			return(CMD_PARAM_ERROR);
	}
	else if (argc != optind)
		return(CMD_PARAM_ERROR);

	startElapsedTimer(&dhcpTmr,RetransmitDelay(DELAY_INIT_DHCP)*1000);

	if (bootp) {
		DHCPState = BOOTPSTATE_INITIALIZE;
		if (DHCPCommandIssued)
			BOOTPStartup(0);
	}
	else {
		DHCPState = DHCPSTATE_INITIALIZE;
		if (DHCPCommandIssued)
			DHCPStartup(0);
	}
	return(CMD_SUCCESS);
}

/* dhcpDumpVsa():
 *	Simply dump the content of the VSA shell variable in DHCP format.
 *	The variable content is stored in ascii and must be converted to binary
 *	prior to calling printDhcpOptions().
 */
void
dhcpDumpVsa(void)
{
	int		i;
	char	tmp[3],  *vsa_b, *vsa_a, len;

	vsa_a = getenv("DHCPVSA");
	if ((!vsa_a) || (!strcmp(vsa_a,"TRUE")))
		return;
	len = strlen(vsa_a);
	vsa_b = malloc(len);
	if (!vsa_b)
		return;

	len >>= 1;
	tmp[2] = 0;
	for(i=0;i<len;i++) {
		tmp[0] = *vsa_a++;	
		tmp[1] = *vsa_a++;	
		vsa_b[i] = (char)strtol(tmp,0,16);
	}
	/* First 4 bytes of DHCPVSA is the cookie, so skip over that. */
	printDhcpOptions((unsigned char *)vsa_b+4);
	free(vsa_b);
}

void
dhcpDisable()
{
	DHCPState = DHCPSTATE_NOTUSED;
}

/* DHCPStartup():
 *	This function is called at the point in which the ethernet interface is
 *	started if, and only if, the IPADD shell variable is set to DHCP.
 *	In older version of DHCP, the default was to use  "LUCENT.PPA.1.1" as
 *	the default vcid.  Now it is only used if specified in the shell variable
 *	DHCPCLASSID.  The same strategy applies to DHCPCLIENTID.
*/
int
DHCPStartup(short seconds)
{
	char	*id, *colon, *rlist;

#if !INCLUDE_TFTP
	printf("WARNING: DHCP can't load bootfile, TFTP not built into monitor.\n");
#endif

	/* The format of DHCPCLASSID is simply a string of characters. */
	id = getenv("DHCPCLASSID");
	if (id)
		DHCPClassId = id;
	else
		DHCPClassId = "";
	DHCPClassIdSize = strlen(DHCPClassId);

	/* The format of DHCPCLIENTID is "TYPE:ClientID" where 'TYPE is a
	 * decimal number ranging from 1-255 used as the "type" portion of
	 * the option, and ClientID is a string of ascii-coded hex pairs
	 * that are converted to binary and used as the client identifier.
	 */
	id = getenv("DHCPCLIENTID");
	if (id) {
		colon = strchr(id,':');
		if ((colon) && (!(strlen(colon+1) & 1))) {
			DHCPClientIdType = atoi(id);
			colon++;
			for(DHCPClientIdSize=0;*colon;DHCPClientIdSize++) {
				uchar tmp;

				tmp = colon[2];
				colon[2] = 0;
				DHCPClientId[DHCPClientIdSize] = (uchar)strtol(colon,0,16);
				colon[2] = tmp;
				colon+=2;
			}
		}
	}
	else
		DHCPClientIdSize = 0;

	/* The format of DHCPRQSTLIST is #:#:#:#:# where each '#' is a decimal
	 * number representing a parameter to be requested via the Parameter
	 * Request List option...
	 */
	rlist = getenv("DHCPRQSTLIST");
	if (rlist) {
		DHCPRqstListSize = 0;
		colon = rlist;
		while(*colon) {
			if (*colon++ == ':')
				DHCPRqstListSize++;
		}
		if (DHCPRqstListSize > sizeof(DHCPRequestList)) {
			printf("DHCPRQSTLIST too big.\n");
			DHCPRqstListSize = 0;
		}
		else {
			char *rqst;

			DHCPRqstListSize = 0;
			rqst = rlist;
			while(1) {
				DHCPRequestList[DHCPRqstListSize++] = strtol(rqst,&colon,0);
				if (*colon != ':')
					break;
				rqst = colon+1;
			}
			DHCPRequestList[DHCPRqstListSize] = 0;
		}
	}
	else
		DHCPRqstListSize = 0;

	return(SendDHCPDiscover(0,seconds));
}

int
BOOTPStartup(short seconds)
{
	return(SendDHCPDiscover(1,seconds));
}

uchar *
dhcpLoadShellVarOpts(uchar *options)
{
	if (DHCPClassIdSize) {
		*options++ = DHCPOPT_CLASSID;
		*options++ = DHCPClassIdSize;
		memcpy((char *)options, (char *)DHCPClassId, DHCPClassIdSize);
		options += DHCPClassIdSize;
	}
	if (DHCPClientIdSize) {
		*options++ = DHCPOPT_CLIENTID;
		*options++ = DHCPClientIdSize+1;
		*options++ = DHCPClientIdType;
		memcpy((char *)options, (char *)DHCPClientId, DHCPClientIdSize);
		options += DHCPClientIdSize;
	}
	if (DHCPRqstListSize) {
		*options++ = DHCPOPT_PARMRQSTLIST;
		*options++ = DHCPRqstListSize;
		memcpy((char *)options, (char *)DHCPRequestList, DHCPRqstListSize);
		options += DHCPRqstListSize;
	}
	return(options);
}

/* SendDHCPDiscover()
 *	The DHCPDISCOVER is issued as an ethernet broadcast.  IF the bootp
 *	flag is non-zero then just do a bootp request (a subset of the 
 *	DHCPDISCOVER stuff).
 */
int
SendDHCPDiscover(int bootp,short seconds)
{
	struct	dhcphdr *dhcpdata;
	struct	bootphdr *bootpdata;
	struct	ether_header *te;
	struct	ip *ti;
	struct	Udphdr *tu;
	ushort	uh_ulen;
	int		optlen;
	char	*dhcpflags;
	ulong	cookie;
	uchar	*dhcpOptions, *dhcpOptionsBase;

	/* Retrieve an ethernet buffer from the driver and populate the
	 * ethernet level of packet:
	 */
	te = (struct ether_header *) getXmitBuffer();
	memcpy((char *)&te->ether_shost, (char *)BinEnetAddr,6);
	memcpy((char *)&te->ether_dhost, (char *)BroadcastAddr,6);
	te->ether_type = ecs(ETHERTYPE_IP);

	/* Move to the IP portion of the packet and populate it appropriately: */
	ti = (struct ip *) (te + 1);
	ti->ip_vhl = IP_HDR_VER_LEN;
	ti->ip_tos = 0;
	ti->ip_id = 0;
	ti->ip_off = ecs(0x4000);	/* No fragmentation allowed */
	ti->ip_ttl = UDP_TTL;
	ti->ip_p = IP_UDP;
	memset((char *)&ti->ip_src.s_addr,0,4);
	memset((char *)&ti->ip_dst.s_addr,0xff,4);

	/* Now udp... */
	tu = (struct Udphdr *) (ti + 1);
	tu->uh_sport = ecs(DhcpClientPort);
	tu->uh_dport = ecs(DhcpServerPort);

	/* First the stuff that is the same for BOOTP or DHCP... */
	bootpdata = (struct bootphdr *)(tu+1);
	dhcpdata = (struct dhcphdr *)(tu+1);
	dhcpdata->op = DHCPBOOTP_REQUEST;
	dhcpdata->htype = 1;
	dhcpdata->hlen = 6;
	dhcpdata->hops = 0;
	dhcpdata->seconds = ecs(seconds);
	memset((char *)dhcpdata->bootfile,0,sizeof(dhcpdata->bootfile));
	memset((char *)dhcpdata->server_hostname,0,sizeof(dhcpdata->server_hostname));

	/* For the first DHCPDISCOVER issued, establish a transaction id based
	 * on a crc32 of the mac address.  For each DHCPDISCOVER after that,
	 * just increment.
	 */
	if (!DHCPTransactionId)
		DHCPTransactionId = crc32(BinEnetAddr,6);
	else
		DHCPTransactionId++;

	memcpy((char *)&dhcpdata->transaction_id,(char *)&DHCPTransactionId,4);
	memset((char *)&dhcpdata->client_ip,0,4);
	memset((char *)&dhcpdata->your_ip,0,4);
	memset((char *)&dhcpdata->server_ip,0,4);
	memset((char *)&dhcpdata->router_ip,0,4);
	memcpy((char *)dhcpdata->client_macaddr, (char *)BinEnetAddr,6);
	dhcpflags = getenv("DHCPFLAGS");
	if (dhcpflags)		/* 0x8000 is the only bit used currently. */
		dhcpdata->flags = (ushort)strtoul(dhcpflags,0,0);
	else
		dhcpdata->flags = 0;

	self_ecs(dhcpdata->flags);

	/* Finally, the DHCP or BOOTP specific stuff...
	 * Based on RFC1534 (Interoperation Between DHCP and BOOTP), any message
	 * received by a DHCP server that contains a 'DHCP_MESSAGETYPE' option
	 * is assumed to have been sent by a DHCP client.  A message without the
	 * DHCP_MESSAGETYPE option is assumed to have been sent by a BOOTP
	 * client.
	 */
	uh_ulen = optlen = 0;
	if (bootp) {
		memset((char *)bootpdata->vsa,0,sizeof(bootpdata->vsa));
		uh_ulen = sizeof(struct Udphdr) + sizeof(struct bootphdr);
		tu->uh_ulen = ecs(uh_ulen);
	}
	else {
		if (!buildDhcpHdr(dhcpdata)) {
			/* The cookie should only be loaded at the start of the
			 * vendor specific area if vendor-specific options are present.
			 */
			cookie = ecl(STANDARD_MAGIC_COOKIE);
			memcpy((char *)&dhcpdata->magic_cookie,(char *)&cookie,4);
			dhcpOptionsBase = (uchar *)(dhcpdata+1);
			dhcpOptions = dhcpOptionsBase;
			*dhcpOptions++ = DHCPOPT_MESSAGETYPE;
			*dhcpOptions++ = 1;
			*dhcpOptions++ = DHCPDISCOVER;
			dhcpOptions = dhcpLoadShellVarOpts(dhcpOptions);
			*dhcpOptions++ = 0xff;
	
			/* Calculate ip and udp lengths after all DHCP options are loaded
			 * so that the size is easily computed based on the value of the
			 * dhcpOptions pointer.  Apparently, the minimum size of the
			 * options space is 64 bytes, we determined this simply because
			 * the DHCP	server we are using complains if the size is smaller.
			 * Also, NULL out the space that is added to get a minimum option
			 * size of 64 bytes, 
			 */
			optlen = dhcpOptions - dhcpOptionsBase;
			if (optlen < 64) {
				memset((char *)dhcpOptions,0,64-optlen);
				optlen = 64;
			}
			uh_ulen = sizeof(struct Udphdr)+sizeof(struct dhcphdr)+optlen;
			tu->uh_ulen = ecs(uh_ulen);
		}
	}
	ti->ip_len = ecs((sizeof(struct ip) + uh_ulen));

	ipChksum(ti);	/* Compute checksum of ip hdr */
	udpChksum(ti);	/* Compute UDP checksum */

	if (bootp) {
		DHCPState = BOOTPSTATE_REQUEST;
		sendBuffer(BOOTPSIZE);
	}
	else {
		DHCPState = DHCPSTATE_SELECT;
		sendBuffer(DHCPSIZE+optlen);
	}
#if INCLUDE_ETHERVERBOSE
	if (EtherVerbose & SHOW_DHCP)
		printf("  %s startup (%d elapsed secs)\n",
			bootp ? "BOOTP" : "DHCP",seconds);
#endif
	return(0);
}

/* SendDHCPRequest()
 *	The DHCP request is broadcast back with the "server identifier" option
 *	set to indicate which server has been selected (in case more than one
 *	has offered).
 */
int
SendDHCPRequest(struct dhcphdr *dhdr)
{
	uchar	*op;
	struct	dhcphdr *dhcpdata;
	struct	ether_header *te;
	struct	ip *ti;
	struct	Udphdr *tu;
	int		optlen;
	uchar	*dhcpOptions, *dhcpOptionsBase;
	ushort	uh_ulen;
	ulong	cookie;

	te = (struct ether_header *) getXmitBuffer();
	memcpy((char *)&te->ether_shost, (char *)BinEnetAddr,6);
	memcpy((char *)&te->ether_dhost, (char *)BroadcastAddr,6);
	te->ether_type = ecs(ETHERTYPE_IP);

	ti = (struct ip *) (te + 1);
	ti->ip_vhl = IP_HDR_VER_LEN;
	ti->ip_tos = 0;
	ti->ip_id = 0;
	ti->ip_off = ecs(IP_DONTFRAG);	/* No fragmentation allowed */
	ti->ip_ttl = UDP_TTL;
	ti->ip_p = IP_UDP;
	memset((char *)&ti->ip_src.s_addr,0,4);
	memset((char *)&ti->ip_dst.s_addr,0xff,4);

	tu = (struct Udphdr *) (ti + 1);
	tu->uh_sport = ecs(DhcpClientPort);
	tu->uh_dport = ecs(DhcpServerPort);

	dhcpdata = (struct dhcphdr *)(tu+1);
	dhcpdata->op = DHCPBOOTP_REQUEST;
	dhcpdata->htype = 1;
	dhcpdata->hlen = 6;
	dhcpdata->hops = 0;
	dhcpdata->seconds = ecs(DHCPElapsedSecs);
	/* Use the same xid for the request as was used for the discover...
	 * (rfc2131 section 4.4.1)
	 */
	memcpy((char *)&dhcpdata->transaction_id,(char *)&dhdr->transaction_id,4);

	dhcpdata->flags = dhdr->flags;
	memset((char *)&dhcpdata->client_ip,0,4);
	memcpy((char *)&dhcpdata->your_ip,(char *)&dhdr->your_ip,4);
	memset((char *)&dhcpdata->server_ip,0,4);
	memset((char *)&dhcpdata->router_ip,0,4);
	cookie = ecl(STANDARD_MAGIC_COOKIE);
	memcpy((char *)&dhcpdata->magic_cookie,(char *)&cookie,4);
	memcpy((char *)dhcpdata->client_macaddr, (char *)BinEnetAddr,6);

	dhcpOptionsBase = (uchar *)(dhcpdata+1);
	dhcpOptions = dhcpOptionsBase;
	*dhcpOptions++ = DHCPOPT_MESSAGETYPE;
	*dhcpOptions++ = 1;
	*dhcpOptions++ = DHCPREQUEST;

	*dhcpOptions++ = DHCPOPT_SERVERID;		/* Server id ID */
	*dhcpOptions++ = 4;
	op = DhcpGetOption(DHCPOPT_SERVERID,(uchar *)(dhdr+1));
	if (op)
		memcpy((char *)dhcpOptions, (char *)op+2,4);
	else
		memset((char *)dhcpOptions, (char)0,4);
	dhcpOptions+=4;

	*dhcpOptions++ = DHCPOPT_REQUESTEDIP;		/* Requested IP */
	*dhcpOptions++ = 4;
	memcpy((char *)dhcpOptions,(char *)&dhdr->your_ip,4);
	dhcpOptions += 4;
	dhcpOptions = dhcpLoadShellVarOpts(dhcpOptions);
	*dhcpOptions++ = 0xff;

	/* See note in SendDHCPDiscover() regarding the computation of the
	 * ip and udp lengths.
	 */
	optlen = dhcpOptions - dhcpOptionsBase;
	if (optlen < 64)
		optlen = 64;
	uh_ulen = sizeof(struct Udphdr) + sizeof(struct dhcphdr) + optlen;
	tu->uh_ulen = ecs(uh_ulen);
	ti->ip_len = ecs((sizeof(struct ip) + uh_ulen));

	ipChksum(ti);		/* Compute checksum of ip hdr */
	udpChksum(ti);		/* Compute UDP checksum */

	DHCPState = DHCPSTATE_REQUEST;
	sendBuffer(DHCPSIZE+optlen);
	return(0);
}

/* randomDhcpStartupDelay():
 *	Randomize the startup for DHCP/BOOTP (see RFC2131 Sec 4.4.1)...
 *	Return a value between 1 and 10 based on the last 4 bits of the 
 *	board's MAC address.
 *	The presence of the DHCPSTARTUPDELAY shell variable overrides
 *	this random value.
 */
int
randomDhcpStartupDelay(void)
{
	char *env;
	int	randomsec;

	env = getenv("DHCPSTARTUPDELAY");
	if (env) {
		randomsec = (int)strtol(env,0,0);
	}
	else {
		randomsec = (BinEnetAddr[5] & 0xf);
		if (randomsec > 10)
			randomsec -= 7;
		else if (randomsec == 0)
			randomsec = 10;
	}
	return(randomsec);
}

/* dhcpStateCheck():
 *	Called by pollethernet() to monitor the progress of DHCPState.
 *	The retry rate is "almost" what is specified in the RFC...
 *	Refer to the RetransmitDelay() function for details.
 *
 *	Regarding timing...
 *	The DHCP startup may be running without an accurate measure of elapsed
 *	time.  The value of LoopsPerMillisecond is used as an approximation of
 *	the number of times this function must be called for one second to 
 *	pass (dependent on network traffic, etc...).  RetransmitDelay() is
 *	called to retrieve the number of seconds that must elapse prior to 
 *	retransmitting the last DHCP message.  The static variables in this
 *	function are used to keep track of that timeout.
 */
void
dhcpStateCheck(void)
{
	int delaysecs;

	/* If the DHCP command has been issued, it is assumed that the script
	 * is handling retries...
	 */
	if (DHCPCommandIssued)
		return;

	/* Return, restart or fall through; depending on DHCPState... */
	switch(DHCPState) {
		case DHCPSTATE_NOTUSED:
		case BOOTPSTATE_COMPLETE:
		case DHCPSTATE_BOUND:
			return;
		case DHCPSTATE_RESTART:
			DHCPStartup(0);
			return;
		case BOOTPSTATE_RESTART:
			BOOTPStartup(0);
			return;
		case DHCPSTATE_INITIALIZE:
		case BOOTPSTATE_INITIALIZE:
			delaysecs = randomDhcpStartupDelay();
			startElapsedTimer(&dhcpTmr,delaysecs * 1000);
#if INCLUDE_ETHERVERBOSE
			if (EtherVerbose & SHOW_DHCP)
				printf("\nDHCP/BOOTP %d sec startup delay...\n",delaysecs);
#endif
			if (DHCPState & BOOTP_MODE)
				DHCPState = BOOTPSTATE_INITDELAY;
			else
				DHCPState = DHCPSTATE_INITDELAY;
			return;
		case DHCPSTATE_INITDELAY:
		case BOOTPSTATE_INITDELAY:
			if (msecElapsed(&dhcpTmr) || (gotachar())) {
				DHCPElapsedSecs = 0;
				startElapsedTimer(&dhcpTmr,
					RetransmitDelay(DELAY_INIT_DHCP)*1000);
				if (DHCPState & BOOTP_MODE)
					BOOTPStartup(0);
				else
					DHCPStartup(0);
			}
			return;
		default:
			break;
	}

	if (msecElapsed(&dhcpTmr)) {
		int lastdelay;
		
		lastdelay = RetransmitDelay(DELAY_RETURN);
		delaysecs = RetransmitDelay(DELAY_INCREMENT);
		
		if (delaysecs != RETRANSMISSION_TIMEOUT) {
			DHCPElapsedSecs += delaysecs;
			startElapsedTimer(&dhcpTmr,delaysecs*1000);

			if (DHCPState & BOOTP_MODE)
				BOOTPStartup(DHCPElapsedSecs);
			else
				DHCPStartup(DHCPElapsedSecs);
#if INCLUDE_ETHERVERBOSE
			if (EtherVerbose & SHOW_DHCP)
				printf("  DHCP/BOOTP retry (%d secs)\n",lastdelay);
#endif
		}
		else {
#if INCLUDE_ETHERVERBOSE
			if (EtherVerbose & SHOW_DHCP)
				printf("  DHCP/BOOTP giving up\n");
#endif
		}
	}
}

/* xidCheck():
 *	Common function used for DHCP and BOOTP to verify incoming transaction
 *	id...
 */
int
xidCheck(char *id,int bootp)
{
	if (memcmp(id,(char *)&DHCPTransactionId,4)) {
#if INCLUDE_ETHERVERBOSE
		if (EtherVerbose & SHOW_DHCP) {
			printf("%s ignored: unexpected transaction id.\n",
				bootp ? "BOOTP":"DHCP");
		}
#endif
		return(-1);
	}
	return(0);
}

int
loadBootFile(int bootp)
{
#if INCLUDE_TFTP
	ulong	addr;
	char	bfile[TFSNAMESIZE+TFSINFOSIZE+32];
	char	*tfsfile, *bootfile, *tftpsrvr, *flags, *info;

	/* If the DHCPDONTBOOT variable is present, then don't put
	 * the file in TFS and don't run it.  Just complete the TFTP
	 * transfer, and allow the user to deal with the downloaded
	 * data using APPRAMBASE and TFTPGET shell variables (probably
	 * through some boot script).
	 */
	if (getenv("DHCPDONTBOOT"))
		tfsfile = (char *)0;
	else
		tfsfile = bfile;

	/* If both bootfile and server-ip are specified, then boot it.
	 * The name of the file must contain information that tells the monitor
	 * what type of file it is, so the first 'comma' extension is used as
	 * the flag field (if it is a valid flag set) and the second 'comma'
	 * extension is used as the info field.
	 */
	bootfile = getenv("BOOTFILE");
	tftpsrvr = getenv("BOOTSRVR");

	if (bootfile && tftpsrvr) {
		int	tftpworked;

		addr = getAppRamStart();
		info = "";
		flags = "e";
		strncpy(bfile,bootfile,sizeof(bfile));

		if (tfsfile) {
			char *icomma, *fcomma;

			fcomma = strchr(bfile,',');
			if (fcomma) {
				icomma = strchr(fcomma+1,',');
				if (icomma) {
					*icomma = 0;
					info = icomma+1;
				}
				*fcomma = 0;
				if (tfsctrl(TFS_FATOB,(long)(fcomma+1),0) != 0)
					flags = fcomma+1;
			}
		}

		/* Since we are about to transition to TFTP, match TFTP's
		 * verbosity to the verbosity currently set for DHCP...
		 */
#if INCLUDE_ETHERVERBOSE
		if (EtherVerbose & SHOW_DHCP)
			EtherVerbose |= SHOW_TFTP_STATE;
#endif

		/* If the TFTP transfer succeeds, attempt to run the boot file;
		 * if the TFTP transfer fails, then re-initialize the tftp state
		 * and set the DHCP state such that dhcpStateCheck() will
		 * cause the handshake to start over again...
		 */
		tftpworked = tftpGet(addr,tftpsrvr,"octet",bfile,tfsfile,flags,info);
		if (tftpworked) {
#if INCLUDE_ETHERVERBOSE
			EtherVerbose = 0;
#endif
			if (tfsfile) {
				int	err;
				char *argv[2];

				argv[0] = bfile;
				argv[1] = 0;
				err = tfsrun(argv,0);
				if (err != TFS_OKAY) 
					printf("DHCP-invoked tfsrun(%s) failed: %s\n",
						bfile,tfserrmsg(err));
			}
		}
		else {
			tftpInit();
			RetransmitDelay(DELAY_INIT_TFTP);
#if INCLUDE_ETHERVERBOSE
			EtherVerbose &= ~SHOW_TFTP_STATE;
#endif
			if (bootp)
				DHCPState = BOOTPSTATE_RESTART;
			else
				DHCPState = DHCPSTATE_RESTART;
		}
	}
#if INCLUDE_ETHERVERBOSE
	else
		EtherVerbose &= ~(SHOW_DHCP|DHCP_VERBOSE);
#endif
#endif
	return(0);
}

/* processBOOTP():
 *	A subset of processDHCP().
 *	We get here from processDHCP, because it detects that the current
 *	value of DHCPState is BOOTPSTATE_REQUEST.
 */
int
processBOOTP(struct ether_header *ehdr,ushort size)
{
	struct	ip *ihdr;
	struct	Udphdr *uhdr;
	struct	bootphdr *bhdr;
	ulong	ip, temp_ip, cookie;
	uchar	buf[16], *op;

#if INCLUDE_ETHERVERBOSE
	if (EtherVerbose & SHOW_HEX)
		printMem((uchar *)ehdr,size,EtherVerbose & SHOW_ASCII);
#endif

	ihdr = (struct ip *)(ehdr + 1);
	uhdr = (struct Udphdr *)((char *)ihdr + IP_HLEN(ihdr));
	bhdr = (struct bootphdr *)(uhdr+1);

	/* Verify incoming transaction id matches the previous outgoing value: */
	if (xidCheck((char *)&bhdr->transaction_id,1) < 0)
		return(-1);

	/* If bootfile is nonzero, store it into BOOTFILE shell var: */
	if (bhdr->bootfile[0])
		DhcpSetEnv("BOOTFILE", (char *)bhdr->bootfile);

	/* Assign IP "server_ip" to the BOOTSRVR shell var (if non-zero): */
	memcpy((char *)&temp_ip,(char *)&bhdr->server_ip,4);
	if (temp_ip)
		DhcpSetEnv("BOOTSRVR",(char *)IpToString(temp_ip,(char *)buf));

	/* Assign IP "router_ip" to the RLYAGNT shell var (if non-zero): */
	memcpy((char *)&temp_ip,(char *)&bhdr->router_ip,4);
	if (temp_ip)
		DhcpSetEnv("RLYAGNT",(char *)IpToString(temp_ip,(char *)buf));

	/* Assign IP address loaded in "your_ip" to the IPADD shell var: */
	memcpy((char *)BinIpAddr,(char *)&bhdr->your_ip,4);
	memcpy((char *)&temp_ip,(char *)&bhdr->your_ip,4);
	DhcpSetEnv("IPADD",(char *)IpToString(temp_ip,(char *)buf));

	/* If STANDARD_MAGIC_COOKIE exists, then process options... */
	memcpy((char *)&cookie,(char *)bhdr->vsa,4);
	if (cookie == ecl(STANDARD_MAGIC_COOKIE)) {
		/* Assign subnet mask option to NETMASK shell var (if found): */
		op = DhcpGetOption(DHCPOPT_SUBNETMASK,&bhdr->vsa[4]);
		if (op) {
			memcpy((char *)&ip,(char *)op+2,4);
			DhcpSetEnv("NETMASK",(char *)IpToString(ip,(char *)buf));
		}
		/* Assign first router option to GIPADD shell var (if found): */
		/* (the router option can have multiple entries, and they are */
		/* supposed to be in order of preference, so use the first one) */
		op = DhcpGetOption(DHCPOPT_ROUTER,&bhdr->vsa[4]);
		if (op) {
			memcpy((char *)&ip, (char *)op+2,4);
			DhcpSetEnv("GIPADD",(char *)IpToString(ip,(char *)buf));
		}
	}

	DhcpBootpDone(1,(struct dhcphdr *)bhdr,
		size - ((int)((int)&bhdr->vsa - (int)ehdr)));

	DHCPState = BOOTPSTATE_COMPLETE;

	/* Call loadBootFile() which will then kick off a tftp client
	 * transfer if both BOOTFILE and BOOTSRVR shell variables are
	 * loaded; otherwise, we are done.
	 */
	loadBootFile(1);

	return(0);
}

int
processDHCP(struct ether_header *ehdr,ushort size)
{
	struct	ip *ihdr;
	struct	Udphdr *uhdr;
	struct	dhcphdr *dhdr;
	uchar	buf[16], *op, msgtype;
	ulong	ip, temp_ip, leasetime;

	if (DHCPState == BOOTPSTATE_REQUEST)
		return(processBOOTP(ehdr,size));

#if INCLUDE_ETHERVERBOSE
	if (EtherVerbose & SHOW_HEX)
		printMem((uchar *)ehdr,size,EtherVerbose & SHOW_ASCII);
#endif

	ihdr = (struct ip *)(ehdr + 1);
	uhdr = (struct Udphdr *)((char *)ihdr + IP_HLEN(ihdr));
	dhdr = (struct dhcphdr *)(uhdr+1);

	/* Verify incoming transaction id matches the previous outgoing value: */
	if (xidCheck((char *)&dhdr->transaction_id,0) < 0)
		return(-1);

	op = DhcpGetOption(DHCPOPT_MESSAGETYPE,(uchar *)(dhdr+1));
	if (op)
		msgtype = *(op+2);
	else
		msgtype = DHCPUNKNOWN;

	if ((DHCPState == DHCPSTATE_SELECT) && (msgtype == DHCPOFFER)) {
		/* Target issued the DISCOVER, the incoming packet is the server's
		 * OFFER reply.  The function "ValidDHCPOffer() will return
		 * non-zero if the request is to be sent.
		 */
		if (ValidDHCPOffer(dhdr))
			SendDHCPRequest(dhdr);
#if INCLUDE_ETHERVERBOSE
		else if (EtherVerbose & SHOW_DHCP) {
			char ip[4];
			memcpy(ip,(char *)&ihdr->ip_src,4);
			printf("  DHCP offer from %d.%d.%d.%d ignored\n",
				ip[0],ip[1],ip[2],ip[3]);
		}
#endif
	}
	else if ((DHCPState == DHCPSTATE_REQUEST) && (msgtype == DHCPACK)) {
		ulong	cookie;
		uchar	ipsrc[4];

		/* Target issued the REQUEST, the incoming packet is the server's
		 * ACK reply.  We're done so load the environment now.
		 */
		memcpy((char *)ipsrc,(char *)&ihdr->ip_src,4);
		shell_sprintf("DHCPOFFERFROM","%d.%d.%d.%d",
			ipsrc[0],ipsrc[1],ipsrc[2],ipsrc[3]);

		/* If bootfile is nonzero, store it into BOOTFILE shell var: */
		if (dhdr->bootfile[0])
			DhcpSetEnv("BOOTFILE",(char *)dhdr->bootfile);

		/* Assign IP "server_ip" to the BOOTSRVR shell var (if non-zero): */
		memcpy((char *)&temp_ip,(char *)&dhdr->server_ip,4);
		if (temp_ip)
			DhcpSetEnv("BOOTSRVR",(char *)IpToString(temp_ip,(char *)buf));

		/* Assign IP "router_ip" to the RLYAGNT shell var (if non-zero): */
		memcpy((char *)&temp_ip,(char *)&dhdr->router_ip,4);
		if (temp_ip)
			DhcpSetEnv("RLYAGNT",(char *)IpToString(temp_ip,(char *)buf));

		/* Assign IP address loaded in "your_ip" to the IPADD shell var: */
		memcpy((char *)BinIpAddr,(char *)&dhdr->your_ip,4);
		memcpy((char *)&temp_ip,(char *)&dhdr->your_ip,4);
		DhcpSetEnv("IPADD",IpToString(temp_ip,(char *)buf));

		op = DhcpGetOption(DHCPOPT_ROOTPATH,(uchar *)(dhdr+1));
		if (op) 
			DhcpSetEnv("ROOTPATH",(char *)op+2);

		/* If STANDARD_MAGIC_COOKIE exists, process options... */
		memcpy((char *)&cookie,(char *)&dhdr->magic_cookie,4);
		if (cookie == ecl(STANDARD_MAGIC_COOKIE)) {
			/* Assign subnet mask to NETMASK shell var (if found): */
			op = DhcpGetOption(DHCPOPT_SUBNETMASK,(uchar *)(dhdr+1));
			if (op) {
				memcpy((char *)&ip,(char *)op+2,4);
				DhcpSetEnv("NETMASK",(char *)IpToString(ip,(char *)buf));
			}

			/* Assign Hostname to HOSTNAME shell var (if found): */
			op = DhcpGetOption(DHCPOPT_HOSTNAME,(uchar *)(dhdr+1));
			if (op) {
				char tmpnam[64];
				int optlen = *(op+1);

				if (optlen < sizeof(tmpnam)-1) {
					memset(tmpnam,0,sizeof(tmpnam));
					memcpy(tmpnam,(char *)(op+2),optlen);
					DhcpSetEnv("HOSTNAME",tmpnam);
				}
			}

			/* Assign gateway IP to GIPADD shell var (if found):
			 * (the router option can have multiple entries, and they are
			 * supposed to be in order of preference, so use the first one).
	 		 */
			op = DhcpGetOption(DHCPOPT_ROUTER,(uchar *)(dhdr+1));
			if (op) {
				memcpy((char *)&ip,(char *)op+2,4);
				DhcpSetEnv("GIPADD",(char *)IpToString(ip,(char *)buf));
			}
			/* Process DHCPOPT_LEASETIME option as follows...
			 * If not set, assume infinite and clear DHCPLEASETIME shellvar.
			 * If set, then look for the presence of the DHCPLEASETIME shell
			 * variable and use it as a minimum.  If the incoming value is
			 * >= what is in the shell variable, accept it and load the shell
			 * variable with this value. If incoming lease time is less than
			 * what is stored in DHCPLEASETIME, ignore the request.
			 * If DHCPLEASETIME is not set, then just load the incoming lease
			 * into the DHCPLEASETIME shell variable and accept the offer.
			 */
			op = DhcpGetOption(DHCPOPT_LEASETIME,(uchar *)(dhdr+1));
			if (op) {
				memcpy((char *)&leasetime,(char *)op+2,4);
				leasetime = ecl(leasetime);
				if (getenv("DHCPLEASETIME")) {
					ulong	minleasetime;
					minleasetime = strtol(getenv("DHCPLEASETIME"),0,0);
					if (leasetime < minleasetime) {
						printf("DHCP: incoming lease time 0x%lx too small.\n",
							leasetime);
						return(-1);
					}
				}
				sprintf((char *)buf,"0x%lx",leasetime);
				setenv("DHCPLEASETIME",(char *)buf);
			}
			else
				setenv("DHCPLEASETIME",(char *)0);
		}

		/* Check for vendor specific stuff... */
		DhcpVendorSpecific(dhdr);

		DhcpBootpDone(0,dhdr,
			size - ((int)((int)&dhdr->magic_cookie - (int)ehdr)));

		DHCPState = DHCPSTATE_BOUND;

		/* Call loadBootFile() which will then kick off a tftp client
		 * transfer if both BOOTFILE and BOOTSRVR shell variables are
		 * loaded; otherwise, we are done.
		 */
		loadBootFile(0);
	}
	return(0);
}

char *
DHCPop(int op)
{
	switch(op) {
	case DHCPBOOTP_REQUEST:
		return("REQUEST");
	case DHCPBOOTP_REPLY:
		return("REPLY");
	default:
		return("???");
	}
}

char *
DHCPmsgtype(int msg)
{
	char *type;

	switch(msg) {
	case DHCPDISCOVER:
		type = "DISCOVER";
		break;
	case DHCPOFFER:
		type = "OFFER";
		break;
	case DHCPREQUEST:
		type = "REQUEST";
		break;
	case DHCPDECLINE:
		type = "DECLINE";
		break;
	case DHCPACK:
		type = "ACK";
		break;
	case DHCPNACK:
		type = "NACK";
		break;
	case DHCPRELEASE:
		type = "RELEASE";
		break;
	case DHCPINFORM:
		type = "INFORM";
		break;
	case DHCPFORCERENEW:
		type = "FORCERENEW";
		break;
	case DHCPLEASEQUERY:
		type = "LEASEQUERY";
		break;
	case DHCPLEASEUNASSIGNED:
		type = "LEASEUNASSIGNED";
		break;
	case DHCPLEASEUNKNOWN:
		type = "LEASEUNKNOWN";
		break;
	case DHCPLEASEACTIVE:
		type = "LEASEACTIVE";
		break;
	default:
		type = "???";
		break;
	}
	return(type);
}

/* printDhcpOptions():
 *	Verbosely display the DHCP options pointed to by the incoming
 *	options pointer.
 */
void
printDhcpOptions(uchar *options)
{
	int	i, safety, opt, optlen;

	safety = 0;
	while(*options != 0xff) {
		if (safety++ > 10000) {
			printf("Aborting, overflow likely\n");
			break;
		}
		opt = (int)*options++;
		if (opt == 0)	/* padding */
			continue;
		printf("    option %3d: ",opt);
		optlen = (int)*options++;
		if (opt==DHCPOPT_MESSAGETYPE) {
			printf("DHCP_%s",DHCPmsgtype(*options++));
		}
		else if ((opt < DHCPOPT_HOSTNAME) ||
		    (opt == DHCPOPT_BROADCASTADDRESS) ||
		    (opt == DHCPOPT_REQUESTEDIP) ||
		    (opt == DHCPOPT_SERVERID) ||
		    (opt == DHCPOPT_NISSERVER)) {
			for(i=0;i<optlen;i++)
				printf("%d ",*options++);
		}
		else if ((opt == DHCPOPT_NISDOMAINNAME) || (opt == DHCPOPT_CLASSID)) {
			for(i=0;i<optlen;i++)
				printf("%c",*options++);
		}
		else if (opt == DHCPOPT_CLIENTID) {
			printf("%d 0x",(int)*options++);
			for(i=1;i<optlen;i++)
				printf("%02x",*options++);
		}
		else {
			printf("0x");
			for(i=0;i<optlen;i++)
				printf("%02x",*options++);
		}
		printf("\n");
	}
}

/* printDhcp():
 *	Try to format the DHCP stuff...
 */
void
printDhcp(struct Udphdr *p)
{
	struct	dhcphdr *d;
	uchar 	*client_ip, *your_ip, *server_ip, *router_ip;
	ulong	cookie, xid;

	d = (struct dhcphdr *)(p+1);

	client_ip = (uchar *)&(d->client_ip);
	your_ip = (uchar *)&(d->your_ip);
	server_ip = (uchar *)&(d->server_ip);
	router_ip = (uchar *)&(d->router_ip);
	memcpy((char *)&xid,(char *)&d->transaction_id,4);
	/* xid = ecl(xid) */

	printf("  DHCP: sport dport ulen  sum\n");
	printf("        %4d  %4d %4d %4d\n",
		ecs(p->uh_sport), ecs(p->uh_dport), ecs(p->uh_ulen),ecs(p->uh_sum));
	printf("    op = %s, htype = %d, hlen = %d, hops = %d\n",
	    DHCPop(d->op),d->htype,d->hlen,d->hops);
	printf("    seconds = %d, flags = 0x%x, xid= 0x%lx\n",
		ecs(d->seconds),ecs(d->flags),xid);
	printf("    client_macaddr = %02x:%02x:%02x:%02x:%02x:%02x\n",
	    d->client_macaddr[0], d->client_macaddr[1],
	    d->client_macaddr[2], d->client_macaddr[3],
	    d->client_macaddr[4], d->client_macaddr[5]);
	printf("    client_ip = %d.%d.%d.%d\n",
	    client_ip[0],client_ip[1],client_ip[2],client_ip[3]);
	printf("    your_ip =   %d.%d.%d.%d\n",
	    your_ip[0],your_ip[1],your_ip[2],your_ip[3]);
	printf("    server_ip = %d.%d.%d.%d\n",
	    server_ip[0],server_ip[1],server_ip[2],server_ip[3]);
	printf("    router_ip = %d.%d.%d.%d\n",
	    router_ip[0],router_ip[1],router_ip[2],router_ip[3]);
	if (d->bootfile[0])
		printf("    bootfile: %s\n", d->bootfile);
	if (d->server_hostname[0])
		printf("    server_hostname: %s\n", d->server_hostname);

	/* If STANDARD_MAGIC_COOKIE doesn't exist, then don't process options... */
	memcpy((char *)&cookie,(char *)&d->magic_cookie,4);
	if (cookie != ecl(STANDARD_MAGIC_COOKIE))
		return;

	printDhcpOptions((uchar *)(d+1));
}

/* DhcpGetOption():
 *	Based on the incoming option pointer and a specified option value,
 *	search through the options list for the value and return a pointer
 *	to that option.
 */
uchar *
DhcpGetOption(unsigned char optval,unsigned char *options)
{
	int		safety;

	safety = 0;
	while(*options != 0xff) {
		if (safety++ > 1000)
			break;
		if (*options == 0) {	/* Skip over padding. */
			options++;
			continue;
		}
		if (*options == optval)
			return(options);
		options += ((*(options+1)) + 2);
	}
	return((uchar *)0);
}

int
DhcpSetEnv(char *name,char *value)
{
#if INCLUDE_ETHERVERBOSE
	if (EtherVerbose & SHOW_DHCP)
		printf("  Dhcp/Bootp SetEnv: %s = %s\n",name,value);
#endif
	return(setenv(name,value));
}

int
DhcpIPCheck(char *ipadd)
{
	char	verbose;

	if (!memcmp(ipadd,"DHCP",4)) {
		verbose = ipadd[4];
		DHCPState = DHCPSTATE_INITIALIZE;
	}
	else if (!memcmp(ipadd,"BOOTP",5)) {
		verbose = ipadd[5];
		DHCPState = BOOTPSTATE_INITIALIZE;
	}
	else {
		if (IpToBin(ipadd,BinIpAddr) < 0) {
			verbose = 0;
			DHCPState = BOOTPSTATE_INITIALIZE;
		}
		else {
			DHCPState = DHCPSTATE_NOTUSED;
			return(0);
		}
	}

	BinIpAddr[0] = 0; 
	BinIpAddr[1] = 0;
	BinIpAddr[2] = 0; 
	BinIpAddr[3] = 0;
#if INCLUDE_ETHERVERBOSE
	if (verbose == 'V')
		EtherVerbose = DHCP_VERBOSE;
	else if (verbose == 'v')
		EtherVerbose = SHOW_DHCP;
#endif
	return(0);
}

char *
dhcpStringState(int state)
{
	switch(state) {
		case DHCPSTATE_INITIALIZE:
			return("DHCP_INITIALIZE");
		case DHCPSTATE_SELECT:
			return("DHCP_SELECT");
		case DHCPSTATE_REQUEST:
			return("DHCP_REQUEST");
		case DHCPSTATE_BOUND:
			return("DHCP_BOUND");
		case DHCPSTATE_RENEW:
			return("DHCP_RENEW");
		case DHCPSTATE_REBIND:
			return("DHCP_REBIND");
		case DHCPSTATE_NOTUSED:
			return("DHCP_NOTUSED");
		case DHCPSTATE_RESTART:
			return("DHCP_RESTART");
		case BOOTPSTATE_INITIALIZE:
			return("BOOTP_INITIALIZE");
		case BOOTPSTATE_REQUEST:
			return("BOOTP_REQUEST");
		case BOOTPSTATE_RESTART:
			return("BOOTP_RESTART");
		case BOOTPSTATE_COMPLETE:
			return("BOOTP_COMPLETE");
		default:
			return("???");
	}
}

void
ShowDhcpStats()
{
	printf("Current DHCP State: %s\n",dhcpStringState(DHCPState));
}

#endif
