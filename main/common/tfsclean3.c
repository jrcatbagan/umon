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
 * tfsclean3.c:
 *
 *  --- NOT READY YET ---
 *
 * This version of defragmentation is power-hit safe and requires
 * that there be double the amount of flash as is needed for use by
 * TFS.  The basic idea is similar to tfsclean2.c...
 * Copy all of the good files over to the "other" flash bank, then have
 * TFS use the "other" bank as the storage area.
 * The idea is that the defrag is simply a copy of the good stuff to
 * the alternate flash block.  This requires that after the
 * good stuff is copied, the now-dirty flash block must be erased in
 * the background prior to the next tfsclean() call.  The fact that
 * there is no sector erase is what makes this faster.
 *
 * If both of these flash banks are in the same flash device, then
 * having a background erase in progress means that it must be an
 * interruptible erase (device specific).  This is necessary because
 * while the background erase is in progress there may be a need to
 * interact with the flash and most devices don't let you do both at the
 * same time.
 *
 * Note that this "background-erase" is what makes this method the
 * fastest defrag method.  It does require that the erase operation be
 * interruptible, and it requires that the application will provide the
 * hooks to do this...
 *
 * Original author:     Ed Sutter (ed.sutter@alcatel-lucent.com)
 *
 */
#include "config.h"
#include "cpu.h"
#include "stddefs.h"
#include "genlib.h"
#include "tfs.h"
#include "tfsprivate.h"
#include "flash.h"
#include "monflags.h"

#if INCLUDE_TFS

int
tfsfixup(int verbose, int dontquery)
{
    return(TFSERR_NOTAVAILABLE);
}

#if DEFRAG_TEST_ENABLED
int
dumpDhdr(DEFRAGHDR *dhp)
{
    return(TFSERR_NOTAVAILABLE);
}

int
dumpDhdrTbl(DEFRAGHDR *dhp, int ftot)
{
    return(TFSERR_NOTAVAILABLE);
}
#endif


/* _tfsclean():
 *  This is an alternative to the complicated defragmentation above.
 *  It simply scans through the file list and copies all valid files
 *  to RAM; then flash is erased and the RAM is copied back to flash.
 *  <<< WARNING >>>
 *  THIS FUNCTION SHOULD NOT BE INTERRUPTED AND IT WILL BLOW AWAY
 *  ANY APPLICATION CURRENTLY IN CLIENT RAM SPACE.
 */
int
_tfsclean(TDEV *tdp, int notused, int verbose)
{
    TFILE   *tfp;
    uchar   *tbuf;
    ulong   appramstart;
    int     dtot, nfadd, len, err, chkstat;

    if(TfsCleanEnable < 0) {
        return(TFSERR_CLEANOFF);
    }

    appramstart = getAppRamStart();

    /* Determine how many "dead" files exist. */
    dtot = 0;
    tfp = (TFILE *)tdp->start;
    while(validtfshdr(tfp)) {
        if(!TFS_FILEEXISTS(tfp)) {
            dtot++;
        }
        tfp = nextfp(tfp,tdp);
    }

    if(dtot == 0) {
        return(TFS_OKAY);
    }

    printf("Reconstructing device %s with %d dead file%s removed...\n",
           tdp->prefix, dtot,dtot>1 ? "s":"");

    tbuf = (char *)appramstart;
    tfp = (TFILE *)(tdp->start);
    nfadd = tdp->start;
    while(validtfshdr(tfp)) {
        if(TFS_FILEEXISTS(tfp)) {
            len = TFS_SIZE(tfp) + sizeof(struct tfshdr);
            if(len % TFS_FSIZEMOD) {
                len += TFS_FSIZEMOD - (len % TFS_FSIZEMOD);
            }
            nfadd += len;
            if(s_memcpy(tbuf,(uchar *)tfp,len,0,0) != 0) {
                return(TFSERR_MEMFAIL);
            }

            ((struct tfshdr *)tbuf)->next = (struct tfshdr *)nfadd;
            tbuf += len;
        }
        tfp = nextfp(tfp,tdp);
    }

    /* Erase the flash device: */
    err = _tfsinit(tdp);
    if(err != TFS_OKAY) {
        return(err);
    }

    /* Copy data placed in RAM back to flash: */
    err = AppFlashWrite((ulong *)(tdp->start),(ulong *)appramstart,
                        (tbuf-(uchar *)appramstart));
    if(err < 0) {
        return(TFSERR_FLASHFAILURE);
    }

    /* All defragmentation is done, so verify sanity of files... */
    chkstat = tfscheck(tdp,verbose);

    return(chkstat);
}
#endif
