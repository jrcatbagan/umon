/* gzio.c -- IO on .gz files
 * Copyright (C) 1995-1998 Jean-loup Gailly.
 * For conditions of distribution and use, see copyright notice in zlib.h
 *
 * This has been seriously re-arranged for use within MicroMonitor.
 * The majority of the rework is to strip out unnecessary parts because this
 * will reside in flash space.  Also, there is no file system interface, all
 * decompression is done from one point in memory to another point in memory.
 */

#include "config.h"
#if INCLUDE_UNZIP
#include "tfs.h"
#include "cli.h"
#include "tfsprivate.h"
#include "genlib.h"
#include "zutil.h"

struct internal_state {
    int dummy;
}; /* for buggy compilers */

#ifndef Z_BUFSIZE
#  ifdef MAXSEG_64K
#    define Z_BUFSIZE 4096 /* minimize memory usage for 16-bit DOS */
#  else
#    define Z_BUFSIZE 16384
#  endif
#endif
#ifndef Z_PRINTF_BUFSIZE
#  define Z_PRINTF_BUFSIZE 4096
#endif

#define ALLOC(size) malloc(size)
#define TRYFREE(p) {if (p) free((char *)p);}

static int gz_magic[2] = {0x1f, 0x8b}; /* gzip magic header */

/* gzip flag byte */
#define ASCII_FLAG   0x01 /* bit 0 set: file probably ascii text */
#define HEAD_CRC     0x02 /* bit 1 set: header CRC present */
#define EXTRA_FIELD  0x04 /* bit 2 set: extra field present */
#define ORIG_NAME    0x08 /* bit 3 set: original file name present */
#define COMMENT      0x10 /* bit 4 set: file comment present */
#define RESERVED     0xE0 /* bits 5..7: reserved */

typedef struct gz_stream {
    z_stream stream;
    int      z_err;         /* error code for last stream operation */
    int      z_eof;         /* set if end of input file */
    Byte     *outbuf;       /* output buffer */
    uLong    crc;           /* crc32 of uncompressed data */
    char     *msg;          /* error message */
    int      transparent;    /* 1 if input file is not a .gz file */
    char     mode;          /* 'w' or 'r' */
} gz_stream;


local int    get_byte     OF((gz_stream *s));
local void   check_header OF((gz_stream *s));
local int    destroy      OF((gz_stream *s));
local uLong  getLong      OF((gz_stream *s));

/* ===========================================================================
    gzInit():
    Rewritten (from gzopen()) to support only decompression of data in
    memory.  The next_in member points directly to the compressed data and
    avail_in is the number of bytes remaining.
    Things get simpler because of the removal of the file system interface
    as well as the "only decompression" mode.
*/
gz_stream *
gzInit(unsigned char *compressed_data,int size)
{
    int err;
    gz_stream *s;

    s = (gz_stream *)ALLOC(sizeof(gz_stream));
    if(!s) {
        return Z_NULL;
    }

    s->stream.zalloc = (alloc_func)0;
    s->stream.zfree = (free_func)0;
    s->stream.opaque = (voidpf)0;
    s->stream.next_in = compressed_data;
    s->stream.next_out = s->outbuf = Z_NULL;
    s->stream.avail_in = size;
    s->stream.avail_out = 0;
    s->z_err = Z_OK;
    s->z_eof = 0;
    s->crc = zcrc32(0L, Z_NULL, 0);
    s->msg = NULL;
    s->transparent = 0;
    s->mode = 'r';

    err = inflateInit2(&(s->stream), -MAX_WBITS);
    /* windowBits is passed < 0 to tell that there is no zlib header.
     * Note that in this case inflate *requires* an extra "dummy" byte
     * after the compressed stream in order to complete decompression and
     * return Z_STREAM_END. Here the gzip CRC32 ensures that 4 bytes are
     * present after the compressed stream.
     */
    if(err != Z_OK) {
        destroy(s);
        return((gz_stream *)Z_NULL);
    }

    s->stream.avail_out = Z_BUFSIZE;
    check_header(s); /* skip the .gz header */
    return(s);
}

/* ===========================================================================
    get_byte()
    Simple now because we assume that avail_in is pointing to the number
    of bytes in the source buffer that are remaining to be decompressed
    and next_in is the pointer to the next byte to be retrieved from the
    buffer.
*/
local int
get_byte(s)
gz_stream *s;
{
    if(s->z_eof) {
        return EOF;
    }
    if(s->stream.avail_in == 0) {
        s->z_eof = 1;
        return EOF;
    }
    s->stream.avail_in--;
    return *(s->stream.next_in)++;
}

/* ===========================================================================
    check_header():
    Check the gzip header of a gz_stream opened for reading. Set the stream
    mode to transparent if the gzip magic header is not present; set s->err
    to Z_DATA_ERROR if the magic header is present but the rest of the header
    is incorrect.
    IN assertion: the stream s has already been created sucessfully;
       s->stream.avail_in is zero for the first time, but may be non-zero
       for concatenated .gz files.
*/
local void
check_header(s)
gz_stream *s;
{
    int method; /* method byte */
    int flags;  /* flags byte */
    uInt len;
    int c;

    /* Check the gzip magic header */
    for(len = 0; len < 2; len++) {
        c = get_byte(s);
        if(c != gz_magic[len]) {
            if(len != 0) {
                s->stream.avail_in++;
                s->stream.next_in--;
            }
            if(c != EOF) {
                s->stream.avail_in++;
                s->stream.next_in--;
                s->transparent = 1;
            }
            s->z_err = s->stream.avail_in != 0 ? Z_OK : Z_STREAM_END;
            return;
        }
    }
    method = get_byte(s);
    flags = get_byte(s);
    if(method != Z_DEFLATED || (flags & RESERVED) != 0) {
        s->z_err = Z_DATA_ERROR;
        return;
    }

    /* Discard time, xflags and OS code: */
    for(len = 0; len < 6; len++) {
        (void)get_byte(s);
    }

    if((flags & EXTRA_FIELD) != 0) {  /* skip the extra field */
        len  = (uInt)get_byte(s);
        len += ((uInt)get_byte(s))<<8;
        /* len is garbage if EOF but the loop below will quit anyway */
        while(len-- != 0 && get_byte(s) != EOF) ;
    }
    if((flags & ORIG_NAME) != 0) {  /* skip the original file name */
        while((c = get_byte(s)) != 0 && c != EOF) ;
    }
    if((flags & COMMENT) != 0) {    /* skip the .gz file comment */
        while((c = get_byte(s)) != 0 && c != EOF) ;
    }
    if((flags & HEAD_CRC) != 0) {   /* skip the header crc */
        for(len = 0; len < 2; len++) {
            (void)get_byte(s);
        }
    }
    s->z_err = s->z_eof ? Z_DATA_ERROR : Z_OK;
}

/* ===========================================================================
    destroy():
    Cleanup then free the given gz_stream. Return a zlib error code.
    Try freeing in the reverse order of allocations.
*/
local int
destroy(s)
gz_stream *s;
{
    int err = Z_OK;

    if(!s) {
        return Z_STREAM_ERROR;
    }

    TRYFREE(s->msg);

    if(s->stream.state != NULL) {
        err = inflateEnd(&(s->stream));
    }

    if(s->z_err < 0) {
        err = s->z_err;
    }

    TRYFREE(s->outbuf);
    TRYFREE(s);
    return err;
}

/* ===========================================================================
    gzRead():
    Reads the given number of uncompressed bytes from the compressed file.
    gzRead returns the number of bytes actually read (0 for end of file).
*/
int ZEXPORT
gzRead(gzFile file, voidp buf,unsigned len)
{
    gz_stream *s = (gz_stream *)file;
    Bytef *start = (Bytef *)buf; /* starting point for crc computation */
    Byte  *next_out; /* == stream.next_out but not forced far (for MSDOS) */

    if(s == NULL || s->mode != 'r') {
        return Z_STREAM_ERROR;
    }

    if(s->z_err == Z_DATA_ERROR || s->z_err == Z_ERRNO) {
        return -1;
    }
    if(s->z_err == Z_STREAM_END) {
        return 0;    /* EOF */
    }

    next_out = (Byte *)buf;
    s->stream.next_out = (Bytef *)buf;
    s->stream.avail_out = len;

    while(s->stream.avail_out != 0) {
        if(s->transparent) {
            /* Copy first the lookahead bytes: */
            uInt n = s->stream.avail_in;
            if(n > s->stream.avail_out) {
                n = s->stream.avail_out;
            }
            if(n > 0) {
                zmemcpy((char *)s->stream.next_out, (char *)s->stream.next_in, n);
                next_out += n;
                s->stream.next_out = next_out;
                s->stream.next_in   += n;
                s->stream.avail_out -= n;
                s->stream.avail_in  -= n;
            }
            if(s->stream.avail_out > 0) {
                int     i, c;

                for(i=0; i<(int)(s->stream.avail_out); i++) {
                    c = get_byte(s);
                    if(c == EOF) {
                        break;
                    }
                    *next_out++ = (Byte)c;
                    s->stream.avail_out--;
                }
            }
            len -= s->stream.avail_out;
            s->stream.total_in  += (uLong)len;
            s->stream.total_out += (uLong)len;
            if(len == 0) {
                s->z_eof = 1;
            }
            return (int)len;
        }
        s->z_err = inflate(&(s->stream), Z_NO_FLUSH);

        if(s->z_err == Z_STREAM_END) {
            /* Check CRC and original size */
            s->crc = zcrc32(s->crc, start, (uInt)(s->stream.next_out - start));
            start = s->stream.next_out;

            if(getLong(s) != s->crc) {
                s->z_err = Z_DATA_ERROR;
            } else {
                (void)getLong(s);
                /* The uncompressed length returned by above getlong() may
                 * be different from s->stream.total_out) in case of
                * concatenated .gz files. Check for such files:
                  */
                check_header(s);
                if(s->z_err == Z_OK) {
                    uLong total_in = s->stream.total_in;
                    uLong total_out = s->stream.total_out;

                    inflateReset(&(s->stream));
                    s->stream.total_in = total_in;
                    s->stream.total_out = total_out;
                    s->crc = zcrc32(0L, Z_NULL, 0);
                }
            }
        }
        if(s->z_err != Z_OK || s->z_eof) {
            break;
        }
    }
    s->crc = zcrc32(s->crc, start, (uInt)(s->stream.next_out - start));
    return (int)(len - s->stream.avail_out);
}

/* ===========================================================================
    getLong():
    Reads a long in LSB order from the given gz_stream. Sets z_err in case
    of error.
*/
local uLong
getLong(s)
gz_stream *s;
{
    uLong x = (uLong)get_byte(s);
    int c;

    x += ((uLong)get_byte(s))<<8;
    x += ((uLong)get_byte(s))<<16;
    c = get_byte(s);
    if(c == EOF) {
        s->z_err = Z_DATA_ERROR;
    }
    x += ((uLong)c)<<24;
    return x;
}

/* ===========================================================================
    unZip():
    This is the front end to the whole zlib decompressor.
    It is a chop-up of the original minigzip.c code that came with
    the zlib source.
*/
int
unZip(char *src, int srclen, char *dest, int destlen)
{
    int len;
    gz_stream   *s;

    if((s = gzInit((unsigned char *)src,srclen)) == Z_NULL) {
        printf("gzInit(0x%lx,%d) failed!\n",(ulong)src,srclen);
        return(-1);
    }
    len = gzRead(s,dest,destlen);
    if(len < 0) {
        printf("gzRead() returned %d\n",len);
    }

    destroy(s);

    if(len > 0) {
        flushDcache(dest,len);
        invalidateIcache(dest,len);
    }

    return(len);
}

char *UnzipHelp[] = {
    "Decompress memory (or file) to some other block of memory.",
    "-[v:] {src} [dest]",
    "  src:  addr,len | filename",
    "  dest: addr[,len]",
    "Options:",
    " -v{varname} place decompress size into shellvar",
    0,
};

/* Unzip():
 * Access ZLIB decompressor from monitor command line.
 */
int
Unzip(int argc,char *argv[])
{
    int     opt, tot;
    ulong   srclen, destlen;
    char    *varname, *asc_src, *asc_dst, *comma, *src, *dest;

    destlen = 99999999;
    varname = asc_dst = (char *)0;
    dest = (char *)getAppRamStart();

    while((opt=getopt(argc,argv,"v:")) != -1) {
        switch(opt) {
        case 'v':
            varname = optarg;
            break;
        default:
            return(CMD_PARAM_ERROR);
        }
    }

    if(argc == optind+1) {
        asc_src = argv[optind];
    } else if(argc == optind+2) {
        asc_src = argv[optind];
        asc_dst = argv[optind+1];
    } else {
        return(CMD_PARAM_ERROR);
    }

    comma = strchr(asc_src,',');
    if(comma) {
        *comma = 0;
        src = (char *)strtoul(asc_src,(char **)0,0);
        srclen = strtoul(comma+1,(char **)0,0);
    } else {
        TFILE *tfp;
        tfp = tfsstat(asc_src);
        if(!tfp) {
            printf("%s: file not found\n",asc_src);
            return(CMD_FAILURE);
        }
        src = TFS_BASE(tfp);
        srclen = TFS_SIZE(tfp);
    }

    if(asc_dst) {
        comma = strchr(asc_dst,',');
        if(comma) {
            *comma = 0;
            destlen = strtoul(comma+1,(char **)0,0);
        }
        dest = (char *)strtoul(asc_dst,(char **)0,0);
    }

    tot = unZip(src,srclen,dest,destlen);
    printf("Decompressed %ld bytes from 0x%lx to %d bytes at 0x%lx.\n",
           srclen,(ulong)src,tot,(ulong)dest);

    if(varname) {
        shell_sprintf(varname,"%d",tot);
    }

    return(0);
}

/* Front end to the rest of the unZip() stuff...
    Return the size of the decompressed data or -1 if failure.
    The same front API end is available if unpack is used instead of unzip.
*/
int
decompress(char *src,int srclen, char *dest)
{
    return(unZip(src,srclen,dest,99999999));
}
#else
int
decompress(char *src,int srclen, char *dest)
{
    return(-1);
}
#endif
