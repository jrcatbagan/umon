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
 * jffs2.c
 *
 * Reads JFFS2-formatted NOR flash.
 *
 * Initial version written by Ed Sutter, with contributions later made
 * by Bill Gatliff (bgat@billgatliff.com).
 *
 */
 
#include "config.h"

#if INCLUDE_JFFS2

#include "stddefs.h"
#include "assert.h"
#include "genlib.h"
#include "cli.h"
#include "tfs.h"
#include "tfsprivate.h"
#include "time.h"
#include "flash.h"

#if INCLUDE_JFFS2ZLIB
#include "jz_zlib.h"
#endif

#ifndef NULL
#define NULL ((void*)0)
#endif


/* override JFFS2_DEFAULT_BASE in target's config.h when necessary! */
#ifndef JFFS2_DEFAULT_BASE
#define JFFS2_DEFAULT_BASE 0
#endif

#define JFFS2_MAXPATH 256

#define JFFS2_MODE_MASK 00170000
#define JFFS2_MODE_REG  0100000
#define JFFS2_MODE_LNK  0120000
#define JFFS2_MODE_BLK  0060000
#define JFFS2_MODE_DIR  0040000
#define JFFS2_MODE_CHR  0020000
#define JFFS2_MODE_FIFO 0010000

#define DT_REG  (JFFS2_MODE_REG  >> 12)
#define DT_LNK  (JFFS2_MODE_LNK  >> 12)
#define DT_BLK  (JFFS2_MODE_BLK  >> 12)
#define DT_DIR  (JFFS2_MODE_DIR  >> 12)
#define DT_CHR  (JFFS2_MODE_CHR  >> 12)
#define DT_FIFO (JFFS2_MODE_FIFO >> 12)

#define S_IRWXU 00700
#define S_IRUSR 00400
#define S_IWUSR 00200
#define S_IXUSR 00100

#define S_IRWXG 00070
#define S_IRGRP 00040
#define S_IWGRP 00020
#define S_IXGRP 00010

#define S_IRWXO 00007
#define S_IROTH 00004
#define S_IWOTH 00002
#define S_IXOTH 00001

#define S_IRUGO		(S_IRUSR|S_IRGRP|S_IROTH)
#define S_IWUGO		(S_IWUSR|S_IWGRP|S_IWOTH)
#define S_IXUGO		(S_IXUSR|S_IXGRP|S_IXOTH)

#define is_reg(mode)    (((mode) & JFFS2_MODE_MASK) == JFFS2_MODE_REG)
#define is_lnk(mode)    (((mode) & JFFS2_MODE_MASK) == JFFS2_MODE_LNK)
#define is_blk(mode)    (((mode) & JFFS2_MODE_MASK) == JFFS2_MODE_BLK)
#define is_dir(mode)    (((mode) & JFFS2_MODE_MASK) == JFFS2_MODE_DIR)
#define is_chr(mode)    (((mode) & JFFS2_MODE_MASK) == JFFS2_MODE_CHR)
#define is_fifo(mode)   (((mode) & JFFS2_MODE_MASK) == JFFS2_MODE_FIFO)

#define JFFS2_MAGIC			0x1985

#define JFFS2_COMPAT_MASK		0xc000
#define JFFS2_FEATURE_INCOMPAT		0xc000
#define JFFS2_FEATURE_ROCOMPAT		0x8000
#define JFFS2_FEATURE_RWCOMPAT_COPY	0x4000
#define JFFS2_FEATURE_RWCOMPAT_DELETE	0x0000

#define JFFS2_NODE_ACCURATE		0x2000

#define JFFS2_NODETYPE_MASK		7
#define JFFS2_NODETYPE_DIRENT		1
#define JFFS2_NODETYPE_INODE		2
#define JFFS2_NODETYPE_CLEANMARKER	3
#define JFFS2_NODETYPE_PADDING		4

/* this is a "fake" type, not part of jffs2 proper */
#define JFFS2_NODETYPE_CLEANREGION	8

#define JFFS2_OBSOLETE_FLAG		1
#define JFFS2_UNUSED_FLAG		2

#define JFFS2_COMPR_NONE	0       /* no compression */
#define JFFS2_COMPR_ZERO	1       /* data is all zeroes */
#define JFFS2_COMPR_RTIME	2
#define JFFS2_COMPR_RUBINMIPS	3
#define JFFS2_COMPR_COPY	4
#define JFFS2_COMPR_DYNRUBIN	5
#define JFFS2_COMPR_ZLIB	6

#define JFFS2_FNAME_STR		"JFFS2FNAME"
#define JFFS2_FTOT_STR		"JFFS2FTOT"
#define JFFS2_FSIZE_STR		"JFFS2FSIZE"

/* TODO: find a better way to deal with char vs. jint8 */
typedef unsigned char jint8;
typedef unsigned short jint16;
typedef unsigned int jint32;

struct jffs2_unknown_node {
  jint16  magic;
  jint16  nodetype;
  jint32  totlen;
  jint32  hdr_crc;
};

struct jffs2_raw_dirent {
  jint16  magic;
  jint16  nodetype;
  jint32  totlen;
  jint32  hdr_crc;
  jint32  pino;
  jint32  version;
  jint32  ino;
  jint32  mctime;
  jint8   nsize;
  jint8   type;
  jint8   unused[2];
  jint32  node_crc;
  jint32  name_crc;
  jint8   name[0];
};

struct jffs2_raw_inode {
  jint16  magic;
  jint16  nodetype;
  jint32  totlen;
  jint32  hdr_crc;
  jint32  ino;
  jint32  version;
  jint32  mode;
  jint16  uid;
  jint16  gid;
  jint32  isize;
  jint32  atime;
  jint32  mtime;
  jint32  ctime;
  jint32  offset;
  jint32  csize;
  jint32  dsize;
  jint8   compr;
  jint8   usercompr;
  jint16  flags;
  jint32  data_crc;
  jint32  node_crc;
  jint8   data[0];
};

struct jffs2_cleanregion_node {
  jint16 magic;
  jint16 nodetype;
  jint32 totlen;
  jint32 physaddr;
};

struct jffs2_umoninfo {
  jint8   quiet;
  jint32  direntsize;
  char	  direntname[JFFS2_MAXPATH+1];
};

/*
 * The crc32 function used in this jffs2 command is from MTD utils.
 *
 *  COPYRIGHT (C) 1986 Gary S. Brown.  You may use this program, or
 *  code or tables extracted from it, as desired without restriction.
 *
 *  First, the polynomial itself and its table of feedback terms.  The
 *  polynomial is
 *  X^32+X^26+X^23+X^22+X^16+X^12+X^11+X^10+X^8+X^7+X^5+X^4+X^2+X^1+X^0
 *
 *  Note that we take it "backwards" and put the highest-order term in
 *  the lowest-order bit.  The X^32 term is "implied"; the LSB is the
 *  X^31 term, etc.  The X^0 term (usually shown as "+1") results in
 *  the MSB being 1
 *
 *  Note that the usual hardware shift register implementation, which
 *  is what we're using (we're merely optimizing it by doing eight-bit
 *  chunks at a time) shifts bits into the lowest-order term.  In our
 *  implementation, that means shifting towards the right.  Why do we
 *  do it this way?  Because the calculated CRC must be transmitted in
 *  order from highest-order term to lowest-order term.  UARTs transmit
 *  characters in order from LSB to MSB.  By storing the CRC this way
 *  we hand it to the UART in the order low-byte to high-byte; the UART
 *  sends each low-bit to hight-bit; and the result is transmission bit
 *  by bit from highest- to lowest-order term without requiring any bit
 *  shuffling on our part.  Reception works similarly
 *
 *  The feedback terms table consists of 256, 32-bit entries.  Notes
 *
 *      The table can be generated at runtime if desired; code to do so
 *      is shown later.  It might not be obvious, but the feedback
 *      terms simply represent the results of eight shift/xor opera
 *      tions for all combinations of data and CRC register values
 *
 *      The values must be right-shifted by eight bits by the "updcrc
 *      logic; the shift must be unsigned (bring in zeroes).  On some
 *      hardware you could probably optimize the shift in assembler by
 *      using byte-swap instructions
 *      polynomial $edb88320
 */

static const jint32 jffs2_crc32_table[256] = {
  0x00000000L, 0x77073096L, 0xee0e612cL, 0x990951baL, 0x076dc419L,
  0x706af48fL, 0xe963a535L, 0x9e6495a3L, 0x0edb8832L, 0x79dcb8a4L,
  0xe0d5e91eL, 0x97d2d988L, 0x09b64c2bL, 0x7eb17cbdL, 0xe7b82d07L,
  0x90bf1d91L, 0x1db71064L, 0x6ab020f2L, 0xf3b97148L, 0x84be41deL,
  0x1adad47dL, 0x6ddde4ebL, 0xf4d4b551L, 0x83d385c7L, 0x136c9856L,
  0x646ba8c0L, 0xfd62f97aL, 0x8a65c9ecL, 0x14015c4fL, 0x63066cd9L,
  0xfa0f3d63L, 0x8d080df5L, 0x3b6e20c8L, 0x4c69105eL, 0xd56041e4L,
  0xa2677172L, 0x3c03e4d1L, 0x4b04d447L, 0xd20d85fdL, 0xa50ab56bL,
  0x35b5a8faL, 0x42b2986cL, 0xdbbbc9d6L, 0xacbcf940L, 0x32d86ce3L,
  0x45df5c75L, 0xdcd60dcfL, 0xabd13d59L, 0x26d930acL, 0x51de003aL,
  0xc8d75180L, 0xbfd06116L, 0x21b4f4b5L, 0x56b3c423L, 0xcfba9599L,
  0xb8bda50fL, 0x2802b89eL, 0x5f058808L, 0xc60cd9b2L, 0xb10be924L,
  0x2f6f7c87L, 0x58684c11L, 0xc1611dabL, 0xb6662d3dL, 0x76dc4190L,
  0x01db7106L, 0x98d220bcL, 0xefd5102aL, 0x71b18589L, 0x06b6b51fL,
  0x9fbfe4a5L, 0xe8b8d433L, 0x7807c9a2L, 0x0f00f934L, 0x9609a88eL,
  0xe10e9818L, 0x7f6a0dbbL, 0x086d3d2dL, 0x91646c97L, 0xe6635c01L,
  0x6b6b51f4L, 0x1c6c6162L, 0x856530d8L, 0xf262004eL, 0x6c0695edL,
  0x1b01a57bL, 0x8208f4c1L, 0xf50fc457L, 0x65b0d9c6L, 0x12b7e950L,
  0x8bbeb8eaL, 0xfcb9887cL, 0x62dd1ddfL, 0x15da2d49L, 0x8cd37cf3L,
  0xfbd44c65L, 0x4db26158L, 0x3ab551ceL, 0xa3bc0074L, 0xd4bb30e2L,
  0x4adfa541L, 0x3dd895d7L, 0xa4d1c46dL, 0xd3d6f4fbL, 0x4369e96aL,
  0x346ed9fcL, 0xad678846L, 0xda60b8d0L, 0x44042d73L, 0x33031de5L,
  0xaa0a4c5fL, 0xdd0d7cc9L, 0x5005713cL, 0x270241aaL, 0xbe0b1010L,
  0xc90c2086L, 0x5768b525L, 0x206f85b3L, 0xb966d409L, 0xce61e49fL,
  0x5edef90eL, 0x29d9c998L, 0xb0d09822L, 0xc7d7a8b4L, 0x59b33d17L,
  0x2eb40d81L, 0xb7bd5c3bL, 0xc0ba6cadL, 0xedb88320L, 0x9abfb3b6L,
  0x03b6e20cL, 0x74b1d29aL, 0xead54739L, 0x9dd277afL, 0x04db2615L,
  0x73dc1683L, 0xe3630b12L, 0x94643b84L, 0x0d6d6a3eL, 0x7a6a5aa8L,
  0xe40ecf0bL, 0x9309ff9dL, 0x0a00ae27L, 0x7d079eb1L, 0xf00f9344L,
  0x8708a3d2L, 0x1e01f268L, 0x6906c2feL, 0xf762575dL, 0x806567cbL,
  0x196c3671L, 0x6e6b06e7L, 0xfed41b76L, 0x89d32be0L, 0x10da7a5aL,
  0x67dd4accL, 0xf9b9df6fL, 0x8ebeeff9L, 0x17b7be43L, 0x60b08ed5L,
  0xd6d6a3e8L, 0xa1d1937eL, 0x38d8c2c4L, 0x4fdff252L, 0xd1bb67f1L,
  0xa6bc5767L, 0x3fb506ddL, 0x48b2364bL, 0xd80d2bdaL, 0xaf0a1b4cL,
  0x36034af6L, 0x41047a60L, 0xdf60efc3L, 0xa867df55L, 0x316e8eefL,
  0x4669be79L, 0xcb61b38cL, 0xbc66831aL, 0x256fd2a0L, 0x5268e236L,
  0xcc0c7795L, 0xbb0b4703L, 0x220216b9L, 0x5505262fL, 0xc5ba3bbeL,
  0xb2bd0b28L, 0x2bb45a92L, 0x5cb36a04L, 0xc2d7ffa7L, 0xb5d0cf31L,
  0x2cd99e8bL, 0x5bdeae1dL, 0x9b64c2b0L, 0xec63f226L, 0x756aa39cL,
  0x026d930aL, 0x9c0906a9L, 0xeb0e363fL, 0x72076785L, 0x05005713L,
  0x95bf4a82L, 0xe2b87a14L, 0x7bb12baeL, 0x0cb61b38L, 0x92d28e9bL,
  0xe5d5be0dL, 0x7cdcefb7L, 0x0bdbdf21L, 0x86d3d2d4L, 0xf1d4e242L,
  0x68ddb3f8L, 0x1fda836eL, 0x81be16cdL, 0xf6b9265bL, 0x6fb077e1L,
  0x18b74777L, 0x88085ae6L, 0xff0f6a70L, 0x66063bcaL, 0x11010b5cL,
  0x8f659effL, 0xf862ae69L, 0x616bffd3L, 0x166ccf45L, 0xa00ae278L,
  0xd70dd2eeL, 0x4e048354L, 0x3903b3c2L, 0xa7672661L, 0xd06016f7L,
  0x4969474dL, 0x3e6e77dbL, 0xaed16a4aL, 0xd9d65adcL, 0x40df0b66L,
  0x37d83bf0L, 0xa9bcae53L, 0xdebb9ec5L, 0x47b2cf7fL, 0x30b5ffe9L,
  0xbdbdf21cL, 0xcabac28aL, 0x53b39330L, 0x24b4a3a6L, 0xbad03605L,
  0xcdd70693L, 0x54de5729L, 0x23d967bfL, 0xb3667a2eL, 0xc4614ab8L,
  0x5d681b02L, 0x2a6f2b94L, 0xb40bbe37L, 0xc30c8ea1L, 0x5a05df1bL,
  0x2d02ef8dL
};

static jint32
jffs2crc32(jint32 val, const void *ss, int len)
{
  const unsigned char *s = ss;

  while (--len >= 0)
    val = jffs2_crc32_table[(val ^ *s++) & 0xff] ^ (val >> 8);

  return val;
}

static char *
compr2str(jint8 compr)
{
  switch(compr) {
  case JFFS2_COMPR_NONE:
    return "none";
  case JFFS2_COMPR_ZERO:
    return "zero";
  case JFFS2_COMPR_RTIME:
    return "rtime";
  case JFFS2_COMPR_RUBINMIPS:
    return "rubinmips";
  case JFFS2_COMPR_COPY:
    return "copy";
  case JFFS2_COMPR_DYNRUBIN:
    return "dynrubin";
  case JFFS2_COMPR_ZLIB:
    return "zlib";
  default:
    return "???";
  }
}

static void
showUnknown(int nodenum, struct jffs2_unknown_node *u)
{
  printf("Node %3d (%04x=UNKNOWN, size=%ld) 0x%08lx...\n",
	 nodenum, u->nodetype, u->totlen, (long)u);
  printf("  type:     0x%04x\n",u->nodetype);
}

static void
showPadding(int nodenum, struct jffs2_raw_inode *i)
{
  printf("Node %3d (PADDING, size=%ld) @ 0x%08lx...\n",
	 nodenum, i->totlen, (long)i);
}

static void
showCleanmarker(int nodenum,struct jffs2_raw_inode *i)
{
  printf("Node %3d (CLEANMARKER, size=%ld) @ 0x%08lx...\n",
	 nodenum,i->totlen, (long)i);
}

static void
showInode(int nodenum,struct jffs2_raw_inode *i)
{
  printf("Node %3d (%04x=INODE, size=%ld) @ 0x%08lx",
	 nodenum, i->nodetype, i->totlen, (long)i);
  if (i->dsize)
    printf(" (data at 0x%08lx)",&i->data);
  printf("...\n");
  printf("    ino: 0x%06lx,   mode: 0x%06lx, version: 0x%06lx\n",
	 i->ino,i->mode,i->version);
  printf("  isize: 0x%06lx,  csize: 0x%06lx,   dsize: 0x%06lx\n",
	 i->isize, i->csize, i->dsize);
  printf(" offset: 0x%06lx,  compr: %8s,  ucompr: %s\n",
	 i->offset,compr2str(i->compr),compr2str(i->usercompr));
}

static void
showDirent(int nodenum, struct jffs2_raw_dirent *d)
{
  jint8 i;

  printf("Node %3d (%04x=DIRENT <\"",nodenum,d->nodetype);
  for(i=0;i<d->nsize;i++)
    putchar(d->name[i]);
  printf("\">, size=%ld)",d->totlen);
  printf(" @ 0x%06lx",(long)d);
  if (d->ino == 0)
    printf(" (deleted)");
  if ((d->nodetype & JFFS2_NODE_ACCURATE) == 0)
    printf(" (obsolete)");
  printf("...\n");
  printf("    ino: 0x%06lx,   pino: 0x%06lx, version: 0x%06lx\n",
	 d->ino, d->pino, d->version);
  printf("  nsize: 0x%06lx,   type: 0x%06lx\n",
	 d->nsize, d->type);
}

static void
showCleanregion(int nodenum, struct jffs2_cleanregion_node *c)
{
  printf("Node %3d (%04x=CLEANREGION)...\n",
	 nodenum, c->nodetype);
  printf("    physaddr: 0x%06lx,  size: %d\n",
	 c->physaddr, c->totlen);
}


/* jzlibcpy():
 * A variation on "memcpy", but using JFFS2's zlib decompression...
 */
static int
jzlibcpy(char *src, char *dest, ulong srclen, ulong destlen)
{
#if INCLUDE_JFFS2ZLIB
  z_stream strm;
  int ret;

  if ((strm.workspace = malloc(zlib_inflate_workspacesize())) == 0)
    return(-1);

  if (Z_OK != zlib_inflateInit(&strm)) {
    free(strm.workspace);
    return -1;
  }
        
  strm.next_in = (unsigned char *)src;
  strm.avail_in = srclen;
  strm.total_in = 0;

  strm.next_out = (unsigned char *)dest;
  strm.avail_out = destlen;
  strm.total_out = 0;

  while((ret = zlib_inflate(&strm, Z_FINISH)) == Z_OK);

  zlib_inflateEnd(&strm);
  free(strm.workspace);
  return(strm.total_out);
#else
  printf("INCLUDE_JFFS2ZLIB not set in config.h, can't decompress\n");
  return -1;
#endif
}


static int
is_accurate_node(struct jffs2_unknown_node *u)
{
  return ((u->magic == JFFS2_MAGIC)
	  && (u->nodetype & JFFS2_NODE_ACCURATE)) ? 1 : 0;
}

static int
is_cleanmarker(struct jffs2_unknown_node *u)
{
  jint16 nodetype = u->nodetype & JFFS2_NODETYPE_MASK;
  return nodetype == JFFS2_NODETYPE_CLEANMARKER ? 1 : 0;
}

static int
is_padding(struct jffs2_unknown_node *u)
{
  jint16 nodetype = u->nodetype & JFFS2_NODETYPE_MASK;
  return nodetype == JFFS2_NODETYPE_PADDING ? 1 : 0;
}

static int
is_inode(struct jffs2_unknown_node *u)
{
  jint16 nodetype = u->nodetype & JFFS2_NODETYPE_MASK;
  return nodetype == JFFS2_NODETYPE_INODE ? 1 : 0;
}

static int
is_dirent(struct jffs2_unknown_node *u)
{
  jint16 nodetype = u->nodetype & JFFS2_NODETYPE_MASK;
  return nodetype == JFFS2_NODETYPE_DIRENT ? 1 : 0;
}

static int
is_deleted_dirent(struct jffs2_unknown_node *u)
{
  struct jffs2_raw_dirent *d;

  if (is_dirent(u)) {
    d = (struct jffs2_raw_dirent *)u;
    if (d->ino == 0)
      return 1;    
  }

  return 0;
}

static int
is_cleanregion(struct jffs2_unknown_node *u)
{
  return u->nodetype == JFFS2_NODETYPE_CLEANREGION ? 1 : 0;
}

static struct jffs2_unknown_node *
allocCleanRegion(jint32 base, jint32 size)
{
  struct jffs2_cleanregion_node *c;

  c = (struct jffs2_cleanregion_node*)malloc(sizeof(*c));
  if (c) {
    memset((void*)c, 0, sizeof(*c));
    c->nodetype = JFFS2_NODETYPE_CLEANREGION;
    c->physaddr = base;
    c->totlen = size;
  }
  else
    printf("%s: malloc() failed\n", __func__);
  return (struct jffs2_unknown_node*)c;
}

static jint32
calcHdrCrc(struct jffs2_unknown_node *u)
{
  return jffs2crc32(0, (void*)u,
		    sizeof(*u) - sizeof(u->hdr_crc));
}

static jint32
calcInodeCrc(struct jffs2_raw_inode *i)
{
  return jffs2crc32(0, (void*)i,
		    sizeof(*i) - sizeof(i->node_crc)
		    - sizeof(i->data_crc));
}

static jint32
calcDirentCrc(struct jffs2_raw_dirent *d)
{
  return jffs2crc32(0, (void*)d,
		    sizeof(*d) - sizeof(d->node_crc)
		    - sizeof(d->name_crc));
}

static jint32
calcDataCrc(void *data, int len)
{
  return jffs2crc32(0, data, len);
}

static int jffs2_verify_crc = 0;

static struct jffs2_unknown_node *
findRawNode(ulong jaddr, ulong jlen,
	    ulong *newjaddr, ulong *newjlen)
{
  struct jffs2_unknown_node *u;
  jint32 crc;
  ulong size = 0;
  long adj;

  while (jlen && !gotachar()) {

    adj = ulceil(jaddr, 4) - jaddr;
    jaddr += adj;
    jlen -= adj;
    
    u = (struct jffs2_unknown_node *)jaddr;

    if (is_accurate_node(u)) {

      if (jffs2_verify_crc) {

	crc = calcHdrCrc(u);
	if (crc != u->hdr_crc)
	  printf("hdr_crc @ 0x%x: "
		 "calculated 0x%x read 0x%x (ignored)\n",
		 u, crc, u->hdr_crc);
	  
	if (is_dirent(u)) {
	  struct jffs2_raw_dirent *d;
	    
	  d = (struct jffs2_raw_dirent*)u;
	  crc = calcDirentCrc(d);
	  if (crc != d->node_crc)
	    printf("dirent node_crc @ 0x%x: "
		   "calculated 0x%x read 0x%x (ignored)\n",
		   d, crc, d->node_crc);
	    
	  crc = calcDataCrc(d->name, d->nsize);
	  if (crc != d->name_crc)
	    printf("dirent name_crc @ 0x%x: "
		   "calculated 0x%x read 0x%x (ignored)\n",
		   d, crc, d->name_crc);
	}
	  
	else if (is_inode(u)) {
	  struct jffs2_raw_inode *i;
	    
	  i = (struct jffs2_raw_inode*)u;
	  crc = calcInodeCrc(i);
	  if (crc != i->node_crc)
	    printf("inode node_crc @ 0x%x: "
		   "calculated 0x%x read 0x%x (ignored)\n",
		   i, crc, i->node_crc);
	    
	  crc = calcDataCrc(i->data,
			    (i->compr != JFFS2_COMPR_NONE) ?
			    i->csize : i->dsize);
	  if (crc != i->data_crc)
	    printf("inode data_crc @ 0x%x: "
		   "calculated 0x%x read 0x%x (ignored)\n",
		   i, crc, i->data_crc);
	}
      }

      if (newjaddr) *newjaddr = jaddr;
      if (newjlen) *newjlen = jlen;
      return u;
    }

    else {
      /* manufacture a node representing unused memory */

      /* TODO: more-carefully verify that this memory is truly
	 "unused", e.g. make sure there is a valid cleanmarker at the
	 beginning of the flash block, we don't span the end of a
	 flash section, etc. */

      /* TODO: it would be much faster to use a binary search for ff's
	 in the current flash sector, rather than a linear search for
	 them */

      for (size = 0; jlen && !gotachar()
	     && *(ulong*)jaddr == 0xffffffffUL;
	   size += 4) {
	jaddr += 4;
	jlen -= 4;
      }

      if (size > sizeof(struct jffs2_raw_inode)) {
	if (newjaddr) *newjaddr = jaddr - size;
	if (newjlen) *newjlen = jlen + size;
	return allocCleanRegion((ulong)u, size);
      }

      if (jlen) {
	jaddr += 4;
	jlen -= 4;
      }
    }
  }
  
  return NULL;
}

struct jffs2_unknown_node_list {
  struct jffs2_unknown_node_list *next;
  struct jffs2_unknown_node_list *prev;
  ulong nodenum;
  struct jffs2_unknown_node *u;
};

static int
is_empty_list (struct jffs2_unknown_node_list *l)
{
  if ((l == NULL) || (l->next == NULL)
      || (l->next->next == NULL)) return 1;
  return 0;
}

static struct jffs2_unknown_node_list *
allocListNode(ulong nodenum, struct jffs2_unknown_node *u)
{
  struct jffs2_unknown_node_list *cu;

  cu = (struct jffs2_unknown_node_list*)malloc(sizeof(*cu));
  if (cu) {
    cu->next = NULL;
    cu->prev = NULL;
    cu->u = u;
    cu->nodenum = nodenum;
  }
  else {
    /* TODO: report/handle allocation failure */
    printf("allocListNode: malloc() failed\n");
  }
  return cu;
}

static struct jffs2_unknown_node_list *
removeNodeFromList(struct jffs2_unknown_node_list *u)
{
  if (u->prev != NULL)
    u->prev->next = u->next;
  if (u->next != NULL)
    u->next->prev = u->prev;
  return u;
}

static int
delListNode(struct jffs2_unknown_node_list *u)
{
  removeNodeFromList(u);
  if (u->u != NULL && is_cleanregion(u->u))
    free(u->u);
  free(u);
  return 0;
}

static struct jffs2_unknown_node_list *
initNodeList(struct jffs2_unknown_node_list **a)
{
  /* create "sentinel" nodes */
  *a = allocListNode(-1, NULL);
  if (*a == NULL) return NULL;
  (*a)->next = allocListNode(-1, NULL);
  if ((*a)->next == NULL) return NULL;
  (*a)->next->prev = *a;

  return *a;
}

static struct jffs2_unknown_node_list *
discardNodeList(struct jffs2_unknown_node_list *list)
{
  if (list == NULL)
    return NULL;
  while (list->prev != NULL)
    list = list->prev;
  while (list->next != NULL)
    delListNode(list->next);
  delListNode(list);
  return NULL;
}




static struct jffs2_unknown_node_list *
insertListNodeAfter(struct jffs2_unknown_node_list *after,
		    struct jffs2_unknown_node_list *n)
{
  n->prev = after;
  n->next = after->next;
  after->next->prev = n;
  after->next = n;

  return n;
}

static int
scanMedium(ulong jaddr, ulong jlen,
	   struct jffs2_unknown_node_list *list,
	   void (*progress)(int percent))
{
  struct jffs2_unknown_node *u;
  struct jffs2_unknown_node_list *n;
  ulong nodetot = 0UL;
  ulong jlen_orig = jlen;

  if (progress) {
    printf("scanning JFFS2 medium "
	   "at 0x%x, %d bytes...\n",
	   jaddr, jlen);
    progress(0);
  }

  while (!gotachar()
	 && jlen
	 && (u = findRawNode(jaddr, jlen, &jaddr, &jlen))) {

    if (progress)
      progress(((unsigned long long)(jlen_orig - jlen) * 100)
	       / jlen_orig);
	
    if (is_inode(u)
	|| is_dirent(u)
	|| is_deleted_dirent(u)
	|| is_cleanmarker(u)
	|| is_cleanregion(u)) {

      n = allocListNode(nodetot++, u);
      if (n != NULL)
	insertListNodeAfter(list, n);
      else {
	printf("%s: allocListNode returned NULL (fatal)\n",
	       __func__);
	return -1;
      }
    }
    else printf("%s: unrecognized nodetype %x (ignored)\n",
		__func__, u->nodetype & JFFS2_NODETYPE_MASK);
    
    jlen -= u->totlen;
    jaddr += u->totlen;
  }

  return nodetot;
}

static int
formatMedium (ulong jaddr, ulong jlen,
	      void (*progress)(int percent))
{
  int ret;
  int start, end, sect, size;
  uchar *addr;
  struct jffs2_unknown_node cm = {
    .magic = JFFS2_MAGIC,
    .nodetype = (JFFS2_NODETYPE_CLEANMARKER
		 | JFFS2_FEATURE_RWCOMPAT_DELETE
		 | JFFS2_NODE_ACCURATE),
    .totlen = sizeof(struct jffs2_unknown_node),
  };

  if (!jlen) return -1;

  cm.hdr_crc = calcHdrCrc(&cm);

  if (addrtosector((uchar*)jaddr, &start,
		   NULL, NULL)) {
    printf("%s: invalid starting address %x (error, abort)\n",
	   __func__, jaddr);
    return -1;
  }

  if (addrtosector((uchar*)(jaddr + jlen - 1), &end,
		   NULL, NULL)) {
    printf("%s: invalid ending address %x (error, abort)\n",
	   __func__, jaddr + jlen - 1);
    return -1;
  }

  ret = sectortoaddr(start, NULL, (uchar**)(&addr));
  if (ret || ((ulong)addr != jaddr)) {
    printf("%s: must start at a sector boundary (abort)\n",
	   __func__);
    return -1;
  }

  ret = sectortoaddr(end, &size, (uchar**)(&addr));
  if (ret || ((ulong)(addr + size) != (jaddr + jlen))) {
    printf("%s: must end at a sector boundary (abort)\n",
	   __func__);
    return -1;
  }

  printf("formatting JFFS2 medium "
	 "at 0x%x, %d sectors, %d bytes...\n",
	 jaddr, end - start, jlen);

  for (sect = start; sect < end; sect++) {
    
    if (progress)
      progress(((sect - start) * 100) / (end - start));
    
    if ((ret = AppFlashErase(sect)) != 1) {
      printf("%s: error %d from AppFlashErase, "
	     "sector %d (abort)\n",
	     __func__, ret, sect);
      return -1;
    }
    
    if ((ret = sectortoaddr(sect, &size, (uchar**)(&addr))) != 0) {
      printf("%s: error %d from sectortoaddr(%d, NULL, &addr) (abort)\n",
	     __func__, ret, sect);
      return -1;
    }
    
    if (size < sizeof(cm)) {
      printf("%s: size of sector %d "
	     "is smaller than a CLEANMARKER (abort)\n",
	     __func__, sect);
      return -1;
    }
    
    if ((ret = AppFlashWrite(addr, (void*)&cm, cm.totlen)) != 0) {
      printf("%s: error %d from AppFlashWrite, sector %d (abort)\n",
	     __func__, ret, sect);
      return -1;
    }
  }

  return 0;
}


static int
showNode(ulong nodenum, struct jffs2_unknown_node *u)
{
  if (is_inode(u))
    showInode(nodenum, (struct jffs2_raw_inode*)u);
  else if (is_dirent(u) || is_deleted_dirent(u))
    showDirent(nodenum, (struct jffs2_raw_dirent*)u);
  else if (is_cleanmarker(u))
    showCleanmarker(nodenum, (struct jffs2_raw_inode*)u);
  else if (is_cleanregion(u))
    showCleanregion(nodenum, (struct jffs2_cleanregion_node*)u);
  else if (is_padding(u))
    showPadding(nodenum, (struct jffs2_raw_inode*)u);
  else showUnknown(nodenum, u);
  return 0;
}

static struct jffs2_unknown_node_list *
findNodeOfType(struct jffs2_unknown_node_list *list,
	       jint16 nodetype)
{
  while (list != NULL && !gotachar()) {
    if (list->u != NULL
	&& ((nodetype == JFFS2_NODETYPE_MASK)
	    || ((list->u->nodetype & JFFS2_NODETYPE_MASK) == nodetype)))
      return list;
    list = list->next;
  }
  
  return NULL;
}

static struct jffs2_unknown_node_list *
findCleanRegion(struct jffs2_unknown_node_list *list,
		ulong minsize)
{
  while (list != NULL && !gotachar()) {
    if (list->u != NULL
	&& is_cleanregion(list->u)) {
      
      if (list->u->totlen >= minsize)
	return list;
    }
    list = list->next;
  }
  
  return NULL;
}

/*
 * Finds an inode entry in <list> where u->ino == <ino>.
 *
 * Returns NULL if no inode of the requested number is found. Otherwise:
 *
 *   <version> == -1: the highest-version entry in the list is returned
 *   <version> != -1: the specified entry in the list is returned
 *
 * For ownership and mode information, you want the latest inode
 * entry.  The only time a specific entry is interesting is when
 * replaying the entire inode log to reconstruct its data or other
 * history.
 */
static struct jffs2_unknown_node_list *
findInode(struct jffs2_unknown_node_list *list,
	  jint32 ino, jint32 version)
{
  struct jffs2_raw_inode *i;
  struct jffs2_unknown_node_list *ulatest = NULL;

  while (list != NULL && !gotachar()
	 && (list = findNodeOfType(list, JFFS2_NODETYPE_INODE)) != NULL) {

    i = (struct jffs2_raw_inode*)list->u;
    if (i->ino == ino) {
      if (version != -1 && version == i->version)
	return list;
      else if (version == -1) {
	if (ulatest == NULL) 
	  ulatest = list;
	else if (i->version > ((struct jffs2_raw_inode*)(ulatest->u))->version)
	  ulatest = list;
      }
    }

    list = list->next;
  }

  if (version == -1 && ulatest != NULL)
    return ulatest;

  return NULL;
}

/*
 * Searches <list> for a dirent that matches <pino>, <ino>, and/or
 * <name>.  Returns a pointer to the first matching dirent node found
 * in <list>, including deleted dirent nodes, or NULL if a matching
 * dirent cannot be found.
 */
static struct jffs2_unknown_node_list *
findNextMatchingDirent(struct jffs2_unknown_node_list *list,
		       jint32 pino, jint32 ino, char *name)
{
  struct jffs2_raw_dirent *d;

  while (!gotachar()
	 && (list = findNodeOfType(list, JFFS2_NODETYPE_DIRENT)) != NULL) {
    
    d = (struct jffs2_raw_dirent*)list->u;
    
    if ((pino == -1 || (pino == d->pino))
	&& (ino == -1 || (ino == d->ino))) {
      
      if ((name == NULL)
	  || ((strlen(name) == d->nsize)
	      && !memcmp((char*)name, (char*)(d->name), d->nsize)))
	return list;
    }

    list = list->next;
  }

  return NULL;
}

/*
 * Finds the highest-version, non-deleted dirent that matches the
 * supplied parameters, or NULL if the highest-version dirent is a
 * deleted one
 */
static struct jffs2_unknown_node_list *
findMatchingDirent(struct jffs2_unknown_node_list *list,
		   jint32 pino, jint32 ino, char *name)
{
  struct jffs2_unknown_node_list *u, *uret;
  struct jffs2_raw_dirent *d, *dret;

  for (uret = NULL, u = list; u != NULL
	 && ((u = findNextMatchingDirent(u, pino, ino, name)) != NULL);
       u = u->next) {

    if (uret == NULL) {
      uret = u;
      continue;
    }
    
    dret = (struct jffs2_raw_dirent*)uret->u;
    d = (struct jffs2_raw_dirent*)u->u;
    
    if (d->version > dret->version)
      uret = u;
  }
  
  if (uret == NULL)
    return NULL;

  if (is_deleted_dirent(uret->u))
    return NULL;

  return uret;
}

static struct jffs2_unknown_node_list *
findNodeOfTypeRange(struct jffs2_unknown_node_list *list,
		    jint16 nodetype,
		    char *pino,
		    char *ino)
{
  struct jffs2_raw_dirent *d;
  struct jffs2_raw_inode *i;

  while (!gotachar()
	 && (list = findNodeOfType(list, nodetype)) != NULL) {

    if (is_dirent(list->u)) {
      d = (struct jffs2_raw_dirent*)(list->u);
      if ((pino == NULL || inRange(pino, d->pino))
	  && (ino == NULL || inRange(ino, d->ino)))
	return list;
    }
    
    else if (is_inode(list->u)) {
      i = (struct jffs2_raw_inode*)(list->u);
      if (ino == NULL || inRange(ino, i->ino))
	return list;
    }
    
    list = list->next;
  }
  
  return NULL;
}

static void
showInodeType(int type)
{
  if      (is_reg(type))  printf("-");
  else if (is_dir(type))  printf("d");
  else if (is_lnk(type))  printf("l");
  else if (is_blk(type))  printf("b");
  else if (is_chr(type))  printf("c");
  else if (is_fifo(type)) printf("p");
  else                    printf("?");
}

static void
showInodePerm(int perm)
{
  printf("%s", perm & S_IRUSR ? "r" : "-");
  printf("%s", perm & S_IWUSR ? "w" : "-");
  printf("%s", perm & S_IXUSR ? "x" : "-");

  printf("%s", perm & S_IRGRP ? "r" : "-");
  printf("%s", perm & S_IWGRP ? "w" : "-");
  printf("%s", perm & S_IXGRP ? "x" : "-");

  printf("%s", perm & S_IROTH ? "r" : "-");
  printf("%s", perm & S_IWOTH ? "w" : "-");
  printf("%s", perm & S_IXOTH ? "x" : "-");
}

static int
showInodeMeta(struct jffs2_raw_inode *i)
{
  char buf[80];
  struct tm t;

  showInodeType(i->mode);
  showInodePerm(i->mode);
  gmtime_r(i->atime, &t);
  asctime_r(&t, buf);

  printf(" %4d %4d %6d %s ",
	 i->uid, i->gid, i->isize, buf);
  return 0;
}

static int
showNodesOfType(struct jffs2_unknown_node_list *list,
		jint16 nodetype)
{
  int count = 0;

  while ((list = findNodeOfType(list, nodetype)) != NULL
	 && !gotachar()) {
    showNode(list->nodenum, list->u);
    count++;
    list = list->next;
  }
  return count; 
}

static int
showNodesOfTypeRange(struct jffs2_unknown_node_list *list,
		     jint16 nodetype,
		     char *pino,
		     char *ino)
{
  int count = 0;

  while (list && !gotachar()) {
    list = findNodeOfTypeRange(list, nodetype, pino, ino);
    if (list != NULL && list->u != NULL) {
      showNode(list->nodenum, list->u);
      count++;
    }
    if (list != NULL)
      list = list->next;
  }
  return count; 
}

static struct jffs2_unknown_node_list *
findNodeOfNumRange(struct jffs2_unknown_node_list *list,
		   char *nodenum)
{
  while (!gotachar() && list) {
    
    if (list->u && inRange(nodenum, list->nodenum))
      return list;
    
    list = list->next;
  }
  
  return NULL;
}

static int
showNodesOfNumRange(struct jffs2_unknown_node_list *list,
		    char *range)
{
  int nodetot = 0;

  while (list && (list = findNodeOfNumRange(list, range))) {
    nodetot++;
    showNode(list->nodenum, list->u);
    list = list->next;
  }

  return nodetot;
}

static struct jffs2_unknown_node_list *
findDirentByPath(struct jffs2_unknown_node_list *list,
		 char *path)
{
#define JFFS2_ROOT_PINO 1
#define JFFS2_PATH_SEPARATOR "/"

  char subpath[JFFS2_MAXPATH + 1];
  int pathlen;
  jint32 pino = JFFS2_ROOT_PINO;
  struct jffs2_unknown_node_list *u = NULL;
  struct jffs2_raw_dirent *d;

  if (strlen(path) > JFFS2_MAXPATH
      || (path == NULL))
    return NULL;

  while (*path) {

    path += strspn(path, JFFS2_PATH_SEPARATOR);
    pathlen = strcspn(path, JFFS2_PATH_SEPARATOR);
    strncpy(subpath, path, pathlen);
    subpath[pathlen] = 0;
    path += pathlen;

    u = findMatchingDirent(list, pino, -1, subpath);
    if (u == NULL)
      return NULL;

    d = (struct jffs2_raw_dirent *)u->u;
    pino = d->ino;
  }

  if (is_deleted_dirent(u->u))
    return NULL;
  return u;
}

static int
listDirent(struct jffs2_unknown_node_list *list,
	   struct jffs2_unknown_node_list *u,
	   struct jffs2_umoninfo *umip)
{
  struct jffs2_raw_dirent *d;
  struct jffs2_raw_inode *i;

  d = (struct jffs2_raw_dirent*)u->u;
  
  u = findInode(list, d->ino, -1);
  if (u == NULL)
    return 0;

  i = (struct jffs2_raw_inode*)u->u;
  if (!umip->quiet)
    showInodeMeta(i);
  
  memset(umip->direntname, 0, sizeof(umip->direntname));
  memcpy(umip->direntname, (void*)d->name, d->nsize);
  umip->direntsize = i->isize;
  
  if (is_dir(i->mode))
    umip->direntname[d->nsize] = '/';

  if (!umip->quiet)
    printf("%s\n", umip->direntname);

  return 1;
}

static int
listDirentByPath(struct jffs2_unknown_node_list *list,
		 char *path, struct jffs2_umoninfo *umip)
{
  struct jffs2_unknown_node_list *u = NULL;
  struct jffs2_unknown_node_list *n;
  struct jffs2_raw_dirent *d = NULL;
  struct jffs2_raw_inode *i;
  jint32 pino = JFFS2_ROOT_PINO;
  int ret = 0;
  
  /* TODO: wildcards? */
  
  if (path != NULL && strlen(path) != 0
      && strcspn(path, JFFS2_PATH_SEPARATOR) != 0) {

    u = findDirentByPath(list, path);
    if (u == NULL)
      goto not_found;

    d = (struct jffs2_raw_dirent*)u->u;
    n = findInode(list, d->ino, -1);
    if (n == NULL)
      goto not_found;

    i = (struct jffs2_raw_inode*)n->u;
    if (!is_dir(i->mode))
      return listDirent(list, u, umip);
    else pino = d->ino;
  }
  
  for (u = list; u != NULL
	 && (u = findMatchingDirent(u, pino, -1, NULL)) != NULL;
       u = u->next) {
    ret += listDirent(list, u, umip);
  }
  
  return ret;

 not_found:
  if (!umip->quiet)
    printf("%s: no such file or directory\n", path);
  return 0;

}


static int
readRawInode(struct jffs2_raw_inode *i,
	     jint32 offset /* ... into start of inode */,
	     jint32 len /* ... of bytes to take from inode */,
	     void (*callback)(void *args, int offset, void *buf, int size),
	     void *callback_args)
{
  static void *temp = NULL;

  /* TODO: we could allocate a 4K buffer here, read into that, and
     then pass that buffer to the callback (instead of having the
     callback read from flash directly); this would allow us to put
     jffs2 into something other than bulk NOR flash

     (is there a uMON flash-reading function that handles
     device-specific i/o details?)
  */
  switch (i->compr) {

  case JFFS2_COMPR_NONE:
    callback(callback_args, i->offset + offset,
	     (void*)(i->data + offset), len);
    break;

  case JFFS2_COMPR_ZERO:
    callback(callback_args, i->offset + offset, 0, len);
    break;

  case JFFS2_COMPR_ZLIB:
    if (temp == NULL && ((temp = malloc(4096)) == NULL)) {
      printf("%s: cannot alloc tempspace (aborted)\n",
	     __func__);
      return -1;
    }

    if (i->dsize > 4096) {
      printf("%s: inode dsize > 4K (aborted)\n", __func__);
      return -1;
    }

    if (jzlibcpy((void*)(i->data), temp, i->csize, i->dsize) < 0)
      return -1;
    callback(callback_args, i->offset + offset, temp + offset, len);
    break;

  case JFFS2_COMPR_RTIME:
  case JFFS2_COMPR_RUBINMIPS:
  case JFFS2_COMPR_DYNRUBIN:
    printf("%s: unsupported compression algorithm (fragment ignored)\n",
	   compr2str(i->compr));
    break;
  }
  
  return len;
}

/*
 * Finds a region in <list> a.k.a. "fragment" that overlaps inode <i>,
 * and populates that fragment with the contents of the inode.  If the
 * fragment is fully covered by the contents of the inode, then the
 * fragment is removed from <list>; otherwise, the fragment list node
 * is modified or bisected to record bytes that are still missing.
 *
 * Returns the number of bytes written, or 0 to indicate there were no
 * fragments that overlapped the inode, or a negative number for
 * error.
 *
 * You'll need to call this function with the same inode until it
 * returns zero or an error, since we return at the first occurence of
 * a valid fragment.  Depending on the sequence of inodes, one or more
 * fragments could be covered by the same inode and/or it might take
 * multiple inodes to completely fill a fragment.  That's just how
 * jffs2 and other log-structured filesystems work.
 */
static int
readFrag(struct jffs2_unknown_node_list *list,
	 struct jffs2_raw_inode *i,
	 void (*callback)(void *args, int offset, void *buf, int size),
	 void *callback_args)
{
  jint32 fstart, istart, fend, iend;
  struct jffs2_cleanregion_node *frag;
  struct jffs2_unknown_node *newfrag;

  for ( ; list->next != NULL; list = list->next) {
    
    /* we "shouldn't" have to do this, but since we're emptying the
       list entirely the list pointer is always aimed at the sentinel
       node; skip it if we find it, and use the ->next == NULL case to
       spot the end of the list (contrary to the rest of the code in
       this compilation unit) */
    if (list->u == NULL) continue;

    frag = (struct jffs2_cleanregion_node*)list->u;

    fstart = frag->physaddr;
    istart = i->offset;
    fend = frag->physaddr + frag->totlen;
    iend = i->offset + i->dsize;

    if (fend <= istart || iend <= fstart)
      continue;

    if (fstart >= istart && fend <= iend) {
      delListNode(list);
      return readRawInode(i, fstart - istart, fend - fstart,
			  callback, callback_args);
    }

    if (fstart <= istart && fend <= iend) {
      frag->totlen = istart - fstart;
      return readRawInode(i, 0, fend - istart,
			  callback, callback_args);
    }

    if (fstart >= istart && fend >= iend) {
      frag->physaddr = iend;
      frag->totlen = fend - iend;
      return readRawInode(i, fstart - istart, iend - fstart,
			  callback, callback_args);
    }
 
    frag->totlen = iend - fend;
    newfrag = allocCleanRegion(iend, fend - iend);
    insertListNodeAfter(list, allocListNode(0, newfrag));

    return readRawInode(i, 0, i->dsize, callback, callback_args);
  }

  return 0;
}


static int
readInode(struct jffs2_unknown_node_list *list,
	  jint32 ino,
	  void (*callback)(void *callback_args,
			   int offset, void *buf, int size),
	  void *callback_args)
{
  struct jffs2_unknown_node_list *frags;
  struct jffs2_unknown_node_list *u;
  struct jffs2_unknown_node *fr;
  struct jffs2_unknown_node_list *f;
  struct jffs2_raw_inode *i;
  int ret;
  int len = 0;

  if ((u = findInode(list, ino, -1)) == NULL) {
    printf("%d: no such inode\n", ino);
    return -1;
  }

  i = (struct jffs2_raw_inode*)u->u;
  if (i->isize == 0)
    return 0;

  if (initNodeList(&frags) == NULL) {
    printf("%s: initNodeList() returned NULL (aborted)\n",
	   __func__);
    return -1;
  }

  if ((fr = allocCleanRegion(0, i->isize)) == NULL)
    goto fail_alloc_initial_region;
  if ((f = allocListNode(0, fr)) == NULL)
    goto fail_alloc_initial_node;
  insertListNodeAfter(frags, f);

  /*
   * Note: The convention in most of this compilation unit is for list
   * pointers to point to the first valid list member.  However, in
   * the case of the fragment list we know the caller is going to be
   * removing nodes, including the first valid list member.  Thus,
   * we'll keep the list pointer on the sentinel node since that one
   * never goes away.  (The readFrag() iterator knows about this, and
   * changes its list walking logic accordingly).
   */

  do {
    i = (struct jffs2_raw_inode*)u->u;
    do {
      ret = readFrag(frags, i, callback, callback_args);
      if (ret >= 0) len += ret;
    } while (ret > 0);

    u = findInode(list, ino, i->version - 1);
  } while (!is_empty_list(frags) && u != NULL);

  if (!is_empty_list(frags))
    printf("%s: warning, nonempty fragment list "
	   "(missing inode?) (ignored)\n");

  while(frags->next)
    delListNode(frags->next);
  delListNode(frags);

  return len;

 fail_alloc_initial_node:
  free(fr);
  printf("%s: allocListNode() returned NULL (aborted)\n",
	 __func__);
  return 0;
 fail_alloc_initial_region:
  printf("%s: allocCleanRegion() returned NULL (aborted)\n",
	 __func__);
  return 0;
}

static int
readDirent(struct jffs2_unknown_node_list *list, char *path,
	   void (*callback)(void *args,
			    int offset, void *buf, int size),
	   void *callback_args)
{
  struct jffs2_unknown_node_list *u;
  struct jffs2_raw_dirent *d;
  struct jffs2_raw_inode *i;

  u = findDirentByPath(list, path);
  if (u == NULL)
    goto no_such_file;
  
  d = (struct jffs2_raw_dirent *)u->u;
  u = findInode(list, d->ino, -1);
  if (u == NULL)
    goto no_such_inode;

  i = (struct jffs2_raw_inode*)u->u;
  if (!is_reg(i->mode))
    goto not_a_file;

  return readInode(list, d->ino, callback, callback_args);
  
 no_such_file:
  printf("%s: no such file or directory\n", path);
  return -1;

 no_such_inode:
  printf("%d: no such inode\n", d->ino);
  return -1;

 not_a_file:
  printf("%s: is %s\n", path,
	 is_dir(i->mode) ? "a directory" : "not a file");
  return -1;
}

static struct jffs2_unknown_node *
commitRawNode(struct jffs2_unknown_node_list *list,
	      struct jffs2_unknown_node *u)
{
  struct jffs2_unknown_node_list *c;
  unsigned long adjtotlen;
  int ret;

  /* everything is 32-bit aligned in jffs2-land, so you always need a
     region slightly larger than the data itself to accomodate the
     address and length rounding we may apply later on */
  c = findCleanRegion(list, u->totlen + 4);
  if (c != NULL) {

    struct jffs2_cleanregion_node *f = (struct jffs2_cleanregion_node*)c->u;

    ret = AppFlashWrite((void*)(f->physaddr), (void*)u, u->totlen);
    if (ret != 0)
      printf("%s: AppFlashWrite returned %d (ignored)\n", __func__, ret);

    ret = f->physaddr;

    /* cleanregions must always begin on a 32-bit boundary */
    adjtotlen = ulceil(u->totlen, 4);
    f->totlen -= adjtotlen;
    f->physaddr += adjtotlen;

    return (struct jffs2_unknown_node*)ret;
  }

  return NULL;
}

static struct jffs2_raw_dirent *
allocDirentNode (char *name)
{
  struct jffs2_raw_dirent *d;
  int namelen = 0;

  if (name != NULL)
    namelen = strlen(name);

  d = (struct jffs2_raw_dirent*)malloc(sizeof(*d) + namelen);
  if (d == NULL) {
    printf("%s: malloc() returned NULL (aborted)\n", __func__);
    return NULL;
  }

  memset((void*)d, 0, sizeof(*d));

  d->magic = JFFS2_MAGIC;
  d->nodetype = JFFS2_NODETYPE_DIRENT
    | JFFS2_NODE_ACCURATE
    | JFFS2_FEATURE_ROCOMPAT
    | JFFS2_FEATURE_RWCOMPAT_COPY;
  memcpy((void*)d->name, name, d->nsize = namelen);
  d->totlen = sizeof(*d) + d->nsize;

  return d;
}

static void
freeDirentNode(struct jffs2_raw_dirent *d)
{
  free(d);
}


static jint32
nextPinoVersion(struct jffs2_unknown_node_list *list, jint32 pino)
{
  jint32 version = 0;

  while(list) {
    if (list->u && is_dirent(list->u)) {
      struct jffs2_raw_dirent *d = (struct jffs2_raw_dirent*)list->u;
      
      if (d->pino == pino && d->version > version)
	version = d->version;
    }
    list = list->next;
  }

  return version + 1;
}

static struct jffs2_unknown_node_list *
commitDirent(struct jffs2_unknown_node_list *list, ulong nodenum, 
	     char *name, jint8 type, jint32 pino, jint32 ino,
	     jint32 version)
{
  struct jffs2_raw_dirent *d;
  struct jffs2_unknown_node *dc;
  struct jffs2_unknown_node_list *n;

  d = allocDirentNode(name);
  if (d == NULL)
    return NULL;

  d->type = type;
  d->version = version;
  d->pino = pino;
  d->ino = ino;

  d->hdr_crc = calcHdrCrc((struct jffs2_unknown_node *)d);
  d->node_crc = calcDirentCrc(d);
  d->name_crc = calcDataCrc(d->name, d->nsize);

  dc = commitRawNode(list, (struct jffs2_unknown_node*)d);
  freeDirentNode(d);

  if (dc == NULL) /* TODO: error handling? */
    return NULL;
  
  if ((n = allocListNode(nodenum, dc)) != NULL)
    return insertListNodeAfter(list, n);

  return NULL;
}

static struct jffs2_unknown_node_list *
moveDirent(struct jffs2_unknown_node_list *list,
	   int nodetot, char *oldpath, char *newpath)
{
  struct jffs2_unknown_node_list *u, *ui, *uold, *unew;
  struct jffs2_raw_dirent *d = NULL;
  struct jffs2_raw_inode *i;
  jint32 pino;
  jint32 nextver;

  char newname[JFFS2_MAXPATH + 1];

  uold = findDirentByPath(list, oldpath);
  if (uold == NULL)
    goto no_such;
  d = (struct jffs2_raw_dirent*)uold->u;

  unew = findDirentByPath(list, newpath);
  if (unew != NULL) {
    struct jffs2_raw_dirent *dnew;

    dnew = (struct jffs2_raw_dirent*)unew->u;
    ui = findInode(list, dnew->ino, -1);
    i = (struct jffs2_raw_inode*)ui->u;
    if (!is_dir(i->mode))
      goto in_the_way;

    pino = dnew->ino;
    strncpy(newname, (void*)d->name, d->nsize);
    newname[d->nsize] = 0;
  }
  else {
    strcpy(newname, newpath);
    pino = JFFS2_ROOT_PINO;
  }
  
  nextver = nextPinoVersion(list, pino);
  u = commitDirent(list, nodetot++, newname,
		   d->type, pino, d->ino, nextver++);
  /* TODO: error handling? */
  u = commitDirent(list, nodetot++, oldpath,
		   d->type, d->pino, 0, nextver);
  return u;
  
 no_such:
  printf("%s: no such file or directory\n", oldpath);
  return NULL;

 in_the_way:
  /* (yes, we could delete this ourselves...) */
  printf("%s already exists\n", newpath);
  return NULL;
}

static struct jffs2_unknown_node_list *
deleteDirent(struct jffs2_unknown_node_list *list,
	     int nodetot, char *path)
{
  struct jffs2_unknown_node_list *u, *ui;
  struct jffs2_raw_dirent *d;
  struct jffs2_raw_inode *i;
  jint32 nextver;
  char name[JFFS2_MAXPATH + 1];

  u = findDirentByPath(list, path);
  if (u == NULL)
    goto no_such;
  d = (struct jffs2_raw_dirent*)u->u;

  ui = findInode(list, d->ino, -1);
  i = (struct jffs2_raw_inode*)ui->u;
  if (is_dir(i->mode)) {
    /* TODO: make sure directory is empty */
    printf("%s: is a directory (abort)\n", path);
    return NULL;
  }

  strncpy(name, (void*)d->name, d->nsize);
  name[d->nsize] = 0;

  nextver = nextPinoVersion(list, d->pino);
  u = commitDirent(list, nodetot++, name,
		   d->type, d->pino, 0, nextver);

  return u;
  
 no_such:
  printf("%s: no such file or directory\n", path);
  return NULL;
}

#define JFFS2_DEFAULT_UID 0
#define JFFS2_DEFAULT_GID 0

static struct jffs2_raw_inode *
allocInode(jint32 ino, jint32 dsize)
{
  struct jffs2_raw_inode *i;

  i = (struct jffs2_raw_inode*)malloc(sizeof(*i) + dsize);
  if (i == NULL) {
    printf("%s: malloc() returned NULL (aborted)\n", __func__);
    return NULL;
  }

  memset((void*)i, 0, sizeof(*i) + dsize);

  i->magic = JFFS2_MAGIC;
  i->ino = ino;
  i->nodetype = JFFS2_NODETYPE_INODE
    | JFFS2_NODE_ACCURATE
    | JFFS2_FEATURE_ROCOMPAT
    | JFFS2_FEATURE_RWCOMPAT_COPY;
  i->compr = JFFS2_COMPR_NONE;
  i->dsize = i->csize = dsize;
  i->totlen = sizeof(*i) + dsize;

  return i;  
}

static void
freeInode(struct jffs2_raw_inode *i)
{
  free(i);
}

static jint32
getUnusedIno(struct jffs2_unknown_node_list *list)
{
  /* by definition, the root pino is always taken... */
  jint32 ino = JFFS2_ROOT_PINO + 1;

  while (!gotachar()
	 && (list = findNodeOfType(list, JFFS2_NODETYPE_INODE)) != NULL) {
    struct jffs2_raw_inode *i = (struct jffs2_raw_inode*)list->u;
    if (i->ino > ino)
      ino = i->ino + 1;
    list = list->next;
  }

  if (gotachar())
    return -1;
  return ino;
}



/* TODO: deleteDirectory() a.k.a. rmdir */

static struct jffs2_unknown_node_list *
appendInode(struct jffs2_unknown_node_list *list,
	    int nodenum,
	    char *path,
	    void *data,
	    int len)
{
  struct jffs2_unknown_node_list *n;
  struct jffs2_raw_inode *iprev, *inew;
  struct jffs2_raw_dirent *d;
  struct jffs2_unknown_node *ic;

  /* TODO: break append up into smaller nodes, if there isn't a single
     region large enough */

  /* TODO: don't deal with large appends yet (nodes are not permitted
     to exceed 4K in size per jffs2) */
  if (len >= 4096) {
    printf("inode extension by >= 4K not supported (yet)\n");
    return NULL;
  }

  n = findDirentByPath(list, path);
  if (n == NULL)
    goto not_found;
  d = (struct jffs2_raw_dirent*)n->u;

  n = findInode(list, d->ino, -1);
  if (n == NULL)
    goto not_found;
  iprev = (struct jffs2_raw_inode*)n->u;

  inew = allocInode(iprev->ino, len);
  if (inew == NULL)
    return NULL;

  inew->ino = iprev->ino;
  inew->version = iprev->version + 1;
  inew->mode = iprev->mode;
  inew->uid = iprev->uid;
  inew->gid = iprev->gid;
  inew->atime = iprev->atime;
  inew->mtime = iprev->mtime;
  inew->ctime = iprev->ctime;

  inew->offset = inew->isize;
  inew->isize += len;
  inew->compr = JFFS2_COMPR_NONE;
  inew->usercompr = 0;
  memcpy((void*)inew->data, data, len);

  inew->hdr_crc = calcHdrCrc((struct jffs2_unknown_node*)inew);
  inew->node_crc = calcInodeCrc(inew);
  inew->data_crc = calcDataCrc(inew->data, inew->dsize);

  ic = commitRawNode(list, (struct jffs2_unknown_node*)inew);
  freeInode(inew);

  if (ic == NULL) /* TODO: error handling? */
    return NULL;

  if ((n = allocListNode(nodenum++, ic)) != NULL)
    return insertListNodeAfter(list, n);

  return NULL;

 not_found:
  printf("%s: no such file or directory\n", path);
  return NULL;
}

static struct jffs2_unknown_node_list *
createInode(struct jffs2_unknown_node_list *list,
	    int nodenum,
	    jint32 mode,
	    char *path)
{
  struct jffs2_unknown_node_list *n;
  struct jffs2_raw_inode *i;
  struct jffs2_unknown_node *ic;
  char *ppath = path;
  jint32 pino = 1;
  jint32 ino;
  jint8 type;
  char name[JFFS2_MAXPATH + 1];

  n = findDirentByPath(list, path);
  if (n != NULL)
    goto already_exists;

  while (*ppath && strstr(ppath, JFFS2_PATH_SEPARATOR) != NULL)
    ppath++;

  if (ppath != path) {
    struct jffs2_raw_dirent *d;
    strncpy(name, path, ppath - path - 1);
    name[ppath - path - 1] = 0;

    n = findDirentByPath(list, name);
    if (n == NULL)
      goto not_found;
    d = (struct jffs2_raw_dirent*)n->u;
    pino = d->ino;
  }

  ino = getUnusedIno(list);
  if (ino == -1)
    return NULL;

  i = allocInode(ino, 0);
  if (i == NULL)
    return NULL;

  i->mode = mode;
  i->uid = JFFS2_DEFAULT_UID;
  i->gid = JFFS2_DEFAULT_GID;
  i->version = 1;

  i->hdr_crc = calcHdrCrc((struct jffs2_unknown_node*)i);
  i->node_crc = calcInodeCrc(i);
  i->data_crc = 0;

  ic = commitRawNode(list, (struct jffs2_unknown_node*)i);
  freeInode(i);

  if (ic == NULL) /* TODO: error handling? */
    return NULL;
  
  if ((n = allocListNode(nodenum++, ic)) == NULL
      || insertListNodeAfter(list, n) == NULL)
    return NULL;
  
  if (is_lnk(mode)) type = DT_LNK;
  else if (is_blk(mode)) type = DT_BLK;
  else if (is_dir(mode)) type = DT_DIR;
  else if (is_chr(mode)) type = DT_CHR;
  else if (is_fifo(mode)) type = DT_FIFO;
  else type = DT_REG;

  i = (struct jffs2_raw_inode*)ic;
  
  return commitDirent(list, nodenum, ppath, type,
		      pino, i->ino, nextPinoVersion(list, pino));

 already_exists:
  printf("%s: file exists\n", path);
  return NULL;

 not_found:
  printf("%s: no such file or directory\n", name);
  return NULL;
}

struct jffs2_callback_args {
  void *dest;
};

static void
callback_to_memory (void *vargs, int offset,
		    void *src, int size)
{
  struct jffs2_callback_args *args = vargs;
  if (src == NULL)
    memset(args->dest + offset, 0, size);
  else
    memcpy(args->dest + offset, src, size);
}

static void
callback_to_console (void *vargs, int offset,
		     void *src, int size)
{
  char *cp = src;

  if (src == NULL)
    return;

  for (cp = src; size > 0; size--)
    putchar(*cp++);
}

char *Jffs2Help[] = {
  "Read and write JFFS2-formatted flash space",
  "[b:cs:] {operation} [args]...",
#if INCLUDE_VERBOSEHELP
  "",
  "Options:",
  " -b {addr}    base address of JFFS2 space (note 1)",
  " -c           check CRCs of all nodes (note 2)",
  " -s {size}    size of JFFS2 space {note 4}",
  " -q           quiet operation",
  "",
  "Operations:",
  " add   {fname} {address} {bytes}",
  "              append {bytes} bytes at {address} to file {fname} (note 3)",
  " cat   {fname}  dump contents of ASCII file {fname} to console",
  " cp    {fname} {to_fname}",
  "              copy a file {fname} to {to_fname}",
  " get   {fname} {to_addr}",
  "              dump contents of file {fname} to {to_addr}",
  " ls    [path]   list files and/or dirs of the specified [path]",
  " mkdir {path} create a directory named {path}",
  " mv    {path} {to_path}",
  "              relocate a file/directory from {path} to {to_path}",
  " rm    {fname}  delete file named {fname}", 
  " mkfs  [{{addr} {size}}]",
  "              build an empty JFFS2 filesystem (notes 1,4)",
  "",
  "Additional operations:",
  " dump         display all nodes",
  " dirent       display all dirent nodes",
  " inode        display all inode nodes",
  " ino   [rng]  display nodes with specified ino field",
  " node  [rng]  display nodes of specified numerical range",
  " pino  [rng]  display nodes with specified pino field",
  " rescan       discard current node list, if any, then scan",
  " scan         scan filesystem image only",

#if 0
  " zinf {src} {dest} {srclen} {destlen}",
  "              inflate using JFFS2's zlib",
#endif
  "",
  "Notes:",
  "   1.  Base address defaults to $JFFS2BASE or zero, if not specified",
  "   2.  Defaults to off",
  "   3.  If {fname} does not exist, it is created",
  "   4.  Size defaults to $JFFS2SIZE or zero, if not specified",
#endif
  0
};

static void
progress_callback(int percent)
{
  static int prev_percent;

  if (prev_percent == percent)
    return;

  prev_percent = percent;
  printf("\r%3d%%\r",
	 percent > 100 ? 100 : percent);
}

int
Jffs2Cmd(int argc, char *argv[])
{
  ulong jaddr, jsize;
  struct jffs2_unknown_node *u;
  int opt;
  char *env, *cmd, *fname, *range;
  int bytes, nodes;
  void (*progress)(int percent) = progress_callback;

  static int nodetot = 0;
  static struct jffs2_unknown_node_list *nodeList = NULL;

  jint32 jffs2_base = JFFS2_DEFAULT_BASE;
  jint32 jffs2_size = 0;

  assert(sizeof(jint32) == 4);
  assert(sizeof(jint16) == 2);
  assert(sizeof(jint8) == 1);

  jffs2_verify_crc = 0;

  if ((env = getenv("JFFS2BASE")))
    jffs2_base = (jint32)strtoul(env,0,0);

  if ((env = getenv("JFFS2SIZE")))
    jffs2_size = (jint32)strtoul(env,0,0);

  while ((opt=getopt(argc,argv,"b:cs:q")) != -1) {
    switch(opt) {
    case 'b':
      jffs2_base = (jint32)strtoul(optarg,0,0);
      break;
    case 'c':
      jffs2_verify_crc = 1;
      break;
    case 's':
      jffs2_size = (jint32)strtoul(optarg,0,0);
      break;
    case 'q':
      progress = NULL;
      break;
    default:
      return(CMD_PARAM_ERROR);
    }
  }

  if (argc < optind + 1)
    return(CMD_PARAM_ERROR);

  cmd = argv[optind];

  if (strcmp(cmd, "mkfs") == 0) {
    if (argc <= optind + 1)
      jaddr = jffs2_base;
    else
      jaddr = strtoul(argv[optind + 1], 0, 0);
    if (argc <= optind + 2)
      jsize = jffs2_size;
    else
      jsize = strtoul(argv[optind + 2], 0, 0);

    nodeList = discardNodeList(nodeList);

    /* TODO: progress feedback needs to be optional? */
    return formatMedium(jaddr, jsize, progress) ?
      CMD_FAILURE : CMD_SUCCESS;
  }

  if (strcmp(cmd, "rescan") == 0)
    nodeList = discardNodeList(nodeList);

  /* commands below this point require a scanMedium() */

  if (nodeList == NULL) {
    initNodeList(&nodeList);
    if (nodeList != NULL) {
      nodetot = scanMedium(jffs2_base, jffs2_size, nodeList, progress);
      while (nodeList->prev)
	nodeList = nodeList->prev;
    }
  }

  if (strcmp(cmd, "scan") == 0
      || strcmp(cmd, "rescan") == 0)
    return CMD_SUCCESS;

  jaddr = jffs2_base;
  u = (struct jffs2_unknown_node *)jaddr;
  
  if (strcmp(cmd,"dump") == 0) {
    printf("JFFS2 base: 0x%lx\n",jffs2_base);
    printf("JFFS2 size: 0x%lx\n",jffs2_size);    
    nodes = showNodesOfNumRange(nodeList, "all");
    printf("%d nodes\n", nodes);
    return CMD_SUCCESS;
  }
  else if (strcmp(cmd,"node") == 0) {
    if (argc == optind + 1)
      range = "all";
    else
      range = argv[optind + 1];
    nodes = showNodesOfNumRange(nodeList, range);
    return CMD_SUCCESS;
  }

  /* if you want a range of dirents or inodes, you need to specify the
     ino and/or pino; don't use "dirent" or "inode" */
  else if (strcmp(cmd,"dirent") == 0) {
    nodes = showNodesOfType(nodeList, JFFS2_NODETYPE_DIRENT);
    printf("%d nodes\n", nodes);
  }
  else if (strcmp(cmd,"inode") == 0) {
    nodes = showNodesOfType(nodeList, JFFS2_NODETYPE_INODE);
    printf("%d nodes\n", nodes);
  }

  /* TODO: this can be reimplemented just like "ino"? */
  else if (strcmp(cmd,"pino") == 0) {
    jint32 pino;
    struct jffs2_unknown_node_list *u = nodeList;
    
    if (argc != optind+2)
      return CMD_PARAM_ERROR;
    
    pino = strtol(argv[optind+1],0,0);
    
    do {
      u = findNextMatchingDirent(u, pino, -1, NULL);
      if (u != NULL && u->u != NULL) {
	showNode(u->nodenum, u->u);
	u = u->next;
      }
    } while (u != NULL && u->u != NULL);
  }

  else if (strcmp(cmd,"ino") == 0) {

    if (argc == optind+1)
      range = "all";
    else
      range = argv[optind+1];

    nodes = showNodesOfTypeRange(nodeList,
				 JFFS2_NODETYPE_DIRENT, NULL, range);
    nodes += showNodesOfTypeRange(nodeList,
				  JFFS2_NODETYPE_INODE, NULL, range);
    printf("%d nodes\n", nodes);
    return CMD_SUCCESS;
  }

  else if (strcmp(cmd,"cp") == 0) {
    printf("TODO: jffs2 cp {fname} {to_fname}\n"
	   "(it's harder than it sounds!)\n");
    return CMD_SUCCESS;
  }

  else if (strcmp(cmd,"get") == 0) {
    struct jffs2_callback_args args;

    if (argc == (optind + 3))
      args.dest = (void*)strtoul(argv[optind + 2], 0, 0);
    else 
      return CMD_PARAM_ERROR;

    setenv(JFFS2_FSIZE_STR,0);
    fname = argv[optind + 1];
    bytes = readDirent(nodeList, fname, callback_to_memory, &args);
    if (bytes >= 0)
      shell_sprintf(JFFS2_FSIZE_STR,"%d",bytes);
    printf("%d bytes\n", bytes);
    return CMD_SUCCESS;
  }

  else if (strcmp(cmd,"cat") == 0) {
    if (argc != optind + 2)
      return CMD_PARAM_ERROR;
    
    setenv(JFFS2_FSIZE_STR,0);
    fname = argv[optind + 1];
    bytes = readDirent(nodeList, fname, callback_to_console, NULL);
    if (bytes >= 0)
      shell_sprintf(JFFS2_FSIZE_STR,"%d",bytes);
    return CMD_SUCCESS;
  }

  else if (strcmp(cmd, "mkdir") == 0) {
    if (argc != optind + 2)
      return CMD_PARAM_ERROR;

    createInode(nodeList, nodetot++,
		JFFS2_MODE_DIR
		| S_IRUGO | S_IXOTH | S_IXGRP | S_IRWXU,
		argv[optind + 1]);

    return CMD_SUCCESS;
  }

  else if (strcmp(cmd, "chmod") == 0) {
    printf("TODO: implement chmod\n");
    return CMD_SUCCESS;
  }
  
  else if (strcmp(cmd, "add") == 0) {
    if (argc != (optind + 4))
      return CMD_PARAM_ERROR;

    char *path;
    long addr, len;

    path = argv[optind + 1];
    addr = strtol(argv[optind + 2], 0, 0);
    len = strtol(argv[optind + 3], 0, 0);

    if (findDirentByPath(nodeList, path) == NULL)
      createInode(nodeList, nodetot++,
		  JFFS2_MODE_REG | S_IRUGO | S_IWUSR,
		  path);

    appendInode(nodeList, nodetot++, path, (void*)addr, len);
    return CMD_SUCCESS;
  }

  else if (strcmp(cmd, "mv") == 0 && (argc == (optind + 3))) {
    char *oldname, *newname;

    oldname = argv[optind + 1];
    newname = argv[optind + 2];

    moveDirent(nodeList, nodetot, oldname, newname);
    return CMD_SUCCESS;
  }
  
  else if(strcmp(cmd, "rm") == 0 && (argc == (optind + 2))) {
    deleteDirent(nodeList, nodetot, argv[optind + 1]);
    return CMD_SUCCESS;
  }

  else if ((strcmp(cmd, "ls") == 0) || (strcmp(cmd, "qry") == 0)) {
    int ftot;
    char *path;
    struct jffs2_umoninfo ju;

    /* If the sub-command is "qry", then we do essentially the same
     * thing as "ls" except quietly (just populate the shell variables).
     */
    if (cmd[0] == 'q')
      ju.quiet = 1;
    else
      ju.quiet = 0;

    ju.direntsize = -1;
    ju.direntname[0] = 0;
    setenv(JFFS2_FNAME_STR,0);
    setenv(JFFS2_FSIZE_STR,0);

    if (argc == optind + 2)
      path = argv[optind + 1];
    else
      path = NULL;
    ftot =  listDirentByPath(nodeList, path, &ju);
    if ((ftot > 0) && (!ju.quiet))
      printf(" Total: %d\n",ftot);

    shell_sprintf(JFFS2_FTOT_STR,"%d",ftot);

    if (ju.direntsize > 0) {
      shell_sprintf(JFFS2_FSIZE_STR,"%d",ju.direntsize);
      setenv(JFFS2_FNAME_STR,ju.direntname);
    }

    return CMD_SUCCESS;
  }

  printf("jffs2 cmd <%s> not found\n",cmd);
  return CMD_FAILURE;
}

#endif
