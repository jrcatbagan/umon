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
 * moncom.c:
 *
 * This function is used by the application to interface to code that
 * is part of the monitor.  A pointer to this function exists at some
 * "well-known" address in the monitors space.  This location must be
 * known by the application (hence... "well-known").
 *
 * Original author:     Ed Sutter (ed.sutter@alcatel-lucent.com)
 *
 */
#include "config.h"
#include "monlib.h"
#include "genlib.h"
#include "stddefs.h"
#include "tfs.h"
#include "tfsprivate.h"
#include "monflags.h"
#include "pci.h"
#include "i2c.h"
#include "ether.h"
#include "flash.h"
#include "date.h"
#include "timer.h"

extern int addcommand(struct monCommand *cmdlist, char *cmdlvl);
extern int profiler(void *);
static int moncom_pcnt;

int monErrorStub(void);

int
moncom(int cmd, void *arg1, void *arg2, void *arg3)
{
    int retval;

    /* eliminate warnings due to arg2 & arg3 not being used...
     */
    if(arg2 == arg3) {
        retval = 0;
    } else {
        retval = 0;
    }

    switch(cmd) {
    case GETMONFUNC_PUTCHAR:
        *(unsigned long *)arg1 = (unsigned long)putchar;
        break;
    case GETMONFUNC_GETCHAR:
        *(unsigned long *)arg1 = (unsigned long)getchar;
        break;
    case GETMONFUNC_GOTACHAR:
        *(unsigned long *)arg1 = (unsigned long)gotachar;
        break;
    case GETMONFUNC_GETBYTES:
        *(unsigned long *)arg1 = (unsigned long)getbytes;
        break;
    case GETMONFUNC_PRINTF:
        *(unsigned long *)arg1 = (unsigned long)printf;
        break;
    case GETMONFUNC_CPRINTF:
        *(unsigned long *)arg1 = (unsigned long)cprintf;
        break;
    case GETMONFUNC_SPRINTF:
        *(unsigned long *)arg1 = (unsigned long)sprintf;
        break;
    case GETMONFUNC_RESTART:
        *(unsigned long *)arg1 = (unsigned long)monrestart;
        break;
    case GETMONFUNC_GETENV:
        *(unsigned long *)arg1 = (unsigned long)getenv;
        break;
    case GETMONFUNC_SETENV:
        *(unsigned long *)arg1 = (unsigned long)setenv;
        break;

#if !INCLUDE_TFS
    case GETMONFUNC_TFSINIT:
    case GETMONFUNC_TFSADD:
    case GETMONFUNC_TFSUNLINK:
    case GETMONFUNC_TFSRUN:
    case GETMONFUNC_TFSNEXT:
    case GETMONFUNC_TFSFSTAT:
    case GETMONFUNC_TFSTRUNCATE:
    case GETMONFUNC_TFSEOF:
    case GETMONFUNC_TFSSTAT:
    case GETMONFUNC_TFSREAD:
    case GETMONFUNC_TFSWRITE:
    case GETMONFUNC_TFSOPEN:
    case GETMONFUNC_TFSCLOSE:
    case GETMONFUNC_TFSSEEK:
    case GETMONFUNC_TFSGETLINE:
    case GETMONFUNC_TFSIPMOD:
    case GETMONFUNC_TFSCTRL:
    case GETMONFUNC_TFSLINK:
    case GETMONFUNC_TFSTELL:
#endif
#if !INCLUDE_UNZIP
    case GETMONFUNC_DECOMPRESS:
#endif
#if !INCLUDE_MALLOC
    case GETMONFUNC_MALLOC:
    case GETMONFUNC_FREE:
    case GETMONFUNC_HEAPXTEND:
#endif
#if !INCLUDE_PROFILER
    case GETMONFUNC_PROFILER:
#endif
#if !INCLUDE_BBC
    case GETMONFUNC_BBC:
#endif
#if !INCLUDE_MEMTRACE
    case GETMONFUNC_MEMTRACE:
#endif
#if !INCLUDE_ETHERNET
    case GETMONFUNC_SENDENETPKT:
    case GETMONFUNC_RECVENETPKT:
#endif
#if !INCLUDE_ETHERVERBOSE
    case GETMONFUNC_PRINTPKT:
#endif
#if !INCLUDE_FLASH
    case GETMONFUNC_FLASHWRITE:
    case GETMONFUNC_FLASHERASE:
    case GETMONFUNC_FLASHINFO:
    case GETMONFUNC_FLASHOVRRD:
#endif
#if !INCLUDE_PORTCMD
    case GETMONFUNC_PORTCMD:
#endif
#ifndef ALLOW_HANDLER_ASSIGNMENT
    case GETMONFUNC_ASSIGNHDLR:
#endif
#ifndef NO_MONLIB_PCI_STUBS
    case GETMONFUNC_PCICFGREAD:
    case GETMONFUNC_PCICFGWRITE:
    case GETMONFUNC_PCICONTROL:
#endif
#ifndef NO_MONLIB_I2C_STUBS
    case GETMONFUNC_I2CWRITE:
    case GETMONFUNC_I2CREAD:
    case GETMONFUNC_I2CCONTROL:
#endif
#if !INCLUDE_TIMEOFDAY
    case GETMONFUNC_TIMEOFDAY:
#endif
#if !INCLUDE_SHELLVARS
    case GETMONFUNC_GETARGV:
#endif
    case GETMONFUNC_PIOGET:     /* As of uMon 1.0, these are */
    case GETMONFUNC_PIOSET:     /* no longer supported. */
    case GETMONFUNC_PIOCLR:
        *(unsigned long *)arg1 = (unsigned long)monErrorStub;
        break;

#if INCLUDE_TFS
    case GETMONFUNC_TFSINIT:
        *(unsigned long *)arg1 = (unsigned long)tfsinit;
        break;
    case GETMONFUNC_TFSADD:
        *(unsigned long *)arg1 = (unsigned long)tfsadd;
        break;
    case GETMONFUNC_TFSUNLINK:
        *(unsigned long *)arg1 = (unsigned long)tfsunlink;
        break;
    case GETMONFUNC_TFSRUN:
        *(unsigned long *)arg1 = (unsigned long)tfsrun;
        break;
    case GETMONFUNC_TFSNEXT:
        *(unsigned long *)arg1 = (unsigned long)tfsnext;
        break;
    case GETMONFUNC_TFSFSTAT:
        *(unsigned long *)arg1 = (unsigned long)tfsfstat;
        break;
    case GETMONFUNC_TFSTRUNCATE:
        *(unsigned long *)arg1 = (unsigned long)tfstruncate;
        break;
    case GETMONFUNC_TFSEOF:
        *(unsigned long *)arg1 = (unsigned long)tfseof;
        break;
    case GETMONFUNC_TFSSTAT:
        *(unsigned long *)arg1 = (unsigned long)tfsstat;
        break;
    case GETMONFUNC_TFSREAD:
        *(unsigned long *)arg1 = (unsigned long)tfsread;
        break;
    case GETMONFUNC_TFSWRITE:
        *(unsigned long *)arg1 = (unsigned long)tfswrite;
        break;
    case GETMONFUNC_TFSOPEN:
        *(unsigned long *)arg1 = (unsigned long)tfsopen;
        break;
    case GETMONFUNC_TFSCLOSE:
        *(unsigned long *)arg1 = (unsigned long)tfsclose;
        break;
    case GETMONFUNC_TFSSEEK:
        *(unsigned long *)arg1 = (unsigned long)tfsseek;
        break;
    case GETMONFUNC_TFSGETLINE:
        *(unsigned long *)arg1 = (unsigned long)tfsgetline;
        break;
    case GETMONFUNC_TFSIPMOD:
        *(unsigned long *)arg1 = (unsigned long)tfsipmod;
        break;
    case GETMONFUNC_TFSCTRL:
        *(unsigned long *)arg1 = (unsigned long)tfsctrl;
        break;
    case GETMONFUNC_TFSLINK:
        *(unsigned long *)arg1 = (unsigned long)tfslink;
        break;
    case GETMONFUNC_TFSTELL:
        *(unsigned long *)arg1 = (unsigned long)tfstell;
        break;
#endif
#if INCLUDE_UNZIP
    case GETMONFUNC_DECOMPRESS:
        *(unsigned long *)arg1 = (unsigned long)decompress;
        break;
#endif
#if INCLUDE_MALLOC
    case GETMONFUNC_MALLOC:
        *(unsigned long *)arg1 = (unsigned long)malloc;
        break;
    case GETMONFUNC_FREE:
        *(unsigned long *)arg1 = (unsigned long)free;
        break;
    case GETMONFUNC_HEAPXTEND:
        *(unsigned long *)arg1 = (unsigned long)extendHeap;
        break;
#endif
#if INCLUDE_PROFILER
    case GETMONFUNC_PROFILER:
        *(unsigned long *)arg1 = (unsigned long)profiler;
        break;
#endif
#if INCLUDE_BBC
    case GETMONFUNC_BBC:
        *(unsigned long *)arg1 = (unsigned long)bbc;
        break;
#endif
#if INCLUDE_MEMTRACE
    case GETMONFUNC_MEMTRACE:
        *(unsigned long *)arg1 = (unsigned long)Mtrace;
        break;
#endif
#if INCLUDE_ETHERNET
    case GETMONFUNC_SENDENETPKT:
        *(unsigned long *)arg1 = (unsigned long)monSendEnetPkt;
        break;
    case GETMONFUNC_RECVENETPKT:
        *(unsigned long *)arg1 = (unsigned long)monRecvEnetPkt;
        break;
#endif
#if INCLUDE_ETHERVERBOSE
    case GETMONFUNC_PRINTPKT:
        *(unsigned long *)arg1 = (unsigned long)AppPrintPkt;
        break;
#endif
#if INCLUDE_FLASH
    case GETMONFUNC_FLASHOVRRD:
        *(unsigned long *)arg1 = (unsigned long)FlashOpOverride;
        break;
    case GETMONFUNC_FLASHWRITE:
        *(unsigned long *)arg1 = (unsigned long)AppFlashWrite;
        break;
    case GETMONFUNC_FLASHERASE:
        *(unsigned long *)arg1 = (unsigned long)AppFlashErase;
        break;
    case GETMONFUNC_FLASHINFO:
        *(unsigned long *)arg1 = (unsigned long)sectortoaddr;
        break;
#endif
#if INCLUDE_PORTCMD
    case GETMONFUNC_PORTCMD:
        *(unsigned long *)arg1 = (unsigned long)portCmd;
        break;
#endif
#ifdef  ALLOW_HANDLER_ASSIGNMENT
    case GETMONFUNC_ASSIGNHDLR:
        *(unsigned long *)arg1 = (unsigned long)assign_handler;
        break;
#endif
#ifdef NO_MONLIB_I2C_STUBS
    case GETMONFUNC_I2CWRITE:
        *(unsigned long *)arg1 = (unsigned long)i2cWrite;
        break;
    case GETMONFUNC_I2CREAD:
        *(unsigned long *)arg1 = (unsigned long)i2cRead;
        break;
    case GETMONFUNC_I2CCONTROL:
        *(unsigned long *)arg1 = (unsigned long)i2cCtrl;
        break;
#endif
#ifdef NO_MONLIB_PCI_STUBS
    case GETMONFUNC_PCICFGREAD:
        *(unsigned long *)arg1 = (unsigned long)pciCfgRead;
        break;
    case GETMONFUNC_PCICFGWRITE:
        *(unsigned long *)arg1 = (unsigned long)pciCfgWrite;
        break;
    case GETMONFUNC_PCICONTROL:
        *(unsigned long *)arg1 = (unsigned long)pciCtrl;
        break;
#endif
#if INCLUDE_TIMEOFDAY
    case GETMONFUNC_TIMEOFDAY:
        *(unsigned long *)arg1 = (unsigned long)timeofday;
        break;
#endif
#if INCLUDE_SHELLVARS
    case GETMONFUNC_GETARGV:
        *(unsigned long *)arg1 = (unsigned long)getargv;
        break;
#endif
    case GETMONFUNC_ADDCOMMAND:
        *(unsigned long *)arg1 = (unsigned long)addcommand;
        break;
    case GETMONFUNC_DOCOMMAND:
        *(unsigned long *)arg1 = (unsigned long)docommand;
        break;
    case GETMONFUNC_CRC16:
        *(unsigned long *)arg1 = (unsigned long)xcrc16;
        break;
    case GETMONFUNC_CRC32:
        *(unsigned long *)arg1 = (unsigned long)crc32;
        break;
    case GETMONFUNC_INTSOFF:
        *(unsigned long *)arg1 = (unsigned long)intsoff;
        break;
    case GETMONFUNC_INTSRESTORE:
        *(unsigned long *)arg1 = (unsigned long)intsrestore;
        break;
    case GETMONFUNC_APPEXIT:
        *(unsigned long *)arg1 = (unsigned long)appexit;
        break;
    case GETMONFUNC_GETLINE:
        *(unsigned long *)arg1 = (unsigned long)getline;
        break;
    case GETMONFUNC_VERSION:
        *(unsigned long *)arg1 = (unsigned long)monVersion;
        break;
    case GETMONFUNC_WARMSTART:
        *(unsigned long *)arg1 = (unsigned long)AppWarmStart;
        break;
    case GETMONFUNC_MONDELAY:
        *(unsigned long *)arg1 = (unsigned long)monDelay;
        break;
    case GETMONFUNC_GETENVP:
        *(unsigned long *)arg1 = (unsigned long)getenvp;
        break;
    case GETMONFUNC_REALLOC:
        *(unsigned long *)arg1 = (unsigned long)realloc;
        break;
    case GETMONFUNC_GETSYM:
        *(unsigned long *)arg1 = (unsigned long)getsym;
        break;
    case GETMONFUNC_WATCHDOG:
        *(unsigned long *)arg1 = (unsigned long)monWatchDog;
        break;
    case GETMONFUNC_PRINTMEM:
        *(unsigned long *)arg1 = (unsigned long)printMem;
        break;
    case GETMONFUNC_TIMER:
        *(unsigned long *)arg1 = (unsigned long)monTimer;
        break;
    case CACHEFTYPE_DFLUSH:
        dcacheFlush = (int(*)(char *,int))arg1;
        break;
    case CACHEFTYPE_IINVALIDATE:
        icacheInvalidate = (int(*)(char *,int))arg1;
        break;
    case CHARFUNC_PUTCHAR:
        remoteputchar = (int(*)(int))arg1;
        break;
    case CHARFUNC_GETCHAR:
        remotegetchar = (int(*)(void))arg1;
        break;
    case CHARFUNC_GOTACHAR:
        remotegotachar = (int(*)(void))arg1;
        break;
    case CHARFUNC_RAWMODEON:
        remoterawon = (int(*)(void))arg1;
        break;
    case CHARFUNC_RAWMODEOFF:
        remoterawoff = (int(*)(void))arg1;
        break;
    case ASSIGNFUNC_GETUSERLEVEL:
        extgetUsrLvl = (int(*)(void))arg1;
        break;
#ifdef WATCHDOG_ENABLED
    case ASSIGNFUNC_WATCHDOG:
        remoteWatchDog = (int(*)(void))arg1;
        break;
#endif
    default:
        printf("moncom unknown command: 0x%x\n",cmd);
        retval = -1;
        break;
    }
    moncom_pcnt++;
    return(retval);
}

int
monErrorStub(void)
{
    printf("ERROR: Facility not included in monitor build\n");
    return(-1);
}
