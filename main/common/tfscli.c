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
 * tfscli.c:
 *
 * This file contains the TFS code that is only needed if the "tfs" command
 * is included in the command set.
 *
 * Original author:     Ed Sutter (ed.sutter@alcatel-lucent.com)
 *
 */
#include "config.h"
#include "stddefs.h"
#include "genlib.h"
#include "tfs.h"
#include "tfsprivate.h"
#include "cli.h"

#if INCLUDE_TFSCLI

/* tfsprflags():
 *   Print the specified set of flags.
 */
static void
tfsprflags(long flags, int verbose)
{
    struct  tfsflg  *tfp;

    if(verbose) {
        printf(" Flags: ");
    }
    tfp = tfsflgtbl;
    while(tfp->sdesc) {
        if((flags & tfp->mask) == tfp->flag) {
            if(verbose) {
                printf("%s", tfp->ldesc);
                if((tfp+1)->flag) {
                    printf(", ");
                }
            } else {
                putchar(tfp->sdesc);
            }
        }
        tfp++;
    }
    if(!(flags & TFS_NSTALE)) {
        printf("stale");
    }

    if(verbose) {
        printf("\n");
    }
}

static char *
tfsmodebtoa(ulong mode,char *buf)
{
    char    *pipe, *bp;

    pipe = "";
    bp = buf;
    *bp = 0;
    if(mode & TFS_RDONLY) {
        bp += sprintf(bp,"rdonly");
        pipe = "|";
    }
    if(mode & TFS_CREATE) {
        bp += sprintf(bp,"%screate",pipe);
        pipe = "|";
    }
    if(mode & TFS_APPEND) {
        sprintf(bp,"%sappend",pipe);
    }

    return(buf);
}

/* tfsld():
 *  If the filename specified is AOUT, COFF or ELF, then load it.
 */
static int
tfsld(char *name,int verbose,char *sname,int verifyonly)
{
    int     err;
    TFILE   *fp;

    err = TFS_OKAY;
    fp = tfsstat(name);

    if(!fp) {
        return (TFSERR_NOFILE);
    }

    if(TFS_USRLVL(fp) > getUsrLvl()) {
        return(TFSERR_USERDENIED);
    }

    if(fp->flags & TFS_EBIN) {
        long    entry;

        err = tfsloadebin(fp,verbose,&entry,sname,verifyonly);
        if(err == TFS_OKAY) {
            char buf[16];
            sprintf(buf,"0x%lx",entry);
            setenv("ENTRYPOINT",buf);
        } else {
            setenv("ENTRYPOINT",0);
        }
    } else {
        err = TFSERR_NOTEXEC;
    }

    return(err);
}

/* listfilter():
 *  If the incoming filename (fname) passes the incoming filter, then
 *  return 1; else return 0.
 *
 *  Examples:
 *      if filter is "*.html" and fname is "index.html" return 1.
 *      if filter is "dir SLASH ASTERISK" and fname is "dir/abc" return 1.
 *      if filter is "abc" and fname is "abc" return 1.
 *
 *  Notes:
 *      * A valid filter will have the asterisk as either the first or last
 *        character of the filter.  If first, assume filter is a suffix,
 *        if last (or none at all), assume filter is a prefix.
 *      * If there is an asterisk in the middle of the filter, it is chopped
 *        at the asterisk without warning.
 */
static int
listfilter(char *filter,char *fname)
{
    int     flen;
    char    *prefix, *suffix, *asterisk, *sp;

    if(!filter) {       /* No filter means match everything. */
        return(1);
    }

    flen = 0;
    prefix = suffix = (char *)0;
    asterisk = strchr(filter,'*');

    /* If no asterisk, then just compare filter to fname... */
    if(!asterisk) {
        if(!strcmp(filter,fname)) {
            return(1);
        }
    } else if(asterisk == filter) {
        suffix = asterisk+1;
        flen = strlen(suffix);
        sp = fname + strlen(fname) - flen;
        if(!strcmp(suffix,sp)) {
            return(1);
        }
    } else {
        *asterisk = 0;
        prefix = filter;
        flen = strlen(prefix);
        if(!strncmp(prefix,fname,flen)) {
            *asterisk = '*';
            return(1);
        }
        *asterisk = '*';
    }
    return(0);
}

/* tfsvlist():  verbose list...
 *  Display all files currently stored.  Do not put them in alphabetical
 *  order; display them as they are physically listed in the file system.
 *  Display complete structure of file header for each file.
 *  Note1: This version of file listing is only called if "tfs -vv ls"
 *  or "tfs -vvv ls" is called.  The first level of verbosity is handled
 *  by tfsqlist to simply display the "dot" files.
 */
static int
tfsvlist(char *filter[], int verbose, int more)
{
    TDEV    *tdp;
    TFILE   *fp;
    int     tot, sizetot;
    char    tbuf[32], **fltrptr;

    tot = 0;
    sizetot = 0;
    for(tdp=tfsDeviceTbl; tdp->start != TFSEOT; tdp++) {
        fltrptr = filter;
        while(1) {
            fp = (TFILE *)tdp->start;
            while(validtfshdr(fp)) {

                if((TFS_DELETED(fp)) && (verbose < 3)) {
                    fp = nextfp(fp,tdp);
                    continue;
                }
                if(!listfilter(*fltrptr,TFS_NAME(fp))) {
                    fp = nextfp(fp,tdp);
                    continue;
                }
                if((fp->flags & TFS_UNREAD) && (TFS_USRLVL(fp)>getUsrLvl())) {
                    fp = nextfp(fp,tdp);
                    continue;
                }
                printf(" Name:  '%s'%s\n",
                       fp->name,TFS_DELETED(fp) ? " (deleted)" : "");
                printf(" Info:  '%s'\n", fp->info);
                if(TFS_FILEEXISTS(fp)) {
                    tfsprflags(fp->flags, 1);
                }
                printf(" Addr:  0x%lx (hdr @ 0x%lx, nxtptr = 0x%lx)\n",
                       (ulong)(TFS_BASE(fp)),(ulong)fp,(ulong)(fp->next));
                printf(" Size:  %ld bytes (crc=0x%lx)",
                       fp->filsize,fp->filcrc);
#if INCLUDE_FLASH
                if((tdp->devinfo & TFS_DEVTYPE_MASK) != TFS_DEVTYPE_RAM) {
                    int start_sector, end_sector;
                    addrtosector((uchar *)fp,&start_sector,0,0);
                    addrtosector((uchar *)fp+fp->filsize+TFSHDRSIZ,
                                 &end_sector,0,0);
                    if(start_sector == end_sector) {
                        printf(" in sector %d",start_sector);
                    } else {
                        printf(" spanning sectors %d-%d",
                               start_sector,end_sector);
                    }
                }
#endif
                putchar('\n');
                sizetot += (fp->filsize + TFSHDRSIZ + DEFRAGHDRSIZ);
                if(TFS_TIME(fp) != TIME_UNDEFINED)
                    printf(" Time:  %s\n",
                           tfsGetAtime(TFS_TIME(fp),tbuf,sizeof(tbuf)));
                printf("\n");
                tot++;
                fp = nextfp(fp,tdp);
                if((more) && ((tot % more) == 0)) {
                    if(!More()) {
                        return(TFS_OKAY);
                    }
                }
            }
            /* If this or the next pointer is null, terminate loop now... */
            if(!*fltrptr) {
                break;
            }
            fltrptr++;
            if(!*fltrptr) {
                break;
            }
        }
    }
    printf("Total: %d accessible file%s (%d bytes).\n",
           tot,tot == 1 ? "" : "s",sizetot);
    return (TFS_OKAY);
}

/* tfsqlist():  quick list...
 *  Display list of files in alphabetical order.
 *  Display only the name and flag summary.
 *
 *  To support listing of files in a bit of an orderly manner, if this
 *  function sees that a filename has a slash in it, it will only print the
 *  characters upto and including the first slash.  This gives the appearance
 *  of a directory structure, even though there really isn't one.
 *  For example, if there are three files...
 *          CONFIG/file1
 *          CONFIG/file2
 *          CONFIG/file3
 *  then if no filter is specified, then that listing will be replaced with
 *          CONFIG/
 *  printed only once.  To display all the files prefixed with CONFIG/, the
 *  user must type "tfs ls CONFIG/\*".
 *
 *  Note: a file with a leading dot ('.') is invisible unless verbose is set.
 */
static int
tfsqlist(char *filter[], int verbose, int more)
{
    TFILE   *fp;
    char    dirname[TFSNAMESIZE+1], tmpname[TFSNAMESIZE+1];
    char    *name, fbuf[16], **fltrptr, *slash, *flags;
    int     idx, sidx, filelisted, err, sizetot;

    if((err = tfsreorder()) < 0) {
        return(err);
    }

    filelisted = 0;
    sizetot = 0;
    dirname[0] = 0;
    fltrptr = filter;
    printf(" Name                        Size   Location   Flags  Info\n");
    while(1) {
        idx = 0;
        while((fp = tfsAlist[idx])) {
            name = TFS_NAME(fp);
            if(((name[0] == '.') && (!verbose)) ||
                    (!listfilter(*fltrptr,name)) ||
                    ((fp->flags & TFS_UNREAD) && (TFS_USRLVL(fp) > getUsrLvl()))) {
                idx++;
                continue;
            }

            /* If name contains a slash, process it differently (but ignore */
            /* any leading slashes) */
            strcpy(tmpname,TFS_NAME(fp));
            for(sidx=0; sidx<TFSNAMESIZE; sidx++) {
                if(tmpname[sidx] != '/') {
                    break;
                }
            }
            slash = strchr(&tmpname[sidx],'/');
            if(slash && !*fltrptr) {
                char tmp;

                tmp = *(slash+1);
                *(slash+1) = 0;
                if(strcmp(dirname,tmpname)) {
                    filelisted++;
                    printf(" %-34s  (dir)\n",tmpname);
                    strcpy(dirname,tmpname);
                    *(slash+1) = tmp;
                } else {
                    idx++;
                    *(slash+1) = tmp;
                    continue;
                }
            } else {
                filelisted++;
                sizetot += TFS_SIZE(fp);
                flags = tfsflagsbtoa(TFS_FLAGS(fp),fbuf);
                if((!flags) || (!fbuf[0])) {
                    flags = " ";
                }
                printf(" %-23s  %7ld  0x%08lx  %-5s  %s\n",TFS_NAME(fp),
                       TFS_SIZE(fp),(ulong)(TFS_BASE(fp)),flags,TFS_INFO(fp));
            }
            idx++;
            if((more) && !(filelisted % more)) {
                if(!More()) {
                    return(TFS_OKAY);
                }
            }
        }
        /* If this or the next pointer is null, terminate loop now... */
        if(!*fltrptr) {
            break;
        }
        fltrptr++;
        if(!*fltrptr) {
            break;
        }
    }
    printf("\nTotal: %d item%s listed (%d bytes).\n",filelisted,
           filelisted == 1 ? "" : "s",sizetot);
    return (TFS_OKAY);
}

/* tfsrm():
 *  Remove all files that pass the incoming filter.
 *  This replaces the older tfs rm stuff in Tfs().
 */
static int
tfsrm(char *filter[])
{
    TFILE   *fp;
    char    *name, **fltrptr;
    int     idx, err, rmtot;

    if((err = tfsreorder()) < 0) {
        return(err);
    }

    fltrptr = filter;
    while(*fltrptr) {
        idx = rmtot = 0;
        while((fp = tfsAlist[idx])) {
            name = TFS_NAME(fp);
            if(listfilter(*fltrptr,name)) {
                if((err = tfsunlink(name)) != TFS_OKAY) {
                    printf("%s: %s\n",name,(char *)tfsctrl(TFS_ERRMSG,err,0));
                }
                rmtot++;
            }
            idx++;
        }
        /* This function will potentially delete many files, but if the */
        /* filter doesn't match at least one file, indicate that... */
        if(!rmtot) {
            printf("%s: file not found\n",*fltrptr);
        }
        fltrptr++;
    }

    return(TFS_OKAY);
}

/* tfscat():
 *  Print each character of the file until NULL terminate. Replace
 *  each instance of CR or LF with CRLF.
 */
static void
tfscat(TFILE *fp, int more)
{
    int i, lcnt;
    char    *cp;

    lcnt = 0;
    cp = (char *)(TFS_BASE(fp));
    for(i=0; i<fp->filsize; i++) {
        if(*cp == 0x1a) {   /* EOF or ctrl-z */
            break;
        }
        putchar(*cp);
        if((*cp == '\r') || (*cp == '\n')) {
            lcnt++;
            if(lcnt == more) {
                if(More() == 0) {
                    break;
                }
                lcnt = 0;
            }
        }
        cp++;
    }
}

int
dumpFhdr(TFILE *fhp)
{
    int     crcok;

    if(tfshdrcrc(fhp) == fhp->hdrcrc) {
        crcok = 1;
    } else {
        crcok = 0;
    }
    printf("hdrsize: 0x%04x (%s)\n",fhp->hdrsize,
           fhp->hdrsize == TFSHDRSIZ ? "ok" : "bad");
    printf("hdrvrsn: 0x%04x\n",fhp->hdrvrsn);
    printf("filsize: 0x%08lx\n",fhp->filsize);
    printf("flags  : 0x%08lx\n",fhp->flags);
#if 0
    printf("filcrc : 0x%08lx (%s)\n",fhp->filcrc,
           crc32((uchar *)(fhp+1),fhp->filsize) == fhp->filcrc ? "ok" : "bad");
#else
    printf("filcrc : 0x%08lx\n",fhp->filcrc);
#endif
    printf("hdrcrc : 0x%08lx (%s)\n",fhp->hdrcrc,crcok ? "ok" : "bad");
    printf("modtime: 0x%08lx\n",fhp->modtime);
    printf("next   : 0x%08lx\n",(ulong)fhp->next);
    if(crcok) {
        printf("name   : %s\n",fhp->name);
        printf("info   : %s\n",fhp->info ? fhp->info : "");
    } else {
        printf("name at: 0x%08lx\n",(ulong)fhp->name);
        printf("info at: 0x%08lx\n",(ulong)fhp->info);
    }
    printf("data at: 0x%08lx\n",(ulong)(fhp+1));
    return(TFS_OKAY);
}

char *TfsHelp[] = {
    "Tiny File System Interface",
    "-[df:i:mv] operation [args]...",
#if INCLUDE_VERBOSEHELP
    "",
    "Options:",
    " -d    tfs device",
    " -f    flags (see below)",
    " -i    info",
    " -m    enable more throttle",
    " -v    enable verbosity",
    "",
    "Operations (alphabetically):",
    " add {name} {src_addr} {size}, base {file} {var}, cat {name}",
    " cfg {start | restore} [{end} [spare_addr]], check [var], clean",
    " cp {from} {to_name | addr}, fhdr {addr}, freemem [var]",
    " info {file} {var}, init, ld[v] {name} [sname]",
    " log {on|off} {msg}, ln {src} {lnk}, ls [filter]",
    " qclean [ramstart] [ramlen], ramdev {name} {base} {size}",
    " rm {filter}, run {name}, size {file} {var}, stat",
    " trace [lvl], uname {prefix} {var}",
#if DEFRAG_TEST_ENABLED
    "",
    "Operations for testing TFS defragmentation:",
    " clean {E|S|F} {tag#} {sector#}",
    "   E=Exit, S=SectorErase F=FlashWrite",
    " rms {size} [exclude_file1] [exclude_file2] ...",
    " dhdr {address}",
    " dht",
#endif
    "",
    "Flags:",
    " E=exec_"
#if TFS_EBIN_AOUT
    "aout"
#elif TFS_EBIN_COFF
    "coff"
#elif TFS_EBIN_ELF
    "elf"
#elif TFS_EBIN_MSBIN
    "msbin"
#elif TFS_EBIN_ELFMSBIN
    "elf|msbin"
#else
#error: No TFS EBIN format specified.
#endif
    ", e=exec_script, c=compressed, l=symlink",
    " b=run_at_boot, B=qry_run_at_boot, i=inplace_modifiable",
    " 0-3=usrlvl_0-3, u=ulvl_unreadable",
#endif
    0,
};

/* Tfs():
 *  Entry point for the tfs command through the monitor's command line
 *  interface.  This function provides access to most TFS functionality
 *  through the CLI.
 */

int
Tfs(int argc, char *argv[])
{
    TDEV    *tdp, *tdptmp;
    TFILE   *fp;
    TINFO   tinfo;
    long    size;
    int     err, opt, verbose, i, more, retval, status;
    char    *src, *name, *info, *flags, *to, *from, *devprefix;
    char    *arg1, *arg2, *arg3, *arg4;

    tdp = 0;
    more = 0;
    retval = CMD_SUCCESS;
    verbose = 0;
    status = TFS_OKAY;
    info = (char *)0;
    flags = (char *)0;
    devprefix = (char *)0;
    while((opt = getopt(argc, argv, "d:vmf:i:")) != -1) {
        switch(opt) {
        case 'd':
            devprefix = optarg;
            tdp = gettfsdev_fromprefix(devprefix,1);
            if(!tdp) {
                return(CMD_FAILURE);
            }
            break;
        case 'f':
            flags = optarg;
            break;
        case 'i':
            info = optarg;
            break;
        case 'm':
            more++;
            break;
        case 'v':
            verbose++;
            break;
        default:
            return(CMD_PARAM_ERROR);
        }
    }

    arg1 = argv[optind];
    arg2 = argv[optind+1];
    arg3 = argv[optind+2];
    arg4 = argv[optind+3];

    if(argc == optind) {
        retval = CMD_PARAM_ERROR;
    } else if(strcmp(arg1, "trace") == 0) {
        if(argc == (optind+2)) {
            tfsTrace = strtoul(arg2,0,0);
        } else if(argc == (optind+1)) {
            printf("Current trace mode: 0x%lx\n",tfsTrace);
        } else {
            retval = CMD_PARAM_ERROR;
        }
    } else if(strcmp(arg1, "init") == 0) {
        if(getUsrLvl() < MAXUSRLEVEL) {
            status = showTfsError(TFSERR_USERDENIED,0);
        } else {
            status = _tfsinit(tdp);
            showTfsError(status,"init");
            if(status == TFS_OKAY) {
                tfsclear(tdp);
            }
        }
    } else if((strcmp(arg1,"ramdev") == 0) && (argc == (optind+4))) {
        err = tfsramdevice(arg2,strtol(arg3,0,0),strtol(arg4,0,0));
        if(err != TFS_OKAY) {
            printf("%s: %s\n",arg2,(char *)tfsctrl(TFS_ERRMSG,err,0));
            retval = CMD_FAILURE;
        }
    } else if(strcmp(arg1, "log") == 0) {
        retval = tfsLogCmd(argc,argv,optind);
    } else if(strcmp(arg1, "cfg") == 0) {
        /* args:
         * tfsstart tfsend spare_address
         */
        ulong start, end, spare;

        if((argc == (optind+2)) && !strcmp(arg2,"restore")) {
            status = tfscfgrestore();
            showTfsError(status,"cfg");
        } else if((argc == (optind+3)) || (argc == (optind+4))) {
            start = strtol(arg2,0,0);
            end = strtol(arg3,0,0);
            if(argc == (optind+4)) {
                spare = strtol(arg4,0,0);
            } else {
                spare = 0xffffffff;
            }
            status = tfscfg(devprefix,start,end,spare);
            showTfsError(status,"cfg");
        } else {
            retval = CMD_PARAM_ERROR;
        }
    } else if(strcmp(arg1, "stat") == 0) {
        char buf[32];
        int  opencnt;
        struct  tfsdat *slot;

        /* Display current TFS memory usage: */
        tfsmemuse(tdp,&tinfo,1);
        printf("TFS Hdr size: %d\n",TFSHDRSIZ);

        /* Display currently opened files: */
        opencnt = 0;
        slot = tfsSlots;
        for(i=0; i<TFS_MAXOPEN; i++,slot++) {
            if(slot->offset >= 0) {
                printf("%3d: 0x%08lx %s %s\n",i,(ulong)(slot->base),
                       tfsmodebtoa(slot->flagmode,buf),slot->hdr.name);
                opencnt++;
            }
        }
        printf("Total files currently opened: %d\n",opencnt);
    } else if(strcmp(arg1, "freemem") == 0) {
        char *prefix;

        tfsmemuse(tdp,&tinfo,0);
        if(tdp) {
            prefix = tdp->prefix;
        } else {
            prefix = "";
        }
        if(argc == (optind+1)) {
            printf("0x%x (%d) bytes available to TFS %s\n",
                   tinfo.memfordata,tinfo.memfordata,prefix);
        } else if(argc == (optind+2)) {
            shell_sprintf(arg2,"0x%x",tinfo.memfordata);
        } else {
            retval = CMD_PARAM_ERROR;
        }
    }
#if DEFRAG_TEST_ENABLED
    else if(strcmp(arg1, "dht") == 0) {
        if(argc == (optind+1)) {
            status = dumpDhdrTbl(0,0);
            showTfsError(status,"dht");
        } else if(argc == (optind+3)) {
            status = dumpDhdrTbl((DEFRAGHDR *)strtol(arg2,0,0),
                                 atoi(arg3));
            showTfsError(status,"dht");
        } else {
            retval = CMD_PARAM_ERROR;
        }
    } else if((strcmp(arg1, "dhdr") == 0) && (argc == (optind+2))) {
        status = dumpDhdr((DEFRAGHDR *)strtol(arg2,0,0));
        showTfsError(status,"dhdr");
    }
#endif
    else if((strcmp(arg1, "fhdr") == 0) && (argc == (optind+2))) {
        status = dumpFhdr((TFILE *)strtol(arg2,0,0));
        showTfsError(status,"fhdr");
    } else if(((strcmp(arg1, "base") == 0) ||
               (strcmp(arg1, "info") == 0) ||
               (strcmp(arg1, "size") == 0)) && (argc == (optind+3))) {
        fp = tfsstat(arg2);
        if((!fp) ||
                ((fp->flags & TFS_UNREAD) && (TFS_USRLVL(fp) > getUsrLvl()))) {
            setenv(arg3,0);
        } else {
            switch(arg1[0]) {
            case 'b':
                shell_sprintf(arg3,"0x%x",(char *)fp+TFSHDRSIZ);
                break;
            case 'i':
                setenv(arg3,fp->info);
                break;
            case 's':
                shell_sprintf(arg3,"%ld",fp->filsize);
                break;
            }
        }
    } else if((strcmp(arg1, "uname") == 0) && (argc == (optind+3))) {
        char uname[TFSNAMESIZE];
        for(i=0;; i++) {
            sprintf(uname,"%s%d",arg2,i);
            if(!tfsstat(uname)) {
                setenv(arg3,uname);
                break;
            }
        }
    } else if(strcmp(arg1, "ls") == 0) {
        if(verbose > 1) {
            status = tfsvlist(&argv[optind+1],verbose,more << 1);
        } else {
            status = tfsqlist(&argv[optind+1],verbose,more << 3);
        }

        showTfsError(status,"ls");
    } else if((strcmp(arg1, "ln") == 0) && (argc == optind+3)) {
        from = arg2;
        to = arg3;
        status = tfslink(from,to);
        showTfsError(status,from);
    } else if((strcmp(arg1, "cp") == 0) && (argc == (optind+3))) {
        char    buf[16], linfo[TFSINFOSIZE];

        from = arg2;
        to = arg3;
        fp = tfsstat(from);
        if(fp) {
            if(flags) {
                if(strcmp(flags,"0") == 0) {
                    flags = (char *)0;
                }
            } else {
                flags = tfsflagsbtoa(fp->flags,buf);
            }
            if(!info) {
                strcpy(linfo,fp->info);
            } else {
                strcpy(linfo,info);
            }
            if((fp->flags & TFS_UNREAD) &&
                    (TFS_USRLVL(fp) > getUsrLvl())) {
                status = showTfsError(TFSERR_USERDENIED,from);
            } else if(to[0] == '0' && to[1] == 'x') {
                memcpy((char *)strtol(to,0,16),TFS_BASE(fp),TFS_SIZE(fp));
                flushDcache((char *)strtol(to,0,16), TFS_SIZE(fp));
                invalidateIcache((char *)strtol(to,0,16), TFS_SIZE(fp));
            } else {
                int freespace;

                /* First check to see if a defrag is needed...
                 */
                freespace = tfsctrl(TFS_MEMAVAIL,0,0);
                if(freespace <= TFS_SIZE(fp)) {
                    tfsctrl(TFS_DEFRAG,0,0);
                    fp = tfsstat(from);
                }
                status = tfsadd(to,linfo,flags,(uchar *)TFS_BASE(fp),
                                TFS_SIZE(fp));
                showTfsError(status,to);
            }
        } else {
            status = showTfsError(TFSERR_NOFILE,from);
        }
    } else if((strcmp(arg1, "cat") == 0) && (argc >= (optind+2))) {
        for(i=optind+1; i<argc; i++) {
            name = argv[i];
            fp = tfsstat(name);
            if(fp) {
                if((fp->flags & TFS_UNREAD) &&
                        (TFS_USRLVL(fp) > getUsrLvl())) {
                    status = showTfsError(TFSERR_USERDENIED,name);
                } else {
                    tfscat(fp,more << 3);   /* more times 8 */
                }
            } else {
                status = showTfsError(TFSERR_NOFILE,name);
            }
        }
    }
#if DEFRAG_TEST_ENABLED
    /* rms: "remove space".  Used for testing only... Keep removing files
     * until the number of bytes freed up is greater than the argument
     * to rms. Skip over all files listed after the first size argument.
     * Arglist to tfs rms is...
     *
     *  tfs rms {SIZE} [EXCLUDEFILE1] [EXCLUDEFILE2] ...
     *  where SIZE is the amount of space to freeup and EXCLUDEFILEN is
     *  a filename that is not to be removed.
     */
    else if((strcmp(arg1, "rms") == 0) && (argc >= (optind+2))) {
        int insize, totsize;

        totsize = 0;
        insize = strtol(arg2,0,0);
        tfsreorder();
        for(i=0; tfsAlist[i]; i++) {
            int j;

            for(j=optind+1; j<argc; j++) {
                if(!strcmp(TFS_NAME(tfsAlist[i]),argv[j])) {
                    break;
                }
            }
            if(j != argc) {
                continue;
            }
            if(devprefix &&
                    strncmp(TFS_NAME(tfsAlist[i]),devprefix,strlen(devprefix))) {
                continue;
            }
            totsize += (TFS_SIZE(tfsAlist[i]) + TFSHDRSIZ);
            if(verbose)
                printf("rms: removing %s (%ld)\n",TFS_NAME(tfsAlist[i]),
                       TFS_SIZE(tfsAlist[i]) + TFSHDRSIZ);
            _tfsunlink(TFS_NAME(tfsAlist[i]));
            if(totsize > insize) {
                break;
            }
            totsize += TFSHDRSIZ;
        }
    }
#endif
    else if((strcmp(arg1, "rm") == 0) && (argc >= (optind+2))) {
        status = tfsrm(&argv[optind+1]);
        showTfsError(status,0);
    } else if(strcmp(arg1, "fix") == 0) {
        tfsfixup(verbose,1);
    } else if(strcmp(arg1, "qclean") == 0) {
        char *ramstart = 0;
        ulong ramlen = 0;

        if(argc >= optind+2) {
            ramstart = (char *)strtoul(arg2,0,0);
        }
        if(argc >= optind+3) {
            ramlen = strtoul(arg2,0,0);
        }

        for(tdptmp=tfsDeviceTbl; tdptmp->start != TFSEOT; tdptmp++) {
            if(!tdp || (tdp == tdptmp)) {
                status = tfsclean_nps(tdptmp,ramstart,ramlen);
                showTfsError(status,tdptmp->prefix);
            }
        }
    } else if(strcmp(arg1, "clean") == 0) {
        int otrace;

#if DEFRAG_TEST_ENABLED
        DefragTestPoint = DefragTestSector = DefragTestType = 0;
        if(argc == (optind+4)) {
            if(arg2[0] == 'S') {
                DefragTestType = DEFRAG_TEST_SERASE;
            } else if(arg2[0] == 'F') {
                DefragTestType = DEFRAG_TEST_FWRITE;
            } else if(arg2[0] == 'E') {
                DefragTestType = DEFRAG_TEST_EXIT;
            } else {
                return(CMD_PARAM_ERROR);
            }
            DefragTestPoint = atoi(arg3);
            DefragTestSector = atoi(arg4);
            printf("Testtype=%c, Testpoint=%d, Testsector=%d\n",
                   arg2[0],DefragTestPoint,DefragTestSector);
        } else
#endif
            if(argc != optind+1) {
                return(CMD_PARAM_ERROR);
            }

        /* If verbosity request is extreme, then turn on TFS trace also...
         */
        otrace = tfsTrace;
        if(verbose >= 5) {
            tfsTrace = 99;
        }

        /* If tdp has been set by the -d option, only defrag the affected
         * device; else, defrag all devices...
         */
        for(tdptmp=tfsDeviceTbl; tdptmp->start != TFSEOT; tdptmp++) {
            if(!tdp || (tdp == tdptmp)) {
                status = tfsclean(tdptmp,verbose+1);
                showTfsError(status,tdptmp->prefix);
            }
        }
        tfsTrace = otrace;
    } else if(strcmp(arg1, "check") == 0) {
        if(argc == optind+1) {
            verbose = 1;
        }

        if(tfscheck(tdp,verbose) != TFS_OKAY) {
            status = TFSERR_CORRUPT;
        }

        /* If an additional argument is provided after the "check" command
         * then assume it is a shell variable name that is to be populated
         * with the pass/fail status of the check...
         */
        if(argc == optind+2) {
            setenv(arg2,status == TFS_OKAY ? "PASS" : "FAIL");
        }
    } else if((!strncmp(arg1, "ld",2)) &&
              ((argc == optind+2) || (argc == optind+3))) {
        int vfy = 0;

        if(arg1[2] == 'v') {
            vfy = 1;
        } else if(arg1[2] != 0) {
            return(CMD_PARAM_ERROR);
        }
        status = tfsld(arg2,verbose,arg3,vfy);
        showTfsError(status,arg2);
    } else if((strcmp(arg1, "run") == 0) && (argc >= (optind+2))) {
        status = tfsrun(&argv[optind+1],verbose);
        showTfsError(status,arg2);
    } else if((!(strcmp(arg1, "add"))) && (argc == (optind+4))) {
        src = (char *) strtol(arg3, (char **) 0, 0);
        size = strtol(arg4, (char **) 0, 0);
        status = tfsadd(arg2, info, flags, (uchar *)src, size);
        showTfsError(status,arg2);
        if(status != TFS_OKAY) {
            retval = CMD_FAILURE;
        }
    } else {
        retval = CMD_PARAM_ERROR;
    }
    return(retval);
}
#endif
