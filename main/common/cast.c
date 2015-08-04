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
 * cast.c:
 *
 *  The cast command is used in the monitor to cast or overlay a structure
 *  onto a block of memory to display that memory in the format specified
 *  by the structure.  The structure definition is found in the file
 *  "structfile" in TFS.  Valid types within structfile are
 *  char, char.c, char.x, short, short.x, long, long.x and struct name.
 *  Default format is decimal.  The '.x' extension tells cast to print
 *  in hex and the '.c' extension tells cast to print the actual character.
 *
 *
 * Original author:     Ed Sutter (ed.sutter@alcatel-lucent.com)
 *
 */
#include "config.h"
#include "tfs.h"
#include "tfsprivate.h"
#include <ctype.h>
#include "genlib.h"
#include "stddefs.h"
#include "cli.h"

#if INCLUDE_CAST

static  ulong memAddr;
static  int castDepth;

#define OPEN_BRACE  '{'
#define CLOSE_BRACE '}'

#define STRUCT_SEARCH   1
#define STRUCT_DISPLAY  2
#define STRUCT_ALLDONE  3
#define STRUCT_ERROR    4

#define STRUCT_SHOWPAD  (1<<0)
#define STRUCT_SHOWADD  (1<<1)
#define STRUCT_VERBOSE  (1<<2)


#define STRUCTFILE "structfile"

struct mbrinfo {
    char *type;
    char *format;
    int size;
    int mode;
};

struct mbrinfo mbrinfotbl[] = {
    { "char",       "%d",       1 },        /* decimal */
    { "char.x",     "0x%02x",   1 },        /* hex */
    { "char.c",     "%c",       1 },        /* character */
    { "short",      "%d",       2 },        /* decimal */
    { "short.x",    "0x%04x",   2 },        /* hex */
    { "long",       "%ld",      4 },        /* decimal */
    { "long.x",     "0x%08lx",  4 },        /* hex */
    { 0,0,0 }
};

/* castIndent():
 *  Used to insert initial whitespace based on the depth of the
 *  structure nesting.
 */
void
castIndent(void)
{
    int i;

    for(i=0; i<castDepth; i++) {
        printf("  ");
    }
}

/* strAddr():
 *  Called by showStruct().  It will populate the incoming buffer pointer
 *  with either NULL or the ascii-hex representation of the current address
 *  pointer.
 */
char *
strAddr(long flags, char *buf)
{
    if(flags & STRUCT_SHOWADD) {
        sprintf(buf,"0x%08lx: ",memAddr);
    } else {
        buf[0] = 0;
    }
    return(buf);
}

/* showStruct():
 *  The workhorse of cast.  This function parses the structfile looking for
 *  the structure type; then it attempts to display the memory block that
 *  begins at memAddr as if it was the structure.  Note that there is no
 *  pre-processing done to verify valid syntax of the structure definition.
 */
int
showStruct(int tfd,long flags,char *structtype,char *structname,char *linkname)
{
    struct mbrinfo *mptr;
    ulong curpos, nextlink;
    int i, state, snl, retval, tblsize;
    char line[96], addrstr[16], format[64];
    char *cp, *eol, *type, *eotype, *name, *bracket, *eoname, tmp;

    type = (char *)0;
    retval = nextlink = 0;
    curpos = tfsctrl(TFS_TELL,tfd,0);
    tfsseek(tfd,0,TFS_BEGIN);
    castIndent();
    if(structname) {
        printf("struct %s %s:\n",structtype,structname);
    } else {
        printf("struct %s @0x%lx:\n",structtype,memAddr);
    }
    castDepth++;

    state = STRUCT_SEARCH;
    snl = strlen(structtype);

    while(1) {
        if(tfsgetline(tfd,line,sizeof(line)-1) == 0) {
            printf("Structure definition '%s' not found\n",structtype);
            break;
        }
        if((line[0] == '\r') || (line[0] == '\n')) { /* empty line? */
            continue;
        }

        eol = strpbrk(line,";#\r\n");
        if(eol) {
            *eol = 0;
        }

        if(state == STRUCT_SEARCH) {
            if(!strncmp(line,"struct",6)) {
                cp = line+6;
                while(isspace(*cp)) {
                    cp++;
                }
                if(!strncmp(cp,structtype,snl)) {
                    cp += snl;
                    while(isspace(*cp)) {
                        cp++;
                    }
                    if(*cp == OPEN_BRACE) {
                        state = STRUCT_DISPLAY;
                    } else {
                        retval = -1;
                        break;
                    }
                }
            }
        } else if(state == STRUCT_DISPLAY) {
            type = line;
            while(isspace(*type)) {
                type++;
            }

            if(*type == CLOSE_BRACE) {
                state = STRUCT_ALLDONE;
                break;
            }

            eotype = type;
            while(!isspace(*eotype)) {
                eotype++;
            }
            *eotype = 0;
            name = eotype+1;
            while(isspace(*name)) {
                name++;
            }
            bracket = strchr(name,'[');
            if(bracket) {
                tblsize = atoi(bracket+1);
            } else {
                tblsize = 1;
            }

            if(*name == '*') {
                castIndent();
                printf("%s%-8s %s: ",strAddr(flags,addrstr),type,name);
                if(!strcmp(type,"char.c")) {
                    printf("\"%s\"\n",*(char **)memAddr);
                } else {
                    printf("0x%lx\n",*(ulong *)memAddr);
                }
                memAddr += 4;
                continue;
            }
            mptr = mbrinfotbl;
            while(mptr->type) {
                if(!strcmp(type,mptr->type)) {
                    castIndent();
                    eoname = name;
                    while(!isspace(*eoname)) {
                        eoname++;
                    }
                    tmp = *eoname;
                    *eoname = 0;

                    if(bracket) {
                        if(!strcmp(type,"char.c")) {
                            printf("%s%-8s %s: ",
                                   strAddr(flags,addrstr),mptr->type,name);
                            cp = (char *)memAddr;
                            for(i=0; i<tblsize && isprint(*cp); i++) {
                                printf("%c",*cp++);
                            }
                            printf("\n");
                        } else
                            printf("%s%-8s %s\n",
                                   strAddr(flags,addrstr),mptr->type,name);
                        memAddr += mptr->size * tblsize;
                    } else {
                        sprintf(format,"%s%-8s %%s: %s\n",
                                strAddr(flags,addrstr),mptr->type,mptr->format);
                        switch(mptr->size) {
                        case 1:
                            printf(format,name,*(uchar *)memAddr);
                            break;
                        case 2:
                            printf(format,name,*(ushort *)memAddr);
                            break;
                        case 4:
                            printf(format,name,*(ulong *)memAddr);
                            break;
                        }
                        memAddr += mptr->size;
                    }
                    *eoname = tmp;
                    break;
                }
                mptr++;
            }
            if(!(mptr->type)) {
                int padsize;
                char *subtype, *subname, *eossn;

                if(!strcmp(type,"struct")) {
                    subtype = eotype+1;
                    while(isspace(*subtype)) {
                        subtype++;
                    }
                    subname = subtype;
                    while(!isspace(*subname)) {
                        subname++;
                    }
                    *subname = 0;

                    subname++;
                    while(isspace(*subname)) {
                        subname++;
                    }
                    eossn = subname;
                    while(!isspace(*eossn)) {
                        eossn++;
                    }
                    *eossn = 0;
                    if(*subname == '*') {
                        castIndent();
                        printf("%s%s %s %s: 0x%08lx\n",strAddr(flags,addrstr),
                               type,subtype,subname,*(ulong *)memAddr);
                        if(linkname) {
                            if(!strcmp(linkname,subname+1)) {
                                nextlink = *(ulong *)memAddr;
                            }
                        }
                        memAddr += 4;
                    } else {
                        for(i=0; i<tblsize; i++) {
                            if(bracket) {
                                sprintf(bracket+1,"%d]",i);
                            }
                            if(showStruct(tfd,flags,subtype,subname,0) < 0) {
                                state = STRUCT_ALLDONE;
                                goto done;
                            }
                        }
                    }
                } else if(!strncmp(type,"pad[",4)) {
                    padsize = atoi(type+4);
                    if(flags & STRUCT_SHOWPAD) {
                        castIndent();
                        printf("%spad[%d]\n",strAddr(flags,addrstr),padsize);
                    }
                    memAddr += padsize;
                } else  {
                    retval = -1;
                    break;
                }
            }
        } else {
            state = STRUCT_ERROR;
            break;
        }
    }
done:

    switch(state) {
    case STRUCT_SEARCH:
        printf("struct %s not found\n",structtype);
        retval = -1;
        break;
    case STRUCT_DISPLAY:
        printf("invalid member type: %s\n",type);
        retval = -1;
        break;
    case STRUCT_ERROR:
        printf("unknown error\n");
        retval = -1;
        break;
    }
    tfsseek(tfd,curpos,TFS_BEGIN);
    if(linkname) {
        memAddr = nextlink;
    }
    castDepth--;
    return(retval);
}

char *CastHelp[] = {
    "Cast a structure definition across data in memory.",
    "-[al:n:pt:] {struct type} {address}",
#if INCLUDE_VERBOSEHELP
    "Options:",
    " -a   show addresses",
    " -l{linkname}",
    " -n{structname}",
    " -p   show padding",
    " -t{tablename}",
#endif
    0,
};

int
Cast(int argc,char *argv[])
{
    long    flags;
    int     opt, tfd, index;
    char    *structtype, *structfile, *tablename, *linkname, *name;

    flags = 0;
    name = (char *)0;
    linkname = (char *)0;
    tablename = (char *)0;
    while((opt=getopt(argc,argv,"apl:n:t:")) != -1) {
        switch(opt) {
        case 'a':
            flags |= STRUCT_SHOWADD;
            break;
        case 'l':
            linkname = optarg;
            break;
        case 'n':
            name = optarg;
            break;
        case 'p':
            flags |= STRUCT_SHOWPAD;
            break;
        case 't':
            tablename = optarg;
            break;
        default:
            return(CMD_PARAM_ERROR);
        }
    }
    if(argc != optind + 2) {
        return(CMD_PARAM_ERROR);
    }

    structtype = argv[optind];
    memAddr = strtoul(argv[optind+1],0,0);

    /* Start by detecting the presence of a structure definition file... */
    structfile = getenv("STRUCTFILE");
    if(!structfile) {
        structfile = STRUCTFILE;
    }

    tfd = tfsopen(structfile,TFS_RDONLY,0);
    if(tfd < 0) {
        printf("Structure definition file '%s' not found\n",structfile);
        return(CMD_FAILURE);
    }

    index = 0;
    do {
        castDepth = 0;
        showStruct(tfd,flags,structtype,name,linkname);
        index++;
        if(linkname) {
            printf("Link #%d = 0x%lx\n",index,memAddr);
        }
        if(tablename || linkname) {
            if(askuser("next?"))  {
                if(tablename) {
                    printf("%s[%d]:\n",tablename,index);
                }
            } else {
                tablename = linkname = (char *)0;
            }
        }
    } while(tablename || linkname);

    tfsclose(tfd,0);
    return(CMD_SUCCESS);
}
#endif
