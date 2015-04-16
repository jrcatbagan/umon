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
 * dhcp_03.c:
 *
 * This is the PPA-specific code used for PATHSTAR.  PATHSTAR uses BOOTP
 * and has their own proprietary string in the vendor-specific-area.
 * This code loads that string into the BOOTPVSA shell variable.
 *
 * Original author:     Ed Sutter (ed.sutter@alcatel-lucent.com)
 *
 */

#include "config.h"
#include "cpuio.h"
#include "ether.h"
#include "tfs.h"
#include "tfsprivate.h"
#include "genlib.h"
#include "stddefs.h"

/* ValidDHCPOffer():
   Target issued the DISCOVER, the incoming packet is the server's
   OFFER reply.
*/
ValidDHCPOffer(struct	dhcphdr *dhdr)
{
	return(1);
}

/* buildDhcpHdr():
	Called by dhcpboot.c to allow application-specific header stuff to
	be added to header.  Return 0 if generic stuff in dhcpboot.c is to be
	used; else return 1 and the calling code will assume this function is
	dealing with it (see dhcpboot.c for basic idea).
*/
int
buildDhcpHdr(struct dhcphdr *dhcpdata)
{
	return(0);
}

/* DhcpBootpDone():
	Called at the end of the Bootp or Dhcp transaction.
	Input...
	bootp:	1 if BOOTP; else DHCP.
	dhdr:	pointer to dhcp or bootp header.
	vsize:	size of vendor specific area (for bootp this is fixed at 64, 
			but for dhcp it is variable).
*/
void
DhcpBootpDone(int bootp, struct dhcphdr *dhdr, int vsize)
{
	if (bootp) {
		ulong	cookie;
		struct bootphdr *bhdr;
		char	bootpvsa[65], *vp;

		bhdr = (struct bootphdr *)dhdr;
		memcpy(&cookie,bhdr->vsa,4);
		if (cookie != STANDARD_MAGIC_COOKIE) {
			memcpy(bootpvsa,bhdr->vsa,64);
			bootpvsa[64] = 0;
			setenv("BOOTPVSA",bootpvsa);
		}
	}
}

/* DhcpVendorSpecific():
	Process vendor specific stuff within the incoming dhcp header.
*/
void
DhcpVendorSpecific(struct	dhcphdr *dhdr)
{
}

/* printDhcpVSopt():
	Print vendor specific stuff within the incoming dhcp header.
*/
int
printDhcpVSopt(int vsopt, int vsoptlen, char *options)
{
	return(0);
}
