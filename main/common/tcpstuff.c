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
 * tcpstuff.c
 *
 * Absolute minimum to properly refuse TCP connection requests to this
 * device.  The processTCP function is called if the incoming protocol is
 * TCP, but this code simply responds with a TCP RESET which is essentially
 * a refusal to establish the connection request being made from a remote
 * machine.
 * This is similar in functionality to the ICMP "destination unreachable
 * message".  Note that this is not an absolute necessity but it does
 * provide a "clean" response to the remote machine that is making the
 * connection request.  Without it, the remote machine would keep trying
 * to connect.
 * For more details, refer to TCP/IP Comer 3rd Edition pg 216-219.
 *
 * Original author:     Ed Sutter (ed.sutter@alcatel-lucent.com)
 *
 */
#include "config.h"
#include "endian.h"
#if INCLUDE_ETHERNET
#include "genlib.h"
#include "ether.h"
#include "stddefs.h"

/* SendTcpConnectionReset():
 *	Called in response to any incoming TCP request.
 *	It is used to let the sender know that the TCP connection is being
 *	refused.
 */
int
SendTcpConnectionReset(struct ether_header *re)
{
	short	datalen, flags;
	ulong	ackno;
	struct ether_header *te;
	struct ip *ti, *ri;
	struct tcphdr *ttcp, *rtcp;

	ri = (struct ip *) (re + 1);
	datalen = ecs(ri->ip_len) - ((ri->ip_vhl & 0x0f) * 4);

	te = EtherCopy(re);
	ti = (struct ip *) (te + 1);
	ti->ip_vhl = ri->ip_vhl;
	ti->ip_tos = ri->ip_tos;
	ti->ip_len = ri->ip_len;
	ti->ip_id = ri->ip_id;
	ti->ip_off = ecs(IP_DONTFRAG);
	ti->ip_ttl = ri->ip_ttl/2;
	ti->ip_p = ri->ip_p;
	memcpy((char *)&(ti->ip_src.s_addr),(char *)&(ri->ip_dst.s_addr),
		sizeof(struct in_addr));
	memcpy((char *)&(ti->ip_dst.s_addr),(char *)&(ri->ip_src.s_addr),
		sizeof(struct in_addr));

	ttcp = (struct tcphdr *) (ti + 1);
	rtcp = (struct tcphdr *) (ri + 1);
	flags = ecs(rtcp->flags);
	flags &= ~TCP_FLAGMASK;
	flags |= (TCP_ACK | TCP_RESET);
	ttcp->flags = ecs(flags);
	ttcp->sport = rtcp->dport;
	ttcp->dport = rtcp->sport;
	memset((char *)&ttcp->seqno,0,4);
	memcpy((char *)&ackno,(char *)&rtcp->seqno,4);
	self_ecl(ackno);
	ackno+=1;
	self_ecl(ackno);
	memcpy((char *)&ttcp->ackno,(char *)&ackno,4);

	ttcp->windowsize = 0;
	ttcp->urgentptr = 0;
	memcpy((char *)(ttcp+1),(char *)(rtcp+1),datalen-sizeof(struct tcphdr));

	ipChksum(ti);	/* compute checksum of ip hdr: (3rd Edition Comer pg 100) */
	tcpChksum(ti);	/* Compute TCP checksum */

	sendBuffer(sizeof(struct ether_header) + ecs(ti->ip_len));
	return(0);
}

/*	tcpChksum():
 *	Compute the checksum of the TCP packet.
 *	The incoming pointer is to an ip header, the tcp header after that ip
 *	header is directly populated with the result.  Similar to the udp
 *	checksum calculation.  This one is mandatory.
 */
void
tcpChksum(struct ip *ihdr)
{
	register int	i;
	register ushort	*sp;
	register ulong	t;
	short 	len;
	struct	tcphdr *thdr;
	struct	UdpPseudohdr	pseudohdr;

	thdr = (struct tcphdr *)(ihdr+1);
	thdr->tcpcsum = 0;

	/* Start by building the pseudo header: */
	memcpy((char *)&pseudohdr.ip_src.s_addr,(char *)&ihdr->ip_src.s_addr,4);
	memcpy((char *)&pseudohdr.ip_dst.s_addr,(char *)&ihdr->ip_dst.s_addr,4);
	pseudohdr.zero = 0;
	pseudohdr.proto = ihdr->ip_p;
	len = ecs(ihdr->ip_len) - sizeof(struct ip);
	pseudohdr.ulen = ecs(len);

	/* Get checksum of pseudo header: */
	sp = (ushort *) &pseudohdr;
	for (i=0,t=0;i<(sizeof(struct UdpPseudohdr)/sizeof(ushort));i++,sp++)
		t += *sp;

	/* If length is odd, pad and add one. */
	if (len & 1) {
		uchar	*ucp;
		ucp = (uchar *)thdr;
		ucp[len] = 0;
		len++;
	}
	len >>= 1;

	sp = (ushort *) thdr;
	for (i=0;i<len;i++,sp++)
		t += *sp;
	t = (t & 0xffff) + (t >> 16);
	thdr->tcpcsum = ~t;
}

void
tcperrmsg(char *msg)
{
	printf("TCP error: %s\n",msg);
}

void
processTCP(struct ether_header *ehdr,ushort size)
{
	//struct	ip *ipp;
	//struct	tcphdr *tcpp;

	//ipp = (struct ip *)(ehdr + 1);
	//tcpp = (struct tcphdr *)((char *)ipp + IP_HLEN(ipp));
	SendTcpConnectionReset(ehdr);
	return;
}

#endif
