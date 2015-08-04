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
 * misccmds:
 *
 * More monitor commands...
 *
 * Original author:     Ed Sutter (ed.sutter@alcatel-lucent.com)
 *
 */
#include "config.h"
#include "stddefs.h"
#include "tfs.h"
#include "tfsprivate.h"
#include "genlib.h"
#include "ether.h"
#include "devices.h"
#include "cli.h"
#include <ctype.h>
#include "warmstart.h"


char    ApplicationInfo[82];
int (*extgetUsrLvl)(void);

#if INCLUDE_USRLVL
static int  setUsrLvl(int, char *);
static int  UserLevel;

char *UlvlHelp[] = {
    "Display or modify current user level.",
    "-[c:hp] [new_level|min|max] [password]",
#if INCLUDE_VERBOSEHELP
    "Options:",
    " -c{cmd,lvl}",
    "      set command's user level",
    " -h   dump system header",
    " -p   build new password file",
    "",
    " Note: cmd==ALL, applies action to all commands.",
#endif
    0
};

int
Ulvl(int argc,char *argv[])
{
    char    passwd[32], *pwp;
    int     level, opt, newulvl;

    newulvl = 0;
    pwp = (char *)0;
    level = MINUSRLEVEL;
    while((opt=getopt(argc,argv,"c:hp")) != -1) {
        switch(opt) {
        case 'c':
            setCmdUlvl(optarg,1);
            newulvl++;
            break;
        case 'h':
            monHeader(0);
            break;
        case 'p':
            newPasswordFile();
            break;
        default:
            return(CMD_PARAM_ERROR);
        }
    }

    /* At this point, the newulvl flag is used to indicate that the
     * -c option was used.  If it was, then we return here.
     */
    if(newulvl) {
        return(CMD_SUCCESS);
    }

    /* If there is one or two arguments on the command line, then
     * the user must want to modify the current user level.  If
     * there are no arguments, then simply display the current
     * user level.
     *
     * If the new user level is lower than the current user level,
     * then the user can simply enter the new level (one argument).
     * If the new user level is higher than the current user level,
     * then the user must also enter a password.  The password is
     * entered either as the second argument or interactively
     * using getpass().
     */
    newulvl = 0;
    if((argc == optind+1) || (argc == optind+2)) {
        if(!strcmp(argv[optind],"min")) {
            level = MINUSRLEVEL;
        } else if(!strcmp(argv[optind],"max")) {
            level = MAXUSRLEVEL;
        } else {
            level = atoi(argv[optind]);
        }

        if(argc == optind+1) {
            if(level > UserLevel) {
                getpass("Password: ",passwd,sizeof(passwd)-1,0);
                pwp = passwd;
            }
        } else {
            pwp = argv[optind+1];
        }
        newulvl = 1;
    } else if(argc != optind) {
        return(CMD_PARAM_ERROR);
    }

    /* At this point,the  newulvl flag is used to indicate that an
     * adjustment to the current user level is to be made.
     */
    if(newulvl) {
        if(level <= UserLevel) {
            UserLevel = level;
        } else {
            setUsrLvl(level,pwp);
        }
    }

    if(extgetUsrLvl) {
        printf("User level controlled by application: %d\n",extgetUsrLvl());
    } else {
        printf("Current monitor user level: %d\n",UserLevel);
    }
    return(CMD_SUCCESS);
}

void
initUsrLvl(int lvl)
{
    extgetUsrLvl = 0;
    UserLevel = lvl;
}

/* getUsrLvl():
 *  This is the ONLY point of access for retrieval of the user level.
 *  This allows the application to redefine how the monitor retrieves
 *  what it thinks of as the user level.
 */
int
getUsrLvl(void)
{
    if(extgetUsrLvl) {
        return(extgetUsrLvl());
    }
    return(UserLevel);
}

int
setUsrLvl(int level, char *password)
{
    int olvl;

    olvl = UserLevel;

    /* If level is -1, then assume this is only a request for the current   */
    /* user level.  If the incoming level is any other value outside the    */
    /* range of MINUSRLEVEL and MAXUSRLEVEL, return -1.                     */
    if(level == -1) {
        return(UserLevel);
    }
    if((level > MAXUSRLEVEL) || (level < MINUSRLEVEL)) {
        return(olvl);
    }

    /* If password pointer is NULL, then don't check for password, just set */
    /* the level and be done.                                               */
    if(!password) {
        UserLevel = level;
    } else {
        if(validPassword(password,level)) {
            UserLevel = level;
        }
    }
    return(olvl);
}

/* returnMaxUsrLvl(), setTmpMaxUsrLvl() & clrTmpMaxUsrLvl():
 *  These three functions are used to allow a few places in the monitor
 *  to temporarily force the user level to MAXUSRLEVEL.  This is necessary
 *  for accessing the password file and the tfs log file.
 *  Call setTmpMaxUsrLvl() to enable MAX mode and then clrTmpMaxUsrLvl()
 *  with the value returned by setTmpMaxUsrLvl() when done.
 */
int
returnMaxUsrLvl(void)
{
    return(MAXUSRLEVEL);
}

void *
setTmpMaxUsrLvl(void)
{
    void *fptr;

    fptr = (void *)extgetUsrLvl;
    extgetUsrLvl = returnMaxUsrLvl;
    return(fptr);
}

void
clrTmpMaxUsrLvl(int (*fptr)(void))
{
    extgetUsrLvl = fptr;
}

#else

int
getUsrLvl(void)
{
    return(MAXUSRLEVEL);
}

#endif

char *VersionHelp[] = {
    "Version information",
    "[application_info]",
    0,
};

int
Version(int argc,char *argv[])
{
    extern  void ShowVersion(void);

    if(argc == 1) {
        ShowVersion();
    } else {
        strncpy(ApplicationInfo,argv[1],80);
        ApplicationInfo[80] = 0;
    }
    return(CMD_SUCCESS);
}

#if INCLUDE_TFSSCRIPT
char *EchoHelp[] = {
    "Print string to local terminal",
    "[arg1] ... [argn]",
#if INCLUDE_VERBOSEHELP
    " Special meaning: \\b \\c \\r \\n \\t \\x",
#endif
    0,
};

int
Echo(int argc,char *argv[])
{
    int     i, done;
    char    *cp, c, hex[3];

    for(i=optind; i<argc; i++) {
        cp = argv[i];
        done = 0;
        while(!done && *cp) {
            if(*cp == '\\') {
                cp++;
                switch(*cp) {
                case 'b':           /* Backspace */
                    c = '\b';
                    break;
                case 'c':           /* No newline, just end here */
                    return(CMD_SUCCESS);
                case 'n':           /* Newline */
                    c = '\n';
                    break;
                case 'r':           /* Carriage Return */
                    c = '\r';
                    break;
                case 't':           /* Tab */
                    c = '\t';
                    break;
                case 'x':           /* Hex conversion */
                    cp++;
                    hex[0] = *cp++;
                    hex[1] = *cp;
                    hex[2] = 0;
                    c = strtol(hex,0,16);
                    break;
                case '\\':          /* Backslash */
                    c = '\\';
                    break;
                default:            /* Ignore backslash */
                    c = *cp;
                    break;
                }
                putchar(c);
            } else {
                putchar(*cp);
            }
            if(cp) {
                cp++;
            }
        }
        if(i != argc-1) {
            putchar(' ');
        }
    }
    putchar('\n');
    flush_console_out();
    return(CMD_SUCCESS);
}
#endif

/* Call():
 *  This function is called when the user wants to execute an
 *  embedded function.
 *  The the argument is preceded by an ampersand, then a pointer
 *  to the argument is passed to the function instead of a
 *  strtol() conversion.
 */
char *CallHelp[] = {
    "Call embedded function",
    "-[Aaqv:] {address} [arg1] [arg2] ...",
#if INCLUDE_VERBOSEHELP
    " -a       build (argc,argv) then call function",
#if INCLUDE_SHELLVARS
    " -A       build arglist for use by mon_getargv()",
#endif
    " -q       quiet mode",
    " -v {var} put return val in varname",
#endif
    0,
};

int
Call(int argc,char *argv[])
{
    char    *varname;
    long    args[10];
    int     i, j, ret, opt, useargc, quiet;
    int (*func)(long,long,long,long,long,long,long,long,long,long);
#if INCLUDE_SHELLVARS
    int     monargs = 0;
#endif

    quiet = 0;
    useargc = 0;
    varname = (char *)0;
    while((opt=getopt(argc,argv,"Aaqv:")) != -1) {
        switch(opt) {
        case 'a':
            useargc = 1;
            break;
        case 'q':
            quiet = 1;
            break;
        case 'v':
            varname = optarg;
            break;
        case 'A':
#if INCLUDE_SHELLVARS
            monargs = 1;
            break;
#endif
        default:
            return(CMD_PARAM_ERROR);
        }
    }

    if((argc < optind+1) || (argc > optind+11)) {
        return(CMD_PARAM_ERROR);
    }

    func = (int(*)(long,long,long,long,long,long,long,long,long,long))
           strtol(argv[optind],(char **)0,0);

    if((func == 0) && (isdigit(argv[optind][0]) == 0)) {
        return(CMD_PARAM_ERROR);
    }

    /* If useargc flag is not set, then retrieve and convert
     * args from command line.  If the first character of the
     * argument is an ampersand (&), then a pointer to the argument
     * is passed; otherwise, the argument is converted to a long
     * integer using strtol()...
     */
    if(!useargc) {
        for(j=0,i=optind+1; i<argc; i++,j++) {
            if(argv[i][0] == '&') {
                args[j] = (ulong)&argv[i][1];
            } else {
                args[j] = strtol(argv[i],(char **)0,0);
            }
        }
    }

#if INCLUDE_SHELLVARS
    if(monargs) {
        for(j=0,i=optind; i<argc; i++,j++) {
            putargv(j,argv[i]);
        }
        putargv(j,(char *)0);
    }
#endif

    if(useargc) {
        ret = func(argc-optind,(long)&argv[optind],0,0,0,0,0,0,0,0);
    } else {
        ret = func(args[0],args[1],args[2],args[3],args[4],args[5],args[6],
                   args[7],args[8],args[9]);
    }

    if(varname) {
        shell_sprintf(varname,"0x%x",ret);
    }

    if(!quiet) {
        printf("Returned: %d (0x%x)\n",ret,ret);
    }

    return(CMD_SUCCESS);
}

/* Reset():
 *  Used to re-initialize the monitor through the command interface.
 */

char *ResetHelp[] = {
    "Reset monitor firmware",
    "-[xt:]",
#if INCLUDE_VERBOSEHELP
    " -x      app_exit",
    " -t ##   warmstart type",
#endif
    0,
};

int
Reset(int argc,char *argv[])
{
    extern  void appexit(int);
    int opt;

    intsoff();

    /* For some systems, the reset occurs while characters are in the
     * UART FIFO (so they don't get printed).  Adding this delay will
     * hopefully allow the characters in the FIFO to drain...
     */
    monDelay(250);

    while((opt=getopt(argc,argv,"xt:")) != -1) {
        switch(opt) {
        case 'x':
            appexit(0);
            break;
        case 't':
            monrestart((int)strtol(optarg,0,0));
            break;
        default:
            return(CMD_PARAM_ERROR);
        }
    }
    target_reset();
    return(CMD_SUCCESS);
}

#if INCLUDE_TFS
/* WhatCmd():
 *  Used to search through a binary file displaying any "WHAT" strings
 *  that may be present...
 *
 *  Just what is "what"?
 *  This is a tool that simply prints all strings found that start with
 *  "@(#)" in an input file.  This is called a "what string".  It is useful
 *  for storage of retrievable information from a binary file.
 */

#define WHATSEARCH  1
#define WHATMATCH   2

char *WhatHelp[] = {
    "Search file for 'what' strings; put last result in 'WHATSTRING' shell var",
    "[-t:v] {filename}",
#if INCLUDE_VERBOSEHELP
    " -t{tok} match on what-string with subtoken",
    " -v      verbose",
#endif
    0,
};


int
WhatCmd(int argc,char **argv)
{
    TFILE   *tfp;
    int     opt, state, verbose, idx;
    char    whatbuf[64], *whattok, *varname;
    char    *ifile, *buf, *end, *bp, *whatstring, *wsp;

    whattok = 0;
    whatstring = "@(#)";
    idx = verbose = 0;
    varname = "WHATSTRING";
    while((opt=getopt(argc,argv,"t:v")) != -1) {
        switch(opt) {
        case 'v':
            verbose++;
            break;
        case 't':
            whattok = optarg;
            break;
        default:
            return(CMD_FAILURE);
        }
    }
    if(argc-optind != 1) {
        return(CMD_PARAM_ERROR);
    }

    ifile = argv[optind];

    /* Open input file: */
    if((tfp = tfsstat(ifile)) == (TFILE *)0) {
        printf("Can't find file: %s\n",ifile);
        return(CMD_FAILURE);
    }
    bp = buf = TFS_BASE(tfp);
    end = buf + TFS_SIZE(tfp);

    state = WHATSEARCH;
    wsp = whatstring;
    setenv(varname,0);
    while(bp < end) {
        switch(state) {
        case WHATSEARCH:
            if(*bp == *wsp) {
                wsp++;
                if(*wsp == 0) {
                    wsp = whatstring;
                    state = WHATMATCH;
                    idx = 0;
                    if(verbose) {
                        putchar('\t');
                    }
                }
            } else {
                wsp = whatstring;
            }
            break;
        case WHATMATCH:
            if(isprint(*bp)) {
                if(verbose) {
                    putchar(*bp);
                }
                if(idx < sizeof(whatbuf)-1) {
                    whatbuf[idx++] = *bp;
                }
            } else {
                if(verbose) {
                    putchar('\n');
                }
                if(idx < sizeof(whatbuf)) {
                    whatbuf[idx] = 0;
                }
                if(whattok) {
                    if(strstr(whatbuf,whattok)) {
                        setenv(varname,whatbuf);
                    }
                } else {
                    setenv(varname,whatbuf);
                }

                state = WHATSEARCH;
            }
            break;
        }
        bp++;
    }
    return(0);
}
#endif
