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
 * cmdtbl.c:
 * This is the command table used by the monitor.
 *
 * Original author:     Ed Sutter (ed.sutter@alcatel-lucent.com)
 *
 */
#include "config.h"
#include "tfs.h"
#include "tfsprivate.h"
#include "cli.h"
#include "genlib.h"
#include "xcmddcl.h"

#define ULVLCMD  "ulvl"

#if INCLUDE_MEMCMDS
#define INCLUDE_PM 1
#define INCLUDE_DM 1
#define INCLUDE_FM 1
#define INCLUDE_CM 1
#define INCLUDE_SM 1
#define INCLUDE_MT 1
#endif

extern  int Arp(int, char **);
extern  int BbcCmd(int, char **);
extern  int BinfoCmd(int, char **);
extern  int BmemCmd(int, char **);
extern  int Call(int, char **);
extern  int Cast(int, char **);
extern  int Cm(int, char **);
extern  int CfCmd(int, char **);
extern  int Dis(int, char **);
extern  int Dm(int, char **);
extern  int Dhcp(int, char **);
extern  int DnsCmd(int, char **);
extern  int Echo(int, char **);
extern  int Edit(int, char **);
extern  int Ether(int, char **);
extern  int Exit(int, char **);
extern  int FatfsCmd(int, char **);
extern  int FbiCmd(int, char **);
extern  int FlashCmd(int, char **);
extern  int Fm(int, char **);
extern  int Gdb(int, char **);
extern  int Goto(int, char **);
extern  int Gosub(int, char **);
extern  int Heap(int, char **);
extern  int Help(int, char **);
extern  int History(int, char **);
extern  int Icmp(int, char **);
extern  int Ide(int, char **);
extern  int I2cCmd(int, char **);
extern  int If(int, char **);
extern  int Igmp(int, char **);
extern  int Item(int, char **);
extern  int Jffs2Cmd(int, char **);
extern  int Mt(int, char **);
extern  int MtraceCmd(int, char **);
extern  int Pm(int, char **);
extern  int Prof(int, char **);
extern  int Read(int, char **);
extern  int Reg(int, char **);
extern  int Reset(int, char **);
extern  int Return(int, char **);
extern  int SdCmd(int, char **);
extern  int Set(int, char **);
extern  int Sleep(int, char **);
extern  int Sm(int, char **);
extern  int SpifCmd(int, char **);
extern  int Strace(int, char **);
extern  int StructCmd(int, char **);
extern  int SyslogCmd(int, char **);
extern  int Tfs(int, char **);
extern  int Tftp(int, char **);
extern  int TsiCmd(int, char **);
extern  int Ulvl(int, char **);
extern  int Unzip(int, char **);
extern  int Version(int, char **);
extern  int WhatCmd(int, char **);
extern  int Xmodem(int, char **);

extern  char *ArpHelp[];
extern  char *BbcHelp[];
extern  char *BinfoHelp[];
extern  char *BmemHelp[];
extern  char *CallHelp[];
extern  char *CastHelp[];
extern  char *CfHelp[];
extern  char *CmHelp[];
extern  char *DisHelp[];
extern  char *DhcpHelp[];
extern  char *DmHelp[];
extern  char *DnsHelp[];
extern  char *EchoHelp[];
extern  char *EditHelp[];
extern  char *EtherHelp[];
extern  char *ExitHelp[];
extern  char *FatfsHelp[];
extern  char *FbiHelp[];
extern  char *FlashHelp[];
extern  char *FmHelp[];
extern  char *GdbHelp[];
extern  char *GosubHelp[];
extern  char *GotoHelp[];
extern  char *HelpHelp[];
extern  char *HeapHelp[];
extern  char *HistoryHelp[];
extern  char *IcmpHelp[];
extern  char *IdeHelp[];
extern  char *I2cHelp[];
extern  char *IfHelp[];
extern  char *IgmpHelp[];
extern  char *ItemHelp[];
extern  char *Jffs2Help[];
extern  char *MtHelp[];
extern  char *MtraceHelp[];
extern  char *PmHelp[];
extern  char *ProfHelp[];
extern  char *ReadHelp[];
extern  char *RegHelp[];
extern  char *ResetHelp[];
extern  char *ReturnHelp[];
extern  char *SdHelp[];
extern  char *SetHelp[];
extern  char *SleepHelp[];
extern  char *SmHelp[];
extern  char *SpifHelp[];
extern  char *StraceHelp[];
extern  char *StructHelp[];
extern  char *SyslogHelp[];
extern  char *TfsHelp[];
extern  char *TftpHelp[];
extern  char *TsiHelp[];
extern  char *UlvlHelp[];
extern  char *UnzipHelp[];
extern  char *VersionHelp[];
extern  char *WhatHelp[];
extern  char *XmodemHelp[];

struct monCommand cmdlist[] = {
#if INCLUDE_ETHERNET
    { "arp",        Arp,        ArpHelp,        CMDFLAG_NOMONRC },
#endif
#if INCLUDE_BBC
    { "bbc",        BbcCmd,     BbcHelp,        0 },
#endif
#if INCLUDE_BMEM
    { "bmem",       BmemCmd,    BmemHelp,       0 },
#endif
#if INCLUDE_BOARDINFO
    { "brdinfo",    BinfoCmd,   BinfoHelp,      0 },
#endif
    { "call",       Call,       CallHelp,       CMDFLAG_NOMONRC },
#if INCLUDE_CAST
    { "cast",       Cast,       CastHelp,       0 },
#endif
#if INCLUDE_CF
    { "cf",         CfCmd,      CfHelp,         0 },
#endif
#if INCLUDE_CM
    { "cm",         Cm,         CmHelp,         0 },
#endif
#if INCLUDE_DHCPBOOT
    { "dhcp",       Dhcp,       DhcpHelp,       CMDFLAG_NOMONRC },
#endif
#if INCLUDE_DISASSEMBLER
    { "dis",        Dis,        DisHelp,        0 },
#endif
#if INCLUDE_DM
    { "dm",         Dm,         DmHelp,         0 },
#endif
#if INCLUDE_DNS
    { "dns",        DnsCmd,     DnsHelp,        0 },
#endif
#if INCLUDE_TFSSCRIPT
    { "echo",       Echo,       EchoHelp,       0 },
#endif
#if INCLUDE_EDIT
    { "edit",       Edit,       EditHelp,       0 },
#endif
#if INCLUDE_ETHERNET
    { "ether",      Ether,      EtherHelp,      CMDFLAG_NOMONRC },
#endif
#if INCLUDE_TFSSCRIPT
    { "exit",       Exit,       ExitHelp,       0 },
#endif
#if INCLUDE_FATFS
    { "fatfs",      FatfsCmd,   FatfsHelp,      0 },
#endif
#if INCLUDE_FBI
    { "fbi",        FbiCmd,     FbiHelp,        0 },
#endif
#if INCLUDE_FLASH
    { "flash",      FlashCmd,   FlashHelp,      0 },
#endif
#if INCLUDE_FM
    { "fm",         Fm,         FmHelp,         0 },
#endif

#if INCLUDE_GDB
    { "gdb",        Gdb,        GdbHelp,        CMDFLAG_NOMONRC },
#endif

#if INCLUDE_TFSSCRIPT
    { "gosub",      Gosub,      GosubHelp,      0 },
    { "goto",       Goto,       GotoHelp,       0 },
#endif

#if INCLUDE_MALLOC
    { "heap",       Heap,       HeapHelp,       0 },
#endif

    { "help",       Help,       HelpHelp,       0 },
    { "?",          Help,       HelpHelp,       0 },

#if INCLUDE_LINEEDIT
    { "history",    History,    HistoryHelp,    0 },
#endif

#if INCLUDE_I2C
    { "i2c",        I2cCmd,     I2cHelp,        0 },
#endif

#if INCLUDE_ICMP
    { "icmp",       Icmp,       IcmpHelp,       CMDFLAG_NOMONRC },
#endif

#if INCLUDE_IDE
    { "ide",        Ide,        IdeHelp,        CMDFLAG_NOMONRC },
#endif

#if INCLUDE_IGMP
    { "igmp",       Igmp,       IgmpHelp,       CMDFLAG_NOMONRC },
#endif

#if INCLUDE_TFSSCRIPT
    { "if",         If,         IfHelp,         0 },
#endif

#if INCLUDE_TFSSCRIPT
    { "item",       Item,       ItemHelp,       0 },
#endif

#if INCLUDE_JFFS2
    { "jffs2",      Jffs2Cmd,   Jffs2Help,      0 },
#endif

#if INCLUDE_MT
    { "mt",         Mt,         MtHelp,         0 },
#endif

#if INCLUDE_MEMTRACE
    { "mtrace",     MtraceCmd,  MtraceHelp,     0 },
#endif

#if INCLUDE_PM
    { "pm",         Pm,         PmHelp,         0 },
#endif

#if INCLUDE_PROFILER
    { "prof",       Prof,       ProfHelp,       0 },
#endif

#if INCLUDE_TFSSCRIPT
    { "read",       Read,       ReadHelp,       0 },
#endif

#if INCLUDE_STRACE
    { "reg",        Reg,        RegHelp,        0 },
#endif

    { "reset",      Reset,      ResetHelp,      0 },

#if INCLUDE_TFSSCRIPT
    { "return",     Return,     ReturnHelp,     0 },
#endif
#if INCLUDE_SD
    { "sd",         SdCmd,      SdHelp,         0 },
#endif

    { "set",        Set,        SetHelp,        0 },

#if INCLUDE_TFSSCRIPT
    { "sleep",      Sleep,      SleepHelp,      0 },
#endif

#if INCLUDE_SM
    { "sm",         Sm,         SmHelp,         0 },
#endif

#if INCLUDE_SPIF
    { "spif",       SpifCmd,    SpifHelp,       0 },
#endif

#if INCLUDE_STRACE
    { "strace",     Strace,     StraceHelp,     0 },
#endif

#if INCLUDE_STRUCT
    { "struct",     StructCmd,  StructHelp,     0 },
#endif

#if INCLUDE_SYSLOG
    { "syslog",     SyslogCmd,  SyslogHelp,     CMDFLAG_NOMONRC },
#endif

#if INCLUDE_USRLVL
    { ULVLCMD,      Ulvl,       UlvlHelp,       0 },
#endif

#if INCLUDE_TFTP
    { "tftp",       Tftp,       TftpHelp,       CMDFLAG_NOMONRC },
#endif

#if INCLUDE_TFSCLI
    { "tfs",        Tfs,        TfsHelp,        0 },
#endif

#if INCLUDE_TSI
    { "tsi",        TsiCmd,     TsiHelp,        0 },
#endif

#if INCLUDE_UNZIP
    { "unzip",      Unzip,      UnzipHelp,      0 },
#endif

#if INCLUDE_XMODEM
    { "xmodem",     Xmodem,     XmodemHelp,     0 },
#endif

    { "version",    Version,    VersionHelp,    0 },

#if INCLUDE_TFS
    { "what",       WhatCmd,    WhatHelp,       0 },
#endif

#include "xcmdtbl.h"                /* For non-generic commands that are */
    /* specific to a particular target.  */
    { 0,0,0,0 },
};

#if INCLUDE_USRLVL

/* cmdUlvl[]:
 *  This table stores one char per command that contains that command's
 *  user level.  The default user level of all commands is 0, but can
 *  be re-defined by the ulvl -c command.
 */
char cmdUlvl[(sizeof(cmdlist)/sizeof(struct monCommand))];

/* setCmdUlvl():
 *  The incoming string is a command name followed by a comma and a user
 *  level (ranging from 0 thru 4).
 *  Return 0 if pass, 1 if new level was user-level rejected, -1 if error.
 */
int
setCmdUlvl(char *cmdlvl, int verbose)
{
    extern char *appcmdUlvl;
    extern struct monCommand *appCmdlist;
    struct monCommand *cptr;
    int newlevel, idx, pass, doall;
    char *comma, *lvlptr, buffer[32], *cmdandlevel;

    /* Make a copy of the incoming string so that we can
     * modify it...
     */
    if(strlen(cmdlvl) > (sizeof(buffer)-1)) {
        goto showerr;
    }

    strcpy(buffer,cmdlvl);
    cmdandlevel = buffer;

    /* First verify that the comma is in the string... */
    comma = strchr(cmdandlevel,',');
    if(!comma) {
        goto showerr;
    }

    /* Retrieve and verify the new level to be assigned...
     * If the level value is the string "off", then we assign a level
     * value that is greater than MAXUSRLEVEL so that the command is
     * essentially disabled as a built-in.
     */
    if(strcmp(comma+1,"off") == 0) {
        newlevel = MAXUSRLEVEL+1;
    } else {
        newlevel = atoi(comma+1);
        if((newlevel < MINUSRLEVEL) || (newlevel > MAXUSRLEVEL)) {
            goto showerr;
        }
    }

    *comma = 0;

    /* Don't allow adjustment of the ulvl command itself.  It must be
     * able to run as user level 0 all the time...
     */
    if(!strcmp(cmdandlevel,ULVLCMD)) {
        printf("Can't adjust '%s' user level.\n",ULVLCMD);
        return(-1);
    }

    if(appCmdlist) {
        pass = 0;
        cptr = appCmdlist;
        lvlptr = appcmdUlvl;
    } else {
        pass = 1;
        cptr = cmdlist;
        lvlptr = cmdUlvl;
    }

    /* If the command string is "ALL" then we set all commands
     * to the requested level.
     */
    if(!strcmp(cmdandlevel,"ALL")) {
        doall = 1;
    } else {
        doall = 0;
    }

    while(pass < 2) {
        if((cptr == cmdlist) && (cmdandlevel[0] == '_')) {
            cmdandlevel++;
        }

        /* Find the command in the table that is to be adjusted... */
        for(idx=0; cptr->name; cptr++,idx++) {
            if(doall || (!strcmp(cmdandlevel,cptr->name))) {
                /* Even with doall set, we don't want to touch
                 * the ULVLCMD level...
                 */
                if(doall && !strcmp(cptr->name,ULVLCMD)) {
                    continue;
                }

                /* If the command's user level is to be lowered, then the
                 * current monitor userlevel must be at least as high as the
                 * command's current user level...
                 */
                if((newlevel < lvlptr[idx]) && (getUsrLvl() < lvlptr[idx])) {
                    if(verbose) {
                        printf("User-level access denied: %s\n",cmdandlevel);
                    }
                    return(1);
                }
                lvlptr[idx] = newlevel;
                if(!doall) {
                    return(0);
                }
            }
        }
        cptr = cmdlist;
        lvlptr = cmdUlvl;
        pass++;
    }

    if(doall) {
        return(0);
    }

showerr:
    if(verbose) {
        printf("Input error: %s\n",cmdlvl);
    }
    return(-1);
}

#endif
