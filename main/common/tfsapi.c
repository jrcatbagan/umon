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
 * tfsapi.c:
 *
 * This file contains the portion of TFS that provides the function-level
 * API to the application.  If this is not being used by the application
 * then it can be omitted from the monitor build.
 * Note that not all of the api-specific code is here; some of it is in
 * tfs.c.  This is because the the MicroMonitor uses some of the api itself,
 * so it cannot be omitted from the TFS package without screwing up some
 * other monitor functionality that needs it.
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
#if INCLUDE_TFSAPI

/* tfstruncate():
 *  To support the ability to truncate a file (make it smaller); this
 *  function allows the user to adjust the high-water point of the currently
 *  opened (and assumed to be opened for modification) file and replaces
 *  that with the incoming argument.  This replacement is only done if the
 *  current high-water point is higher than the incoming length.
 *  MONLIB NOTICE: this function is accessible through monlib.c.
 */
int
tfstruncate(int fd, long len)
{
    struct tfsdat *tdat;

    if(tfsTrace > 1) {
        printf("tfstruncate(%d,%ld)\n",fd,len);
    }

    /* Verify valid range of incoming file descriptor. */
    if((fd < 0) || (fd >= TFS_MAXOPEN)) {
        return(TFSERR_BADFD);
    }

    tdat = &tfsSlots[fd];

    /* Make sure the file pointed to by the incoming descriptor is active
     * and that the incoming length is greater than the current high-water
     * point...
     */
    if(tdat->offset == -1) {
        return(TFSERR_BADFD);
    }
    if(len > tdat->hwp) {
        return(TFSERR_BADARG);
    }

    /* Make the adjustment... */
    tdat->hwp = len;
    return(TFS_OKAY);
}

/* tfseof():
 *  Return 1 if at the end of the file, else 0 if not at end; else negative
 *  if error.
 *  MONLIB NOTICE: this function is accessible through monlib.c.
 */
int
tfseof(int fd)
{
    struct tfsdat *tdat;

    /* Verify valid range of incoming file descriptor. */
    if((fd < 0) || (fd >= TFS_MAXOPEN)) {
        return(TFSERR_BADARG);
    }

    tdat = &tfsSlots[fd];

    /* Make sure the file pointed to by the incoming descriptor is active. */
    if(tdat->offset == -1) {
        return(TFSERR_BADFD);
    }

    if(tdat->offset >= tdat->hdr.filsize) {
        return(1);
    } else {
        return(0);
    }
}

/* tfsread():
 *  Similar to a standard read call to a file.
 *  MONLIB NOTICE: this function is accessible through monlib.c.
 */
int
tfsread(int fd, char *buf, int cnt)
{
    struct tfsdat *tdat;
    uchar *from;

    if(tfsTrace > 1) {
        printf("tfsread(%d,0x%lx,%d)\n",fd,(ulong)buf,cnt);
    }

    /* Verify valid range of incoming file descriptor. */
    if((cnt < 1) || (fd < 0) || (fd >= TFS_MAXOPEN)) {
        return(TFSERR_BADARG);
    }

    tdat = &tfsSlots[fd];

    /* Make sure the file pointed to by the incoming descriptor is active. */
    if(tdat->offset == -1) {
        return(TFSERR_BADFD);
    }

    if(tdat->offset >= tdat->hdr.filsize) {
        return(TFSERR_EOF);
    }

    from = (uchar *) tdat->base + tdat->offset;

    /* If request size is within the range of the file and current
     * then copy the data to the requestors buffer, increment offset
     * and return the count.
     */
    if((tdat->offset + cnt) <= tdat->hdr.filsize) {
        if(s_memcpy((char *)buf, (char *)from, cnt,0,0) != 0) {
            return(TFSERR_MEMFAIL);
        }
    }
    /* If request size goes beyond the size of the file, then copy
     * to the end of the file and return that smaller count.
     */
    else {
        cnt = tdat->hdr.filsize - tdat->offset;
        if(s_memcpy((char *)buf, (char *)from, cnt, 0, 0) != 0) {
            return(TFSERR_MEMFAIL);
        }
    }
    tdat->offset += cnt;
    return(cnt);
}

/* tfswrite():
 *  Similar to a standard write call to a file.
 *  MONLIB NOTICE: this function is accessible through monlib.c.
 */
int
tfswrite(int fd, char *buf, int cnt)
{
    struct tfsdat *tdat;

    if(tfsTrace > 1) {
        printf("tfswrite(%d,0x%lx,%d)\n", fd,(ulong)buf,cnt);
    }

    /* Verify valid range of incoming file descriptor. */
    if((cnt < 1) || (fd < 0) || (fd >= TFS_MAXOPEN)) {
        return(TFSERR_BADARG);
    }

    /* Make sure the file pointed to by the incoming descriptor is active. */
    if(tfsSlots[fd].offset == -1) {
        return(TFSERR_BADARG);
    }

    tdat = &tfsSlots[fd];

    /* Make sure file is not opened as read-only */
    if(tdat->flagmode & TFS_RDONLY) {
        return(TFSERR_RDONLY);
    }

    if(s_memcpy((char *)tdat->base+tdat->offset,(char *)buf,cnt,0,0) != 0) {
        return(TFSERR_MEMFAIL);
    }

    tdat->offset += cnt;

    /* If new offset is greater than current high-water point, then
     * adjust the high water point so that it is always reflecting the
     * highest offset into which the file has had some data written.
     */
    if(tdat->offset > tdat->hwp) {
        tdat->hwp = tdat->offset;
    }

    return(TFS_OKAY);
}

/* tfsseek():
 *  Adjust the current pointer into the specified file.
 *  If file is read-only, then the offset cannot exceed the file size;
 *  otherwise, the only check made to the offset is that it is positive.
 *  MONLIB NOTICE: this function is accessible through monlib.c.
 */
int
tfsseek(int fd, int offset, int whence)
{
    int o_offset;
    struct tfsdat *tdat;

    if(tfsTrace > 1) {
        printf("tfsseek(%d,%d,%d)\n",fd,offset,whence);
    }

    if((fd < 0) || (fd >= TFS_MAXOPEN)) {
        return(TFSERR_BADARG);
    }

    tdat = &tfsSlots[fd];
    o_offset = tdat->offset;

    switch(whence) {
    case TFS_BEGIN:
        tdat->offset = offset;
        break;
    case TFS_CURRENT:
        tdat->offset += offset;
        break;
    default:
        return(TFSERR_BADARG);
    }

    /* If new offset is less than zero or if the file is read-only and the
     * new offset is greater than the file size, return EOF...
     */
    if((tdat->offset < 0) ||
            ((tdat->flagmode & TFS_RDONLY) && (tdat->offset > tdat->hdr.filsize))) {
        tdat->offset = o_offset;
        return(TFSERR_EOF);
    }
    return(tdat->offset);
}

/* tfsipmod():
 *  Modify "in-place" a portion of a file in TFS.
 *  This is a cheap and dirty way to modify a file...
 *  The idea is that a file is created with a lot of writeable flash space
 *  (data = 0xff).  This function can then be called to immediately modify
 *  blocks of space in that flash.  It will not do any tfsunlink/tfsadd, and
 *  it doesn't even require a tfsopen() tfsclose() wrapper.  Its a fast and
 *  efficient way to modify flash in the file system.
 *  Arguments:
 *  name    =   name of the file to be in-place-modified;
 *  buf     =   new data to be written to flash;
 *  offset  =   offset into file into which new data is to be written;
 *  size    =   size of new data (in bytes).
 *
 *  With offset of -1, set offset to location containing first 0xff value.
 *  MONLIB NOTICE: this function is accessible through monlib.c.
 */
int
tfsipmod(char *name,char *buf,int offset,int size)
{
    int     rc;
    TFILE   *fp;
    uchar   *cp;

    fp = tfsstat(name);
    if(!fp) {
        return (TFSERR_NOFILE);
    }
    if(!(fp->flags & TFS_IPMOD)) {
        return(TFSERR_NOTIPMOD);
    }

    if(offset == -1) {
        cp = (uchar *)(TFS_BASE(fp));
        for(offset=0; offset<fp->filsize; offset++,cp++) {
            if(*cp == 0xff) {
                break;
            }
        }
    } else if(offset < -1) {
        return(TFSERR_BADARG);
    }

    if((offset + size) > fp->filsize) {
        return(TFSERR_WRITEMAX);
    }

    /* BUG fixed: 2/21/2001:
     * The (ulong *) cast was done prior to adding offset to the base.
     * This caused the offset to be quadrupled.
     */
    rc = tfsflashwrite((uchar *)(TFS_BASE(fp)+offset),(uchar *)buf,size);
    if(rc != TFS_OKAY) {
        return(rc);
    }

    tfslog(TFSLOG_IPM,name);
    return(TFS_OKAY);
}

/* tfsopen():
 *  Open a file for reading or creation.  If file is opened for writing,
 *  then the caller must provide a RAM buffer  pointer to be used for
 *  the file storage until it is transferred to flash by tfsclose().
 *  Note that the "buf" pointer is only needed for opening a file for
 *  creation or append (writing).
 *  MONLIB NOTICE: this function is accessible through monlib.c.
 */
int
tfsopen(char *file,long flagmode,char *buf)
{
    register int i;
    int     errno, retval;
    long    fmode;
    TFILE   *fp;
    struct  tfsdat *slot;

    errno = TFS_OKAY;

    fmode = flagmode & (TFS_RDONLY | TFS_APPEND | TFS_CREATE | TFS_CREATERM);

    /* See if file exists... */
    fp = tfsstat(file);

    /* If file exists, do a crc32 on the data.
     * If the file is in-place-modifiable, then the only legal flagmode
     * is TFS_RDONLY.  Plus, in this case, the crc32 test is skipped.
     */
    if(fp) {
        if(!((fmode == TFS_RDONLY) && (fp->flags & TFS_IPMOD))) {
            if(crc32((unsigned char *)TFS_BASE(fp),fp->filsize) != fp->filcrc) {
                retval = TFSERR_BADCRC;
                goto done;
            }
        }
    }

    /* This switch verifies...
     * - that the file exists if TFS_RDONLY or TFS_APPEND
     * - that the file does not exist if TFS_CREATE
     */
    switch(fmode) {
    case TFS_RDONLY:    /* Read existing file only, no change to file at all. */
        if(!fp) {
            if(_tfsstat(file,0)) {
                errno = TFSERR_LINKERROR;
            } else {
                errno = TFSERR_NOFILE;
            }
        } else {
            if((fp->flags & TFS_UNREAD) && (TFS_USRLVL(fp) > getUsrLvl())) {
                errno = TFSERR_USERDENIED;
            }
        }
        break;
    case TFS_APPEND:    /* Append to the end of the current file. */
        if(!fp) {
            errno = TFSERR_NOFILE;
        } else {
            if(TFS_USRLVL(fp) > getUsrLvl()) {
                errno = TFSERR_USERDENIED;
            }
        }
        break;
    case TFS_CREATERM:      /* Create a new file, allow tfsadd() to remove */
        fmode = TFS_CREATE; /* it if it exists. */
        break;
    case TFS_CREATE:    /* Create a new file, error if it exists. */
        if(fp) {
            errno = TFSERR_FILEEXISTS;
        }
        break;
    case(TFS_APPEND|TFS_CREATE):    /* If both mode bits are set, clear one */
        if(fp) {                    /* based on the presence of the file. */
            if(TFS_USRLVL(fp) > getUsrLvl()) {
                errno = TFSERR_USERDENIED;
            }
            fmode = TFS_APPEND;
        } else {
            fmode = TFS_CREATE;
        }
        break;
    default:
        errno = TFSERR_BADARG;
        break;
    }

    if(errno != TFS_OKAY) {
        retval = errno;
        goto done;
    }

    /* Find an empty slot...
     */
    slot = tfsSlots;
    for(i=0; i<TFS_MAXOPEN; i++,slot++) {
        if(slot->offset == -1) {
            break;
        }
    }

    /* Populate the slot structure if a slot is found to be
     * available...
     */
    if(i < TFS_MAXOPEN) {
        retval = i;
        slot->hwp = 0;
        slot->offset = 0;
        slot->flagmode = fmode;
        if(fmode & TFS_CREATE) {
            strncpy(slot->hdr.name,file,TFSNAMESIZE);
            slot->flagmode |= (flagmode & TFS_FLAGMASK);
            slot->base = (uchar *)buf;
        } else if(fmode & TFS_APPEND) {
            memcpy((char *)&slot->hdr,(char *)fp,sizeof(struct tfshdr));
            if(s_memcpy((char *)buf,(char *)(TFS_BASE(fp)),
                        fp->filsize,0,0) != 0) {
                retval = TFSERR_MEMFAIL;
                goto done;
            }
            slot->flagmode = fp->flags;
            slot->flagmode |= TFS_APPEND;
            slot->base = (uchar *)buf;
            slot->hwp = fp->filsize;
            slot->offset = fp->filsize;
        } else {
            slot->base = (uchar *)(TFS_BASE(fp));
            memcpy((char *)&slot->hdr,(char *)fp,sizeof(struct tfshdr));
        }
    } else {
        retval = TFSERR_NOSLOT;
    }

done:
    if(tfsTrace > 0) {
        printf("tfsopen(%s,0x%lx,0x%lx)=%d\n",file,flagmode,(ulong)buf,retval);
    }

    return(retval);
}

/* tfsclose():
 *  If the file was opened for reading only, then just close out the
 *  entry in the tfsSlots table.  If the file was opened for creation,
 *  then add it to the tfs list.  Note the additional argument is
 *  only needed for tfsclose() of a newly created file.
 *  info  = additional text describing the file.
 *  MONLIB NOTICE: this function is accessible through monlib.c.
 */
int
tfsclose(int fd,char *info)
{
    int     err;
    struct tfsdat *tdat;

    if(!info) {
        info = "";
    }

    if(tfsTrace > 0) {
        printf("tfsclose(%d,%s)\n",fd,info);
    }

    if((fd < 0) || (fd >= TFS_MAXOPEN)) {
        return(TFSERR_BADARG);
    }

    tdat = &tfsSlots[fd];

    if(tdat->offset == -1) {
        return(TFSERR_BADFD);
    }

    /* Mark the file as closed by setting the offset to -1.
     * Note that this is done prior to potentially calling tfsadd() so
     * that tfsadd() will not think the file is opened and reject the add...
     */
    tdat->offset = -1;

    /* If the file was opened for creation or append, and the hwp
     * (high-water-point) is greater than zero, then add it now.
     *
     * Note regarding hwp==0...
     * In cases where a non-existent file is opened for creation,
     * but then nothing is written to the file, the hwp value will
     * be zero; hence, no need to call tfsadd().
     */
    if((tdat->flagmode & (TFS_CREATE | TFS_APPEND)) && (tdat->hwp > 0)) {
        char    buf[16];

        err = tfsadd(tdat->hdr.name, info, tfsflagsbtoa(tdat->flagmode,buf),
                     tdat->base, tdat->hwp);
        if(err != TFS_OKAY) {
            printf("%s: %s\n",tdat->hdr.name,tfserrmsg(err));
            return(err);
        }
    }
    return(TFS_OKAY);
}

#else /* INCLUDE_TFSAPI */

int
tfstruncate(int fd, long len)
{
    return(TFSERR_NOTAVAILABLE);
}

int
tfseof(int fd)
{
    return(TFSERR_NOTAVAILABLE);
}

int
tfsread(int fd, char *buf, int cnt)
{
    return(TFSERR_NOTAVAILABLE);
}

int
tfswrite(int fd, char *buf, int cnt)
{
    return(TFSERR_NOTAVAILABLE);
}

int
tfsseek(int fd, int offset, int whence)
{
    return(TFSERR_NOTAVAILABLE);
}

int
tfsopen(char *file,long flagmode,char *buf)
{
    return(TFSERR_NOTAVAILABLE);
}

int
tfsclose(int fd,char *info)
{
    return(TFSERR_NOTAVAILABLE);
}

int
tfsipmod(char *name,char *buf,int offset,int size)
{
    return(TFSERR_NOTAVAILABLE);
}

#endif  /* INCLUDE_TFSAPI else */

#if INCLUDE_TFSAPI || INCLUDE_TFSSCRIPT

/* tfsgetline():
 *  Read into the buffer a block of characters upto the next EOL delimiter
 *  the file.  After the EOL, or after max-1 chars are loaded, terminate
 *  with a NULL.  Return the number of characters loaded.
 *  At end of file return 0.
 *  MONLIB NOTICE: this function is accessible through monlib.c.
 */
int
tfsgetline(int fd,char *buf,int max)
{
    uchar   *from;
    int     tot, rtot;
    struct  tfsdat *tdat;
    volatile char   *to;

    max--;

    if(tfsTrace > 1) {
        printf("tfsgetline(%d,0x%lx,%d)\n",fd,(ulong)buf,max);
    }

    /* Verify valid range of incoming file descriptor. */
    if((max < 1) || (fd < 0) || (fd >= TFS_MAXOPEN)) {
        return(TFSERR_BADARG);
    }

    /* Make sure the file pointed to by the incoming descriptor is active. */
    if(tfsSlots[fd].offset == -1) {
        return(TFSERR_BADARG);
    }

    tdat = &tfsSlots[fd];

    if(tdat->offset == -1) {
        return(TFSERR_BADFD);
    }

    if(tdat->offset >= tdat->hdr.filsize) {
        return(0);
    }

    from = (uchar *) tdat->base + tdat->offset;
    to = buf;

    /* If the incoming buffer size is larger than needed, adjust the
     * 'max' value so that we don't pass the end of the file...
     */
    if((tdat->offset + max) > tdat->hdr.filsize) {
        max = tdat->hdr.filsize - tdat->offset + 1;
    }

    /* Read from the file data area until newline (0x0a) is found
     * (or until the 'max buffer space' value is reached).
     * Strip 0x0d (if present) and terminate with NULL  in all cases.
     */
    for(rtot=0,tot=0; tot < max; from++) {
        /* Terminate on Ctrl-Z, non-ASCII, Newline or NULL.
         * Ignore carriage return completely...
         */
        if((*from == 0x1a) || (*from > 0x7f) || (*from == 0)) {
            break;
        }

        tot++;

        if(*from == 0x0d) {
            continue;
        }

        *to = *from;
        if(*to != *from) {
            return(TFSERR_MEMFAIL);
        }

        to++;
        rtot++;

        if(*from == 0x0a) {
            break;
        }
    }
    *to = 0;

    tdat->offset += tot;
    return(rtot);
}

#else

int
tfsgetline(int fd,char *buf,int max)
{
    return(TFSERR_NOTAVAILABLE);
}

#endif
