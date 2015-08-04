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
 * memcmds.c:
 *
 * This code allows the monitor to display, modify, search, copy, fill
 * and test memory in a variety of different ways.
 *
 * Original author:     Ed Sutter (ed.sutter@alcatel-lucent.com)
 *
 */
#include "config.h"
#include "genlib.h"
#include "stddefs.h"
#include <ctype.h>
#include "cli.h"

/* With INCLUDE_MEMCMDS defined in config.h, all uMon commands in this
 * file are automatically pulled into the build.  If there is a need to
 * be more selective, then set INCLUDE_MEMCMDS to zero in config.h and
 * define only the INCLUDE_XX macros needed (shown below) to one in
 * config.h.
 */
#if INCLUDE_MEMCMDS
#define INCLUDE_PM 1
#define INCLUDE_DM 1
#define INCLUDE_FM 1
#define INCLUDE_CM 1
#define INCLUDE_SM 1
#define INCLUDE_MT 1
#endif

#if INCLUDE_PM

/* Pm():
 *  Put to memory.
 *
 *  Arguments...
 *  arg1:       address.
 *  arg2-argN:  data to put beginning at specified address (max of 8).
 *
 *  Options...
 *  -2  access as a short.
 *  -4  access as a long.
 *  -f  fifo access (address does not increment).
 *  -s  data is ascii string.
 *  -S  data is ascii string and should be concatenated with the
 *      string that starts at the specified address.
 */

char *PmHelp[] = {
    "Put to Memory",
    "-[24aefosSx] {addr} {val|string} [val] ...",
#if INCLUDE_VERBOSEHELP
    "Options:",
    " -2   short access",
    " -4   long access",
    " -a   AND operation",
    " -e   endian swap",
    " -f   fifo mode",
    " -o   OR operation",
    " -s   strcpy",
    " -S   strcat",
    " -x   XOR operation",
#endif
    0,
};

#define PM_EQ_OPERATION     1
#define PM_AND_OPERATION    2
#define PM_OR_OPERATION     3
#define PM_XOR_OPERATION    4

int
Pm(int argc,char *argv[])
{
    ulong   val4, add, base;
    ushort  val2;
    uchar   val1, c;
    int opt, width, ascii, fifo, i, j, concatenate, endian_swap, pmop;

    pmop = PM_EQ_OPERATION;
    width = 1;
    ascii = fifo = 0;
    concatenate = 0;
    endian_swap = 0;
    while((opt=getopt(argc,argv,"24aefosSx")) != -1) {
        switch(opt) {
        case '2':
            width = 2;
            break;
        case '4':
            width = 4;
            break;
        case 'a':
            pmop = PM_AND_OPERATION;
            break;
        case 'e':
            endian_swap = 1;
            break;
        case 'f':
            fifo = 1;
            break;
        case 'o':
            pmop = PM_OR_OPERATION;
            break;
        case 's':
            ascii = 1;
            break;
        case 'S':
            ascii = 1;
            concatenate = 1;
            break;
        case 'x':
            pmop = PM_XOR_OPERATION;
            concatenate = 1;
            break;
        default:
            return(CMD_PARAM_ERROR);
        }
    }

    if(argc < (optind+2)) {
        return(CMD_PARAM_ERROR);
    }

    add = strtoul(argv[optind],(char **)0,0);

    if(ascii) {
        base = add;
        if(concatenate) {           /* If concatenate, skip to */
            while(*(uchar *)add) {  /* end of string, then start. */
                add++;
            }
        }
        for(i=optind+1; i<argc; i++) {
            j = 0;
            while(argv[i][j]) {
                c = argv[i][j++];
                if(c == '\\') {
                    c = argv[i][j++];
                    if(c == 'n') {
                        *(char *)add = '\n';
                    } else if(c == 'r') {
                        *(char *)add = '\r';
                    } else if(c == 't') {
                        *(char *)add = '\t';
                    } else {
                        *(char *)add = '\\';
                        if(!fifo) {
                            add++;
                        }
                        *(char *)add = c;
                    }
                } else {
                    *(char *)add = c;
                }
                if(!fifo) {
                    add++;
                }
            }
        }
        *(uchar *)add = 0;
        shell_sprintf("STRLEN","%d",add-base);
        return(CMD_SUCCESS);
    }
    for(i=optind+1; i<argc; i++) {
        switch(width) {
        case 1:
            val1 = (uchar)strtoul(argv[i],(char **)0,0);
            switch(pmop) {
            case PM_EQ_OPERATION:
                *(uchar *)add = val1;
                break;
            case PM_AND_OPERATION:
                *(uchar *)add &= val1;
                break;
            case PM_OR_OPERATION:
                *(uchar *)add |= val1;
                break;
            case PM_XOR_OPERATION:
                *(uchar *)add ^= val1;
                break;
            }
            if(!fifo) {
                add++;
            }
            break;
        case 2:
            val2 = (ushort)strtoul(argv[i],(char **)0,0);
            switch(pmop) {
            case PM_EQ_OPERATION:
                *(ushort *)add = endian_swap ? swap2(val2) : val2;
                break;
            case PM_AND_OPERATION:
                *(ushort *)add &= endian_swap ? swap2(val2) : val2;
                break;
            case PM_OR_OPERATION:
                *(ushort *)add |= endian_swap ? swap2(val2) : val2;
                break;
            case PM_XOR_OPERATION:
                *(ushort *)add ^= endian_swap ? swap2(val2) : val2;
                break;
            }
            if(!fifo) {
                add += 2;
            }
            break;
        case 4:
            val4 = (ulong)strtoul(argv[i],(char **)0,0);
            switch(pmop) {
            case PM_EQ_OPERATION:
                *(ulong *)add = endian_swap ? swap4(val4) : val4;
                break;
            case PM_AND_OPERATION:
                *(ulong *)add &= endian_swap ? swap4(val4) : val4;
                break;
            case PM_OR_OPERATION:
                *(ulong *)add |= endian_swap ? swap4(val4) : val4;
                break;
            case PM_XOR_OPERATION:
                *(ulong *)add ^= endian_swap ? swap4(val4) : val4;
                break;
            }
            if(!fifo) {
                add += 4;
            }
            break;
        }
    }
    return(CMD_SUCCESS);
}
#endif

#if INCLUDE_DM
/* Dm():
 *  Display memory.
 *
 *  Arguments...
 *  arg1: address to start display
 *  arg2: if present, specifies the number of units to be displayed.
 *
 *  Options...
 *  -2  a unit is a short.
 *  -4  a unit is a long.
 *  -b  print chars out as is (binary).
 *  -d  display in decimal.
 *  -e  endian-swap for short/long display.
 *  -f  fifo-type access (address does not increment).
 *  -l# size of line to be printed.
 *  -m  prompt user for more.
 *  -s  print chars out as is (binary) and terminates at a null.
 *  -v {varname} assign last value displayed to shell var varname.
 *
 *  Defaults...
 *  Display in hex, and unit type is byte.
 */

char *DmHelp[] = {
    "Display Memory",
    "-[24bdefl:msv:] {addr} [byte-cnt]",
#if INCLUDE_VERBOSEHELP
    "Options:",
    " -2   short access",
    " -4   long access",
    " -b   binary",
    " -d   decimal",
    " -e   endian swap",
    " -f   fifo mode",
    " -l#  size of line (in bytes)",
    " -m   use 'more'",
    " -s   string",
    " -v {var} load 'var' with element",
#endif
    0,
};

#define BD_NULL         0
#define BD_RAWBINARY    1
#define BD_ASCIISTRING  2

int
Dm(int argc,char *argv[])
{
    ushort  *sp;
    uchar   *cp, cbuf[128];
    ulong   *lp, add, count_rqst;
    char    *varname, *prfmt, *vprfmt;
    int     i, count, width, opt, more, size, fifo;
    int     hex_display, bin_display, endian_swap, linesize, sol;

    linesize = 0;
    width = 1;
    more = fifo = 0;
    bin_display = BD_NULL;
    hex_display = 1;
    endian_swap = 0;
    varname = (char *)0;
    while((opt=getopt(argc,argv,"24bdefl:msv:")) != -1) {
        switch(opt) {
        case '2':
            width = 2;
            break;
        case '4':
            width = 4;
            break;
        case 'b':
            bin_display = BD_RAWBINARY;
            break;
        case 'd':
            hex_display = 0;
            break;
        case 'e':
            endian_swap = 1;
            break;
        case 'f':
            fifo = 1;
            break;
        case 'l':
            linesize = atoi(optarg);
            break;
        case 'm':
            more = 1;
            break;
        case 'v':
            varname = optarg;
            break;
        case 's':
            bin_display = BD_ASCIISTRING;
            break;
        default:
            return(CMD_PARAM_ERROR);
        }
    }

    if(argc < (optind+1)) {
        return(CMD_PARAM_ERROR);
    }

    add = strtoul(argv[optind],(char **)0,0);
    if(hex_display) {
        vprfmt = "0x%x";
    } else {
        vprfmt = "%d";
    }

    if(argc-(optind-1) == 3) {
        count_rqst = strtoul(argv[optind+1],(char **)0,0);
        count_rqst *= width;
    } else {
        count_rqst = 128;
    }


    if(bin_display != BD_NULL) {
        cp = (uchar *)add;
        if(bin_display == BD_ASCIISTRING) {
            putstr((char *)cp);
            if(varname) {
                shell_sprintf(varname,vprfmt,cp+strlen((char *)cp)+1);
            }
        } else {
            for(i=0; i<count_rqst; i++) {
                putchar(*cp++);
            }
        }
        putchar('\n');
        return(CMD_SUCCESS);
    }

    sol = linesize == 0 ? 16 : linesize;
    do {
        count = count_rqst;

        if(width == 1) {
            cp = (uchar *)add;
            if(hex_display) {
                prfmt = "%02X ";
            } else {
                prfmt = "%3d ";
            }

            if(varname) {
                shell_sprintf(varname,vprfmt,*cp);
            } else {
                while(count > 0) {
                    printf("%08lx: ",(ulong)cp);
                    if(count > sol) {
                        size = sol;
                    } else {
                        size = count;
                    }

                    for(i=0; i<sol; i++) {
                        if(i >= size) {
                            putstr("   ");
                        } else  {
                            cbuf[i] = *cp;
                            printf(prfmt,cbuf[i]);
                        }
                        if((linesize == 0) && (i == 7)) {
                            putstr("  ");
                        }
                        if(!fifo) {
                            cp++;
                        }
                    }
                    if((hex_display) && (!fifo)) {
                        putstr("  ");
                        prascii(cbuf,size);
                    }
                    putchar('\n');
                    count -= size;
                    if(!fifo) {
                        add += size;
                        cp = (uchar *)add;
                    }
                }
            }
        } else if(width == 2) {
            sp = (ushort *)add;
            if(hex_display) {
                prfmt = "%04X ";
            } else {
                prfmt = "%5d ";
            }

            if(varname) {
                shell_sprintf(varname,vprfmt,endian_swap ? swap2(*sp) : *sp);
            } else {
                while(count>0) {
                    printf("%08lx: ",(ulong)sp);
                    if(count > sol) {
                        size = sol;
                    } else {
                        size = count;
                    }

                    for(i=0; i<size; i+=2) {
                        printf(prfmt,endian_swap ? swap2(*sp) : *sp);
                        if(!fifo) {
                            sp++;
                        }
                    }
                    putchar('\n');
                    count -= size;
                    if(!fifo) {
                        add += size;
                        sp = (ushort *)add;
                    }
                }
            }
        } else if(width == 4) {
            lp = (ulong *)add;
            if(hex_display) {
                prfmt = "%08X  ";
            } else {
                prfmt = "%12d  ";
            }

            if(varname) {
                shell_sprintf(varname,vprfmt,endian_swap ? swap4(*lp) : *lp);
            } else {
                while(count>0) {
                    printf("%08lx: ",(ulong)lp);
                    if(count > sol) {
                        size = sol;
                    } else {
                        size = count;
                    }
                    for(i=0; i<size; i+=4) {
                        printf(prfmt,endian_swap ? swap4(*lp) : *lp);
                        if(!fifo) {
                            lp++;
                        }
                    }
                    putchar('\n');
                    count -= size;
                    if(!fifo) {
                        add += size;
                        lp = (ulong *)add;
                    }
                }
            }
        }
    } while(more && More());
    return(CMD_SUCCESS);
}
#endif


#if INCLUDE_FM
/* Fm():
 *  Fill memory with user-specified data.
 *  Syntax:
 *      fm [options] {start} {count | finish} {value}
 */

char *FmHelp[] = {
    "Fill Memory",
    "-[24cinp] {start} {finish|byte-cnt} {value|pattern}",
#if INCLUDE_VERBOSEHELP
    "Options:",
    " -2   short access",
    " -4   long access",
    " -c   arg2 is count (in bytes)",
    " -i   increment {value|pattern}",
    " -n   no verification",
    " -p   arg3 is a pattern",
#endif
    0,
};

int
Fm(int argc,char *argv[])
{
    char    *pp, *pattern;
    ulong   start, finish;
    uchar   *cptr, cdata;
    ushort  *wptr, wdata;
    ulong   *lptr, ldata, error_at;
    int width, opt, arg2iscount, arg3ispattern, err, verify, increment;

    width = 1;
    verify = 1;
    increment = 0;
    error_at = 0;
    arg2iscount = 0;
    arg3ispattern = 0;
    pattern = pp = (char *)0;
    while((opt=getopt(argc,argv,"24cinp")) != -1) {
        switch(opt) {
        case '2':
            width = 2;
            break;
        case '4':
            width = 4;
            break;
        case 'c':
            arg2iscount = 1;
            break;
        case 'i':
            increment = 1;
            break;
        case 'n':
            verify = 0;
            break;
        case 'p':
            arg3ispattern = 1;
            break;
        default:
            return(CMD_PARAM_ERROR);
        }
    }

    /* Three args required...
     * If -p is specfied, then width can only be 1...
     */
    if(argc != optind+3) {
        return(CMD_PARAM_ERROR);
    }

    if((arg3ispattern) && (width != 1)) {
        printf("Note: -p uses byte-wide access (-%d ignored)\n",width);
        width = 1;
    }

    start = strtoul(argv[optind],(char **)0,0);
    finish = strtoul(argv[optind+1],(char **)0,0);
    if(arg3ispattern) {
        pattern = pp = argv[optind+2];
        ldata = 0;
    } else {
        ldata = strtoul(argv[optind+2],0,0);
    }

    if(arg2iscount) {
        finish = start + finish;
    } else if(start >= finish) {
        printf("start > finish\n");
        return(CMD_PARAM_ERROR);
    }

    err = 0;
    switch(width) {
    case 1:
        cdata = (uchar) ldata;
        for(cptr=(uchar *)start; cptr<(uchar *)finish; cptr++) {
            if(arg3ispattern) {
                if(*pp == 0) {
                    pp = pattern;
                }
                cdata = (uchar)*pp++;
            }
            *cptr = cdata;
            if(verify) {
                if(*cptr != cdata) {
                    err = 1;
                    error_at = (ulong)cptr;
                    break;
                }
            }
            cdata += increment;
        }
        break;
    case 2:
        wdata = (ushort) ldata;
        for(wptr=(ushort *)start; wptr<(ushort *)finish; wptr++) {
            *wptr = wdata;
            if(verify) {
                if(*wptr != wdata) {
                    err = 1;
                    error_at = (ulong)wptr;
                    break;
                }
            }
            wdata += increment;
        }
        break;
    case 4:
        for(lptr=(ulong *)start; lptr<(ulong *)finish; lptr++) {
            *lptr = ldata;
            if(verify) {
                if(*lptr != ldata) {
                    err = 1;
                    error_at = (ulong)lptr;
                    break;
                }
            }
            ldata += increment;
        }
        break;
    }
    if(err) {
        printf("Error at 0x%lx\n",error_at);
        return(CMD_FAILURE);
    }
    return(CMD_SUCCESS);
}
#endif

#if INCLUDE_SM

/* Sm():
 *  Search memory.
 */

char *SmHelp[] = {
    "Search Memory",
    "-[24cnqsx] {start} {finish|byte-cnt} {srchfor}",
#if INCLUDE_VERBOSEHELP
    "Options:",
    " -2   short access",
    " -4   long access",
    " -c   arg2 is count (in bytes)",
    " -n   srchfor_not (NA for -s or -x)",
    " -q   quit after first search hit",
    " -s   string srch",
    " -x   hexblock srch",
#endif
    0,
};

#define SM_NORMAL       1       /* Search for 1 value */
#define SM_STRING       2       /* Search for ascii string */
#define SM_HEXBLOCK     3       /* Search for block of hex data */

int
Sm(int argc,char *argv[])
{
    ulong   start, finish;
    int     width, opt, i, j, mode, arg2iscount, not, quit;
    char    *srchfor, tmp;
    uchar   *cptr, cdata, data[32];
    ushort  *wptr, wdata;
    ulong   *lptr, ldata;

    not = 0;
    quit = 0;
    width = 1;
    arg2iscount = 0;
    mode = SM_NORMAL;
    while((opt=getopt(argc,argv,"24cnqsx")) != -1) {
        switch(opt) {
        case '2':
            width = 2;
            break;
        case '4':
            width = 4;
            break;
        case 'c':
            arg2iscount = 1;
            break;
        case 'n':
            not = 1;                /* opposite logic SM_NORMAL only. */
            break;
        case 'q':
            quit = 1;               /* quit after first [no]match */
            break;
        case 's':
            mode = SM_STRING;       /* ascii string */
            break;
        case 'x':
            mode = SM_HEXBLOCK;     /* hex data */
            break;
        default:
            return(CMD_PARAM_ERROR);
        }
    }

    if(argc != optind+3) {
        return(CMD_PARAM_ERROR);
    }

    start = strtoul(argv[optind],(char **)0,0);
    finish = strtoul(argv[optind+1],(char **)0,0);
    if(arg2iscount) {
        finish = start + finish;
    }
    srchfor = argv[optind+2];

    if(mode == SM_HEXBLOCK) {
        char    *ex;

        if(not) {
            return(CMD_PARAM_ERROR);
        }
        ex = strpbrk(srchfor,"xX");
        if(ex) {
            srchfor=ex+1;
        }
        if(strlen(srchfor) % 2) {
            return(CMD_PARAM_ERROR);
        }
        for(i=0,j=0; i<(sizeof data); i++,j+=2) {
            if(srchfor[j] == 0) {
                break;
            }
            tmp = srchfor[j+2];
            srchfor[j+2] = 0;
            data[i] = (uchar)strtoul(&srchfor[j],0,16);
            srchfor[j+2] = tmp;
        }
        for(cptr=(uchar *)start; cptr<(uchar *)finish; cptr++) {
            if(memcmp((char *)cptr,(char *)data,i) == 0) {
                printf("Match @ 0x%lx\n",(ulong)cptr);
                if(quit || gotachar()) {
                    break;
                }
            }
        }
        return(CMD_SUCCESS);
    } else if(mode == SM_STRING) {
        int len;

        if(not) {
            return(CMD_PARAM_ERROR);
        }
        len = strlen(srchfor);
        for(cptr=(uchar *)start; cptr<(uchar *)finish; cptr++) {
            if(strncmp((char *)cptr,srchfor,len) == 0) {
                printf("Match @ 0x%lx\n",(ulong)cptr);
                if(quit || gotachar()) {
                    break;
                }
            }
        }
    } else if(width == 1) {
        cdata = (uchar)strtoul(srchfor,(char **)0,0);
        for(cptr=(uchar *)start; cptr<(uchar *)finish; cptr++) {
            if(not) {
                if(*cptr != cdata) {
                    printf("Nomatch @ 0x%lx (0x%x)\n",(ulong)cptr,*cptr);
                    if(quit || gotachar()) {
                        break;
                    }
                }
            } else if(*cptr == cdata) {
                printf("Match @ 0x%lx\n",(ulong)cptr);
                if(quit || gotachar()) {
                    break;
                }
            }
        }
    } else if(width == 2) {
        wdata = (ushort)strtoul(srchfor,(char **)0,0);
        for(wptr=(ushort *)start; wptr<(ushort *)finish; wptr++) {
            if(not) {
                if(*wptr != wdata) {
                    printf("Nomatch @ 0x%lx (0x%x)\n",(ulong)wptr,*wptr);
                    if(quit || gotachar()) {
                        break;
                    }
                }
            } else if(*wptr == wdata) {
                printf("Match @ 0x%lx\n",(ulong)wptr);
                if(quit || gotachar()) {
                    break;
                }
            }
        }
    } else {
        ldata = (ulong)strtoul(srchfor,(char **)0,0);
        for(lptr=(ulong *)start; lptr<(ulong *)finish; lptr++) {
            if(not) {
                if(*lptr != ldata) {
                    printf("Nomatch @ 0x%lx (0x%lx)\n",(ulong)lptr,*lptr);
                    if(quit || gotachar()) {
                        break;
                    }
                }
            } else if(*lptr == ldata) {
                printf("Match @ 0x%lx\n",(ulong)lptr);
                if(quit || gotachar()) {
                    break;
                }
            }
        }
    }
    return(CMD_SUCCESS);
}
#endif

#if INCLUDE_CM

/* Cm():
 *  Copy memory.
 *
 *  Arguments...
 *  arg1:       source address
 *  arg2:       destination address
 *  arg3:       byte count
 *
 *  Options...
 *  -2  access as a short.
 *  -4  access as a long.
 *  -f  fifo access (address does not increment).
 *  -v  verify (only) that the range specified, is copied.
 */

char *CmHelp[] = {
    "Copy Memory",
    "-[24fv] {src} {dst} {byte-cnt}",
#if INCLUDE_VERBOSEHELP
    "Options:",
    " -2   short access",
    " -4   long access",
    " -f   fifo mode",
    " -v   verify only",
#endif
    0,
};

int
Cm(int argc,char *argv[])
{
    ulong   src, dest, end, bytecount;
    int opt, width, fifo, verify, verify_failed;

    width = 1;
    fifo = verify = 0;
    verify_failed = 0;
    while((opt=getopt(argc,argv,"24fv")) != -1) {
        switch(opt) {
        case '2':
            width = 2;
            break;
        case '4':
            width = 4;
            break;
        case 'f':
            fifo = 1;
            break;
        case 'v':
            verify = 1;
            break;
        default:
            return(CMD_PARAM_ERROR);
        }
    }

    if(argc != optind+3) {
        return(CMD_PARAM_ERROR);
    }
    if((verify) && (fifo)) {
        printf("Can't verify in fifo mode\n");
        return(CMD_FAILURE);
    }

    src = strtoul(argv[optind],(char **)0,0);
    dest = strtoul(argv[optind+1],(char **)0,0);
    bytecount = strtoul(argv[optind+2],(char **)0,0);

    if(bytecount <= 0) {
        printf("Invalid byte count\n");
        return(CMD_FAILURE);
    }

    end = src+bytecount-1;
    if(src > end) {
        printf("Invalid range (possibly overlapping address 0?)\n");
        return(CMD_FAILURE);
    }

    if(width == 1) {
        while(src <= end) {
            if(verify) {
                if(*(uchar *)dest != *(uchar *)src) {
                    verify_failed = 1;
                    break;
                }
            } else {
                *(uchar *)dest = *(uchar *)src;
            }
            if(!fifo) {
                dest++;
            }
            src++;
        }
    } else if(width == 2) {
        while(src <= end) {
            if(verify) {
                if(*(ushort *)dest != *(ushort *)src) {
                    verify_failed = 1;
                    break;
                }
            } else {
                *(ushort *)dest = *(ushort *)src;
            }
            if(!fifo) {
                dest += 2;
            }
            src += 2;
        }
    } else { /* width == 4 */
        while(src <= end) {
            if(verify) {
                if(*(ulong *)dest != *(ulong *)src) {
                    verify_failed = 1;
                    break;
                }
            } else {
                *(ulong *)dest = *(ulong *)src;
            }
            if(!fifo) {
                dest += 4;
            }
            src += 4;
        }
    }
    if((verify) && (verify_failed)) {
        printf("Verify failed: *0x%lx != *0x%lx\n",src,dest);
        return(CMD_FAILURE);
    }
    return(CMD_SUCCESS);
}

#endif

#if INCLUDE_MT

/* Mt():
 *  Memory test
 *  Walking ones and address-in-address tests for data and address bus
 *  testing respectively.  This test, within the context of a monitor
 *  command, has limited value.  It must assume that the memory space
 *  from which this code is executing is sane (obviously, or the code
 *  would not be executing) and the ram used for stack and bss must
 *  already be somewhat useable.
 */
char *MtHelp[] = {
    "Memory test",
    "-[CcqSs:t:v] {addr} {len}",
#if INCLUDE_VERBOSEHELP
    "Options:",
    " -c    continuous",
    " -C    crc32 calculation",
    " -q    quit on error",
    " -S    determine size of on-board memory (see note below)",
    " -s##  sleep ## seconds between adr-in-addr write and readback",
    " -t##  toggle data on '##'-bit boundary (##: 32 or 64)",
    " -v    cumulative verbosity...",
    "       -v=<ticker>, -vv=<ticker + msg-per-error>",
    "",
    "Memory test is walking ones followed by address-in-address",
    "",
    "Note1: For normal memory test, hit any key to abort test with errors",
    "Note2: With -S option, {addr} is the base of physical memory and",
    "       {len} is the maximum expected size of that memory.  The",
    "       shell variable MEMSIZE is loaded with the result.",
#endif
    0,
};

int
Mt(int argc,char *argv[])
{
    int     errcnt, len, testaborted, opt, runcrc;
    int     quitonerr, verbose, continuous, testtot, sizemem;
    ulong   arg1, arg2, *start, rwsleep, togglemask;
    ulong   *end, walker, readback, shouldbe;
    volatile ulong *addr;

    runcrc = 0;
    sizemem = 0;
    continuous = 0;
    quitonerr = 0;
    verbose = 0;
    rwsleep = 0;
    togglemask = 0;
    while((opt=getopt(argc,argv,"cCqSs:t:v")) != -1) {
        switch(opt) {
        case 'c':
            continuous = 1;
            break;
        case 'C':
            runcrc = 1;
            break;
        case 'q':
            quitonerr = 1;
            break;
        case 'S':
            sizemem = 1;
            break;
        case 's':
            rwsleep = strtoul(optarg,0,0);
            break;
        case 't':
            switch(atoi(optarg)) {
            case 32:
                togglemask = 0x00000004;
                break;
            case 64:
                togglemask = 0x00000008;
                break;
            default:
                return(CMD_PARAM_ERROR);
            }
            break;
        case 'v':
            verbose++;      /* Cumulative verbosity */
            break;
        default:
            return(CMD_PARAM_ERROR);
        }
    }

    if(argc != optind+2) {
        return(CMD_PARAM_ERROR);
    }

    arg1 = strtoul(argv[optind],(char **)0,0);
    arg2 = strtoul(argv[optind+1],(char **)0,0);

    /* If -S option, then run a memory size test.
     * For this case, arg1 is the base and arg2 is the maximum expected
     * size of the on-board memory...
     */
    if(sizemem) {
        vulong tmp1, base, test;

        base = arg1;
        test = arg2;

        tmp1 = *(vulong *)base;
        *(vulong *)base = 0xDEADBEEF;

        /* Got this algorithm from Bill Gatliff...
         * Assume that memory always starts at address 0, for
         * simplicity in the explanation.  What you do is write 256
         * at 256MB, 128 at 128MB, 64@64MB, and so on.  When you get
         * down to zero (or the smallest available memory address, if
         * you want to stop sooner), you read the value stored at 0.
         * That value is the size of available memory.
         */
        while(test >= 0x100000) {
            *(vulong *)(base + test) = test/0x100000;
            if(verbose)
                printf("*0x%lx = 0x%lx (0x%lx)\n",(base+test),test/0x100000,
                       *(vulong *)(base + test));
            test >>= 1;
        }

        /* If the base location was not changed by the above algorithm
         * then the address bus doesn't alias, so try something else...
         * This time, walk up the address space starting at 0x100000
         * and doubling each time until the write to the address (minus 4)
         * fails.  The last successful write indicates the top of memory.
         */
        if(*(vulong *)base == 0xDEADBEEF) {
            test = 0x100000;
            while(test <= arg2) {
                vulong *lp = (vulong *)(base+test-sizeof(long));

                if(verbose) {
                    printf("write to 0x%lx, ",(long)lp);
                }
                *lp = 0xdeadbeef;
                monDelay(100);
                if(*lp != 0xdeadbeef) {
                    if(verbose) {
                        printf("failed\n");
                    }
                    break;
                }
                if(verbose) {
                    printf("passed\n");
                }
                test <<= 1;
            }
            test >>= 1;
            printf("Size: %ld MB\n",test/0x100000);
            shell_sprintf("MEMSIZE","0x%lx",test);
        } else {
            printf("Size: %ld MB\n",*(vulong *)base);
            shell_sprintf("MEMSIZE","0x%lx",*(vulong *)base * 0x100000);
        }

        *(vulong *)base = tmp1;
        return(CMD_SUCCESS);
    }

    /* If -C option, then run a CRC32 on a specified block of memory...
     */
    if(runcrc) {
        ulong mtcrc;

        mtcrc = crc32((uchar *)arg1,arg2);
        printf("CRC32 @ 0x%lx (0x%lx bytes) = 0x%lx\n",arg1,arg2,mtcrc);
        shell_sprintf("MTCRC","0x%lx",mtcrc);
        return(CMD_SUCCESS);
    }

    arg1 &= ~0x3;   /* Word align */
    arg2 &= ~0x3;

    testtot = 0;
    printf("Testing 0x%lx .. 0x%lx\n",arg1,arg1+arg2);

    start = (ulong *)arg1;
    len = (int)arg2;
    testaborted = errcnt = 0;

    while(1) {
        /* Walking Ones test:
         * This test is done to verify that no data bits are shorted.
         * Write 32 locations, each with a different bit set, then
         * read back those 32 locations and make sure that bit (and only
         * that bit) is set.
         */
        walker = 1;
        end = start + 32;
        for(addr=start; addr<end; addr++) {
            *addr = walker;
            walker <<= 1;
        }

        walker = 1;
        for(addr=start; addr<end; addr++) {
            readback = *addr;
            if(readback != walker) {
                errcnt++;
                if(verbose > 1)
                    printf("WalkingOneErr @ x%lx: read x%lx expected x%lx\n",
                           (ulong)addr,readback,walker);
                if(quitonerr) {
                    break;
                }
            }
            walker <<= 1;
        }

        /* Address-in-address test:
         * This test serves three purposes...
         * 1. the "address-in-address" tests for stuck address bits.
         * 2. the "not-address-in-address" (-t option) exercises the
         *    data bus.
         * 3. the sleep between read and write tests for valid memory
         *    refresh in SDRAM based systems.
         *
         * The togglemask is used to determine at what rate the data
         * should be flipped... every 32 bits or every 64 bits.
         */
        if((!testaborted) && ((!errcnt) || (errcnt && !quitonerr))) {

            /* In each 32-bit address, store either the address or the
             * complimented address...
             */
            end = (ulong *)((int)start + len);
            for(addr=start; addr<end; addr++) {
                if(((ulong)addr & 0x3ffff) == 0) {
                    if(gotachar()) {
                        testaborted = 1;
                        break;
                    }
                    if(verbose) {
                        ticktock();
                    }
                }

                if((ulong)addr & togglemask) {
                    *addr = ~(ulong)addr;
                } else {
                    *addr = (ulong)addr;
                }
            }

            /* If -s is specified, then delay for the specified number
             * of seconds.  This option just allows the ram to "sit"
             * for a a while; hence, verifying automatic refresh in
             * the case of SDRAM.
             */
            if(rwsleep && !testaborted) {
                int i;
                for(i=0; i<rwsleep; i++) {
                    monDelay(1000);
                    if(gotachar()) {
                        testaborted = 1;
                        break;
                    }
                    if(verbose) {
                        ticktock();
                    }
                }
            }

            if(testaborted) {
                break;
            }

            /* For each 32-bit address, verify either the address or the
             * complimented address...
             */
            for(addr=start; addr<end; addr++) {
                if(((ulong)addr & 0x3ffff) == 0) {
                    if(gotachar()) {
                        testaborted = 1;
                        break;
                    }
                    if(verbose) {
                        ticktock();
                    }
                }

                readback = *addr;
                if((ulong)addr & togglemask) {
                    shouldbe = ~(ulong)addr;
                } else {
                    shouldbe = (ulong)addr;
                }
                if(readback != shouldbe) {
                    errcnt++;
                    if(verbose > 1)
                        printf("AdrInAdrErr @ x%lx: read x%lx expected x%lx\n",
                               (ulong)addr,readback,shouldbe);
                    if(quitonerr) {
                        testaborted = 1;
                        break;
                    }
                }
            }
        }
        testtot++;
        if(!continuous || testaborted || (errcnt && quitonerr)) {
            break;
        }
    }


    printf("Found %d errors", errcnt);
    if(continuous && testaborted) {
        printf(" after %d test loops",testtot);
    }
    printf(".\n");

    if(errcnt) {
        return(CMD_FAILURE);
    } else {
        return(CMD_SUCCESS);
    }
}
#endif
