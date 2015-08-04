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
 * pci.c:
 *
 * This file provides the monitor with a reusable mechanism for
 * interfacing with a PCI bus.  Four target-specific functions are
 * required:
 *
 *  pciCtrl(), pciCfgRead(), pciCfgWrite(), pciShow()
 *
 * The two most important are pciCfgRead() and pciCfgWrite().  Refer to
 * the bottom of pci.h for further details.
 *
 * Original author:     Ed Sutter (ed.sutter@alcatel-lucent.com)
 *
 */
#include "config.h"
#include "pci.h"
#include "stddefs.h"
#include "genlib.h"
#include "cli.h"

#if INCLUDE_PCI
int pciVerbose;
int pciBusNum;

#define IMPLEMENTED 0x80000000

#ifdef USE_DEFAULT_PCISHOW
void
pciShow(int interface)
{
    printf("No fixed devices on this platform\n");
}
#endif

/* pciCfgAddress():
 * Return a 32-bit value based on the input
 * bus number, device number, function number and register
 * number:
 *
 *  31 30 .... 24 23 ... 16 15 ... 11 10 ....  8 7 ...    2 1 0
 * --------------------------------------------------------------
 * |  |          |  Bus    | Device  | Function | Register |0|T|
 * |En| Reserved |  Number | Number  | Number   | Number   | | |
 * --------------------------------------------------------------
 *  ^                                                         ^
 *  |---- Enable bit 1=enabled, 0=disabled                    |
 *                                                            |
 *       Type0: 0 --------------------------------------------|
 *       Type1: 1 --------------------------------------------|
 *
 * See pg 32 of the PCI2.2 spec for more details.
 */

unsigned long
pciCfgAddress(int busno,int devno,int fncno,int regno)
{
    int type;

    if(busno > 0) {
        type = 1;
    } else {
        type = 0;
    }

    return((type | PCICFG_ENABLE_BIT |
            ((busno & PCICFG_BUSNO_MASK) << PCICFG_BUSNO_SHIFT) |
            ((devno & PCICFG_DEVNO_MASK) << PCICFG_DEVNO_SHIFT) |
            ((fncno & PCICFG_FNCNO_MASK) << PCICFG_FNCNO_SHIFT) |
            ((regno & PCICFG_REGNO_MASK) << PCICFG_REGNO_SHIFT)));
}

/* pciBaseClass():
 * Based on appendix D of spec, return a simple string that describes
 * the base class of the incoming class code.
 */
char *
pciBaseClass(unsigned long classcode)
{
    unsigned long baseclass;
    unsigned long subclass_progif;

    baseclass = (classcode >> 16) & 0xff;
    subclass_progif = classcode & 0xffff;

    switch(baseclass) {
    case 0:
        return("pre-class-code-definitions");
    case 1:
        return("mass storage ctrlr");
    case 2:
        return("network ctrlr");
    case 3:
        return("display ctrlr");
    case 4:
        return("multimedia device");
    case 5:
        return("memory ctrlr");
    case 6: /* Supply additional information for bridge class...
                 */
        switch(subclass_progif) {
        case 0x0000:
            return("host/pci bridge");
        case 0x0100:
            return("pci/isa bridge");
        case 0x0200:
            return("pci/eisa bridge");
        case 0x0300:
            return("pci/microchannel bridge");
        case 0x0400:
            return("pci/pci bridge");
        case 0x0500:
            return("pci/pcmcia bridge");
        case 0x0600:
            return("pci/nubus bridge");
        case 0x0700:
            return("pci/cardbus bridge");
        case 0x8000:
            return("other bridge type");
        default:
            return("bridge device");
        }
    case 7:
        return("simple communication ctrlr");
    case 8:
        return("base system peripheral");
    case 9:
        return("input device");
    case 10:
        return("docking station");
    case 11:
        return("processor");
    case 12:
        return("serial bus ctrlr");
    case 13:
        return("wireless ctrlr");
    case 14:
        return("intelligent io ctrlr");
    case 15:
        return("satellite communication ctrlr");
    case 16:
        return("encrypt/decrypt ctrlr");
    case 17:
        return("data acquisition ctrlr");
    case 256:
        return("no fit");
    default:
        return("reserved");
    }
}

/* pciscan():
 * This function is used by "pci scan" and "pci enum" to look
 * at the devices on the pci bus.  When the enumerate flag is set,
 * this function will recursively nest itself each time it sees a
 * PCI-to-PCI bridge device on the bus, and while doing this, it will
 * assign bus numbers to each bridge appropriately.  This function does
 * not assign address ranges or anything else, it simply provides a quick
 * means of scanning all devices on the bus(es).
 *
 * NOTE: This has only been tested with simple PCI bus configurations (one
 * bridge deep) so deeper configurations (bridges on bridges on bridges...)
 * are untested as far as I know.
 */
void
pciscan(long interface, long bus, long func, int showhdr, int enumerate)
{
    long    device;
    uchar   hdr_type, rev_id;
    ushort  vendor_id, device_id;
    ulong   value, class_code;

    if(showhdr) {
        printf("\nInterface %ld...\n",interface);
        printf("Bus Dev Vndr  Dev   Rev Hdr  Class\n");
        printf("Num Num Id    Id    Id  Type Code\n");
    }

    if((enumerate == 1) && (bus == 0)) {
        pciBusNum = 0;
    }

    for(device=0; device<=31; device++) {
        /* Retrieve portions of the configuration header that
         * are required by all PCI compliant devices...
         * Vendor, Device and Revision IDs, Class Code and Header Type
         * (see pg 191 of spec).
         */

        /* Read reg_0 for vendor and device ids:
         */
        value = pciCfgRead(interface,bus,device,func,0);
        if(value == NO_DEVICE) {
            continue;
        }

        vendor_id = (ushort)(value & 0xffff);
        device_id = (ushort)((value>>16) & 0xffff);

        /* Read reg_2 for class code and revision id:
         */
        value = pciCfgRead(interface,bus,device,func,2);
        rev_id = (uchar)(value & 0xff);
        class_code = (ulong)((value>>8) & 0xffffff);

        /* Read reg_3:  header type:
         */
        value = pciCfgRead(interface,bus,device,func,3);
        hdr_type = (uchar)((value>>16) & 0xff);

        printf("%2ld  %02ld  x%04x x%04x",bus,
               device,vendor_id,device_id);
        printf(" x%02x x%02x  x%06lx (%s)\n",rev_id,
               hdr_type,class_code,pciBaseClass(class_code));

        /* If enumeration is enabled, see if this is a PCI-to-PCI
         * bridge.  If it is, then nest into pciscan...
         */
        if((enumerate) && (class_code == 0x060400)) {
            ulong pribus, secbus, subbus;

            pribus = pciBusNum & 0x0000ff;
            pciBusNum++;
            secbus = ((pciBusNum << 8) & 0x00ff00);
            subbus = ((pciBusNum << 16) & 0xff0000);

            value = pciCfgRead(interface,bus,device,func,6);
            value &= 0xffff0000;
            value |= (pribus | secbus);
            pciCfgWrite(interface,bus,device,func,6,value);

            pciscan(interface,pciBusNum,func,0,1);

            value = pciCfgRead(interface,bus,device,func,6);
            value &= 0xff000000;
            value |= (subbus | pribus | secbus);
            pciCfgWrite(interface,bus,device,func,6,value);
        }
    }
}

/* getBarInfo():
 * Apply the algorithm as specified in PCI spec...
 * Place size information in sizehi & sizelo (to support 64-bit).
 * Return 0 if not implemented; else return value to indicate
 * size (32 or 64 bit) and type (mem or io).
 */
ulong
getBarInfo(long interface,long bus,long device,long func,int barnum,
           ulong *sizehi, ulong *sizelo)
{
    int barregno;
    ulong implemented, barval1, barval2, barinfo1, barinfo2, cmd;

    /* Translate the incoming bar number to a register number
     * in PCI config space:
     */
    barregno = barnum + 4;

    /* Disable decoding through the command register:
     */
    cmd = pciCfgRead(interface,bus,device,func,1);
    pciCfgWrite(interface,bus,device,func,1,
                cmd & ~(IO_SPACE | MEMORY_SPACE));

    /* Read the BAR:
     */
    barval1 = pciCfgRead(interface,bus,device,func,barregno);

    /* Write 0xffffffff to the BAR:
     */
    pciCfgWrite(interface,bus,device,func,barregno,0xffffffff);

    /* Read the value returned as a result of writing
     * 0xffffffff to the BAR:
     */
    barinfo1 = pciCfgRead(interface,bus,device,func,barregno);

    /* Restore original bar:
     */
    pciCfgWrite(interface,bus,device,func,barregno,barval1);

    if(barinfo1 == 0) {
        implemented = 0;
    } else {
        implemented = IMPLEMENTED;

        if(barval1 & BASEADDRESS_IO) {
            implemented |= BASEADDRESS_IO;

            if(sizelo) {
                /* Clear encoding bits:
                 */
                barinfo1 &= 0xfffffffe;
                /* Invert and add 1:
                 */
                *sizelo = (~barinfo1 + 1) & 0xffff;

                if(sizehi) {
                    *sizehi = 0;
                }
            }
        } else {
            implemented |= (barinfo1 & PREFETCHABLE);

            if(barval1 & TYPE_64) {
                implemented |= TYPE_64;

                /* Apply same sequence as above to the next bar...
                 */
                barregno++;
                barval2 = pciCfgRead(interface,bus,device,func,barregno);
                pciCfgWrite(interface,bus,device,func,barregno,0xffffffff);
                barinfo2 = pciCfgRead(interface,bus,device,func,barregno);
                pciCfgWrite(interface,bus,device,func,barregno,barval2);


                if(sizelo) {
                    barinfo1 &= 0xfffffff0;
                    *sizelo = ~barinfo1 + 1;
                    if(sizehi) {
                        *sizehi = ~barinfo2 + 1;
                    }
                }
            } else {
                if(sizelo) {
                    barinfo1 &= 0xfffffff0;
                    *sizelo = ~barinfo1 + 1;

                    if(sizehi) {
                        *sizehi = 0;
                    }
                }
            }
        }
    }

    /* Now that we've completed messing with the BARS,
     * restore original cmd:
     */
    pciCfgWrite(interface,bus,device,func,1,cmd);

    return(implemented);
}

int
showBar(int barnum,long interface,long bus,long device,long func)
{
    int rtot;
    ulong bar, barnext, sizehi, sizelo, implemented;

    if((barnum < 0) || (barnum > 5)) {
        return(-1);
    }

    bar = pciCfgRead(interface,bus,device,func,barnum+4);

    implemented = getBarInfo(interface,bus,device,func,
                             barnum,&sizehi,&sizelo);

    if(!implemented) {
        printf("%02d BAR%d  : not implemented\n",barnum+4,barnum);
        return(0);
    }

    printf("%02d BAR%d",barnum+4,barnum);
    if(!(implemented & BASEADDRESS_IO) && (implemented & TYPE_64)) {
        barnext = pciCfgRead(interface,bus,device,func,barnum+5);
        printf("-%d: 0x%08lx 0x%08lx", barnum+1,bar,barnext);
    } else {
        printf("  : 0x%08lx",bar);
    }

    printf(" Size: 0x");
    if(!(implemented & BASEADDRESS_IO) && (implemented & TYPE_64)) {
        printf("%08lx%08lx ",sizehi,sizelo);
        rtot = 2;
    } else {
        printf("%08lx ",sizelo);
        rtot = 1;
    }

    if(bar & BASEADDRESS_IO)  {
        printf("IO");
    } else {
        printf("MEM %sbit",bar & TYPE_64 ? "64" : "32");
    }

    if(bar & PREFETCHABLE) {
        printf(" prefetchable");
    }

    putchar('\n');
    return(rtot);
}

void
dumpRawCfg(long interface,long bus,long device,long func,char *range,
           char *varname)
{
    int gotone;
    long regno;
    ulong value;

    value = 0;
    gotone = 0;
    for(regno=0; regno<64; regno++) {
        if(inRange(range,regno)) {
            value = pciCfgRead(interface,bus,device,func,regno);
            printf("Cfg reg #%02ld: 0x%08lx\n",regno,value);
            gotone = 1;
        }
    }
    if((varname) && (gotone)) {
        shell_sprintf(varname,"0x%08lx",value);
    }
}

void
dumpConfig(long interface,long bus,long device,long func)
{
    int i;
    char *multifunc, *type;
    ulong hdrtype, values[16];

    for(i=0; i<16; i++) {
        values[i] = pciCfgRead(interface,bus,device,func,i);
    }

    hdrtype = values[3] & HDR_MASK;

    if(hdrtype & HDR_MULTIFUNC) {
        multifunc = "multi-function ";
        hdrtype &= ~HDR_MULTIFUNC;
    } else {
        multifunc = "";
    }

    switch(hdrtype) {
    case HDR_PCI2PCI:
        type = "PCI-to-PCI";
        break;
    case HDR_CARDBUS:
        type = "CardBus";
        break;
    case HDR_STANDARD:
        type = "Standard";
        break;
    default:
        printf("dumpConfig(): hdrtype 0x%08lx not supported\n",hdrtype);
        return;
    }
    printf("%s %sconfig...\n",type,multifunc);

    printf("00 DevId/VendorId:   0x%04lx/%04lx\n",
           (values[0] & 0xffff0000) >> 16,values[0] & 0xffff);

    printf("01 Status/Command:   0x%04lx/%04lx\n",
           (values[1] & 0xffff0000) >> 16,values[1] & 0xffff);

    printf("02 ClassCode/RevId:  0x%06lx/%02lx\n",
           (values[2] & 0xffffff00) >> 8,values[2] & 0xff);


    printf("03 BIST/HdrType/LatencyTmr/CacheLnSz: 0x%02lx/%02lx/%02lx/%02lx\n",
           (values[3] & 0xff000000) >> 24, (values[3] & 0xff0000) >> 16,
           (values[3] & 0xff00) >> 8,values[3] & 0xff);

    if(showBar(0,interface,bus,device,func) == 1) {
        showBar(1,interface,bus,device,func);
    }

    if(hdrtype == HDR_STANDARD) {
        if(showBar(2,interface,bus,device,func) == 1) {
            showBar(3,interface,bus,device,func);
        }
        if(showBar(4,interface,bus,device,func) == 1) {
            showBar(5,interface,bus,device,func);
        }

        printf("10 Cardbus CIS Ptr:        0x%08lx\n",values[10]);
        printf("11 SubSysId/SubVendorId:   0x%04lx/%04lx\n",
               (values[11] & 0xffff0000) >> 16,values[11] & 0xffff);
        printf("12 Expansion ROM BaseAddr: 0x%08lx\n",values[12]);
    } else if(hdrtype == HDR_PCI2PCI) {
        printf("06 Secondary Latency Tmr:  0x%02lx\n",
               (values[6] & 0xff000000) >> 24);
        printf("06 BusNum Subordinate/Secondary/Primary: 0x%02lx/%02lx/%02lx\n",
               (values[6] & 0xff0000) >> 16,
               (values[6] & 0xff00) >> 8,values[6] & 0xff);

        printf("07 Secondary Status: 0x%04lx\n",
               (values[7] & 0xffff0000) >> 16);
        printf("07 IO Limit/Base:    0x%02lx/%02lx\n",
               (values[7] & 0xff00) >> 8,(values[7] & 0xff));

        printf("08 Memory Limit/Base:                0x%04lx/%04lx\n",
               (values[8] & 0xffff0000) >> 16,values[8] & 0xffff);

        printf("09 Prefetchable Memory Limit/Base:   0x%04lx/%04lx\n",
               (values[9] & 0xffff0000) >> 16,values[9] & 0xffff);

        printf("10 Prefetchable Base Upper 32 bits:  0x%08lx\n",values[10]);
        printf("11 Prefetchable Limit Upper 32 bits: 0x%08lx\n",values[11]);

        printf("12 IO Upper 16 Bits Limit/Base:      0x%04lx/%04lx\n",
               (values[12] & 0xffff0000) >> 16,values[12] & 0xffff);
    }

    printf("13 Capabilities Ptr:       0x%02lx\n",values[13] & 0xff);

    if(hdrtype == HDR_STANDARD) {
        printf("15 MaxLat/MinGnt:          0x%02lx/%02lx\n",
               (values[15] & 0xff000000) >> 24, (values[15] & 0xff0000) >> 16);
    } else {
        printf("14 Expansion ROM BaseAddr: 0x%08lx\n",
               values[14]);
        printf("15 BridgeControl:          0x%04lx\n",
               (values[15] & 0xffff0000) >> 16);
    }

    printf("15 Interrupt Pin/Line:     0x%02lx/%02lx\n",
           (values[15] & 0xff00) >> 8, values[15] & 0xff);
}

char *PciHelp[] = {
    "PCI Config Interface",
    "-[b:d:f:i:v] {operation} [args]",
#if INCLUDE_VERBOSEHELP
    "Options:",
    " -b{###}   bus #",
    " -d{###}   device #",
    " -f{###}   function #",
    " -i{###}   interface #",
    " -v        verbose",
    "Operations:",
    " scan, enum, bist, show, size {bar#}",
    " crd [reg#] [varname], cwr {reg#} {value}",
    "Note:",
    " bus, device, function & interface default to zero",
#endif
    0
};

/* PciCmd():
 * General purpose command to provide access to the config portion of
 * a PCI interface.
 *
 * Notice that there is reference to an "interface number" as well as a
 * "bus number".  In most systems, there will be one host-to-pci interface.
 * and one pci bus.   Some targets will have more than one host-to-pci
 * interface (galileo 64260A, for example, has 2 distinct host-to-PCI
 * interface).  Some targets will have more than one bus (depends on whether
 * or not there is a pci-to-pci bridge hanging off the bus).
 *
 */
int
PciCmd(int argc, char *argv[])
{
    ulong   value, bar;
    long    bus, device, interface, regno, func;
    int     enumerate, opt;

    bus = 0;
    func = 0;
    device = 0;
    enumerate = 0;
    interface = 0;
    pciVerbose = 0;
    while((opt=getopt(argc,argv,"b:d:f:i:v")) != -1) {
        switch(opt) {
        case 'b':
            bus = strtol(optarg,0,0);
            break;
        case 'd':
            device = strtol(optarg,0,0);
            break;
        case 'f':
            func = strtol(optarg,0,0);
            break;
        case 'i':   /* Most systems will only have 1 interface */
            interface = strtol(optarg,0,0);
            break;
        case 'v':
            pciVerbose = 1;
            break;
        default:
            return(CMD_FAILURE);
        }
    }

    if(argc < optind+1) {
        return(CMD_PARAM_ERROR);
    }

    /* The "scan" and "enum" commands are very similar.  They both use
     * the pciscan() function.
     *
     * Scan: look at the devices on the specified bus.
     * Enum: a recursive scan... start with bus 0 and attempt to query
     * all devices on all busses, making the necessary bus-number assignments
     * along the way.
     */

    /* For scan, the device number is ignored, all devices on a particular
     * interface/bus are checked.  If ths bus number is something other than
     * zero, then this function assumes that the appropriate pci-to-pci
     * bridge device has already had its bus numbers assigned.
     */
    if(!strcmp(argv[optind],"scan")) {
        if(argc != optind+1) {
            return(CMD_PARAM_ERROR);
        }

        pciscan(interface,bus,func,1,0);
        return(CMD_SUCCESS);
    }

    /* For enum, we start with bus 0, and attempt to enumerate all busses...
     */
    if(!strcmp(argv[optind],"enum")) {
        if(argc != optind+1) {
            return(CMD_PARAM_ERROR);
        }

        pciscan(interface,0,func,1,1);
        return(CMD_SUCCESS);
    }

    if(!strcmp(argv[optind],"size")) {  /* See pg 204 of PCI2.2 spec */
        int barnum;
        ulong implemented, sizehi, sizelo;

        if(argc != optind+2) {
            return(CMD_PARAM_ERROR);
        }

        /* The argument to size is the BAR #.  The value can be a single
         * digit or a range specification...
         */
        printf(" Bar  Type Value      Size\n");
        for(barnum=0; barnum<6; barnum++) {
            setenv("PCISIZE",0);
            if(inRange(argv[optind+1],barnum)) {
                /* Disable decoding through the command register:
                 * See section 6.2.2, pg 193.
                 */
                value = pciCfgRead(interface,bus,device,func,1);
                pciCfgWrite(interface,bus,device,0,1,
                            value & ~(IO_SPACE | MEMORY_SPACE));

                bar = pciCfgRead(interface,bus,device,func,barnum+4);
                implemented = getBarInfo(interface,bus,device,func,
                                         barnum,&sizehi,&sizelo);

                printf(" %d    ",barnum);

                if(implemented) {
                    shell_sprintf("PCISIZE","0x%08lx",bar);
                    printf("%s  0x%08lx ",
                           implemented & BASEADDRESS_IO ? "io " : "mem", bar);
                    if(implemented & TYPE_64) {
                        printf("0x%08lx%08lx",sizehi,sizelo);
                    } else {
                        printf("0x%08lx",sizelo);
                    }
                    if(implemented & PREFETCHABLE) {
                        printf(" (prefetchable)");
                    }
                    putchar('\n');
                } else {
                    printf("not implemented\n");
                }
            }
        }
    } else if(!strcmp(argv[optind],"bist")) {
        if(argc != optind+1) {
            return(CMD_PARAM_ERROR);
        }

        value = pciCfgRead(interface,bus,device,func,3);

        if(value & BIST_CAPABLE) {
            /* Set the BIST_START bit to begin the test, then wait for
             * the BIST_START bit to clear as an indication that the
             * test has completed.
             */
            pciCfgWrite(interface,bus,device,func,3,value | BIST_START);
            while(1) {
                value = pciCfgRead(interface,bus,device,func,3);
                if((value & BIST_START) != BIST_START) {
                    break;
                }
            }
            if((value & BIST_COMPCODE_MASK) != 0) {
                printf("BIST failed: 0x%lx\n",value & BIST_COMPCODE_MASK);
            } else {
                printf("BIST passed\n");
            }
        } else {
            printf("Device %ld is not BIST-capable\n",device);
        }
    } else if(!strcmp(argv[optind],"crd")) {
        char *varname = (char *)0;

        if(argc == optind+3) {      /* varname specified ? */
            varname = argv[optind+2];
            argc--;
        }

        if(argc == optind+2) {
            dumpRawCfg(interface,bus,device,func,argv[optind+1],varname);
        } else if(argc == optind+1) {
            dumpConfig(interface,bus,device,func);
        } else {
            return(CMD_PARAM_ERROR);
        }
    } else if(!strcmp(argv[optind],"cwr")) {
        if(argc != optind+3) {
            return(CMD_PARAM_ERROR);
        }

        regno = strtol(argv[optind+1],0,0);
        value = strtol(argv[optind+2],0,0);
        pciCfgWrite(interface,bus,device,func,regno,value);
    } else if(!strcmp(argv[optind],"init")) {
        pciCtrl(interface,PCICTRL_INIT,0,0);
    } else if(!strcmp(argv[optind],"show")) {
        pciShow(interface);
    } else {
        return(CMD_PARAM_ERROR);
    }
    return(CMD_SUCCESS);
}
#endif
