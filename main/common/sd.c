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
 * sd.c:
 *
 * This code is the user interface portion of the sd (compact flash)
 * command for uMon.
 * This command is intended to be the "interface" portion of some
 * other command (for example "fatfs").  Refer to the discussion in
 * fatfs.c for more details.
 *
 * Original author:     Ed Sutter (ed.sutter@alcatel-lucent.com)
 *
 */

#include "config.h"
#if INCLUDE_SD
#include "stddefs.h"
#include "genlib.h"
#include "cli.h"
#include "timer.h"
#include "tfs.h"
#include "tfsprivate.h"
#include "sd.h"

struct sdinfo sdInfoTbl[SD_DEVTOT];
char sdVerbose;
static int sdInum;      /* Interface number: to support multiple SD interfaces.
                         * Typically this will always be zero.
                         */


static struct sdcmd sdCardCmdTbl[] = {
    { GO_IDLE_STATE,            R1_RLEN,    R1  },
    { SEND_OP_COND,             R1_RLEN,    R1  },
    { SWITCH_FUNC,              R1_RLEN,    R1  },
    { SEND_IF_COND,             R7_RLEN,    R7  },
    { SEND_CSD,                 R1_RLEN,    R1  },
    { SEND_CID,                 R1_RLEN,    R1  },
    { STOP_TRANSMISSION,        R1_RLEN,    R1B },
    { SEND_STATUS,              R2_RLEN,    R2  },
    { SET_BLOCKLEN,             R1_RLEN,    R1  },
    { READ_SINGLE_BLK,          R1_RLEN,    R1  },
    { READ_MULTIPLE_BLK,        R1_RLEN,    R1  },
    { WRITE_BLK,                R1_RLEN,    R1B },
    { WRITE_MULTIPLE_BLK,       R1_RLEN,    R1B },
    { PROGRAM_CSD,              R1_RLEN,    R1  },
    { SET_WRITE_PROT,           R1_RLEN,    R1B },
    { CLR_WRITE_PROT,           R1_RLEN,    R1B },
    { SEND_WRITE_PROT,          R1_RLEN,    R1  },
    { ERASE_WR_BLK_START_ADDR,  R1_RLEN,    R1  },
    { ERASE_WR_BLK_END_ADDR,    R1_RLEN,    R1  },
    { ERASE,                    R1_RLEN,    R1B },
    { LOCK_UNLOCK,              R1_RLEN,    R1  },
    { APP_CMD,                  R1_RLEN,    R1  },
    { GEN_CMD,                  R1_RLEN,    R1  },
    { READ_OCR,                 R3_RLEN,    R3  },
    { CRC_ON_OFF,               R1_RLEN,    R1  },
    { SD_SEND_OP_COND,          R1_RLEN,    R1  },
    { -1,-1,-1  }
};

char *SdHelp[] = {
    "Secure Digital Flash Interface",
    "[options] {operation} [args]...",
#if INCLUDE_VERBOSEHELP
    "",
    "Options:",
    " -i ##  interface # (default is 0)",
    " -v     additive verbosity",
    "",
    "Operations:",
    " init [prefix]",
    " cmd {cmdnum} [arg]",
    " read {dest} {blk} {blktot}",
    " write {src} {blk} {blktot}",
#endif
    0
};

int
SdCmd(int argc, char *argv[])
{
    char *cmd, *buf, *prefix, varname[16];
    int opt, verbose, sdret, blknum, blkcnt;

    verbose = 0;
    while((opt=getopt(argc,argv,"i:v")) != -1) {
        switch(opt) {
        case 'i':
            sdInum = atoi(optarg);  /* sticky */
            break;
        case 'v':
            verbose++;
            break;
        default:
            return(CMD_PARAM_ERROR);
        }
    }

    if(argc < optind + 1) {
        return(CMD_PARAM_ERROR);
    }

    cmd = argv[optind];

    if(sdInum >= SD_DEVTOT) {
        printf("Configured to support %d SD interface%s\n",
               SD_DEVTOT,SD_DEVTOT == 1 ? "" : "s");
        return(CMD_FAILURE);
    }

    if(sdInstalled(sdInum) == 0) {
        printf("SDCard not installed\n");
        return(CMD_FAILURE);
    }

    if(strcmp(cmd,"init") == 0) {
        sdret = sdInit(sdInum, verbose);
        if(sdret < 0) {
            printf("sdInit returned %d\n",sdret);
            return(CMD_FAILURE);
        }

        // If prefix is specified, then load shell variables:
        if(argc == optind+2) {
            prefix = argv[optind+1];
            if(strlen(prefix)+4 > sizeof(varname)) {
                printf("prefix %s too long\n",prefix);
                return(CMD_PARAM_ERROR);
            }

            sprintf(varname,"%s_RD",prefix);
            shell_sprintf(varname,"0x%lx",(long)sdRead);

            sprintf(varname,"%s_WR",prefix);
            shell_sprintf(varname,"0x%lx",(long)sdWrite);
        }

        shell_sprintf("SD_BLKSIZE","0x%lx",SD_BLKSIZE);
    } else if(strcmp(cmd,"cmd") == 0) {
        ulong cmdarg;
        uchar resp[8];
        int rtot, i, cmdnum;

        cmdarg = 0;
        memset((char *)resp,0xff,sizeof(resp));

        if((argc != (optind+2)) && (argc != (optind+3))) {
            return(CMD_PARAM_ERROR);
        }

        cmdnum = (uchar)strtoul(argv[optind+1],0,0);
        if(argc == optind+3) {
            cmdarg = strtoul(argv[optind+2],0,0);
        }
        sdret = sdCardCmd(sdInum, cmdnum , cmdarg ,resp);
        if(sdret < 0) {
            printf("sdCardCmd returned %d\n",sdret);
            return(CMD_FAILURE);
        }

        rtot = sdCmdrlen(cmdnum);

        printf("CMD_%d resp: ",cmdnum);
        for(i=0; i<rtot; i++) {
            printf("%02x ",resp[i]);
        }
        printf("\n");
    } else if(strcmp(cmd,"read") == 0) {
        if(argc != optind+4) {
            return(CMD_PARAM_ERROR);
        }

        buf = (char *)strtoul(argv[optind+1],0,0);
        blknum = strtoul(argv[optind+2],0,0);
        blkcnt = strtoul(argv[optind+3],0,0);

        sdret = sdRead(sdInum,buf,blknum,blkcnt);
        if(sdret < 0) {
            printf("sdRead returned %d\n",sdret);
            return(CMD_FAILURE);
        }
    } else if(strcmp(cmd,"write") == 0) {
        if(argc != optind+4) {
            return(CMD_PARAM_ERROR);
        }
        buf = (char *)strtoul(argv[optind+1],0,0);
        blknum = strtoul(argv[optind+2],0,0);
        blkcnt = strtoul(argv[optind+3],0,0);

        sdret = sdWrite(sdInum,buf,blknum,blkcnt);
        if(sdret < 0) {
            printf("sdWrite returned %d\n",sdret);
            return(CMD_FAILURE);
        }
    } else {
        printf("sd op <%s> not found\n",cmd);
        return(CMD_FAILURE);
    }

    return(CMD_SUCCESS);
}

/* sdCmdrlen():
 * Given the command number, return the response length.
 */
int
sdCmdrlen(int cmd)
{
    struct sdcmd *sdp = sdCardCmdTbl;

    while(sdp->cmd != -1) {
        if(cmd == sdp->cmd) {
            return(sdp->rlen);
        }
        sdp++;
    }
    return(-1);
}

/* sdCmdrtype():
 * Given the command number, return the response type.
 */
int
sdCmdrtype(int cmd)
{
    struct sdcmd *sdp = sdCardCmdTbl;

    while(sdp->cmd != -1) {
        if(cmd == sdp->cmd) {
            return(sdp->rtype);
        }
        sdp++;
    }
    return(-1);
}

static char *months[] = {
    "???", "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec",
};

void
sdShowCID(uchar *cid)
{
    int     year, mon;

    printf("  Manufacturer ID:    x%02x\n", cid[0]);
    printf("  OEM/Application ID: x%02x%02x\n", cid[1],cid[2]);
    printf("  Product name:       %c%c%c%c%c\n",
           cid[3],cid[4],cid[5],cid[6],cid[7]);
    printf("  Product rev:        x%02x\n", cid[8]);
    printf("  Product serialno:   x%02x%02x%02x%02x\n",
           cid[9],cid[10],cid[11],cid[12]);

    year = (((cid[13] & 0x0f) << 4) | ((cid[14] & 0xf0) >> 4));
    mon = (cid[14] & 0x0f);
    if((mon < 1) || (mon > 12)) {
        mon = 0;
    }
    printf("  Manufactured:       %s/%d\n",months[mon],2000+year);
}

void
sdShowCSD(uchar *csd)
{
    uchar csdver;
    long long capacity;
    int mult, csize, csm, rbl;
    int blocknr, blocklen, i;

    printf("  CSD Response: ");
    for(i=0; i<16; i++) {
        printf("%02x ",csd[i]);
    }
    printf("\n");

    mult = csize = csm = rbl = 0;

    if((csd[0] & 0xc0) == 0) {
        csdver = 1;
    } else if((csd[0] & 0xc0) == 0x40) {
        csdver = 2;
    } else {
        printf("Invalid CSD structure type\n");
        return;
    }

    printf("  CSD version %d.0\n",csdver);
    if(csdver == 1) {
        rbl = csd[5] & 0x0f;
        csize = ((csd[8] & 0xC0) >> 6);
        csize |= (csd[7] << 2);
        csize |= ((csd[6] & 0x03) << 10);
        csm = (csd[9] & 0x03);
        csm <<= 1;
        csm |= ((csd[10] & 0x80) >> 7);

        mult = (1 << (csm+2));
        blocknr = (csize+1)*mult;
        blocklen = (1 << rbl);
        capacity = (long long)((long long)blocknr * (long long)blocklen);
        //printf("  (csm=%d, csize=%d, rbl=%d, mult=%d, blknr=%d, blkln=%d)\n",
        //  csm, csize, rbl, mult, blocknr, blocklen);
    } else if(csdver == 2) {
        rbl = csd[5] & 0x0f;
        csize = (csd[7] & 0x3f) << 16;
        csize |= (csd[8] << 8);
        csize |= csd[9];
        capacity = (long long)((long long)(csize + 1) * 512LL * 1024LL);
    } else {
        printf("  Unrecognized CSD version.\n");
        return;
    }
    printf("  Card capacity: %lld bytes\n",capacity);
}

/* sdGenericStartup():
 * Called by the interface-specific sdInit() code to do the generic
 * portion of the SD-Card initialization.
 *
 * Refer to the flowchart shown in figure 7.2 of the Simplified
 * Physical Layer Specification for an overview of the initialization.
 *
 * The card wakes up in SD bus mode, so the first thing we need to d
 * here (after the very basic initialization of the SPI pin config) is
 * to get the card into SPI mode.
 * By default, the card assumes CRC checking is disabled.  The field used
 * to hold the CRC is still part of the protocol, but it is ignored.  If
 * needed, the master can turn it on with CMD59.
 */
int
sdGenericStartup(int interface)
{
    ulong   hcs;
    uchar   resp[16], cid[16], csd[16];
    int     retry, rc, version;
    struct elapsed_tmr tmr;
    int check_pattern_retry = 2;

    sdInfoTbl[interface].initialized = 0;

    rc = 0;
    check_pattern_retry = 2;

    // Start with at least 74 clocks to powerup the card...
    sdPowerup(10);

    // Put card in SPI mode:

    // This command always seems to fail on the first try after a
    // powerup, so give it a few attempts...
    for(retry=0; retry<3; retry++) {
        if((rc = sdCardCmd(interface,GO_IDLE_STATE,0,resp)) != -1) {
            break;
        }
    }

    if(retry == 3) {
        printf("\nSD GO_IDLE_STATE failed, card installed?\n");
        return(-1);
    }

    // According to Physical Layer Simplified Spec (PLSS), it is mandatory
    // for the host compliant to Version 2.00 to send CMD8 (SEND_IF_COND).
    // If CMD8 is not recognized (illegal command), then we can assume that
    // the card is version 1.00.
    // The argument sent with the command is defined in section 4.3.13
    // of the PLSS.
    // The spec says that if the check-patter test fails, to retry...
    do {
        if((rc = sdCardCmd(interface,SEND_IF_COND,SEND_IF_COND_ARG,resp)) == -1) {
            return(-1);
        }

        if(resp[0] & R1_ILLEGAL_CMD) {
            version = VERSION_10;
            hcs = 0;
            break;  // Check pattern only applies to V2 and later
        } else {
            // The card should echo back the VHS and check pattern...
            if(resp[3] != VHS27_36) {
                if(sdVerbose & 2) {
                    printf("SDCARD not 3.3v compliant!\n");
                }
                return(-1);
            }
            if(resp[4] != CHECK_PATTERN) {
                if(--check_pattern_retry <= 0) {
                    if(sdVerbose & 2) {
                        printf("SDCARD check-pattern failed.\n");
                    }
                    return(-1);
                }
            }
            version = VERSION_20;
            hcs = HCS;
        }
    } while(resp[4] != CHECK_PATTERN);

    // Read OCR to make sure the card will run at 3.3v...
    if((rc = sdCardCmd(interface,READ_OCR,hcs,resp)) == -1) {
        return(-1);
    }

    if((resp[2] & MSK_OCR_33) != MSK_OCR_33) {
        if(sdVerbose & 2) {
            printf("SDCARD: OCR_33 failed, card isn't 3.3v capable\n");
        }
        return(-1);
    }

    // Wait for card to complete initialization:
    startElapsedTimer(&tmr,1000);
    while(1) {
        if((rc = sdCardCmd(interface,SD_SEND_OP_COND,HCS,resp)) == -1) {
            return(-1);
        }
        if((resp[0] & R1_IDLE) == 0) {
            break;
        }
        if(msecElapsed(&tmr)) {
            printf("SDCARD: gaveup waiting for init to complete.\n");
            return(-1);
        }
    }

    if(version == VERSION_20) {
        // Get CCS...
        if((rc = sdCardCmd(interface,READ_OCR,0,resp)) == -1) {
            return(-1);
        }
    } else {
        if((rc = sdCardCmd(interface,SET_BLOCKLEN,SD_BLKSIZE,resp)) == -1) {
            return(-1);
        }
    }

    if(sdVerbose) {
        printf("SD/SPI Initialized (version %s)\n",
               version == VERSION_10 ? "1.0" : ">=2.0");
    }

    sdInfoTbl[interface].cardversion = version;

    sdReadCxD(interface,cid,SEND_CID);
    if(sdVerbose) {
        sdShowCID(cid);
    }
    sdReadCxD(interface,csd,SEND_CSD);
    if(sdVerbose) {
        sdShowCSD(csd);
    }

    if((csd[0] & 0xc0) == 0x00) {
        sdInfoTbl[interface].highcapacity = 0;
    } else {
        sdInfoTbl[interface].highcapacity = 1;
    }

    sdInfoTbl[interface].initialized = 1;
    return(0);
}

/* Got this crc7 code off the web...
 * The original text claimed "use-as-you-wish"...
 */
#define POLYNOM (0x9)        // polynomical value to XOR when 1 pops out.

static unsigned char
Encode(uchar seed, uchar input, uchar depth)
{
    uchar regval, count, cc;

    regval = seed;
    cc = input;

    for(count = depth ; count--   ;  cc <<= 1) {
        regval = (regval << 1) + ((cc & 0x80) ? 1 : 0);
        if(regval & 0x80) {
            regval ^= POLYNOM;
        }
    }
    return(regval & 0x7f); // return lower 7 bits of CRC as value to use.
}

uchar
crc7(uchar seed, uchar *buf, int len)
{
    int i;
    uchar crc;

    crc = seed;
    for(i = 0; i < len; i++) {
        crc = Encode(crc, buf[i], 8);
    }

    crc = Encode(crc,0,7);
    crc = (crc << 1) + 1;
    return(crc);
}

#ifdef INCLUDE_SD_DUMMY_FUNCS
/* This code is included here just for simulating the SD
 * interface (temporarily if a real one isn't ready.  In a real system,
 * the INCLUDE_SD_DUMMY_FUNCS definition would be off.
 */

int
sdInit(int interface, int verbose)
{
    if(interface != 0) {
        return(-1);
    }

    return(0);
}

int
sdRead(int interface, char *buf, int blk, int blkcnt)
{
    char *from;
    int size;

    if(interface != 0) {
        return(-1);
    }

    from = (char *)(blk * SD_BLKSIZE);
    size = blkcnt * SD_BLKSIZE;
    memcpy(buf,from,size);
    return(0);
}

int
sdWrite(int interface, char *buf, int blk, int blkcnt)
{
    char *to;
    int size;

    if(interface != 0) {
        return(-1);
    }

    to = (char *)(blk * SD_BLKSIZE);
    size = blkcnt * SD_BLKSIZE;
    memcpy(to,buf,size);
    return(0);
}

/* sdInstalled():
 * Return 1 if installed, 0 if not installed or -1 if
 * the hardware can't detect installation.
 */
int
sdInstalled(int interface)
{
    return(-1);
}

#endif

#endif
