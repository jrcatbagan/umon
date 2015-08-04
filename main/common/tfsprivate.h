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
 * tfsprivate.h:
 *
 * Header file for TFS used only by the monitor code.
 * (that is... this should not be used by anything under umon_apps)
 *
 * Original author:     Ed Sutter (ed.sutter@alcatel-lucent.com)
 *
 */
#ifndef _TFSPRIVATE_H_
#define _TFSPRIVATE_H_

#define SANITY          0xBEEF
#define ERASED16        0xFFFF
#define ERASED32        0xFFFFFFFF
#define TFS_MAXOPEN     10      /* Maximum of 10 open files */
#define TFS_QRYTMOUT    100000  /* Timeout for QRYBRUN */
#define TFS_FSIZEMOD    16      /* File size must be a multiple of this value */
#define MINUSRLEVEL     0       /* Minimum user level supported. */
#define MAXUSRLEVEL     3       /* Maximum user level supported. */

#define TFSHDRVERSION   1       /* Increment this if TFS header changes. */

#if TFS_EBIN_COFF
#define TFS_EBIN_NAME   "coff"
#elif TFS_EBIN_AOUT
#define TFS_EBIN_NAME   "aout"
#elif TFS_EBIN_ELF
#define TFS_EBIN_NAME   "elf"
#elif TFS_EBIN_MSBIN
#define TFS_EBIN_NAME   "msbin"
#elif TFS_EBIN_ELFMSBIN
#define TFS_EBIN_NAME   "elf_msbin"
#else
#define TFS_EBIN_NAME   "NA"
#endif

/* Two different types of Defragmentation tests:
 * sector erase and flash write.
 */
#define DEFRAG_TEST_SERASE  1
#define DEFRAG_TEST_FWRITE  2
#define DEFRAG_TEST_EXIT    3

/* Masks used to allow flags and modes to be part of the same long word. */
#define TFS_FLAGMASK    0x0000ffff
#define TFS_MODEMASK    0xffff0000

/* Definitions related to tfslog: */
#define TFSLOG_ADD      0
#define TFSLOG_DEL      1
#define TFSLOG_IPM      2
#define TFSLOG_ON       3
#define TFSLOG_OFF      4
#define TIME_UNDEFINED  0xffffffff

/* Used by tfsscript.. */
#define EXIT_SCRIPT         (1 << 0)
#define REMOVE_SCRIPT       (1 << 1)
#define EXECUTE_AFTER_EXIT  (1 << 2)
#define EXIT_ALL_SCRIPTS    (1 << 3)

/* Device Table structure and definitions:
 *  The Device table is typically only one entry in length.  It is defined
 *  on a per-target basis in the file tfsdev.h.  In the simplest case, the
 *  table in tfsdev.h has all of the information in it.  This is fine as long
 *  as there is no need to support different devices with the same monitor
 *  binary.  To support the ability to have a device-table that is constructed
 *  based on the type of flash on-board, the TFS_DEVINFO_DYNAMIC bit must
 *  be set in the devinfo member of tfsdevtbl (in tfsdev.h).  This tells TFS
 *  to figure out the addresses based on a few assumptions...
 *   * Regardless of the device type, the start address will be the same.
 *   * The spare sector starts immediately after the end of TFS storage space.
 *  In dynamic mode, all that needs to be specified in the tfsdevtbl of
 *  tfsdev.h is a prefix, start and devinfo fields.  The other fields will
 *  be built based on information taken from the flash interface.
 *  One important note: if there is a block of contiguous flash space that
 *  spans accross multiple flash banks, then the bank # of the LAST bank in
 *  that block is what should be specified in the devinfo BANKMASK field.
 */

#define TFSEOT  0xffffffff
#define TDEV    struct tfsdev

#define TFS_DEVTYPE_RAM             0x00100000
#define TFS_DEVTYPE_FLASH           0x00200000
#define TFS_DEVTYPE_NVRAM           0x00300000
#define TFS_DEVTYPE_MASK            0x00f00000
#define TFS_DEVINFO_DYNAMIC         0x00080000
#define TFS_DEVINFO_AUTOINIT        0x00040000
#define TFS_DEVINFO_BANKMASK        0x00000fff
#define TFSDEVTOT ((sizeof(tfsdevtbl))/(sizeof(struct tfsdev)))

#define TFS_DEVTYPE_ISRAM(tdp)  \
    (((tdp->devinfo & TFS_DEVTYPE_MASK) == TFS_DEVTYPE_RAM) || \
     ((tdp->devinfo & TFS_DEVTYPE_MASK) == TFS_DEVTYPE_NVRAM))

struct tfsdev {
    char            *prefix;        /* Device name prefix.                  */
    unsigned long   start;          /* First location into which TFS can    */
    /* begin to store files.                */
    unsigned long   end;            /* Last address into which TFS can      */
    /* place file data.  This is usually    */
    /* one unit below the start of the      */
    /* spare sector.                        */
    unsigned long   spare;          /* Start address of device spare        */
    /* sector.                              */
    unsigned long   sparesize;      /* Size of device spare sector.         */
    unsigned long   sectorcount;    /* Number of sectors in this device.    */
    unsigned long   devinfo;        /* RAM or FLASH, etc...                 */
};

/* TFS defragmentation header:
 * This header is used during defragmentation to record the state of the
 * files in flash prior to the start of the defragmentation process.
 */
struct defraghdr {
    struct tfshdr *ohdr;        /* Original location of header. */
    struct tfshdr *nextfile;    /* Location of next file. */
    long filsize;               /* Size of file. */
    unsigned long crc;          /* 32 bit crc of this header. */
    unsigned long ohdrcrc;      /* Copy of file's original hdrcrc. */
    long idx;                   /* Index into defrag hdr table. */
    int nesn;                   /* New end sector number. */
    long neso;                  /* New end sector offset. */
    long oeso;                  /* Original end sector offset. */
    char *nda;                  /* New destination address. */
    char fname[TFSNAMESIZE+1];  /* Name of file. */
};

/* sectorcrc:
 * This structure is used to store the crc of the sectors before and
 * after a defragmentation.  A table of these structures is built prior
 * to startup of any file relocation.
 */
struct sectorcrc {
    unsigned long   precrc;
    unsigned long   postcrc;
};

#define DEFRAGHDRSIZ    sizeof(struct defraghdr)
#define DEFRAGHDR       struct defraghdr

/* States used during defragmentation: */
#define SECTOR_DEFRAG_INACTIVE          1
#define SCANNING_ACTIVE_SECTOR          2
#define SCANNING_ACTIVE_SECTOR_1        3
#define SCANNING_ACTIVE_SECTOR_2        4
#define SCANNING_ACTIVE_SECTOR_3        5
#define SCANNING_ACTIVE_SECTOR_4        6
#define SCANNING_ACTIVE_SECTOR_5        7
#define SECTOR_DEFRAG_ALMOST_DONE       8
#define SECTOR_DEFRAG_ABORT_RESTART     9

/* Different ways in which a file can span across the active sector. */
#define SPANTYPE_UNDEF          0   /* Span type undefined */
#define SPANTYPE_BPEP           1   /* Begin-previous/end-previous */
#define SPANTYPE_BPEC           2   /* Begin-previous/end-current */
#define SPANTYPE_BPEL           3   /* Begin-previous/end-later */
#define SPANTYPE_BCEC           4   /* Begin-current/end-current */
#define SPANTYPE_BCEL           5   /* Begin-current/end-later */
#define SPANTYPE_BLEL           6   /* Begin-later/end-later */

/* struct tfsdat:
    Used by TFS to keep track of an opened file.
*/
struct tfsdat {
    long    offset;             /* Current offset into file. */
    long    hwp;                /* High water point for modified file. */
    unsigned char   *base;      /* Base address of file. */
    long    flagmode;           /* Flags & mode file was opened with. */
    struct  tfshdr hdr;         /* File structure. */
};

/* struct tfsdfg, tfsflg & tfserr:
    Structures provide an easy means of translation between values and
    strings that explain those values.
*/
struct tfsflg {
    long    flag;
    char    sdesc;
    char    *ldesc;
    long    mask;
};

struct tfserr {
    long    err;
    char    *msg;
};

struct tfsdfg {
    long    state;
    char    *msg;
};

struct tfsinfo {
    int liveftot;       /* Number of live files. */
    int livedata;       /* Space used by living file data. */
    int liveovrhd;      /* Space used by living file overhead. */
    int deadftot;       /* Number of dead files. */
    int deaddata;       /* Space used by dead file data. */
    int deadovrhd;      /* Space used by dead file overhead. */
    int pso;            /* Per-sector (not per file) TFS overhead. */
    int sos;            /* Size of spare sector(s). */
    int memtot;         /* Total tfs memory space in device(s). */
    int memfree;        /* Memory space available for new file data. */
    int memused;        /* Total memory space used by files & overhead. */
    int memfordata;     /* Total memory available for new file data. */
    int sectortot;      /* Total number of sectors in this tfs device. */
};
typedef struct tfsinfo TINFO;


/* Extern data: */
extern  long tfsTrace;
extern  long tfsFmodCount;
extern  TFILE **tfsAlist;
extern  TDEV tfsDeviceTbl[];
#ifdef TFS_ALTDEVTBL_BASE
extern  TDEV *alt_tfsdevtbl;
#endif
extern  TDEV alt_tfsdevtbl_base;
extern  struct tfsdat tfsSlots[];
extern  struct tfsflg tfsflgtbl[];
extern  int ScriptExitFlag;
extern  int TfsCleanEnable;
extern  int DefragTestPoint;
extern  int DefragTestType;
extern  int DefragTestSector;

/* Extern functions: */
extern  int tfseof(int);
extern  int tfsinit(void);
extern  int _tfsinit(TDEV *);
extern  int tfsreorder(void);
extern  int tfsrunboot(void);
extern  int tfsfixup(int,int);
extern  int tfsunlink(char *);
extern  int tfslink(char *,char *);
extern  int tfsclean_on(void);
extern  int tfsclean_off(void);
extern  int _tfsunlink(char *);
extern  int tfsflasherase(int);
extern  int tfsrun(char **,int);
extern  int tfscheck(TDEV *,int);
extern  int validtfshdr(TFILE *);
extern  int tfstruncate(int,long);
extern  int tfsclose(int, char *);
extern  int tfsscript(TFILE *,int);
extern  int tfsseek(int, int, int);
extern  int tfsread(int,char *,int);
extern  int tfsspace(char *);
extern  int showTfsError(int,char *);
extern  int tfsflasheraseall(TDEV *);
extern  int tfsflasherased(TDEV *,int);
extern  int tfswrite(int, char *, int);
extern  int tfsgetline(int,char *,int);
extern  int tfsLogCmd(int,char **,int);
extern  int tfsopen(char *,long,char *);
extern  int tfsfstat(char *name,TFILE *);
extern  int tfsmemuse(TDEV *,TINFO *,int);
extern  int tfsipmod(char *,char *,int,int);
extern  int tfsclean_nps(TDEV *, char *, unsigned long);
extern  int tfsloadebin(TFILE *,int,long *,char *,int);
extern  int tfsloadebin_l(TFILE *,int,long *,int);
extern  int tfsadd(char *,char *,char *,unsigned char *,int);
extern  int tfsflashwrite(unsigned char *,unsigned char *,long);
extern  int tfsclean(TDEV *,int);
extern  int _tfsclean(TDEV *,int,int);
extern  int tfsautoclean(TDEV *,int);
extern  int (*tfsDocommand)(char *,int);
extern  int dumpDhdr(struct defraghdr *), dumpDhdrTbl(struct defraghdr *,int);
extern  int dumpFhdr(TFILE *);
extern  int tfsRunningMonrc(void);
extern  int tfsramdevice(char *,long,long);
extern  int tfscfg(char *,unsigned long, unsigned long, unsigned long);
extern  int tfscfgrestore(void);
extern  int tfsflagsatob(char *, long *);


extern  TDEV *gettfsdev_fromprefix(char *,int);

extern  TFILE *tfsnext(TFILE *);
extern  TFILE *tfsstat(char *name);
extern  TFILE *_tfsstat(char *name,int uselink);
extern  TFILE *nextfp(TFILE *,TDEV *);

extern  long tfstell(int);
extern  long (*tfsGetLtime)(void);
extern  long tfsctrl(int,long,long);

extern  unsigned long tfshdrcrc(TFILE *);

extern  char *tfsBase(TFILE *);
extern  char *tfserrmsg(int);
extern  char *tfsscriptname(void);
extern  char *tfsflagsbtoa(long,char *);
extern  char *(*tfsGetAtime)(long,char *,int);

extern  void tfsclear(TDEV *);
extern  void gototag(char *);
extern  void gosubtag(char *);
extern  void gosubret(char *);
extern  void exitscript(char *);
extern  void tfsstartup(void);
extern  void tfslog(int,char *);
extern  void tfsFacilityUnavailable(char *);
extern  void tfsrunrcfile(void);
#endif
