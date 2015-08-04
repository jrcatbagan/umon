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
 * spif.h:
 *
 * SPI-flash iterface. See spif.c for details.
 *
 * Original author:     Ed Sutter (ed.sutter@alcatel-lucent.com)
 *
 */
/* Status register bits for ATMEL device: */
#define SPRL    0x80    // Sector protection registers locked (if 1)
#define RES     0x40    // Reserved for future use
#define EPE     0x20    // Erase or program error (if 1)
#define WPP     0x10    // *WP pin is deasserted (if 1)
#define SWPS    0x0c    // Software protection status bits
#define SWPS00  0x00    //   00: all sectors unprotected
#define SWPS01  0x04    //   01: some sectors unprotected
#define SWPS10  0x08    //   10: reserved for future use
#define SWPS11  0x0c    //   11: all sectors protected (default)
#define WEL     0x02    // Write enable latch (1=write enabled)
#define BSY     0x01    // Busy (1=busy, 0=ready)

/* Baudrate could be much higher, but we keep it low here for easier
 * ability to trace out on the scope...
 */
#ifndef SPIFBAUD
#define SPIFBAUD 25000000
#endif

/* The ATMEL part supports three different erase block sizes (not
 * including the 'whole-chip' erase)...
 */
#define BLKSZ_4K    0x20
#define BLKSZ_32K   0x52
#define BLKSZ_64K   0xD8

extern void spifQinit(void);
extern int spifInit(void);
extern int spifWaitForReady(void);
extern int spifId(int verbose);
extern int spifWriteEnable(void);
extern int spifWriteDisable(void);
extern int spifChipErase(void);
extern int spifGlobalUnprotect(void);
extern int spifGlobalProtect(void);
extern int spifReadBlock(unsigned long addr,char *data,int len);
extern int spifWriteBlock(unsigned long addr, char *data, int len);
extern int spifBlockErase(int bsize, long addr);
extern unsigned short spifReadStatus(int verbose);
extern unsigned long spifGetTFSBase(void);
extern unsigned long spifGetTFSSize(void);
