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
 * flash.h:
 *
 * Device-independent macros and data structures used by flash driver.
 *
 * Original author:     Ed Sutter (ed.sutter@alcatel-lucent.com)
 *
 */
#ifndef _FLASH_H_
#define _FLASH_H_

#define FLASH_PROTECT_WINDOW_CLOSED 0
#define ALL_SECTORS -1

#ifndef FLASH_LOOP_TIMEOUT
#define FLASH_LOOP_TIMEOUT 1000000
#endif

#define FLASH_LOCK          1
#define FLASH_UNLOCK        2
#define FLASH_LOCKDWN       3
#define FLASH_LOCKQRY       4
#define FLASH_LOCKABLE      5       /* query driver for lock support */

/* Device ID used for ram that is "pretending" to be a flash bank. */
#define FLASHRAM    0x9999

struct  flashdesc {
    unsigned long   id;         /* manufacturer & device id */
    char        *desc;          /* ascii string */
};

struct  sectorinfo {
    long    size;               /* size of sector */
    int     snum;           /* number of sector (amongst possibly */
    /* several devices) */
    int     protected;      /* if set, sector is protected by window */
    unsigned char   *begin;         /* base address of sector */
    unsigned char   *end;           /* end address of sector */
};

struct  flashinfo {
    unsigned long   id;         /* manufacturer & device id */
    unsigned char   *base;          /* base address of device */
    unsigned char   *end;           /* end address of device */
    int     sectorcnt;      /* number of sectors */
    int     width;          /* 1, 2, or 4 */
    int (*fltype)(struct flashinfo *);
    int (*flerase)(struct flashinfo *, int);
#if INCLUDE_FLASHREAD
    int (*flread)(struct flashinfo *,unsigned char *,\
                  unsigned char *,long);
#endif
    int (*flwrite)(struct flashinfo *,unsigned char *,\
                   unsigned char *,long);
    int (*flewrite)(struct flashinfo *,unsigned char *,\
                    unsigned char *,long);
    int (*fllock)(struct flashinfo *,int,int);
    struct sectorinfo *sectors;
};

extern int      FlashTrace;
extern int      FlashProtectWindow;
extern int      FlashCurrentBank;
extern struct   flashinfo FlashBank[FLASHBANKS];
extern int      flashopload(unsigned long *begin,unsigned long *end, \
                            unsigned long *copy,int size);

extern int showflashtype(unsigned long, int);
extern int showflashinfo(char *);
extern int flashopload(unsigned long *,unsigned long *,unsigned long *,int);
extern int flashtype(struct flashinfo *);
extern int flasherase(int snum);
extern int flashwrite(struct flashinfo *,unsigned char *,unsigned char *,long);
extern int flashewrite(unsigned char *,unsigned char *,long);
extern int flasherased(unsigned char *,unsigned char *);
extern int flashlock(int, int);
extern int flashlocked(int, int);
extern int addrtosector(unsigned char *,int *,int *,unsigned char **);
extern struct flashinfo *snumtofdev(int);
extern struct flashinfo *addrtobank(unsigned char *);
extern int sectortoaddr(int,int *,unsigned char **);
extern int flashbankinfo(int,unsigned char **,unsigned char **,int *);
extern void LowerFlashProtectWindow(void);
extern int AppFlashWrite(unsigned char *,unsigned char *,long);
extern int AppFlashEraseAll(void);
extern int AppFlashErase(int);
extern int srange(char *,int *,int *);
extern int sectorProtect(char *,int);
extern int FlashOpNotSupported(void);
extern int FlashLockNotSupported(struct flashinfo *,int,int);
extern int lastlargesector(int,unsigned char *,int,int *,int *,unsigned char **);
extern int lastflashsector(void);
extern int FlashRamInit(int, int, struct flashinfo *,struct sectorinfo *,int *);
extern int InFlashSpace(unsigned char *begin, int size);
extern int FlashOpOverride(void *flashinfo,int get,int bank);

#define NotAligned16(add)   ((long)add & 1)
#define NotAligned32(add)   ((long)add & 3)

#ifdef FLASHRAM_BASE
extern int ramSectors[];
extern struct sectorinfo sinfoRAM[];
#endif

#endif
