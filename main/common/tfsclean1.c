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
 * tfsclean1.c:
 *
 * This is one of several different versions of tfsclean().  This version
 * is by far the most complex, but offers power-hit safety and minimal
 * flash overhead.  It uses a "spare" sector to backup the
 * "one-sector-at-a-time" defragmentation process.
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
#include "warmstart.h"

#if INCLUDE_TFS

#if !INCLUDE_FLASH
#error: If flash is not included, then use tfsclean2.c
#endif

#if DEFRAG_TEST_ENABLED
void
defragExitTestPoint(int val)
{
    if((DefragTestType == DEFRAG_TEST_EXIT) && (DefragTestPoint == val)) {
        printf("\n+++++++++ EXIT @ TEST POINT(%d)\n",val);
        CommandLoop();
    }
}
#else
#define defragExitTestPoint(val)
#endif

/* Variables for testing tfsclean():
 * They are set up through arguments to "tfs clean" in tfscli.c.
 */
int DefragTestType;
int DefragTestPoint;
int DefragTestSector;

/* defragTick():
 * Used to show progress, just to let the user know that we aren't
 * dead in the water.
 */
static void
defragTick(int verbose)
{
    static int tick;
    static char clockhand[] = { '|', '/', '-', '\\' };

    if(!verbose && (!MFLAGS_NODEFRAGPRN())) {
        if(tick > 3) {
            tick = 0;
        }
        printf("%c\b",clockhand[tick++]);
    }
}

/* defragCrcTable():
 * Return a pointer to the crc table for the specified TFS device.
 */
struct sectorcrc *
defragCrcTable(TDEV *tdp)
{
    return((struct sectorcrc *)(tdp->end+1) - tdp->sectorcount);
}

/* defragSerase():
 * Common function to call from within tfsclean() to erase a sector
 * and generate an error message if necessary.
 *
 * If DEFRAG_TEST_ENABLED is defined and the type/sector/point criteria
 * is met, then instead of erasing the sector; just change the first non-zero
 * byte to zero to corrupt it.  This essentially makes it a corrupted erase.
 */
static int
defragSerase(int tag, int snum)
{
    int ret = 0;

#if DEFRAG_TEST_ENABLED
    int ssize;
    uchar *sbase, *send, zero;

    printf("     serase_%02d(%02d)\n",tag,snum);

    if((DefragTestType == DEFRAG_TEST_SERASE) &&
            (DefragTestSector == snum) && (DefragTestPoint == tag)) {
        sectortoaddr(snum,&ssize,&sbase);
        send = sbase+ssize;
        zero = 0;
        while(sbase < send) {
            if(*sbase != 0) {
                tfsflashwrite((ulong *)sbase,(ulong *)&zero,1);
                break;
            }
            sbase++;
        }
        printf("DEFRAG_TEST_SERASE activated @ %d sector %d\n",tag,snum);
        CommandLoop();
    } else
#endif
        ret = tfsflasherase(snum);
    if(ret <= 0) {
        printf("tfsclean() serase erase failed: %d,%d,%d\n",snum,tag,ret);
    }
    return(ret);
}

/* defragFwrite():
 * Common function to call from within tfsclean() to write to flash
 * and generate an error message if necessary.
 * If DEFRAG_TEST_ENABLED is defined and the test type is set to
 * DEFRAG_TEST_FWRITE, then use APPRAMBASE as the source of the data
 * so that the end result is an errored flash write.
 */
static int
defragFwrite(int tag, uchar *dest,uchar *src,int size)
{
    int ret = 0;

#if DEFRAG_TEST_ENABLED
    int snum;

    addrtosector((char *)dest,&snum,0,0);
    printf("     fwrite_%02d(%d,0x%lx,0x%lx,%d)\n",
           tag,snum,(ulong)dest,(ulong)src,size);

    if((DefragTestType == DEFRAG_TEST_FWRITE) &&
            (DefragTestSector == snum) && (DefragTestPoint == tag)) {
        tfsflashwrite((ulong *)dest,(ulong *)getAppRamStart(),size/2);
        printf("DEFRAG_TEST_FWRITE activated @ %d sector %d\n",tag,snum);
        CommandLoop();
    } else
#endif
        ret = tfsflashwrite(dest,src,size);
    if(ret != TFS_OKAY) {
        printf("tfsclean() fwrite failed: 0x%lx,0x%lx,%d,%d\n",
               (ulong)dest,(ulong)src,size,tag);
    }
    return(ret);
}

/* defragGetSpantype():
 * With the incoming sector base and end (s_base, s_end),
 * determine the type of span that the incoming file (f_base, f_end)
 * has across it.  There are six different ways the spanning can
 * occur:
 *   1. begin and end in previous active sector (bpep);
 *   2. begin in previously active sector, end in this one (bpec);
 *   3. begin in previously active sector, end in later one (bpel);
 *   4. begin and end in this active sector (bcec);
 *   5. begin in this active sector, end in later one (bcel);
 *   6. begin and end in later active sector (blel);
 */
static int
defragGetSpantype(char *s_base,char *s_end,char *f_base,char *f_end)
{
    int spantype;

    if(f_base < s_base) {
        if((f_end > s_base) && (f_end <= s_end)) {
            spantype = SPANTYPE_BPEC;
        } else if(f_end > s_end) {
            spantype = SPANTYPE_BPEL;
        } else {
            spantype = SPANTYPE_BPEP;
        }
    } else {
        if(f_base > s_end) {
            spantype = SPANTYPE_BLEL;
        } else if(f_end <= s_end) {
            spantype = SPANTYPE_BCEC;
        } else {
            spantype = SPANTYPE_BCEL;
        }
    }
    return(spantype);
}

/* defragGetSpantypeStr():
 * Return a string that corresponds to the incoming state value.
 */
static char *
defragGetSpantypeStr(int spantype)
{
    char *str;

    switch(spantype) {
    case SPANTYPE_BPEC:
        str = "BPEC";
        break;
    case SPANTYPE_BLEL:
        str = "BLEL";
        break;
    case SPANTYPE_BPEL:
        str = "BPEL";
        break;
    case SPANTYPE_BPEP:
        str = "BPEP";
        break;
    case SPANTYPE_BCEC:
        str = "BCEC";
        break;
    case SPANTYPE_BCEL:
        str = "BCEL";
        break;
    default:
        str = "???";
        break;
    }
    return(str);
}

/* defragEraseSpare():
 * Erase the spare sector associated with the incoming TFS device.
 * The underlying flash driver SHOULD have a check so that it only
 * erases the sector if the sector is not already erased, so this
 * extra check (call to flasherased()) may not be necessary in
 * most cases.
 */
static int
defragEraseSpare(TDEV *tdp)
{
    int snum, ssize;
    uchar *sbase;

    if(addrtosector((unsigned char *)tdp->spare,&snum,&ssize,&sbase) < 0) {
        return(TFSERR_FLASHFAILURE);
    }

    if(!flasherased(sbase,sbase+(ssize-1))) {
        if(defragSerase(1,snum) < 0) {
            return(TFSERR_FLASHFAILURE);
        }
    }
    return(TFS_OKAY);
}

/* defragValidDSI():
 * Test to see if we have a valid defrag state information (DSI)
 * area.  The DSI area, working back from tdp->end, consists of a
 * table of 32-bit crcs (one per sector), a table of defraghdr
 * structures (one per active file) and a 32-bit crc of the DSI
 * itself.  Knowing this format, we can easily step backwards into
 * the DSI space to see if it all makes sense.
 * If the table is 100% valid, then we will be able to step
 * backwards through the DSI to find the 32-bit crc of the DSI area.
 * If it matches, then we can be sure that the DSI is valid.
 * Return total number of files in header if the defrag header
 * table appears to be sane, else 0.
 */
static int
defragValidDSI(TDEV *tdp, struct sectorcrc **scp)
{
    int     ftot, valid, lastssize;
    uchar   *lastsbase;
    struct  sectorcrc *crctbl;
    ulong   hdrcrc, *crc;
    struct  defraghdr   *dhp, dfhcpy;

    ftot = valid = 0;
    crctbl = defragCrcTable(tdp);
    dhp = (struct defraghdr *)crctbl - 1;
    /* next line was <dfhcpy = *dhp> ... */
    memcpy((char *)&dfhcpy,(char *)dhp,sizeof(struct defraghdr));
    hdrcrc = dfhcpy.crc;
    dfhcpy.crc = 0;
    if(crc32((uchar *)&dfhcpy,DEFRAGHDRSIZ) == hdrcrc) {
        ftot = dhp->idx + 1;
        dhp = (struct defraghdr *)crctbl - ftot;
        crc = (ulong *)dhp - 1;
        if(crc32((uchar *)dhp,(uchar *)tdp->end-(uchar *)dhp) == *crc) {
            if(scp) {
                *scp = crctbl;
            }
            return(ftot);
        }
    }

    /* It's possible that the DSI space has been relocated to the spare
     * sector, so check for that here...
     */
    addrtosector((unsigned char *)tdp->end,0,&lastssize,&lastsbase);

    crctbl = ((struct sectorcrc *)(tdp->spare+lastssize) - tdp->sectorcount);
    dhp = (struct defraghdr *)crctbl - 1;
    /* next line was <dfhcpy = *dhp> ... */
    memcpy((char *)&dfhcpy,(char *)dhp,sizeof(struct defraghdr));
    hdrcrc = dfhcpy.crc;
    dfhcpy.crc = 0;
    if(crc32((uchar *)&dfhcpy,DEFRAGHDRSIZ) == hdrcrc) {
        ftot = dhp->idx + 1;
        dhp = (struct defraghdr *)crctbl - ftot;
        crc = (ulong *)dhp - 1;
        if(crc32((uchar *)dhp,
                 (uchar *)(tdp->spare+lastssize-1) - (uchar *)dhp) == *crc) {
#if DEFRAG_TEST_ENABLED
            printf("TFS: DSI in spare\n");
#endif
            if(scp) {
                *scp = crctbl;
            }
            return(ftot);
        }
    }
    return(0);
}

/* defragSectorInSpare():
 * For each sector, run a CRC32 on the content of the spare
 * using the size of the sector in question.  If the calculated
 * crc matches that of the table, then we have located the sector
 * that has been copied to the spare.
 * This is a pain in the butt because we can't just run a CRC32 on
 * the spare sector itself because the size of the spare may not match
 * the size of the sector that was copied to it.  The source sector
 * might have been smaller; hence when we calculate the CRC32, we need
 * to use the size of the potential source sector.
 */
static int
defragSectorInSpare(TDEV *tdp, struct sectorcrc *crctbl)
{
    uchar   *sbase;
    struct  defraghdr *dhp;
    int     i, ssize, snum, ftot;

    sbase = (uchar *)tdp->start;
    dhp = (struct defraghdr *)crctbl - 1;
    ftot = dhp->idx + 1;

    for(i=0; i<tdp->sectorcount; i++) {
        addrtosector(sbase,&snum,&ssize,0);
        if(i == tdp->sectorcount - 1) {
            ssize -=                            /* CRC table */
                (tdp->sectorcount * sizeof(struct sectorcrc));
            ssize -= (ftot * DEFRAGHDRSIZ);     /* DHT table */
            ssize -= 4;                         /* Crc of the tables */
        }
        if(crc32((uchar *)tdp->spare,ssize) == crctbl[i].precrc) {
            return(snum);
        }
        sbase += ssize;
    }
    return(-1);
}

/* defragTouchedSectors():
 * Step through the crc table and TFS flash space to find the first
 * and last sectors that have been touched by defragmentation.
 * This is used by defragGetState() to recover from an interrupted
 * defragmentation, so a few verbose messages are useful to indicate
 * status to the user.
 */
void
defragTouchedSectors(TDEV *tdp,int *first, int *last)
{
    uchar   *sbase;
    struct  defraghdr *dhp;
    struct  sectorcrc   *crctbl;
    int     i, ssize, snum, ftot;

    *first = -1;
    *last = -1;
    sbase = (uchar *)tdp->start;
    crctbl = defragCrcTable(tdp);
    dhp = (struct defraghdr *)crctbl - 1;
    ftot = dhp->idx + 1;

    printf("TFS: calculating per-sector crcs... ");
    for(i=0; i<tdp->sectorcount; i++) {
        addrtosector(sbase,&snum,&ssize,0);
        if(i == tdp->sectorcount - 1) {
            ssize -=                            /* CRC table */
                (tdp->sectorcount * sizeof(struct sectorcrc));
            ssize -= (ftot * DEFRAGHDRSIZ);     /* DHT table */
            ssize -= 4;                         /* Crc of the tables */
        }
        if(crc32(sbase,ssize) != crctbl[i].precrc) {
            if(*first == -1) {
                *first = snum;
            }
            *last = snum;
        }
        sbase += ssize;
        defragTick(0);
    }
    printf("done\n");
    return;
}

/* defragPostCrcCheck():
 * Return 1 if the post-crc check of the incoming sector number passes;
 * else 0.
 */
int
defragPostCrcCheck(TDEV *tdp, int isnum)
{
    uchar   *sbase;
    struct  defraghdr *dhp;
    struct  sectorcrc *crctbl;
    int     ftot, i, snum, ssize, lastsnum, lastssize;

    sbase = (uchar *)tdp->start;
    crctbl = defragCrcTable(tdp);
    dhp = (struct defraghdr *)crctbl - 1;
    ftot = dhp->idx + 1;

    addrtosector((uchar *)tdp->end,&lastsnum,&lastssize,0);

    for(i=0; i<tdp->sectorcount; i++) {
        addrtosector(sbase,&snum,&ssize,0);
        if(snum == isnum) {
            break;
        }
        sbase += ssize;
    }
    if(isnum == lastsnum) {
        ssize -= (tdp->sectorcount * sizeof(struct sectorcrc));
        ssize -= (ftot * DEFRAGHDRSIZ);
        ssize -= 4;
    }
    if(crctbl[i].postcrc == crc32(sbase,ssize)) {
        return(1);
    } else {
        return(0);
    }
}

/* defragGetStateStr():
 * Return a string that corresponds to the incoming state value.
 */
static char *
defragGetStateStr(int state)
{
    char *str;

    switch(state) {
    case SECTOR_DEFRAG_INACTIVE:
        str = "SectorDefragInactive";
        break;
    case SECTOR_DEFRAG_ABORT_RESTART:
        str = "DefragRestartAborted";
        break;
    case SCANNING_ACTIVE_SECTOR_1:
        str = "ScanningActiveSector1";
        break;
    case SCANNING_ACTIVE_SECTOR_2:
        str = "ScanningActiveSector2";
        break;
    case SCANNING_ACTIVE_SECTOR_3:
        str = "ScanningActiveSector3";
        break;
    case SCANNING_ACTIVE_SECTOR_4:
        str = "ScanningActiveSector4";
        break;
    case SCANNING_ACTIVE_SECTOR_5:
        str = "ScanningActiveSector5";
        break;
    case SECTOR_DEFRAG_ALMOST_DONE:
        str = "DefragAlmostDone";
        break;
    default:
        str = "???";
        break;
    }
    return(str);
}

/* defragRestart():
 * Poll the console allowing the user to abort the auto-restart of
 * the defragmentation.  If a character is received on the console,
 * then return 0 indicating that the defrag should not be restarted;
 * else return 1.
 */
int
defragRestart(int state,int snum)
{
    printf("TFS defrag restart state: %s sector %d\n",
           defragGetStateStr(state),snum);
    if(pollConsole("Hit any key to abort...")) {
        return(0);
    }
    return(1);
}

/* defragGetState():
 * Step through the files in the specified device and check for
 * sanity.  Return SECTOR_DEFRAG_INACTIVE if it is determined that
 * a defragmentation was not in progress; otherwise, return one of
 * several different defrag state values depending on what is found
 * in the TFS flash area.
 *
 * NOTE:
 *   As of Jan 2008, the code wrapped within the ifdef/endif
 *   ENABLE_FLASHERASED_CHECK_AT_STARTUP is not used by default
 *   (the macro must be defined in config.h to pull it in).
 *   This change has been determined to be safe and allows startup
 *   to be much faster for systems that have a lot of empty TFS
 *   space.  The code was used to verify that all flash space after
 *   the last stored file in TFS was actually erased.  If not, then
 *   it erases it.
 *   This turns out to not really be necessary because if tfsadd()
 *   runs and finds that the area it needs isn't erased, it will
 *   automatically fall into a tfsclean anyway.  As a result, to
 *   make startup quicker, this code is not enabled by default.  If
 *   it is needed, then just define the macro in config.h.
 *
 *   As of Mar 2011, thanks to input from Jamie Randall, I'm essentially
 *   undoing the previous change by defining the
 *   ENABLE_FLASHERASED_CHECK_AT_STARTUP right here.  Turns out that
 *   while removal of this snippet of code does make bootup faster for
 *   those cases where the flash has a large number of sectors, it does
 *   cause powersafe defrag to fail in certain cases if a hit occurs.
 *
 */
#define ENABLE_FLASHERASED_CHECK_AT_STARTUP     /* see note above */

static int
defragGetState(TDEV *tdp, int *activesnum)
{
    TFILE   *tfp;
    struct  defraghdr   *dhp;
    struct  sectorcrc *crctbl;
    int     snum_in_spare, firstsnum;
    int     first_touched_snum, last_touched_snum;
    int     break_cause, break1_cause, spare_is_erased, ftot, ftot1, errstate;

    /* Establish state of spare sector:
     */
    spare_is_erased = flasherased((uchar *)tdp->spare,
                                  (uchar *)tdp->spare+tdp->sparesize-1);

    ftot = 0;
    break_cause = break1_cause = 0;
    for(tfp=(TFILE *)tdp->start; tfp < (TFILE *)tdp->end; tfp=tfp->next) {
        /* If we are legally at the end of file storage space, then we
         * will hit a header size that is ERASED16.  If we reach this
         * point and the remaining space dedicated to file storage is
         * erased and the spare is erased, it is safe to assume that we
         * were not in the middle of a defrag.
         */
        if(tfp->hdrsize == ERASED16) {
#ifdef ENABLE_FLASHERASED_CHECK_AT_STARTUP  /* (see note above) */
            /* Is space from last file to end of TFS space erased? */
            if(!flasherased((uchar *)tfp,(uchar *)tdp->end)) {
                break_cause = 1;
                break;
            }
            if(!spare_is_erased) {
                break_cause = 2;
                break;
            }
#if DEFRAG_TEST_ENABLED
            printf("\ndefragGetState: inactive_1\n");
#endif
#endif
            return(SECTOR_DEFRAG_INACTIVE);
        }

        /* If the crc32 of the header is corrupt, or if the next pointer
         * doesn't make any sense, then we must assume that a defrag
         * was in progress...
         */
        if(tfshdrcrc(tfp) != tfp->hdrcrc) {
            break;
        }

        if(!(tfp->next) || (tfp->next <= (TFILE *)tdp->start) ||
                (tfp->next >= (TFILE *)tdp->end)) {
            break;
        }
        if(TFS_FILEEXISTS(tfp)) {
            ftot++;
        }
    }
    /* If we are here, then something is not "perfect" with the flash
     * space used by TFS.  If break_cause is non-zero, there is a chance
     * that the only problem is that a file-write was interrupted and
     * we did not actually interrupt an in-progress-defrag.  An interrupted
     * file write would place some incomplete data after the last file.
     */
    ftot1 = defragValidDSI(tdp,&crctbl);

    /* If we don't have valid defrag state info (DSI), then we can assume
     * that the files in TFS have not yet been touched (since if we had
     * touched them, we would have already successfully created the DSI).
     * This being the case, then we will not continue with any defrag,
     * let TFS clean things up when the space is needed.
     */
    if(!ftot1) {
        if(break_cause) {
            /* Hmmm... Should something be done here? */
        }
#if DEFRAG_TEST_ENABLED
        printf("\ndefragGetState: inactive_2\n");
#endif
        return(SECTOR_DEFRAG_INACTIVE);
    }

    /* If we get here, then we have a valid defrag header table, so we
     * can use it and the state of each of the sectors to figure out
     * where we are in the defragmentation process.  We need to determine
     * which sector was being worked on at the point in time when the
     * defragmentation was interrupted.  A sector is in the "touched"
     * state if a crc32 on its content does not match the crc32 stored
     * in the crc table above the defrag header table.
     *
     * Here we step through the defrag header table and see if each file
     * in the header table exists in TFS.  If all files exist, then we
     * must have been very close to completion of the defrag process.
     */
    printf("TFS: scanning DSI space... ");
    dhp = (struct defraghdr *)crctbl - ftot1;
    while(dhp < (struct defraghdr *)crctbl) {
        tfp = (TFILE *)dhp->nda;
        if(tfp->hdrcrc != dhp->ohdrcrc) {
            break;
        }

        if(tfshdrcrc(tfp) != tfp->hdrcrc) {
            break1_cause = 1;
            break;
        }
        if(crc32((uchar *)(tfp+1),tfp->filsize) != tfp->filcrc) {
            break1_cause = 2;
            break;
        }
        dhp++;
        defragTick(0);
    }
    printf("done\n");

    /* If we stepped through the entire table, then we've completed the
     * file relocation process, but we still have to clean up...
     */
    if(dhp >= (struct defraghdr *)crctbl) {
        if(defragRestart(SECTOR_DEFRAG_ALMOST_DONE,0)) {
            return(SECTOR_DEFRAG_ALMOST_DONE);
        } else {
            return(SECTOR_DEFRAG_ABORT_RESTART);
        }
    }

    if(addrtosector((unsigned char *)tdp->start,&firstsnum,0,0) < 0) {
        errstate = 50;
        goto state_error;
    }
    defragTouchedSectors(tdp,&first_touched_snum,&last_touched_snum);

    /* If there are no touched sectors, then we will not continue with
     * the defrag because we didn't start relocation of any of the files
     * yet.
     */
    if(first_touched_snum == -1) {
#if DEFRAG_TEST_ENABLED
        printf("\ndefragGetState: inactive_3\n");
#endif
        return(SECTOR_DEFRAG_INACTIVE);
    }

    if(spare_is_erased) {
        snum_in_spare = -1;
    } else {
        snum_in_spare = defragSectorInSpare(tdp,crctbl);
    }

#if DEFRAG_TEST_ENABLED
    printf("\ndefragGetState info: %d %d %d\n",
           first_touched_snum, last_touched_snum, snum_in_spare);
#endif

    /* At this point we know what sector was the last to be touched.
     * What we don't know is whether or not the "touch" was completed.
     * So we don't know if the active sector is last_touched_snum or
     * last_touched_snum+1.
     * The only useful piece of data we 'might' have is the fact that
     * the spare may contain the content of the last touched sector.
     */

    /* If the spare is erased, it may be because defrag was just getting
     * ready to start working on the next sector (meaning that the active
     * sector is last_touched_snum+1) or the sector was in the process of
     * being modified.  We use the post-crc in the DHT to determine what
     * the active sector is...
     */
    if(spare_is_erased) {
        if(last_touched_snum >= 0) {
            if(defragPostCrcCheck(tdp,last_touched_snum)) {
                *activesnum = last_touched_snum + 1;

                if(defragRestart(SCANNING_ACTIVE_SECTOR_1,*activesnum)) {
                    return(SCANNING_ACTIVE_SECTOR_1);
                } else {
                    return(SECTOR_DEFRAG_ABORT_RESTART);
                }
            } else {
                if(defragSerase(2,last_touched_snum) < 0) {
                    errstate = 51;
                    goto state_error;
                }
                *activesnum = last_touched_snum;

                if(defragRestart(SCANNING_ACTIVE_SECTOR_2,*activesnum)) {
                    return(SCANNING_ACTIVE_SECTOR_2);
                } else {
                    return(SECTOR_DEFRAG_ABORT_RESTART);
                }
            }

        } else {
            errstate = 52;
            goto state_error;
        }
    }

    /* If the sector copied to spare is one greater than the last touched
     * sector, then the active sector is last_touched_snum+1 and it was
     * just copied to the spare.  In this case we erase the spare and
     * return indicating the active sector.
     */
    if(snum_in_spare == last_touched_snum+1) {
        if(defragEraseSpare(tdp) < 0) {
            errstate = 53;
            goto state_error;
        }
        *activesnum = snum_in_spare;

        if(defragRestart(SCANNING_ACTIVE_SECTOR_3,*activesnum)) {
            return(SCANNING_ACTIVE_SECTOR_3);
        } else {
            return(SECTOR_DEFRAG_ABORT_RESTART);
        }
    }

    /* If the spare is not erased, but it does not match any of the
     * sector CRCs, then we must have been in the process of copying
     * the active sector to the spare, so we can erase it and return
     * to the SCANNING_ACTIVE_SECTOR state.
     */
    if(snum_in_spare == -1) {
        if(last_touched_snum >= 0) {
            *activesnum = last_touched_snum + 1;

            if(!defragRestart(SCANNING_ACTIVE_SECTOR_4,*activesnum)) {
                return(SECTOR_DEFRAG_ABORT_RESTART);
            }

            if(defragEraseSpare(tdp) < 0) {
                errstate = 54;
                goto state_error;
            }
            return(SCANNING_ACTIVE_SECTOR_4);
        } else {
            errstate = 55;
            goto state_error;
        }
    }

    /* If the sector copied to spare is the number of the last touched
     * sector, then we were in the middle of modifying the sector, so
     * we have to erase that sector, copy the spare to it and return
     * to the scanning state.
     */
    if(snum_in_spare == last_touched_snum) {
        int ssize;
        uchar *sbase;

        *activesnum = snum_in_spare;

        if(!defragRestart(SCANNING_ACTIVE_SECTOR_5,*activesnum)) {
            return(SECTOR_DEFRAG_ABORT_RESTART);
        }

        if(defragSerase(3,snum_in_spare) < 0) {
            errstate = 56;
            goto state_error;
        }
        if(sectortoaddr(snum_in_spare,&ssize,&sbase) < 0) {
            errstate = 57;
            goto state_error;
        }
        if(defragFwrite(1,sbase,(uchar *)tdp->spare,ssize) < 0) {
            errstate = 58;
            goto state_error;
        }
        if(defragEraseSpare(tdp) < 0) {
            errstate = 59;
            goto state_error;
        }
        return(SCANNING_ACTIVE_SECTOR_5);
    }

    /* If we got here, then we are confused, so don't do any defrag
     * continuation...
     */
    errstate = 90;

state_error:
    printf("DEFRAG_STATE_ERROR: #%d.\n",errstate);
    return(SECTOR_DEFRAG_INACTIVE);
}

/* inSector():
 * We are trying to figure out if the address space that we want to copy
 * from is within the active sector.  If it is, then we need to adjust
 * our pointers so that we retrieve the at least some of data from the
 * spare.
 * If the range specified by 'i_base' and 'i_size' overlays (in any way)
 * the address space used by the sector specified by 'snum',
 * then return the address in the spare and the size of the overlay.
 */
static int
inSector(TDEV *tdp,int snum,uchar *i_base,int i_size,uchar **saddr,int *ovlysz)
{
    int     s_size;
    uchar   *s_base, *s_end, *i_end;

    /* Retrieve information about the sector: */
    if(sectortoaddr(snum,&s_size,&s_base) == -1) {
        return(TFSERR_MEMFAIL);
    }

    i_end = i_base + i_size;
    s_end = s_base + s_size;

    if((i_end < s_base) || (i_base > s_end)) {
        *ovlysz = 0;
        return(0);
    }

    if(i_base < s_base) {
        if(i_end > s_end) {
            *ovlysz = s_size;
        } else {
            *ovlysz = (i_size - (s_base - i_base));
        }
        *saddr = (uchar *)tdp->spare;
    } else {
        if(i_end > s_end) {
            *ovlysz = (i_size - (i_end - s_end));
        } else {
            *ovlysz = i_size;
        }
        *saddr = (uchar *)tdp->spare + (i_base - s_base);
    }
    return(0);
}

/* struct fillinfo & FILLMODE definitions:
 * Structure used by the "Fill" functions below.
 */
#define FILLMODE_FWRITE         1   /* Do the flash write */
#define FILLMODE_SPAREOVERLAP   2   /* Determine if there is SPARE overlap */
#define FILLMODE_CRCONLY        3   /* Calculate a 32-bit crc on the data */

struct fillinfo {
    struct defraghdr *dhp;  /* pointer to defrag header table */
    TDEV    *tdp;           /* pointer to TFS device */
    ulong   crc;            /* used in FILLMODE_CRCONLY mode */
    int     crcsz;          /* size of crc calculation */
    int     fhdr;           /* set if we're working on a file header */
    int     asnum;          /* the active sector */
    int     mode;           /* see FILLMODE_xxx definitions */
};

/* defragFillFlash():
 * This function is called by the defragFillActiveSector() function
 * below.  It covers the four different cases of a file spanning over
 * the active sector, plus it deals with the possibility that the source
 * of the file data may be the same sector as the active one (meaning that
 * the source is taken from the spare).  It is within this function that
 * the active sector is actually modified and it assumes that the portion
 * of the active sector to be written to is already erased.
 *
 *
 * SPANTYPE_BCEC:
 * In this case, the file starts in the active sector and ends in
 * the active sector...
 * -----------|----------|----------|----------|---------|---------|----------
 * |          |          |          |          |         |         |         |
 * |          |          |<-active->|          |         |         |  SPARE  |
 * |          |          |  sector  |          |         |         |  SECTOR |
 * |          |          |          |          |         |         |         |
 * |          |          | newfile  |          |         |         |         |
 * |          |          | |<-->|   |          |         |         |         |
 * -----------|----------|----------|----------|---------|---------|----------
 *
 *
 * SPANTYPE_BPEC:
 * In this case, the file starts in a sector prior to the currently active
 * sector and ends in the active sector...
 * -----------|----------|----------|----------|---------|---------|----------
 * |          |          |          |          |         |         |         |
 * |          |          |          |<-active->|         |         |  SPARE  |
 * |          |          |          |  sector  |         |         |  SECTOR |
 * |          |          |          |          |         |         |         |
 * |          |      |<----newfile----->|      |         |         |         |
 * |          |          |          |          |         |         |         |
 * -----------|----------|----------|----------|---------|---------|----------
 *
 *
 * SPANTYPE_BPEL:
 * In this case, the file starts in some sector prior to the currently
 * active sector and ends in some sector after the currently active
 * sector...
 * -----------|----------|----------|----------|---------|---------|----------
 * |          |          |          |          |         |         |         |
 * |          |          |<-active->|          |         |         |  SPARE  |
 * |          |          |  sector  |          |         |         |  SECTOR |
 * |          |          |          |          |         |         |         |
 * |       |<---------- newfile------------------->|     |         |         |
 * |          |          |          |          |         |         |         |
 * -----------|----------|----------|----------|---------|---------|----------
 *
 *
 * SPANTYPE_BCEL:
 * In this case, the file starts in the active sector and ends in
 * a later sector.
 * -----------|----------|----------|----------|---------|---------|----------
 * |          |          |          |          |         |         |         |
 * |          |<-active->|          |          |         |         |  SPARE  |
 * |          |  sector  |          |          |         |         |  SECTOR |
 * |          |          |          |          |         |         |         |
 * |          |      |<----newfile----->|      |         |         |         |
 * |          |      ****|          |          |         |         |         |
 * -----------|----------|----------|----------|---------|---------|----------
 */
static int
defragFillFlash(struct fillinfo *fip,int spantype,char **activeaddr,int verbose)
{
    char    *hp;
    TFILE   nfhdr;
    struct  defraghdr *dhp;
    int     ohdroffset, nhdroffset;
    uchar   *ovly, *src, *activesbase;
    int     ovlysz, srcsz, activessize;

    src = 0;
    ovly = 0;
    srcsz = 0;
    nhdroffset = ohdroffset = 0;
    dhp = fip->dhp;

    if(verbose >= 2) {
        printf("   defragFillFlash %s %s (%s %d)\n",fip->fhdr ? "hdr" : "dat",
               defragGetSpantypeStr(spantype), dhp->fname,fip->asnum);
    }

    if(spantype == SPANTYPE_BCEC) {
        if(fip->fhdr) {
            src = (uchar *)dhp->ohdr;
            srcsz = TFSHDRSIZ;
        } else {
            src = (uchar *)dhp->ohdr+TFSHDRSIZ;
            srcsz = dhp->filsize;
        }
    } else if(spantype == SPANTYPE_BPEC) {
        if(fip->fhdr) {
            /* Calculate the offset into the header at which point a
             * sector boundary occurs.  Do this for both the old (before
             * defrag relocation) and new (after defrag relocation)
             * location of the header.
             */
            /* Changed as of Dec 2010, based on error found by Leon...
             * We need to figure out how the header overlaps the sector
             * boundary.  Prior to 12/2010, this code did not account for
             * the case where the file size spans beyond the currently
             * active sector.
             */
            if((dhp->neso > dhp->filsize) && (dhp->oeso > dhp->filsize)) {
                nhdroffset = TFSHDRSIZ - (dhp->neso - dhp->filsize);
                ohdroffset = TFSHDRSIZ - (dhp->oeso - dhp->filsize);
                srcsz = (dhp->oeso - dhp->filsize) + (ohdroffset - nhdroffset);
                src = (uchar *)dhp->ohdr + nhdroffset;
            } else {
                int ssz;
                int sno = dhp->nesn;
                int fsz = dhp->filsize;
                while(sno > fip->asnum) {
                    if(sectortoaddr(sno,&ssz,0) < 0) {
                        return(TFSERR_MEMFAIL);
                    }
                    if(fsz == dhp->filsize) {
                        fsz -= dhp->neso;
                    } else {
                        fsz -= ssz;
                    }
                    sno--;
                }
                if(sectortoaddr(fip->asnum,&ssz,0) < 0) {
                    return(TFSERR_MEMFAIL);
                }
                srcsz = ssz - fsz;
                src = (uchar *)dhp->ohdr;
                src += (TFSHDRSIZ-srcsz);
                nhdroffset = (TFSHDRSIZ-srcsz);
            }
        } else {
            src = (uchar *)dhp->ohdr + TFSHDRSIZ + (dhp->filsize - dhp->neso);
            srcsz = dhp->neso;
        }
    } else if(spantype == SPANTYPE_BCEL) {
        if(sectortoaddr(fip->asnum,&activessize,&activesbase) == -1) {
            return(TFSERR_MEMFAIL);
        }

        if(fip->fhdr) {
            src = (uchar *)dhp->ohdr;
        } else {
            src = (uchar *)dhp->ohdr+TFSHDRSIZ;
        }
        srcsz = (activesbase + activessize) - (uchar *)*activeaddr;
    } else if(spantype == SPANTYPE_BPEL) {
        if(sectortoaddr(fip->asnum,&activessize,0) == -1) {
            return(TFSERR_MEMFAIL);
        }

        if(fip->fhdr) {
            src = (uchar *)dhp->ohdr;
        } else {
            src = (uchar *)dhp->ohdr+TFSHDRSIZ;
        }

        src += ((*activeaddr - dhp->nda) - TFSHDRSIZ);
        srcsz = activessize;
    } else {
        return(0);
    }

    /* Do some error checking on the computed size:
     */
    if(srcsz < 0) {
        printf("defragFillFlash: srcsz < 0\n");
        return(TFSERR_MEMFAIL);
    }

    if(fip->fhdr) {
        if(srcsz > TFSHDRSIZ) {
            printf("defragFillFlash: srcsz > TFSHDRSIZ\n");
            return(TFSERR_MEMFAIL);
        }
    } else {
        if(srcsz > dhp->filsize) {
            printf("defragFillFlash: srcsz > filsize\n");
            return(TFSERR_MEMFAIL);
        }
    }

    /* Determine if any portion of the source was part of the sector that
     * is now the active sector..  If yes (ovlysz > 0), then we must
     * deal with the fact that some (or all) of the fill source is in the
     * spare sector...
     */
    if(inSector(fip->tdp,fip->asnum,src,srcsz,&ovly,&ovlysz) < 0) {
        return(TFSERR_MEMFAIL);
    }

    /* If the mode is not FILLMODE_FWRITE, then we don't do any of the
     * flash operations.  We are in this function only to determine
     * if we need to copy the active sector to the spare prior to
     * starting the modification of the active sector.
     */
    if(fip->mode == FILLMODE_FWRITE) {
        if(fip->fhdr) {
            hp = (char *)&nfhdr;
            if(ovlysz) {
                memcpy((char *)hp+nhdroffset,(char *)ovly,ovlysz);
                if(ovlysz != srcsz) {
                    memcpy(hp+nhdroffset+ovlysz,(char *)src+ovlysz,
                           srcsz-ovlysz);
                }
            } else {
                /* next line was <nfhdr = *dhp->ohdr> ... */
                memcpy((char *)&nfhdr,(char *)dhp->ohdr,sizeof(TFILE));
            }
            nfhdr.next = dhp->nextfile;
            if(defragFwrite(2,(uchar *)*activeaddr,(uchar *)hp+nhdroffset,srcsz) == -1) {
                return(TFSERR_FLASHFAILURE);
            }
        } else {
            if(ovlysz) {
                if(defragFwrite(3,(uchar *)*activeaddr,(uchar *)ovly,ovlysz) == -1) {
                    return(TFSERR_FLASHFAILURE);
                }
                if(ovlysz != srcsz) {
                    if(defragFwrite(4,(uchar *)*activeaddr+ovlysz,(uchar *)src+ovlysz,
                                    srcsz-ovlysz) == -1) {
                        return(TFSERR_FLASHFAILURE);
                    }
                }
            } else {
                if(defragFwrite(5,(uchar *)*activeaddr,(uchar *)src,srcsz) == -1) {
                    return(TFSERR_FLASHFAILURE);
                }
            }
        }
    } else if(fip->mode == FILLMODE_CRCONLY) {
        register uchar  *bp;
        int     sz, temp;

        if(fip->fhdr) {
            hp = (char *)&nfhdr;
            /* next line was <nfhdr = *dhp->ohdr> ... */
            memcpy((char *)&nfhdr,(char *)dhp->ohdr,sizeof(TFILE));
            nfhdr.next = dhp->nextfile;
            bp = (uchar *)hp + nhdroffset;
        } else {
            bp = (uchar *)src;
        }
        sz = srcsz;
        fip->crcsz += sz;
        while(sz) {
            temp = (fip->crc ^ *bp++) & 0x000000FFL;
            fip->crc = ((fip->crc >> 8) & 0x00FFFFFFL) ^ crc32tab[temp];
            sz--;
        }
    }
    *activeaddr += srcsz;

    if((spantype == SPANTYPE_BCEC || spantype == SPANTYPE_BPEC) &&
            (!fip->fhdr) && ((ulong)*activeaddr & 0xf)) {
        int     sz, temp, modfixsize;

        modfixsize = (TFS_FSIZEMOD - ((ulong)*activeaddr & (TFS_FSIZEMOD-1)));
        *activeaddr += modfixsize;
        if(fip->mode == FILLMODE_CRCONLY) {
            sz = modfixsize;
            fip->crcsz += sz;
            while(sz) {
                temp = (fip->crc ^ 0xff) & 0x000000FFL;
                fip->crc = ((fip->crc >> 8) & 0x00FFFFFFL) ^ crc32tab[temp];
                sz--;
            }
        }
    }

    /* Return ovlysz so that the caller will know if this function
     * needed the spare sector.  This is used in the "mode = SPARE_OVERLAP"
     * pass of defragFillActiveSector().
     */
    return(ovlysz);
}

/* defragFillActiveSector():
 * This and defragFillFlash() are the workhorses of the tfsclean() function.
 * The bulk of this function is used to determine if we need to do anything
 * to the active sector and if so, do we need to copy the active sector to
 * the spare prior to erasing it.
 * The first loop in this function determines whether we need to do anything
 * at all with this sector (it may not be touched by the defragmentation).
 * The second loop determines if we have to copy the active sector to the
 * spare prior to erasing the active sector.
 * The final loop in this function does the call to defragFillFlash()
 * to do the actual flash writes.
 */
static int
defragFillActiveSector(TDEV *tdp, int ftot, int snum, int verbose)
{
    int     firstsnum;      /* number of first TFS sector */
    int     activesnum;     /* number of sector currently being written to */
    int     activessize;    /* size of active sector */
    char    *activeaddr;    /* offset being written to in the active sector */
    uchar   *activesbase;   /* base address of active sector */
    char    *activesend;    /* end address of active sector */
    struct  defraghdr *sdhp;/* pointer into defrag hdr table in spare */
    struct  defraghdr *dhp; /* pointer into defrag header table */
    int     fullsize;       /* size of file and header */
    char    *new_dend;      /* new end of data */
    char    *new_dbase;     /* new base of data */
    char    *new_hend;      /* new end of header */
    char    *new_hbase;     /* new base of header */
    char    *new_fend;      /* new end of file */
    char    *new_fbase;     /* new base of file */
    int     new_fspan;      /* span type for new file */
    int     new_hspan;      /* span type for new header */
    int     new_dspan;      /* span type for new data */
    int     fillstat;       /* result of defragFillFlash() function call */
    int     noreloctot;     /* number of files spanning the active sector */
    /* that do not have to be relocated */
    int     tmptot;         /* temps used for the "SPARE_OVERLAP" mode */
    char    *tmpactiveaddr;
    struct  defraghdr *tmpdhp;
    struct  fillinfo finfo;
    struct  sectorcrc   *crctbl;
    int     lastsnum, tot;
    int     copytospare;
    int     lastfileisnotrelocated;

    /* Retrieve number of first TFS sector: */
    if(addrtosector((uchar *)tdp->start,&firstsnum,0,0) < 0) {
        return(TFSERR_MEMFAIL);
    }

    activesnum = snum + firstsnum;
    crctbl = defragCrcTable(tdp);

    /* Retrieve information about active sector: */
    if(sectortoaddr(activesnum,&activessize,&activesbase) == -1) {
        return(TFSERR_MEMFAIL);
    }

    if(verbose) {
        printf(" Active sector: %3d @ 0x%lx\n",activesnum,(ulong)activesbase);
    }

    if(addrtosector((uchar *)tdp->end,&lastsnum,0,0) < 0) {
        return(TFSERR_MEMFAIL);
    }

    /* Establish a pointer to the defrag header table.
     * For the case when the active sector is the last sector, the defrag
     * header table will be in the spare, so we also establish a pointer
     * to that space...
     */
    dhp = (struct defraghdr *)crctbl;
    dhp -= ftot;
    sdhp = (struct defraghdr *)(tdp->spare+activessize -
                                (tdp->sectorcount * sizeof(struct sectorcrc)));
    sdhp -= ftot;

    activeaddr = (char *)activesbase;
    tmpactiveaddr = (char *)activesbase;
    activesend = (char *)activesbase + activessize - 1;

    /* FIRST LOOP:
     * See if we need to do anything...
     * In this state, we are simply checking to see if anything is going
     * to cause the currently active sector to be written to.
     * If yes, then we need to copy it to the spare and start the
     * modification process; else, we just return and do nothing
     * to this sector.
     * For each file in the defrag header table that is destined for
     * the address space occupied by the currently active sector, copy
     * that file (header and data) to the active sector...
     * Note that it may only be a partial copy, depending on the size
     * of the file and the amount of space left in the active sector.
     */

    noreloctot = 0;
    new_fspan = SPANTYPE_UNDEF;
    lastfileisnotrelocated = 0;
    for(tot=0; tot<ftot; tot++,dhp++,sdhp++) {
        fullsize = TFSHDRSIZ + dhp->filsize;
        new_fbase = dhp->nda;
        new_fend = (new_fbase + fullsize);

        /* We must figure out how the new version of the file will
         * span across the active sector.
         * See defragGetSpantype() for details.
         */
        new_fspan = defragGetSpantype((char *)activesbase,(char *)activesend,
                                      (char *)new_fbase,(char *)new_fend);

        /* If the file we are looking at entirely spans a sector that is
         * prior to the currently active sector, then we just continue
         * through the list.
         * If the file entirely spans a sector that is after the
         * currently active sector, then we are done with this active sector.
         * If the file falls within the active sector in any way, and its
         * new location does not match its old location, then we
         * break out of this loop and begin the modification of this
         * active sector...
         */
        if(new_fspan == SPANTYPE_BLEL) {
            return(0);
        }

        if(new_fspan != SPANTYPE_BPEP) {
            if(dhp->nda == (char *)dhp->ohdr) {
                noreloctot++;
                if(tot == ftot-1) {
                    lastfileisnotrelocated = 1;
                    if(verbose > 1) {
                        printf("  last file not relocated\n");
                    }
                }
            } else {
                break;
            }
        }
    }

    /* If tot == ftot, then we got through the entire loop above without
     * finding a file that needs to be relocated into this active sector.
     * This means one of two things: either all the files fall into a
     * sector prior to this active sector, or the files in this active
     * sector do not need to be relocated.  In either case, we simply
     * return without touching this sector.
     * Note that we also keep track of the possibility that the last file
     * may not be relocated.  If this ends up to be the case, then we are
     * simply cleaning up one or more dead files after a full set of active
     * files, so we should clean up the sector.
     */
    if((tot == ftot) && (lastfileisnotrelocated == 0)) {
        return(0);
    }

    /* If tot != ftot, then we must subtract noreloctot from tot so that
     * we establish 'tot' as the index into the first file that must be
     * copied to the active sector...
     */
    if(noreloctot) {
        tot -= noreloctot;
        dhp -= noreloctot;
        sdhp -= noreloctot;
    }

    /* Exit immediately before cleaning up the spare... */
    defragExitTestPoint(10000+activesnum);

    /* Since we got here, we know that we have to do some work on the
     * currently active sector.  We may not have to copy it to the spare,
     * but we will erase the spare anyway because the sector erase is
     * supposed to be smart enough to avoid the erase if it is already
     * erased.  This should be handled by the flash driver because in
     * ALL cases the erase should be avoided if possible.
     */
    if(defragEraseSpare(tdp) < 0) {
        return(TFSERR_FLASHFAILURE);
    }

    /* Exit immediately after cleaning up the spare... */
    defragExitTestPoint(10001+activesnum);

    /* If the active sector is the last sector (which would contain the
     * defrag header table), then we reference the copy of the table that
     * is in the spare...
     * Also, if this is the last sector, then we HAVE to copy it to
     * spare, so we can skip the 2nd loop that attempts to determine
     * if we need to do it.
     */
    if(activesnum == lastsnum) {
        dhp = sdhp;
        copytospare = 1;
    } else {
        /* SECOND LOOP:
         * See if we need to copy the active sector to the spare...
         * We do this by continuing the loop we started above.  Notice that
         * we do an almost identical loop again below this.
         * On this pass through the loop we are only checking to see if it is
         * necessary to copy this active sector to the spare.
         */
        tmptot = tot;
        tmpdhp = dhp;
        copytospare = 0;
        finfo.mode = FILLMODE_SPAREOVERLAP;
        for(; tmptot<ftot; tmptot++,tmpdhp++) {
            finfo.tdp = tdp;
            finfo.dhp = tmpdhp;
            finfo.asnum = activesnum;

            fullsize = TFSHDRSIZ + tmpdhp->filsize;
            new_fbase = tmpdhp->nda;
            new_fend = (new_fbase + fullsize);

            new_fspan = defragGetSpantype((char *)activesbase,(char *)activesend,
                                          (char *)new_fbase,(char *)new_fend);

            if(new_fspan == SPANTYPE_BPEP) {
                continue;
            } else if(new_fspan == SPANTYPE_BLEL) {
                break;
            }

            /* Now retrieve span information about header and data
             * portions of the file (new and orig)...
             */
            new_hbase = new_fbase;
            new_hend = new_hbase + TFSHDRSIZ;
            new_dbase = new_hbase + TFSHDRSIZ;
            new_dend = new_fend;

            new_hspan = defragGetSpantype((char *)activesbase,(char *)activesend,
                                          (char *)new_hbase,(char *)new_hend);
            new_dspan = defragGetSpantype((char *)activesbase,(char *)activesend,
                                          (char *)new_dbase,(char *)new_dend);

            /* If defragFillFlash() returns positive (with mode ==
             * FILLMODE_SPAREOVERLAP set above), then we know that the
             * spare sector must be loaded with a copy of this active
             * sector, so we can break out of this loop at that point...
             */
            finfo.fhdr = 1;
            fillstat = defragFillFlash(&finfo,new_hspan,&tmpactiveaddr,verbose);
            if(fillstat < 0) {
                return(fillstat);
            }
            if(fillstat > 0) {
                copytospare = 1;
                break;
            }
            if(new_hspan == SPANTYPE_BCEL) {
                break;
            }

            finfo.fhdr = 0;
            fillstat = defragFillFlash(&finfo,new_dspan,&tmpactiveaddr,verbose);
            if(fillstat < 0) {
                return(fillstat);
            }
            if(fillstat > 0) {
                copytospare = 1;
                break;
            }
            if(new_dspan == SPANTYPE_BCEL || new_dspan == SPANTYPE_BPEL) {
                break;
            }
        }
    }

    finfo.mode = FILLMODE_FWRITE;

    defragExitTestPoint(10002+activesnum);

    if(copytospare) {
        defragTick(verbose);
#if DEFRAG_TEST_ENABLED
        printf("     copying sector %d to spare\n",activesnum);
#endif
        if(defragFwrite(6,(uchar *)tdp->spare,activesbase,activessize) == -1) {
            printf("Failed to copy active %d to spare\n",activesnum);
            return(TFSERR_FLASHFAILURE);
        }
    }
#if DEFRAG_TEST_ENABLED
    else {
        printf("     copy saved\n");
    }
#endif

    defragTick(verbose);

    /* We can now begin actual modification of the active sector,
     * so start off by eraseing it...
     */
    defragExitTestPoint(10003+activesnum);

    if(defragSerase(4,activesnum) < 0) {
        return(TFSERR_FLASHFAILURE);
    }

    defragExitTestPoint(10004+activesnum);

    /* THIRD LOOP:
     * Now we pass through the loop to do the real flash modifications...
     */
    for(; tot<ftot; tot++,dhp++) {
        finfo.tdp = tdp;
        finfo.dhp = dhp;
        finfo.asnum = activesnum;

        fullsize = TFSHDRSIZ + dhp->filsize;
        new_fbase = dhp->nda;
        new_fend = (new_fbase + fullsize);

        new_fspan = defragGetSpantype((char *)activesbase,(char *)activesend,
                                      (char *)new_fbase,(char *)new_fend);

        if(new_fspan == SPANTYPE_BPEP) {
            continue;
        } else if(new_fspan == SPANTYPE_BLEL) {
            break;
        }

        if(verbose) {
            printf("  File: %s\n",dhp->fname);
        }

        /* Now retrieve span information about header and data
         * portions of the file (new and orig)...
         */
        new_hbase = new_fbase;
        new_hend = new_hbase + TFSHDRSIZ;
        new_dbase = new_hbase + TFSHDRSIZ;
        new_dend = new_fend;

        new_hspan = defragGetSpantype((char *)activesbase,(char *)activesend,
                                      (char *)new_hbase,(char *)new_hend);
        new_dspan = defragGetSpantype((char *)activesbase,(char *)activesend,
                                      (char *)new_dbase,(char *)new_dend);

        /* At this point we have all the information we need to copy
         * the appropriate amount of the file from orignal space
         * to new space.
         * We have to break the write up into two parts, the header
         * (new_hspan) and the data (new_dspan) so we have to look
         * at the spantype for each to determine what part of the
         * header and/or data we are going to copy.
         *
         * Also, we must consider the possibility that the source
         * data may be in the spare sector.  This would be the case
         * if the active sector is the same sector that the original
         * data was in.  If the source data is in the spare sector,
         * then an added complication is the fact that it may not
         * all be there, we may have to copy some from the spare,
         * then some from the original space.
         */
        finfo.fhdr = 1;
        fillstat = defragFillFlash(&finfo,new_hspan,&activeaddr,verbose);
        if(fillstat < 0) {
            return(fillstat);
        }
        if(new_hspan == SPANTYPE_BCEL) {
            break;
        }

        finfo.fhdr = 0;
        fillstat = defragFillFlash(&finfo,new_dspan,&activeaddr,verbose);
        if(fillstat < 0) {
            return(fillstat);
        }
        if(new_dspan == SPANTYPE_BCEL || new_dspan == SPANTYPE_BPEL) {
            break;
        }
        defragTick(verbose);
    }
    return(0);
}

static int
defragNewSectorCrc(TDEV *tdp, struct defraghdr *dht, int snum,
                   ulong *newcrc, int verbose)
{
    int     firstsnum;      /* number of first TFS sector */
    int     activesnum;     /* number of sector currently being written to */
    int     activessize;    /* size of active sector */
    char    *activeaddr;    /* offset being written to in the active sector */
    uchar   *activesbase;   /* base address of active sector */
    char    *activesend;    /* end address of active sector */
    struct  defraghdr *dhp; /* pointer into defrag header table */
    int     fullsize;       /* size of file and header */
    char    *new_dend;      /* new end of data */
    char    *new_dbase;     /* new base of data */
    char    *new_hend;      /* new end of header */
    char    *new_hbase;     /* new base of header */
    char    *new_fend;      /* new end of file */
    char    *new_fbase;     /* new base of file */
    int     new_fspan;      /* span type for new file */
    int     new_hspan;      /* span type for new header */
    int     new_dspan;      /* span type for new data */
    int     fillstat;       /* result of defragFillFlash() function call */
    int     ftot;
    struct  fillinfo finfo;
    struct  sectorcrc   *crctbl;
    int     lastsnum, tot, sz, temp;

    /* Retrieve number of first TFS sector: */
    if(addrtosector((uchar *)tdp->start,&firstsnum,0,0) < 0) {
        return(TFSERR_MEMFAIL);
    }

    activesnum = snum + firstsnum;
    crctbl = defragCrcTable(tdp);

    /* Retrieve information about active sector: */
    if(sectortoaddr(activesnum,&activessize,&activesbase) == -1) {
        return(TFSERR_MEMFAIL);
    }

    if(addrtosector((uchar *)tdp->end,&lastsnum,0,0) < 0) {
        return(TFSERR_MEMFAIL);
    }

    dhp = (struct defraghdr *)crctbl - 1;
    ftot = dhp->idx + 1;
    dhp = dht;
    activeaddr = (char *)activesbase;
    activesend = (char *)activesbase + activessize - 1;

    finfo.tdp = tdp;
    finfo.crcsz = 0;
    finfo.crc = 0xffffffff;
    finfo.asnum = activesnum;
    finfo.mode = FILLMODE_CRCONLY;

    for(tot=0; tot<ftot; tot++,dhp++) {
        finfo.dhp = dhp;

        fullsize = TFSHDRSIZ + dhp->filsize;
        new_fbase = dhp->nda;
        new_fend = (new_fbase + fullsize);

        new_fspan = defragGetSpantype((char *)activesbase,(char *)activesend,
                                      (char *)new_fbase,(char *)new_fend);

        if(new_fspan == SPANTYPE_BPEP) {
            continue;
        } else if(new_fspan == SPANTYPE_BLEL) {
            break;
        }

        /* Now retrieve span information about header and data
         * portions of the file (new and orig)...
         */
        new_hbase = new_fbase;
        new_hend = new_hbase + TFSHDRSIZ;
        new_dbase = new_hbase + TFSHDRSIZ;
        new_dend = new_fend;

        new_hspan = defragGetSpantype((char *)activesbase,(char *)activesend,
                                      (char *)new_hbase,(char *)new_hend);
        new_dspan = defragGetSpantype((char *)activesbase,(char *)activesend,
                                      (char *)new_dbase,(char *)new_dend);

        finfo.fhdr = 1;
        fillstat = defragFillFlash(&finfo,new_hspan,&activeaddr,verbose);
        if(fillstat < 0) {
            return(fillstat);
        }
        if(new_hspan == SPANTYPE_BCEL) {
            break;
        }

        finfo.fhdr = 0;
        fillstat = defragFillFlash(&finfo,new_dspan,&activeaddr,verbose);
        if(fillstat < 0) {
            return(fillstat);
        }
        if(new_dspan == SPANTYPE_BCEL || new_dspan == SPANTYPE_BPEL) {
            break;
        }
    }
    sz = activessize - finfo.crcsz;

    /* If this is the last sector, then we must not include the space used
     * for defrag state storage in the crc calculation.
     * We deduct size of CRC table, DHT table and the crc of the DSI space...
     */
    if(activesnum == lastsnum) {
        sz -= (tdp->sectorcount * sizeof(struct sectorcrc));
        sz -= (ftot * DEFRAGHDRSIZ);
        sz -= 4;
    }
    while(sz) {
        temp = (finfo.crc ^ 0xff) & 0x000000FFL;
        finfo.crc = ((finfo.crc >> 8) & 0x00FFFFFFL) ^ crc32tab[temp];
        sz--;
    }
    *newcrc = ~finfo.crc;
    return(0);
}

/* defragBuildCrcTable():
 * Build the table of sector crcs.
 * This consists of a set of CRCs representing each sector
 * before defrag starts and after defrag has completed.
 * One set for each sector.  This table is then used if the
 * defrag process is interrupted to determine the state of
 * the interrupted defragmentation process.
 */
int
defragBuildCrcTable(TDEV *tdp, struct defraghdr *dht, int verbose)
{
    ulong   crc;
    uchar   *sbase;
    int     i, ssize, dhstsize;
    struct  sectorcrc *crctbl;

    dhstsize = (ulong)(tdp->end+1) - (ulong)dht + 4;
    crctbl = defragCrcTable(tdp);

    /* The pre-defrag crc table...
     * This one's easy because it is simply a crc for each of the current
     * sectors.
     */
    sbase = (uchar *)tdp->start;
    for(i=0; i<tdp->sectorcount; i++) {
        if(addrtosector(sbase,0,&ssize,0) < 0) {
            return(-1);
        }

        if(i == tdp->sectorcount-1) {
            ssize -= dhstsize;
        }

        /* The pre-defrag crc: */
        crc = crc32(sbase,ssize);
        if(defragFwrite(7,(uchar *)&crctbl[i].precrc,
                        (uchar *)&crc,4) == -1) {
            return(-1);
        }

        /* The post-defrag crc: */
        if(defragNewSectorCrc(tdp,dht,i,&crc,verbose) < 0) {
            return(-1);
        }

        if(defragFwrite(8,(uchar *)&crctbl[i].postcrc,
                        (uchar *)&crc,4) == -1) {
            return(-1);
        }

        sbase += ssize;

        defragTick(0);
    }
    return(0);
}

/* _tfsclean():
 * This is the front-end of the defragmentation process, following are the
 * basic steps of defragmentation...
 *
 * Build the Defrag State Information (DSI) area:
 * 1. Create a table of 32-bit CRCs, two for each sector.  One is the CRC
 *    of the sector prior to beginning defragmentation and the other is
 *    what will be the CRC of the sector after defragmentation has completed.
 *    These CRCs are used to help recover from an interrupted defragmentation.
 * 2. Create a table of struct defraghdr structures, one for each file in
 *    TFS that is currently active (not dead).
 * 3. Create a CRC of the tables created in steps 1 & 2.
 *
 * The data created in steps 1-3 is stored at the end of the last sector
 * used by TFS for file storage.  After this is created, the actual flash
 * defragmentation process starts.
 *
 * File relocation:
 * 4. Step through each sector in TFS flash space, process each file whose
 *    relocated space overlaps with that sector.  As each sector is being
 *    re-built, the original version of that sector is stored in the spare.
 *
 * End of flash cleanup:
 * 5. Run through the remaining, now unsused, space in TFS flash and make
 *    sure it is erased.
 *
 * File check:
 * 6. Run a check of all of the relocated files to make sure everything is
 *    still sane.
 *
 * Defragmentation success depends on some coordination with tfsadd()...
 * Whenever a file is added to TFS, tfsadd() must verify that the space
 * needed for defrag overhead (defrag state & header tables) will be
 * available.  Also, tfsadd() must make sure that the defrag overhead will
 * always fit into one sector (the sector just prior to the spare).
 */

int
_tfsclean(TDEV *tdp, int restart, int verbose)
{
    int     dhstsize;       /* Size of state table overhead */
    int     firstsnum;      /* Number of first sector in TFS device. */
    int     lastsnum;       /* Number of last sector in TFS device. */
    int     lastssize;      /* Size of last sector in TFS device. */
    uchar   *lastsbase;     /* Base address of last sector in TFS device. */
    int     sectorcheck;    /* Used to verify proper TFS configuration. */
    struct  defraghdr *dht; /* Pointer to defrag header table. */
    int     chkstat;        /* Result of tfscheck() after defrag is done. */
    int     ftot;           /* Total number of active files in TFS. */
    int     dtot;           /* Total number of deleted files in TFS. */
    int     fcnt;           /* Running file total, used in hdrtbl build. */
    TFILE   *tfp;           /* Misc file pointer */
    char    *newaddress;    /* Used to calculate "new" location of file. */
    struct  defraghdr   dfhdr;  /* Used to build defrag header table. */
    int     activesnum;     /* Sector being worked on restarted defrag. */
    struct  sectorcrc   *crctbl;    /* Pointer to table of per-sector crcs. */
    int     defrag_state;
    int     sidx, snum;
    char    *end;

    if(TfsCleanEnable < 0) {
        return(TFSERR_CLEANOFF);
    }

    /* If incoming TFS device pointer is NULL, return error
     */
    if(!tdp) {
        return(TFSERR_BADARG);
    }

    activesnum = 0;

    /* If the 'restart' flag is set, then we only want to do a defrag if
     * we determine that one is already in progress; so we have to look at
     * the current state of the defrag state table to figure out if a defrag
     * was active.  If not, just return.
     */
    if(restart) {
        defrag_state = defragGetState(tdp,&activesnum);
        switch(defrag_state) {
        case SECTOR_DEFRAG_INACTIVE:
        case SECTOR_DEFRAG_ABORT_RESTART:
            return(TFS_OKAY);
        case SCANNING_ACTIVE_SECTOR_1:
        case SCANNING_ACTIVE_SECTOR_2:
        case SCANNING_ACTIVE_SECTOR_3:
        case SCANNING_ACTIVE_SECTOR_4:
        case SCANNING_ACTIVE_SECTOR_5:
            defrag_state = SCANNING_ACTIVE_SECTOR;
            break;
        }
    } else {
        defrag_state = SECTOR_DEFRAG_INACTIVE;
    }

    if(verbose || restart || (!MFLAGS_NODEFRAGPRN())) {
        printf("TFS device '%s' powersafe defragmentation\n",tdp->prefix);
        if((restart) && pollConsole("ok?")) {
            printf("aborted\n");
            return(TFS_OKAY);
        }
    }

    if(addrtosector((uchar *)tdp->start,&firstsnum,0,0) < 0) {
        return(TFSERR_MEMFAIL);
    }
    lastsnum = firstsnum + tdp->sectorcount - 1;
    if(addrtosector((uchar *)tdp->end,&sectorcheck,0,0) < 0) {
        return(TFSERR_MEMFAIL);
    }
    if(lastsnum != sectorcheck) {
        /* If this error occurs, it is an indication that TFS was not
         * properly configured in config.h, this error should not occur
         * if TFS is properly configured.
         */
        printf("%s: SECTORCOUNT != TFSSTART <-> TFSEND\n", tdp->prefix);
        printf("First TFS sector = %d, last = %d\n",firstsnum,sectorcheck);
        return(TFSERR_MEMFAIL);
    }

    if(defrag_state == SECTOR_DEFRAG_INACTIVE) {
        activesnum = firstsnum;
    }

    /* Retrieve information about last sector:
     */
    if(sectortoaddr(lastsnum,&lastssize,&lastsbase) == -1) {
        return(TFSERR_MEMFAIL);
    }

    /* Establish a pointer to a table of CRCs that will contain
     * one 32-bit CRC for each sector (prior to starting the defrag).
     */
    crctbl = defragCrcTable(tdp);

    /* Retrieve the number of "dead" and "living" files:
     * If there are no dead files, then there is no need to defrag.
     * If there are no "living" files, then we can just init the flash.
     */
    ftot = dtot = 0;
    if(restart && defragValidDSI(tdp,0)) {
        dht = (struct defraghdr *)crctbl - 1;
        ftot = dht->idx + 1;
    } else {
        tfp = (TFILE *)tdp->start;
        while(validtfshdr(tfp)) {
            if(TFS_FILEEXISTS(tfp)) {
                ftot++;
            } else {
                dtot++;
            }
            tfp = nextfp(tfp,tdp);
        }
        if(dtot == 0) {
            if(verbose) {
                printf("No dead files in %s.\n",tdp->prefix);
            }
            if(tfsflasherased(tdp,verbose)) {
                return(0);
            }
            if(verbose) {
                printf("Cleaning up end of flash...\n");
            }
        }
    }
    if(ftot == 0) {
        if(verbose)
            printf("No active files detected, erasing all %s flash...\n",
                   tdp->prefix);
        _tfsinit(tdp);
        return(0);
    }

    /* Now that we know how many files are in TFS, we can establish
     * a pointer to the defrag header table, and the size of the table...
     */
    dht = (struct defraghdr *)crctbl - ftot;
    dhstsize = (ulong)(tdp->end+1) - (ulong)dht;
    dhstsize += 4; /* Account for the CRC of the state tables. */

    if(defrag_state == SECTOR_DEFRAG_INACTIVE) {
        ulong   crc;

        if(verbose) {
            printf("TFS defrag: building DSI space...\n");
        }

        /* We start by making sure that the space needed by the
         * defrag header and state table at the end of the last
         * sector is clear...
         */
        if(!flasherased((uchar *)dht, (uchar *)(tdp->end))) {
            if(defragEraseSpare(tdp) < 0) {
                return(TFSERR_FLASHFAILURE);
            }

            if(defragFwrite(9,(uchar *)(tdp->spare),lastsbase,
                            lastssize-dhstsize) == -1) {
                return(TFSERR_FLASHFAILURE);
            }
            if(defragSerase(5,lastsnum) < 0) {
                return(TFSERR_FLASHFAILURE);
            }
            if(defragFwrite(10,lastsbase,(uchar *)(tdp->spare),
                            lastssize) == -1) {
                return(TFSERR_FLASHFAILURE);
            }
        }

        /* Erase the spare then copy the portion of the last TFS
         * sector that does not overlap with the defrag header and
         * state table area to the spare.  We do this so that the spare
         * sector contains a defrag header and state table area that
         * is erased.
         */
        if(defragEraseSpare(tdp) < 0) {
            return(TFSERR_FLASHFAILURE);
        }

        if(defragFwrite(11,(uchar *)tdp->spare,
                        lastsbase,lastssize-dhstsize) == -1) {
            return(TFSERR_FLASHFAILURE);
        }

        /* At this point we have a valid copy of the last sector in
         * the spare.  If any portion of the last sector is not identical
         * to what is in the spare, then we need to erase the last sector
         * and re-copy what is in the spare to the last sector.  This is
         * necessary because an interrupt may have occurred while writing
         * to the last sector, and it may have corrupted something.
         */

        if((memcmp((char *)lastsbase,(char *)(tdp->spare),lastssize-dhstsize)) ||
                (!flasherased((uchar *)dht,(uchar *)tdp->end))) {
            if(defragSerase(6,lastsnum) < 0) {
                return(TFSERR_FLASHFAILURE);
            }
            if(defragFwrite(12,lastsbase,(uchar *)(tdp->spare),
                            lastssize) == -1) {
                return(TFSERR_FLASHFAILURE);
            }
        }
        /* Build the header table:
         */
        fcnt = 0;
        tfp = (TFILE *)tdp->start;
        newaddress = (char *)tdp->start;

        if(verbose > 2) {
            printf("\nDEFRAG HEADER DATA (dht=0x%lx, ftot=%d):\n",
                   (ulong)dht,ftot);
        }

        while(validtfshdr(tfp)) {
            if(TFS_FILEEXISTS(tfp)) {
                uchar   *base, *eof, *neof, *nbase;
                int     size, slot;
                struct  tfsdat *slotptr;

                strcpy(dfhdr.fname,TFS_NAME(tfp));
                dfhdr.ohdr = tfp;
                dfhdr.ohdrcrc = tfp->hdrcrc;
                dfhdr.filsize = TFS_SIZE(tfp);
                if(addrtosector((uchar *)tfp,0,0,&base) < 0) {
                    return(TFSERR_MEMFAIL);
                }

                eof = (uchar *)(tfp+1)+TFS_SIZE(tfp)-1;
                if(addrtosector((uchar *)eof,0,0,&base) < 0) {
                    return(TFSERR_MEMFAIL);
                }
                dfhdr.oeso = eof - base + 1;

                neof = (uchar *)newaddress+TFSHDRSIZ+TFS_SIZE(tfp)-1;
                if(addrtosector((uchar *)neof,&dfhdr.nesn,0,&nbase) < 0) {
                    return(TFSERR_MEMFAIL);
                }
                dfhdr.neso = neof - nbase + 1;

                dfhdr.crc = 0;
                dfhdr.idx = fcnt;
                dfhdr.nda = newaddress;

                /* If the file is currently opened, adjust the base address. */
                slotptr = tfsSlots;
                for(slot=0; slot<TFS_MAXOPEN; slot++,slotptr++) {
                    if(slotptr->offset != -1) {
                        if(slotptr->base == (uchar *)(TFS_BASE(tfp))) {
                            slotptr->base = (uchar *)(newaddress+TFSHDRSIZ);
                        }
                    }
                }
                size = TFS_SIZE(tfp) + TFSHDRSIZ;
                if(size & 0xf) {
                    size += TFS_FSIZEMOD;
                    size &= ~(TFS_FSIZEMOD-1);
                }
                newaddress += size;
                dfhdr.nextfile = (TFILE *)newaddress;
                dfhdr.crc = crc32((uchar *)&dfhdr,DEFRAGHDRSIZ);
                if(verbose > 2) {
                    printf(" File %s (sz=%d):\n",TFS_NAME(tfp),TFS_SIZE(tfp));
                    printf("     nda=  0x%08lx, ohdr= 0x%08lx, nxt=  0x%08lx\n",
                           (ulong)(dfhdr.nda),(ulong)(dfhdr.ohdr),
                           (ulong)(dfhdr.nextfile));
                    printf("     oeso= 0x%08lx, nesn= 0x%08lx, neso= 0x%08lx\n",
                           (ulong)(dfhdr.oeso),(ulong)(dfhdr.nesn),
                           (ulong)(dfhdr.neso));
                }
                if(defragFwrite(13,(uchar *)(&dht[fcnt]),
                                (uchar *)(&dfhdr),DEFRAGHDRSIZ) == -1) {
                    return(TFSERR_FLASHFAILURE);
                }
                fcnt++;
                defragTick(0);
            }
            tfp = nextfp(tfp,tdp);
        }

        if(defragBuildCrcTable(tdp,dht,verbose) < 0) {
            return(TFSERR_FLASHFAILURE);
        }

        /* Now, the last part of the state table build is to store a
         * 32-bit crc of the data we just wrote...
         */
        crc = crc32((uchar *)dht,(uchar *)tdp->end - (uchar *)dht);
        if(defragFwrite(14,(uchar *)((ulong *)dht-1),(uchar *)&crc,4) == -1) {
            return(TFSERR_FLASHFAILURE);
        }

        defrag_state = SCANNING_ACTIVE_SECTOR;
    }

    /* Exit here to have a complete defrag header installed. */
    defragExitTestPoint(1000);

    if(defrag_state == SCANNING_ACTIVE_SECTOR) {

        if(verbose) {
            printf("TFS: updating sectors %d-%d...\n",
                   activesnum,lastsnum);
        }

        /* Now we begin the actual defragmentation.  We have built enough
         * state information (defrag header and state table) into the last
         * TFS sector, so now we can start the cleanup.
         */
        for(sidx = activesnum - firstsnum; sidx < tdp->sectorcount; sidx++) {
            if(defragFillActiveSector(tdp,ftot,sidx,verbose) < 0) {
                return(TFSERR_FLASHFAILURE);
            }
        }

        defrag_state = SECTOR_DEFRAG_ALMOST_DONE;
    }

    /* Exit here to test "almost-done" state detection. */
    defragExitTestPoint(1001);

    if(defrag_state == SECTOR_DEFRAG_ALMOST_DONE) {

        /* We've completed the relocation of all files into a defragmented
         * area of TFS flash space.  Now we have to erase all sectors after
         * the sector used by the last file in TFS (including the spare)...
         * If the last file in TFS uses the last sector, then the defrag
         * header table will be erased and there is nothing left to do
         * except erase the spare.
         */
        if(verbose) {
            printf("TFS: clearing available space...\n");
        }
        if(dht[ftot-1].crc != ERASED32) {
            end = (dht[ftot-1].nda + dht[ftot-1].filsize + TFSHDRSIZ) - 1;
            if(addrtosector((uchar *)end,&snum,0,0) < 0) {
                return(TFSERR_FLASHFAILURE);
            }
            snum++;
            while(snum <= lastsnum) {
                if(defragSerase(7,snum) < 0) {
                    return(TFSERR_FLASHFAILURE);
                }
                snum++;
                defragTick(0);
            }
        }
        if(defragEraseSpare(tdp) < 0) {
            return(TFSERR_FLASHFAILURE);
        }

        /* All defragmentation is done, so verify sanity of files... */
        chkstat = tfscheck(tdp,verbose);
    } else {
        chkstat = TFS_OKAY;
    }

    if((verbose) || (!MFLAGS_NODEFRAGPRN())) {
        printf("Defragmentation complete\n");
    }

    return(chkstat);
}

/* tfsfixup():
 *  Called at system startup to finish up a TFS defragmentation if one
 *  was in progress.
 */
int
tfsfixup(int verbose, int dontquery)
{
    TDEV    *tdp;

    /* Clear test data... */
    DefragTestType = 0;
    DefragTestPoint = 0;
    DefragTestSector = 0;

#if !DEFRAG_TEST_ENABLED
    tfsTrace = 99;
#endif

    /* For each TFS device, run defrag with "fixup" flag set to let
     * the defragger know that it should only defrag if a defrag was
     * in progress.
     */
    for(tdp=tfsDeviceTbl; tdp->start != TFSEOT; tdp++) {
        /* Call tfsclean() with fixup flag set... */
#if TFS_VERBOSE_STARTUP
        if(StateOfMonitor == INITIALIZE) {
            printf("TFS Scanning %s...\n",tdp->prefix);
        }
#endif
        _tfsclean(tdp,1,99);
    }

#if !DEFRAG_TEST_ENABLED
    tfsTrace = 0;
#endif
    return(0);
}

#if DEFRAG_TEST_ENABLED
int
dumpDhdr(DEFRAGHDR *dhp)
{
    printf("ohdr: 0x%08lx\n",(ulong)dhp->ohdr);
    printf("nextfile: 0x%08lx\n",(ulong)dhp->nextfile);
    printf("filsize: 0x%08lx\n",(ulong)dhp->filsize);
    printf("crc: 0x%08lx\n",(ulong)dhp->crc);
    printf("idx: 0x%08lx\n",(ulong)dhp->idx);
    printf("nesn: 0x%08lx\n",(ulong)dhp->nesn);
    printf("neso: 0x%08lx\n",(ulong)dhp->neso);
    printf("nda: 0x%08lx\n",(ulong)dhp->nda);
    printf("fname: %s\n",dhp->fname);
    return(TFS_OKAY);
}

int
dumpDhdrTbl(DEFRAGHDR *dhp, int ftot)
{
    TDEV    *tdp;
    uchar   *sbase;
    ulong   *crc, calccrc;
    int     i, ssize, snum;
    struct  sectorcrc *crctbl;

    for(tdp=tfsDeviceTbl; tdp->start != TFSEOT; tdp++) {

        if(!dhp) {
            crctbl = defragCrcTable(tdp);
            printf("Device %s...\n",tdp->prefix);
            dhp = (struct defraghdr *)crctbl - 1;
            ftot = dhp->idx + 1;
            printf("(%d files):\n",ftot);
            dhp = (struct defraghdr *)crctbl - ftot;
            crc = (ulong *)dhp-1;
            if(*crc != crc32((uchar *)dhp,(uchar *)tdp->end - (uchar *)dhp)) {
                printf("Table CRC failure\n");
                return(TFS_OKAY);
            }
        } else {
            crctbl = (struct sectorcrc *)(dhp + ftot);
        }

        printf("dhp=0x%lx, ftot=%d\n",(ulong)dhp,ftot);

        while(dhp < (struct defraghdr *)crctbl) {
            printf(" %s (dhp=0x%lx, idx=%ld):\n",
                   dhp->fname,(ulong)dhp,dhp->idx);
            printf("   nda: 0x%08lx, ohdr: 0x%08lx\n",
                   (ulong)dhp->nda,(ulong)dhp->ohdr);
            dhp++;
        }

        sbase = (uchar *)tdp->start;
        printf("crctbl at 0x%lx\n",(ulong)crctbl);
        for(i=0; i<tdp->sectorcount; i++) {
            if(addrtosector(sbase,&snum,&ssize,0) < 0) {
                return(0);
            }
            if(i == tdp->sectorcount-1) {
                ssize -=                            /* CRC table */
                    (tdp->sectorcount * sizeof(struct sectorcrc));
                ssize -= (ftot * DEFRAGHDRSIZ);     /* DHT table */
                ssize -= 4;                         /* Crc of the tables */
            }
            calccrc = crc32(sbase,ssize);
            if(calccrc == crctbl[i].precrc) {
                printf("crctbl[%d] (snum=%d) pre-pass\n",i,snum);
            } else if(calccrc == crctbl[i].postcrc) {
                printf("crctbl[%d] (snum=%d) post-pass\n",i,snum);
            } else {
                printf("crctbl[%d] (snum=%d) test failed\n",i,snum);
                printf("pre: 0x%lx, post: 0x%lx,calc: 0x%lx\n",
                       crctbl[i].precrc,crctbl[i].postcrc,calccrc);
            }
            sbase += ssize;
        }
    }
    return(TFS_OKAY);
}
#endif  /* DEFRAG_TEST_ENABLED */
#endif  /* INCLUDE_TFS */
