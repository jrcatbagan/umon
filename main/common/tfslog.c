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
 * tfslog.c:
 *
 * Optional facility for TFS that supports the ability to log all
 * tfs actions that affect flash, to a log file.
 *
 * This function is called by tfsadd(), tfsunlink() and tfsipmod() to
 * write to a log file indicating that something in tfs has been changed.
 * Note that the function can be called at any user level, but it must be
 * able to modify the TFS_CHANGELOG_FILE that has "u3" flags.  The
 * user level must be temporarily forced to MAXUSRLEVEL for this.
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

#if TFS_CHANGELOG_SIZE
static int  tfsLogging;
#endif

void
tfslog(int action, char *string)
{
#if TFS_CHANGELOG_SIZE
    static char *tfslogaction[] = { "ADD", "DEL", "IPM", " ON", "OFF" };

    extern  void *setTmpMaxUsrLvl();
    static  char buf[TFS_CHANGELOG_SIZE];
    TFILE   *tfp;
    int (*fptr)();
    char    *eol, *eob, *logaction, tbuf[32];
    int     newfsize, fsize, lsize, tfd, err, len, tbuflen;

    switch(action) {
    case TFSLOG_ADD:        /* Return here if logging is off,   */
    case TFSLOG_DEL:        /* or this tfslog() call is on the  */
    case TFSLOG_IPM:        /* TFS_CHANGELOG_FILE itself.       */
        if(!tfsLogging || !strcmp(string,TFS_CHANGELOG_FILE)) {
            return;
        }
        break;
    case TFSLOG_ON:
        if(tfsLogging == 1) {
            return;
        }
        tfsLogging = 1;
        break;
    case TFSLOG_OFF:
        if(tfsLogging == 0) {
            return;
        }
        tfsLogging = 0;
        break;
    }

    /* Force the getUsrLvl() function to return MAX: */
    fptr = (int(*)())setTmpMaxUsrLvl();

    logaction = tfslogaction[action];
    tfp = tfsstat(TFS_CHANGELOG_FILE);
    tfsGetAtime(0,tbuf,sizeof(tbuf));
    tbuflen = strlen(tbuf);

    if(tfp) {
        tfd = tfsopen(TFS_CHANGELOG_FILE,TFS_RDONLY,0);
        fsize = tfsread(tfd,buf,TFS_CHANGELOG_SIZE);
        tfsclose(tfd,0);

        newfsize = (fsize+strlen(logaction)+strlen(string)+3);
        if(tbuflen) {
            newfsize += tbuflen + 3;
        }

        eob = buf + fsize;

        /* If newfsize is greater than the maximum size the file is
         * allowed to grow, then keep removing the first line
         * (oldest entry) until new size is within the limit...
         */
        if(newfsize > TFS_CHANGELOG_SIZE) {
            lsize = 0;
            eol = buf;
            while((newfsize-lsize) > TFS_CHANGELOG_SIZE) {
                while((*eol != '\r') && (*eol != '\n')) {
                    eol++;
                }
                while((*eol == '\r') || (*eol == '\n')) {
                    eol++;
                }
                lsize = eol-buf;
            }
            fsize -= lsize;
            newfsize -= lsize;
            eob -= lsize;
            memcpy(buf,eol,fsize);
        }
        if(tbuflen) {
            sprintf(eob,"%s: %s @ %s\n",logaction,string,tbuf);
        } else {
            sprintf(eob,"%s: %s\n",logaction,string);
        }
        err = _tfsunlink(TFS_CHANGELOG_FILE);
        if(err < 0)
            printf("%s: %s\n",TFS_CHANGELOG_FILE,
                   (char *)tfsctrl(TFS_ERRMSG,err,0));
        err = tfsadd(TFS_CHANGELOG_FILE,0,"u3",buf,newfsize);
        if(err < 0)
            printf("%s: %s\n",TFS_CHANGELOG_FILE,
                   (char *)tfsctrl(TFS_ERRMSG,err,0));
    } else {
        if(tbuflen) {
            len = sprintf(buf,"%s: %s @ %s\n",logaction,string,tbuf);
        } else {
            len = sprintf(buf,"%s: %s\n",logaction,string);
        }
        err = tfsadd(TFS_CHANGELOG_FILE,0,"u3",buf,len);
        if(err < 0)
            printf("%s: %s\n",TFS_CHANGELOG_FILE,
                   (char *)tfsctrl(TFS_ERRMSG,err,0));
    }

    /* Restore the original getUsrLvl() functionality: */
    clrTmpMaxUsrLvl(fptr);
#endif
}

int
tfsLogCmd(int argc,char *argv[], int optind)
{
#if TFS_CHANGELOG_SIZE
    int status;
    int retval = 0;

    if(getUsrLvl() < MAXUSRLEVEL) {
        status = showTfsError(TFSERR_USERDENIED,0);
    } else {
        if(argc == optind + 3) {
            if(!strcmp(argv[optind+1],"on")) {
                tfslog(TFSLOG_ON,argv[optind+2]);
            } else if(!strcmp(argv[optind+1],"off")) {
                tfslog(TFSLOG_OFF,argv[optind+2]);
            } else {
                retval = CMD_PARAM_ERROR;
            }
        } else if(argc == optind + 1) {
            printf("TFS logging %sabled\n",tfsLogging ? "en" : "dis");
        } else {
            retval = CMD_PARAM_ERROR;
        }
    }
    return(retval);
#else
    return(CMD_FAILURE);
#endif
}
