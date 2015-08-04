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
 * dhcp_01.c:
 * This is the PPA-specific code used for Videotron Market trial.
 * It was built from the dhcp_00.c DHCP-extension template file.
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

/* Vendor specific option definitions: */
#define VS_PROXYIP      1
#define VS_PHONENUM     2
#define VS_CODER        3
#define VS_PPADHCPSRVR  4
#define VS_MINNETDELAY  5
#define VS_MAXNETDELAY  6
#define VS_APPINFO      7
#define VS_CODERCTL     8
#define VS_NAMEVAL      99

#define PPADHCPSRVR_STR "PPADHCPSRVR"

/* ValidDHCPOffer():
 *  Target issued the DISCOVER, the incoming packet is the server's
 *  OFFER reply.  If the offer contains the vendor specific
 *  VS_PPADHCPSRVR option and the string within that
 *  option is correct, then accept the offer; by issuing a request.
 */
ValidDHCPOffer(struct   dhcphdr *dhdr)
{
    uchar   *op, *op1;

    op = op1 = 0;
    op1 = DhcpGetOption(DHCPOPT_VENDORSPECIFICINFO,dhdr+1);
    if(op1) {
        op = DhcpGetOption(VS_PPADHCPSRVR,op1+2);
    }
    if(op) {
        if(!strncmp(op+2,PPADHCPSRVR_STR,sizeof(PPADHCPSRVR_STR)-1)) {
            return(1);
        }
    }
    return(0);
}

/* buildDhcpHdr():
 *  Called by dhcpboot.c to allow application-specific header stuff to
 *  be added to header.  Return 0 if generic stuff in dhcpboot.c is to be
 *  used; else return 1 and the calling code will assume this function is
 *  dealing with it (see dhcpboot.c for basic idea).
 */
int
buildDhcpHdr(struct dhcphdr *dhcpdata)
{
    return(0);
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
    return;
}

void
SetPPAOption(type,op1,varname)
int     type;
uchar   *op1;
char    *varname;
{
    uchar   *op, tmp;

    op = DhcpGetOption(type,op1+2);
    if(op) {
        tmp = op[*(op+1)+2];
        op[*(op+1)+2] = 0;
        DhcpSetEnv(varname,op+2);
        op[*(op+1)+2] = tmp;
    }
}

/* DhcpVendorSpecific():
 *  Process vendor specific stuff within the incoming dhcp header.
 */
DhcpVendorSpecific(struct   dhcphdr *dhdr)
{
    ulong   ip;
    uchar   *op, *op1, buf[16], tmp;

    op = op1 = 0;
    op1 = DhcpGetOption(DHCPOPT_VENDORSPECIFICINFO,dhdr+1);
    if(op1) {
        /* Get PROXY_IP and BPROXY_IP (optionally): */
        op = DhcpGetOption(VS_PROXYIP,op1+2);
        if(op) {
            memcpy(&ip,op+2,4);
            DhcpSetEnv("PROXY_IP",IpToString(ip,buf));
            if(*(op+1) == 8) {
                memcpy(&ip,op+6,4);
                DhcpSetEnv("BPROXY_IP",IpToString(ip,buf));
            }
        }

        /* Get PPA phone numbers (1 or 2). */
        op = DhcpGetOption(VS_PHONENUM,op1+2);
        if(op) {
            char *space;
            tmp = op[*(op+1)+2];
            op[*(op+1)+2] = 0;
            space = strchr(op+2,' ');
            if(space) {
                *space = 0;
                DhcpSetEnv("LINE0_NUMBER",op+2);
                DhcpSetEnv("LINE1_NUMBER",space+1);
                *space = ' ';
            } else {
                DhcpSetEnv("LINE0_NUMBER",op+2);
            }
            op[*(op+1)+2] = tmp;
        }

        /* Set Coder type: */
        SetPPAOption(VS_CODER,op1,"CODER");

        /* Set Coder control: */
        SetPPAOption(VS_CODERCTL,op1,"CODERCTL");

        /* Set minimum network delay setting: */
        SetPPAOption(VS_MINNETDELAY,op1,"MIN_NET_DELAY");

        /* Set maximum network delay setting: */
        SetPPAOption(VS_MAXNETDELAY,op1,"MAX_NET_DELAY");

        /* Set appinfo: */
        SetPPAOption(VS_APPINFO,op1,"APPINFO");
    }

    /* Check for VS_NAMEVAL here...
     * If the Vendor-Specific-Information is present, and within that
     * information there is a sub-option of VS_NAMEVAL,
     * then consider the content of that sub-option to be one or more
     * strings (separated by a comma) of the format "VARNAME=VALUE".
     * This is used to allow the DHCP server to configure shell variables
     * into the environment prior to the monitor turning over control to
     * the application.
     * Two examples of VS_NAMEVAL strings:
     * First, just one name-value combination...
     *  VARNAME=VALUE
     * Second, a multiple name-value combination...
     *  VARNAME=VALUE,VAR1=ABC,IP=1.2.3.4
     */

    op = op1 = 0;
    op1 = DhcpGetOption(DHCPOPT_VENDORSPECIFICINFO,dhdr+1);
    if(op1) {
        op = DhcpGetOption(VS_NAMEVAL,op1+2);
    }
    if(op) {
        int     len;
        uchar   *end, tmp;
        uchar   *name, *value, *eqsign, *comma, *base;

        op++;
        len = (int)*op++;
        base = op;
        tmp = base[len];
        base[len] = 0;
        end = base + len;
        while(op < end) {
            eqsign = (uchar *)strchr(op,'=');
            if(!eqsign) {
                break;
            }
            name = op;
            *eqsign = 0;
            value = eqsign+1;
            comma = (uchar *)strchr(value,',');
            if(comma) {
                *comma = 0;
                op = comma+1;
            } else {
                op = end;
            }
            DhcpSetEnv(name,value);
            *eqsign = '=';
            *comma = ',';
        }
        base[len] = tmp;
    }
}

int
printDhcpVSopt(int vsopt, int vsoptlen, char *options)
{
    switch(vsopt) {
    case VS_PROXYIP:
        for(i=0; i<vsoptlen; i++) {
            printf("%d ",(unsigned int)*options++);
        }
        break;
    case VS_CODER:
    case VS_NAMEVAL:
    case VS_MINNETDELAY:
    case VS_MAXNETDELAY:
    case VS_PPADHCPSRVR:
    case VS_PHONENUM:
    case VS_APPINFO:
        for(i=0; i<vsoptlen; i++) {
            printf("%c",*options++);
        }
        break;
    default:
        return(0);
    }
    return(1);
}
