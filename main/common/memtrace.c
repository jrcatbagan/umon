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
 * memtrace.c:
 *
 * This file contains CLI and API code to support a simple memory trace
 * capability that allows application developers to call mon_memtrace()
 * with a "printf-like" formatted arglist and the formatted string is
 * put into a circular buffer in some allocated RAM space.
 * The circular buffer is established by "mtrace cfg" command, then all
 * subsequent calls to mon_memtrace() will have the formatted string
 * destined for that buffer.
 *
 * To keep the output formatted clean, the user of mon_memtrace() should
 * not include any line-feeds in the string.  Each time mon_memtrace() is
 * called, a line feed and sequence number is prepended to the string.
 * This allows the dump to simply run through the buffer using putchar().
 *
 * Both the mon_memtrace() API function and the dump facility in the CLI
 * will deal with buffer wrapping.
 *
 * Original author:     Ed Sutter (ed.sutter@alcatel-lucent.com)
 *
 */

#include "config.h"
#include <stdarg.h>
#include "stddefs.h"
#include "genlib.h"
#include "cli.h"

#if INCLUDE_MEMTRACE

#define MINBUFSIZE 512
#define MAXLINSIZE MINBUFSIZE/2

#define MODE_PRINT      (1<<0)      /* mtrace text is output to console */
#define MODE_NOWRAP     (1<<1)      /* when mtrace buffer fills, stop */

/* struct mtInfo:
 *  This structure is at the base of the memory space allocated for
 *  the print buffer.  The control structure is part of the print buffer
 *  because the print buffer is assumed to be outside of the bss area
 *  of the monitor; hence, if a reset occurs and the monitor clears out
 *  its bss space, this structure will still be accessible and contain
 *  the data prior to the reset.
 *  So, to initialize a memory trace buffer use...
 *       mtrace cfg BASE SIZE
 *  and to re-establish the trace after a reset, use...
 *       mtrace mip BASE
 */
struct mtInfo {
    char *base;     /* Base of ram space allocated for print buffer. */
    char *ptr;      /* Running pointer into circular print buffer. */
    char *end;      /* End of ram space allocated. */
    int size;       /* Size of space originally allocated for mtrace. */
    int off;        /* Set if tracing is disabled. */
    int sno;        /* Sequence number of Mtrace() call. */
    int wrap;       /* Wrap counter. */
    int mode;       /* See MODE_XXX bits above. */
    int reentered;  /* Reentry counter. */
};

static struct mtInfo *Mip;

/* Mtrace():
 * Memory trace... This function can be used to place some trace statements
 * (readable text) in some memory location specified by the
 * setting of mtracebuf.  This was originally written for debugging Xmodem
 * because you can't use printf() since the protocol is using the serial
 * port.  I have since pulled it out of the Xmodem.c file and placed it in
 * generally accessible space so that it can be made available to the
 * application code and other monitor code.
 */

int
Mtrace(char *fmt,...)
{
    static  int inMtraceNow;
    int len;
    char *eolp;
    va_list argp;

    /* Mtrace not configured or disabled, so just return.
     */
    if(!Mip || Mip->off) {
        return(0);
    }

    /* This may be called from interrupt and/or non-interrupt space of
     * an application, so we must deal with possible reentrancy here.
     */
    if(inMtraceNow) {
        Mip->reentered++;
        return(0);
    }

    inMtraceNow = 1;

    Mip->ptr += snprintf(Mip->ptr,MAXLINSIZE,"\n<%04d> ",Mip->sno++);

    va_start(argp,fmt);
    len = vsnprintf(Mip->ptr,MAXLINSIZE,fmt,argp);
    va_end(argp);

    /* Strip all CR/LFs from the incoming string.
     * The incoming string can have CR/LFs in it; however, they are stripped
     * so that the format of the dump is stable (one line per Mtrace call).
     * Notice that the top line of this function inserts a newline ahead
     * of the sequence number; hence, additional CR/LFs in the text would
     * just confuse the output.
     */
    eolp = Mip->ptr;
    while(*eolp) {
        if((*eolp == '\r') || (*eolp == '\n')) {
            strcpy(eolp,eolp+1);
            len--;
        } else {
            eolp++;
        }
    }

    /* If print flag is set, then dump to the console...
     */
    if(Mip->mode & MODE_PRINT) {
        int i;
        for(i=0; i<len; i++) {
            putchar(*Mip->ptr++);
        }
        putchar('\n');
    } else {
        Mip->ptr += len;
    }

    if(Mip->ptr >= Mip->end) {
        Mip->ptr = Mip->base;
        if(Mip->mode & MODE_NOWRAP) {
            Mip->off = 1;
        } else {
            Mip->wrap++;
        }
    }

    /* Flush the d-cache of the mtrace buffer and Mip structure after each
     * transfer...
     * This is important because if this is being accessed from an
     * application that has d-cache enabled, then the hardware is reset,
     * there is a chance that the data written was in cache and would be
     * lost.
     */
    flushDcache((char *)Mip,sizeof(struct mtInfo));
    flushDcache((char *)Mip->base,Mip->end - Mip->base);

    inMtraceNow = 0;
    return(len);
}

void
MtraceReset(void)
{
    Mip->ptr = Mip->base;
    Mip->sno = 1;
    Mip->wrap = 0;
    Mip->off = 0;
    Mip->mode = 0;
    Mip->reentered = 0;
    memset(Mip->base,0,Mip->size-sizeof(struct mtInfo));
}

void
MtraceInit(char *base, int size)
{
    if(size < MINBUFSIZE) {
        return;
    }

    Mip = (struct mtInfo *)base;
    Mip->base = base + sizeof(struct mtInfo);
    Mip->size = size;
    Mip->end = (Mip->base + size - MAXLINSIZE);
    MtraceReset();
}

char *
mDump(char *bp, int more)
{
    int line;

    line = 0;

    while((bp < Mip->end) && (*bp)) {
        putchar(*bp);
        if(more && (*bp == '\n')) {
            if(++line == 24) {
                line = 0;
                if(!More()) {
                    return(0);
                }
            }
        }
        bp++;
    }
    while(*bp) {
        putchar(*bp);
        if(more && (*bp == '\n')) {
            if(++line == 24) {
                line = 0;
                if(!More()) {
                    return(0);
                }
            }
        }
        bp++;
    }
    return(bp);
}

/* If Mip pointer is configured, return 1; else return zero
 * and print an error message.
 */
static int
MipConfigured(void)
{
    if(Mip) {
        return(1);
    }
    printf("Not configured\n");
    return(0);
}

char *MtraceHelp[] = {
    "Configure/Dump memory trace.",
    "-[nm] {cmd} [cmd specific args]",
#if INCLUDE_VERBOSEHELP
    "Options:",
    " -m  enable 'more' flag for dump.",
    " -n  disable wrapping.",
    "Cmd:",
    " on",
    " off",
    " dump",
    " pron",
    " reset",
    " log {msg}",
    " mip {base}",
    " cfg [{base} {size}]",
#endif
    0
};

int
MtraceCmd(int argc,char *argv[])
{
    char    *bp;
    int     more, opt, nowrap;

    more = 0;
    nowrap = 0;
    while((opt=getopt(argc,argv,"nm")) != -1) {
        switch(opt) {
        case 'n':
            nowrap = 1;
            break;
        case 'm':
            more = 1;
            break;
        default:
            return(CMD_PARAM_ERROR);
        }
    }

    if(argc <= optind) {
        return(CMD_PARAM_ERROR);
    }

    if(!strcmp(argv[optind],"cfg")) {
        if(argc == optind + 3) {
            MtraceInit((char *)strtoul(argv[optind+1],0,0),
                       strtoul(argv[optind+2],0,0));
            if(nowrap) {
                Mip->mode |= MODE_NOWRAP;
            }
        } else if(argc == optind + 1) {
            if(MipConfigured()) {
                printf("Base: 0x%lx, End: 0x%lx\n",
                       (ulong)Mip->base,(ulong)Mip->end);
                printf("Ptr:  0x%lx, Sno: %d\n",(ulong)Mip->ptr,Mip->sno);
                printf("Wrap: %d\n",Mip->wrap);
            }
        } else {
            return(CMD_PARAM_ERROR);
        }
    } else if(!strcmp(argv[optind],"on")) {
        if(MipConfigured()) {
            Mip->mode &= ~MODE_PRINT;
            Mip->off = 0;
        }
    } else if(!strcmp(argv[optind],"pron")) {
        if(MipConfigured()) {
            Mip->mode |= MODE_PRINT;
            Mip->off = 0;
        }
    } else if(!strcmp(argv[optind],"off")) {
        if(MipConfigured()) {
            Mip->off = 1;
        }
    } else if(!strcmp(argv[optind],"reset")) {
        if(MipConfigured()) {
            MtraceReset();
        }
    } else if(!strcmp(argv[optind],"mip")) {
        if(argc != optind + 2) {
            return(CMD_PARAM_ERROR);
        }
        Mip = (struct mtInfo *)strtoul(argv[optind+1],0,0);
    } else if(!strcmp(argv[optind],"log")) {
        if(argc != optind + 2) {
            return(CMD_PARAM_ERROR);
        }
        Mtrace(argv[optind+1]);
    } else if(!strcmp(argv[optind],"dump")) {
        if(MipConfigured()) {
            if(Mip->reentered) {
                printf("Reentry count: %d\n",Mip->reentered);
            }
            if(Mip->wrap) {
                printf("Buffer wrapped...\n");
                bp =  Mip->ptr;
                while(bp < Mip->end) {
                    if(*bp == '\n') {
                        bp = mDump(bp,more);
                        break;
                    }
                    bp++;
                }
                if(bp) {
                    bp =  Mip->base;
                    while(bp < Mip->ptr) {
                        putchar(*bp++);
                    }
                }
            } else {
                bp = mDump(Mip->base,more);
            }
            printf("\n\n");
        }
    } else {
        return(CMD_PARAM_ERROR);
    }

    return(CMD_SUCCESS);
}

#else

void
MtraceInit(char *base, int size)
{
    printf("Mtrace() facility not built in.\n");
}

int
Mtrace(char *fmt, ...)
{
    return(0);
}

#endif
