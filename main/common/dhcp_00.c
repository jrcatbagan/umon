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
 * dhcp_00.c:
 *
 * This file contains the functions that are called from dhcpboot.c if the
 * the default DHCP interaction is to be done by the monitor.  Whenever a
 * new dhcp_XX.c file is created, this file should be used as the template.
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
#include "endian.h"
#include "stddefs.h"

#if INCLUDE_DHCPBOOT

/* ValidDHCPOffer():
 *  Target issued the DISCOVER, the incoming packet is the server's
 *  OFFER reply.  If the DHCPOFFRFLTR shell variable is not present, then
 *  return 1 indicating acceptance of the offer.  If the DHCPOFFRFLTR variable
 *  exists, then use its content to determine if the offer should be accepted...
 *
 *  Shell variable syntax:
 *
 *  DHCP_FIELD_IDENTIFIER,EXPECTED_STRING
 *
 *  where DHCP_FIELD_IDENTIFIER can be...
 *      BFN     bootfile name
 *      SHN     server-host name
 *      VSO###  vendor-specific option number (options encapsulated within
 *              the Vendor-Specific Information (#43) option)
 *      SSO###  site-specific option number (range 128-254)
 *
 *  For example:
 *  1...
 *      if DHCPOFFRFLTR contains "BFN,abcfile"
 *      then the offer will only be accepted if the bootfile specified by the
 *      dhcp server contains the string "abcfile".
 *  2...
 *      if DHCPOFFRFLTR contains "SHN,a_host_name"
 *      then the offer will only be accepted if the server-hostname specified
 *      by the dhcp server contains the string "a_host_name".
 *  3...
 *      if DHCPOFFRFLTR contains "VSO18,some_String"
 *      then the offer will only be accepted if the server returns vendor-
 *      specific option # 18 and it contains the string "some_String".
 *
 *  Note that "containing the string" means that the string specified in
 *  the DHCPOFFRFLTR shell variable can be the entire string or a sub-string
 *  within the server's response.
 */
int
ValidDHCPOffer(struct   dhcphdr *dhdr)
{
    char *var;
    int offeraccepted, badsyntax;

    /* If no DHCPOFFRFLTR var, accept the offer... */
    var = getenv("DHCPOFFRFLTR");
    if(!var) {
        return(1);
    }

    /* Start off by assuming acceptance... */
    badsyntax = 0;
    offeraccepted = 1;

    /* Now process the DHCPOFFRFLTR variable and incoming dhcp data,
     * and clear the offeraccepted (or set badsyntax) flag if necessary...
     */

    if(!strncmp(var,"BFN,",4)) {
        if(!strstr((char *)dhdr->bootfile,var+4)) {
            offeraccepted = 0;
        }
    } else if(!strncmp(var,"SHN,",4)) {
        if(!strstr((char *)dhdr->server_hostname,var+4)) {
            offeraccepted = 0;
        }
    } else if((!strncmp(var,"SSO",3)) || (!strncmp(var,"VSO",3))) {
        int optno;
        ulong cookie;
        char *comma, *optval, *vso;

        optno = atoi(var+3);
        comma = strchr(var,',');
        memcpy((char *)&cookie,(char *)&dhdr->magic_cookie,4);
        if(cookie != ecl(STANDARD_MAGIC_COOKIE)) {
            offeraccepted = 0;
        } else if(!comma) {
            badsyntax = 1;
        } else if(var[0] == 'S') {
            if((optno < 128) || (optno > 254)) {
                badsyntax = 1;
            } else {
                optval = (char *)DhcpGetOption(optno,(unsigned char *)(dhdr+1));
                if(!optval) {
                    offeraccepted = 0;
                } else {
                    if(!strstr(optval+2,comma+1)) {
                        offeraccepted = 0;
                    }
                }
            }
        } else if(var[0] == 'V') {
            if((optno < 0) || (optno > 254)) {
                badsyntax = 1;
            } else {
                vso = (char *)DhcpGetOption(DHCPOPT_VENDORSPECIFICINFO,
                                            (uchar *)dhdr+1);
                if(!vso) {
                    offeraccepted = 0;
                } else {
                    optval = (char *)DhcpGetOption(optno,(unsigned char *)vso+2);
                    if(!optval) {
                        offeraccepted = 0;
                    } else {
                        if(!strstr(optval+2,comma+1)) {
                            offeraccepted = 0;
                        }
                    }
                }
            }
        }
    } else {
        badsyntax = 1;
    }
    if(badsyntax) {
        printf("Bad DHCPOFFRFLTR syntax.\n");
        offeraccepted = 0;
    }
    return(offeraccepted);
}

/* DhcpVendorSpecific():
 *  Process any vendor specific data from the incoming header.
 */
void
DhcpVendorSpecific(struct dhcphdr *dhdr)
{
    return;         /* Obviously, the default is no processing. */
}

/* printDhcpVSopt():
 *  Input is the option, the option length and the pointer to the options.
 *  Print the option.
 */
int
printDhcpVSopt(int vsopt, int vsoptlen, char *options)
{
    return(0);
}

/* buildDhcpHdr():
 *  Called by dhcpboot.c to allow application-specific header stuff to
 *  be added to header.  Return 0 if generic stuff in dhcpboot.c is to be
 *  used; else return 1 and the calling code will assume this function is
 *  dealing with it (see dhcpboot.c for basic idea).
 */
int
buildDhcpHdr(struct dhcphdr *dhdr)
{
    return(0);
}

/* DhcpBootpLoadVSA():
 *  Called by DhcpBootpDone to store an ascii-coded-hex copy of the
 *  vendor-specific-area (BOOTP) or options (DHCP) in the shell variable
 *  DHCPVSA.
 */
void
DhcpBootpLoadVSA(uchar *vsabin, int vsize)
{
    int i;
    uchar   *cp, *vsaacx;

    /* If after the transaction has completed, the RLYAGNT is   */
    /* set, but GIPADD is not set, copy RLYAGNT to GIPADD...    */
    if(!getenv("GIPADD") && getenv("RLYAGNT")) {
        setenv("GIPADD",getenv("RLYAGNT"));
    }

    /* Since the VSA (bootp) or options list (DHCP) can be large,
     * This code looks for the presence of the DHCPVSA shell variable
     * as an indication as to whether or not this data should be stored
     * in the DHCPVSA variable...
     * If the variable is present, then this code will load the DHCPVSA
     * shell variable; else it just returns here.
     * Note that it doesn't matter what the content of DHCPVSA is, as
     * long as it is present, so just loading it with "TRUE" in monrc
     * will be sufficient to tell this logic to load the variable with
     * the data.
     */
    if(!getenv("DHCPVSA")) {
        return;
    }

    /* If allocation succeeds, then copy BOOTP VendorSpecificArea to the */
    /* DHCPVSA shell variable. Copy it as ascii-coded hex */
    vsaacx = (uchar *)malloc((vsize*2)+4);
    if(vsaacx) {
        cp = vsaacx;
        for(i=0; i<vsize; i++,cp+=2) {
            sprintf((char *)cp,"%02x", vsabin[i]);
        }
        *cp = 0;
        setenv("DHCPVSA", (char *)vsaacx);
        free((char *)vsaacx);
    } else {
        printf("DHCPVSA space allocation failed\n");
    }
}


/* DhcpBootpDone():
 *  Called at the end of the Bootp or Dhcp transaction.
 *  Input...
 *  bootp:  1 if BOOTP; else DHCP.
 *  dhdr:   pointer to dhcp or bootp header.
 *  vsize:  size of vendor specific area (for bootp this is fixed at 64,
 *          but for dhcp it is variable).
 */
void
DhcpBootpDone(int bootp, struct dhcphdr *dhdr, int vsize)
{
    if(bootp) {
        struct bootphdr *bhdr;

        bhdr = (struct bootphdr *)dhdr;
        DhcpBootpLoadVSA(bhdr->vsa,vsize);
    } else {
        DhcpBootpLoadVSA((uchar *)&dhdr->magic_cookie,vsize);
    }
    return;
}

#endif
