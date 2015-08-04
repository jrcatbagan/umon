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
 * struct.c:
 *
 *  This command allows the user to build a structure into memory based
 *  on a structure set up in the definition file ("structfile").
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
#include "ether.h"
#include <stdarg.h>

#if INCLUDE_STRUCT

#define OPENBRACE   '{'
#define CLOSEBRACE  '}'
#define PTRSIZE     4

int structsize(char *name);

struct mbrtype {
    char *type;
    int size;
};

struct mbrtype types[] = {
    { "long",       4 },
    { "short",      2 },
    { "char",       1 },
    { 0, -1 }
};

static int struct_sfd;
static ulong struct_base;
static char *struct_fname;
static char struct_verbose;
static char struct_scriptisstructfile;

/* err_general(), err_nostruct(), err_nomember():
 * Error processing functions...
 */
static void
err_general(int errnum,int linenum, long tfsloc)
{
    printf("%s: err%d at ln %d\n",struct_fname,errnum,linenum);
    tfsseek(struct_sfd,tfsloc,TFS_BEGIN);
}

static void
err_nostruct(char *structname, long tfsloc)
{
    printf("%s: can't find struct '%s'\n",struct_fname,structname);
    tfsseek(struct_sfd,tfsloc,TFS_BEGIN);
}

static void
err_nomember(char *structname, char *mbrname, long tfsloc)
{
    printf("%s: member '%s' not in struct '%s'\n",
           struct_fname,mbrname,structname);
    tfsseek(struct_sfd,tfsloc,TFS_BEGIN);
}

/* struct_printf():
 * Use this function instead of "if (struct_verbose) printf(...)"
 * all over the place.
 */
static int
struct_printf(int mask, char *fmt, ...)
{
    int tot;
    va_list argp;

    if(!(struct_verbose & mask)) {
        return(0);
    }

    va_start(argp,fmt);
    tot = vsnprintf(0,0,fmt,argp);
    va_end(argp);
    return(tot);
}

/* struct_prleft():
 * Print all characters of the incoming string up to the
 * end of the string or the first occurrence of the equals sign.
 * Then print the formatted string that follows.
 */
static int
struct_prleft(int mask, char *str, char *fmt, ...)
{
    int tot = 0;
    va_list argp;

    if(!(struct_verbose & mask)) {
        return(tot);
    }

    while((*str) && (*str != '=')) {
        tot++;
        putchar(*str++);
    }

    va_start(argp,fmt);
    tot += vsnprintf(0,0,fmt,argp);
    va_end(argp);
    return(tot);
}

/* skipline():
 * If the line is filled with only whitespace, or if the first non-whitespace
 * character is a poundsign, return 1; else return 0.
 */
static int
skipline(char *line)
{
    if(struct_scriptisstructfile) {
        if(strncmp(line,"###>>",5) == 0) {
            strcpy(line,line+5);
            return(0);
        } else {
            return(1);
        }
    } else {
        while(isspace(*line)) {
            line++;
        }

        switch(*line) {
        case 0:
        case '#':
            return(1);
        }
        return(0);
    }
}

/* typesize():
 * Given the incoming type name, return the size of that symbol.
 * Take into account the possibility of it being a pointer (leading '*')
 * or an array (trailing [xx]).
 */
static int
typesize(char *typename, char *name)
{
    char *np;
    struct mbrtype *mp;
    int nlen, ptr, arraysize;

    ptr = 0;
    arraysize = 1;

    struct_printf(4,"typesize(%s,%s)\n",typename,name);

    if(name[0] == '*') {
        ptr = 1;
    }

    nlen = strlen(name);
    np = name+nlen-1;
    if(*np == ']') {
        while(*np && (*np != '[')) {
            np--;
        }
        if(*np == 0) {
            return(-1);
        }
        arraysize = atoi(np+1);
    }

    mp = types;
    while(mp->type) {
        if(strncmp(mp->type,typename,strlen(mp->type)) == 0) {
            break;
        }
        mp++;
    }

    if(ptr) {
        return(PTRSIZE * arraysize);
    }

    return(mp->size * arraysize);
}

/* structline():
 * Assume that the incoming line is text within a structure definition
 * from the structure file.  Parse it, expecting either two or three
 * tokens (2 if the member is a basic type, 3 if the member is another
 * structure).  Return the count and pointers to each token.
 */
static int
structline(char *line, char **type, char **name)
{
    int tot;
    char *cp, *token1, *token2, *token3;

    cp = line;
    while(isspace(*cp)) {
        cp++;
    }
    token1 = cp;
    while(!isspace(*cp)) {
        cp++;
    }
    *cp++ = 0;

    if(!strcmp(token1,"struct")) {
        while(isspace(*cp)) {
            cp++;
        }
        token2 = cp;
        while(!isspace(*cp)) {
            cp++;
        }
        *cp++ = 0;
        while(isspace(*cp)) {
            cp++;
        }
        token3 = cp;
        while((!isspace(*cp)) && (*cp != ';')) {
            cp++;
        }
        if(*cp != ';') {
            return(-1);
        }
        *cp++ = 0;
        *type = token2;
        *name = token3;
        tot = 3;
    } else {
        while(isspace(*cp)) {
            cp++;
        }
        token2 = cp;
        while((!isspace(*cp)) && (*cp != ';')) {
            cp++;
        }
        if(*cp != ';') {
            return(-1);
        }
        *cp++ = 0;
        *type = token1;
        *name = token2;
        tot = 2;
    }
    struct_printf(4,"structline: type='%s', name='%s'\n",*type,*name);
    return(tot);

}

/* structsize():
 * Assuming the incoming string is a pointer to some structure definition
 * in the structure file, parse the file and figure out how big the
 * structure is.  Return the size of successful; else return -1 to indicate
 * some kind of parsing error.
 */
int
structsize(char *structname)
{
    long loc;
    int offset, slen, lno, size;
    char line[80], *cp, *cp1, *type, *name;

    struct_printf(4,"structsize(%s)\n",structname);

    loc = tfstell(struct_sfd);

    if(tfsseek(struct_sfd,0,TFS_BEGIN) < 0) {
        tfsseek(struct_sfd,loc,TFS_BEGIN);
        return(-1);
    }

    offset = 0;
    slen = strlen(structname);
    lno = 0;
    while(tfsgetline(struct_sfd,line,sizeof(line)) > 0) {
        lno++;
        if(skipline(line)) {
            continue;
        }
        if(!strncmp(line,"struct ",7)) {
            cp = line+7;
            while(isspace(*cp)) {
                cp++;
            }
            if((!strncmp(structname,cp,slen)) && (isspace(cp[slen]))) {
                cp1 = cp+slen;
                while(isspace(*cp1)) {
                    cp1++;
                }
                if(*cp1 != OPENBRACE) {
                    err_general(1,lno,loc);
                    return(-1);
                }

                while(tfsgetline(struct_sfd,line,sizeof(line)) > 0) {
                    lno++;
                    if(skipline(line)) {
                        continue;
                    }
                    if(line[0] == CLOSEBRACE) {
                        tfsseek(struct_sfd,loc,TFS_BEGIN);
                        return(offset);
                    }
                    switch(structline(line,&type,&name)) {
                    case 2:
                        if((size = typesize(type,name)) < 0) {
                            err_general(2,lno,loc);
                            return(-1);
                        }
                        break;
                    case 3:
                        if((size = structsize(type)) < 0) {
                            err_general(3,lno,loc);
                            return(-1);
                        }
                        break;
                    default:
                        err_general(4,lno,loc);
                        return(-1);
                    }
                    offset += size;
                }
                err_general(5,lno,loc);
                return(-1);
            }
        }
    }
    err_nostruct(structname,loc);
    return(-1);
}

int
membertype(char *structname, char *mbrname, char *mbrtype)
{
    long    loc;
    int     rc, slen, lno;
    char    line[80], *cp, *cp1, *type, *name;

    struct_printf(4,"membertype(%s,%s)\n",structname,mbrname);

    loc = tfstell(struct_sfd);

    if(tfsseek(struct_sfd,0,TFS_BEGIN) < 0) {
        tfsseek(struct_sfd,loc,TFS_BEGIN);
        return(-1);
    }

    slen = strlen(structname);
    lno = 0;
    while(tfsgetline(struct_sfd,line,sizeof(line)) > 0) {
        lno++;
        if(skipline(line)) {
            continue;
        }
        if(!strncmp(line,"struct ",7)) {
            cp = line+7;
            while(isspace(*cp)) {
                cp++;
            }
            if((!strncmp(structname,cp,slen)) && (isspace(cp[slen]))) {
                cp1 = cp+slen;
                while(isspace(*cp1)) {
                    cp1++;
                }
                if(*cp1 != OPENBRACE) {
                    err_general(6,lno,loc);
                    return(-1);
                }

                while(tfsgetline(struct_sfd,line,sizeof(line)) > 0) {
                    lno++;
                    if(skipline(line)) {
                        continue;
                    }
                    if(line[0] == CLOSEBRACE) {
                        err_nomember(structname,mbrname,loc);
                        return(-1);
                    }
                    if((rc = structline(line,&type,&name)) < 0) {
                        err_general(7,lno,loc);
                        return(-1);
                    }
                    if(!strcmp(name,mbrname)) {
                        tfsseek(struct_sfd,loc,TFS_BEGIN);
                        if(mbrtype) {
                            strcpy(mbrtype,type);
                        }
                        return(rc);
                    }
                }
                err_general(8,lno,loc);
                return(-1);
            }
        }
    }
    err_nostruct(structname,loc);
    return(-1);
}

/* memberoffset():
 * Return the offset into the structure at which point the member resides.
 * If mbrsize is non-zero, then assume it is a pointer to an integer
 * into which this function will also load the size of the member.
 */
int
memberoffset(char *structname, char *mbrname, int *mbrsize)
{
    long loc;
    int offset, slen, size, lno;
    char line[80], *cp, *cp1, *type, *name;

    struct_printf(4,"memberoffset(%s,%s)\n",structname,mbrname);

    loc = tfstell(struct_sfd);

    if(tfsseek(struct_sfd,0,TFS_BEGIN) < 0) {
        tfsseek(struct_sfd,loc,TFS_BEGIN);
        return(-1);
    }

    offset = 0;
    slen = strlen(structname);
    lno = 0;
    while(tfsgetline(struct_sfd,line,sizeof(line)) > 0) {
        lno++;
        if(skipline(line)) {
            continue;
        }
        if(!strncmp(line,"struct ",7)) {
            cp = line+7;
            while(isspace(*cp)) {
                cp++;
            }
            if((!strncmp(structname,cp,slen)) && (isspace(cp[slen]))) {
                cp1 = cp+slen;
                while(isspace(*cp1)) {
                    cp1++;
                }
                if(*cp1 != OPENBRACE) {
                    err_general(9,lno,loc);
                    return(-1);
                }

                while(tfsgetline(struct_sfd,line,sizeof(line)) > 0) {
                    lno++;
                    if(skipline(line)) {
                        continue;
                    }
                    if(line[0] == '#') {
                        continue;
                    }
                    if(line[0] == CLOSEBRACE) {
                        err_nomember(structname,mbrname,loc);
                        return(-1);
                    }
                    switch(structline(line,&type,&name)) {
                    case 2:
                        if((size = typesize(type,name)) < 0) {
                            err_general(10,lno,loc);
                            return(-1);
                        }
                        break;
                    case 3:
                        if((size = structsize(type)) < 0) {
                            err_general(11,lno,loc);
                            return(-1);
                        }
                        break;
                    default:
                        err_general(12,lno,loc);
                        return(-1);
                    }
                    if(!strcmp(name,mbrname)) {
                        tfsseek(struct_sfd,loc,TFS_BEGIN);
                        if(mbrsize) {
                            *mbrsize = size;
                        }
                        return(offset);
                    }
                    offset += size;
                }
                err_general(13,lno,loc);
                return(-1);
            }
        }
    }
    err_nostruct(structname,loc);
    return(-1);
}

char *StructHelp[] = {
    "Build a structure in memory",
    "-[b:f:v] {struct.mbr[=val]} [struct.mbr1[=val1]] ...",
#if INCLUDE_VERBOSEHELP
    "Options:",
    " -b{addr}   base address of structure (overrides STRUCTBASE)",
    " -f{fname}  structure definition filename (overrides STRUCTFILE)",
    " -v         additive verbosity",
    "",
    "Notes:",
    "  * The 'val' can be a normal value or a processed function...",
    "    strcpy(str|src), strcat(str|src), memcpy(src,size)",
    "    sizeof(structtype), tagsiz(str1,str2), e2b(enet), i2l(ip)",
    "  * If '=val' is not specified, then the size and offset of the",
    "    specified structure/member is loaded into STRUCTSIZE and",
    "    STRUCTOFFSET shellvars respectively.",
#endif
    0,
};

int
StructCmd(int argc,char *argv[])
{
    unsigned long lval, dest;
    char    copy[CMDLINESIZE], type[32];
    char    *eq, *dot, *str, *mbr, *env, *equation;
    int     opt, i, offset, mbrtype, rc, size, processlval;

    struct_fname = 0;
    struct_verbose = 0;
    struct_base = 0xffffffff;
    while((opt=getopt(argc,argv,"b:f:v")) != -1) {
        switch(opt) {
        case 'b':
            struct_base = strtoul(optarg,0,0);
            break;
        case 'f':
            struct_fname = optarg;
            break;
        case 'v':
            struct_verbose++;
            break;
        default:
            return(CMD_PARAM_ERROR);
        }
    }

    if(argc < optind + 1) {
        return(CMD_PARAM_ERROR);
    }

    /* If base and/or filename are not set, get them from environment
     * variables...
     */
    if(struct_base == 0xffffffff) {
        if((env = getenv("STRUCTBASE")) == 0) {
            printf("No struct base\n");
            return(CMD_FAILURE);
        }
        struct_base = strtoul(env,0,0);
    }
    if(struct_fname == 0) {
        if((struct_fname = getenv("STRUCTFILE")) == 0) {
            printf("No struct filename\n");
            return(CMD_FAILURE);
        }
    }

    struct_sfd = tfsopen(struct_fname,TFS_RDONLY,0);

    if(struct_sfd < 0) {
        printf("Can't find file '%s'\n",struct_fname);
        return(CMD_FAILURE);
    }

    /* If the specified structure description file is the currently
     * running script, then set a flag so that this code will look
     * for the "###>>" prefix as a required line prefix in the
     * structure file.
     */
    if(strcmp(struct_fname,tfsscriptname()) == 0) {
        struct_scriptisstructfile = 1;
    } else {
        struct_scriptisstructfile = 0;
    }

    /* Assume each command line argument is some "struct=val" statement,
     * and process each one...
     */
    for(i=optind; i<argc; i++) {
        equation = argv[i];

        /* Make a copy of the argument so we can modify it.
         * (no need to check length because sizeof(copy) is CMDLINESIZE).
         */
        strcpy(copy,equation);

        /* Check for equal sign...
         */
        if((eq = strchr(copy,'='))) {
            *eq = 0;
        }

        /* Start parsing and processing the structure request...
         */
        offset = mbrtype = 0;
        dot = strchr(copy,'.');
        if(!dot) {
            size = structsize(copy);
        } else {
            while(dot) {
                *dot++ = 0;
                mbr = dot;
                str = dot-2;
                while((*str != 0) && (str != copy) && (*str != '.')) {
                    str--;
                }
                if(str != copy) {
                    str++;
                }
                while((dot != 0) && (*dot != '.')) {
                    dot++;
                }
                *dot = 0;

                if(mbrtype == 3) {
                    str = type;
                }

                if((rc = memberoffset(str,mbr,&size)) < 0) {
                    goto done;
                }

                if((mbrtype = membertype(str,mbr,type)) < 0) {
                    goto done;
                }

                offset += rc;
                *dot = '.';
                dot = strchr(mbr,'.');
            }
        }

        shell_sprintf("STRUCTOFFSET","0x%lx",offset);
        shell_sprintf("STRUCTSIZE","0x%lx",size);

        /* If there is no "right half" of the equation, then just
         * continue here...
         */
        if(!eq) {
            continue;
        }

        /* At this point we have the size of the member and its offset
         * from the base, so now we parse the "val" side of the
         * equation...
         */

        /* Check for functions (sizeof, strcpy, enet, ip, etc...).
         * If found, then something other than just normal lval processing
         * is done...
         */
        eq++;
        lval = processlval = 0;
        dest = struct_base + offset;
        if(!strncmp(eq,"sizeof(",7)) {
            char *cp;

            eq += 7;
            cp = eq;
            if((cp = strchr(cp,')')) == 0) {
                goto paramerr;
            }
            *cp=0;
            lval = structsize(eq);
            processlval = 1;
        } else if(!strncmp(eq,"i2l(",4)) {
            char *cp, *cp1;

            eq += 4;
            cp = cp1 = eq;
            if((cp1 = strchr(cp,')')) == 0) {
                goto paramerr;
            }
            *cp1 = 0;
            if(struct_verbose & 1) {
            }
            struct_printf(3,"i2l(0x%lx,%s)\n", dest, cp);

            if(IpToBin(cp,(uchar *)dest) < 0) {
                goto paramerr;
            }
        } else if(!strncmp(eq,"e2b(",4)) {
            char *cp, *cp1;

            cp = cp1 = eq+4;
            if((cp1 = strchr(cp,')')) == 0) {
                goto paramerr;
            }
            *cp1 = 0;
            struct_printf(3,"e2b(0x%lx,%s)\n", dest, cp);

            if(EtherToBin(cp,(uchar *)dest) < 0) {
                goto paramerr;
            }
        } else if(!strncmp(eq,"tagsiz(",7)) {
            char *comma, *paren;
            int siz1, siz2;

            eq += 7;
            comma = eq;
            if((comma = strchr(comma,',')) == 0) {
                goto paramerr;
            }
            *comma++ = 0;
            if((paren = strrchr(comma,')')) == 0) {
                goto paramerr;
            }
            *paren = 0;
            if((siz1=structsize(eq)) < 0) {
                goto paramerr;
            }
            if((siz2=structsize(comma)) < 0) {
                goto paramerr;
            }
            lval = (siz1+siz2)/4;
            processlval = 1;
        } else if(!strncmp(eq,"memcpy(",7)) {
            int len;
            char *comma, *cp;

            eq += 7;
            comma = eq;
            if((comma = strchr(comma,',')) == 0) {
                goto paramerr;
            }
            *comma++ = 0;
            cp = comma;
            cp = (char *)strtoul(eq,0,0);
            len = (int)strtoul(comma,0,0);
            struct_printf(3,"memcpy(0x%lx,0x%lx,%d)\n",dest,(long)cp,len);
            memcpy((void *)dest,(void *)cp,len);
        } else if(!strncmp(eq,"strcpy(",7)) {
            char *cp, *cp1;

            eq += 7;
            cp = cp1 = eq;
            if((cp1 = strrchr(cp,')')) == 0) {
                goto paramerr;
            }
            *cp1 = 0;


            if(!strncmp(cp,"0x",2)) {
                cp = (char *)strtoul(eq,0,0);
                struct_printf(3,"strcpy(0x%lx,0x%lx)\n",dest,(long)cp);
            } else {
                struct_prleft(1,equation," = \"%s\"\n",cp);
                struct_printf(2,"strcpy(0x%lx,%s)\n", dest,cp);
            }
            strcpy((char *)dest,cp);
        } else if(!strncmp(eq,"strcat(",7)) {
            char *cp, *cp1;

            eq += 7;
            cp = cp1 = eq;
            if((cp1 = strrchr(cp,')')) == 0) {
                goto paramerr;
            }
            *cp1 = 0;

            if(!strncmp(cp,"0x",2)) {
                cp = (char *)strtoul(eq,0,0);
                struct_printf(3,"strcat(0x%lx,0x%lx)\n",dest,(long)cp);
            } else {
                struct_prleft(1,equation," += \"%s\"\n",cp);
                struct_printf(2,"strcat(0x%lx,%s)\n", dest,cp);
            }
            strcat((char *)dest,cp);
        } else {
            lval = strtoul(eq,0,0);
            processlval = 1;
        }

        if(processlval) {
            switch(size) {
            case 1:
                struct_prleft(1,equation," = %d (0x%x)\n",
                              (uchar)lval,(uchar)lval);
                struct_printf(2,"*(uchar *)0x%lx = %d (0x%x)\n",
                              dest,(uchar)lval,(uchar)lval);
                *(uchar *)(dest) = (uchar)lval;
                break;
            case 2:
                struct_prleft(1,equation," = %d (0x%x)\n",
                              (ushort)lval,(ushort)lval);
                struct_printf(2,"*(ushort *)0x%lx = %d (0x%x)\n",
                              dest,(ushort)lval,(ushort)lval);
                *(ushort *)dest = (ushort)lval;
                break;
            case 4:
                struct_prleft(1,equation," = %ld (0x%lx)\n",
                              (ulong)lval,(ulong)lval);
                struct_printf(2,"*(ulong *)0x%lx = %d (0x%x)\n",
                              dest,lval,lval);
                *(ulong *)dest = lval;
                break;
            default:
                struct_printf(3,"memset(0x%lx,0x%x,%d)\n",
                              dest,(uchar)lval,size);
                memset((void *)dest,lval,size);
                break;
            }
        }
    }
done:
    tfsclose(struct_sfd,0);
    return(CMD_SUCCESS);

paramerr:
    tfsclose(struct_sfd,0);
    return(CMD_PARAM_ERROR);
}
#endif
