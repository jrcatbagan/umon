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
 * sd.h:
 *
 * Header file supporting SD card interface command.
 *
 * Much of this information is extracted from the Physical Layer Simplified
 * Specification Version 2.00 (PLSS) document.
 *
 * Note that as of this writing, this header file is limited in scope to
 * SPI-based SD.
 *
 * Original author:     Ed Sutter (ed.sutter@alcatel-lucent.com)
 *
 */

#ifndef SD_BLKSIZE
#define SD_BLKSIZE  512
#endif

#define SPISD_LO_SPEED  400000
#define SPISD_HI_SPEED  25000000

#define VERSION_10  1
#define VERSION_20  2

#ifndef SD_DEVTOT
#define SD_DEVTOT 1
#endif

#define MSK_OCR_33  0xC0

// Fields used by SEND_IF_COND command:
// (refer to section 4.3.13 of PLSS)
#define VHS27_36            0x01
#define CHECK_PATTERN       0xAA
#define SEND_IF_COND_ARG    (((long)VHS27_36 << 8) | CHECK_PATTERN)

#define HCS 0x40000000      // High capacity support
#define CCS 0x40000000      // Card capacity status


/* Response type 1 (section 7.3.2.1 of PLSS):
 */
#define R1_RLEN             1
#define R1_STARTBIT         0x80
#define R1_PARAM_ERR        0x40
#define R1_ADDRESS_ERR      0x20
#define R1_ERASESEQ_ERR     0x10
#define R1_CRC_ERR          0x08
#define R1_ILLEGAL_CMD      0x04
#define R1_ERASE_RESET      0x02
#define R1_IDLE             0x01
#define R1_ERRMASK          (R1_PARAM_ERR | R1_ADDRESS_ERR | R1_ERASESEQ_ERR | R1_CRC_ERR | R1_ILLEGAL_CMD)


/* Response type 2 (section 7.3.2.3 of PLSS):
 */
#define R2_RLEN             2
#define R2H_STARTBIT        R1_STARTBIT
#define R2H_PARAM_ERR       R1_PARAM_ERR
#define R2H_ADDRESS_ERR     R1_ADDRESS_ERR
#define R2H_ERASESEQ_ERR    R1_ERASESEQ_ERR
#define R2H_CRC_ERR         R1_CRC_ERR
#define R2H_ILLEGAL_CMD     R1_ILLEGAL_CMD
#define R2H_ERASE_RESET     R1_ERASE_RESET
#define R2H_IDLE            R1_IDLE
#define R2L_OUTOFRANGE      0x80
#define R2L_ERASEPARAM      0x40
#define R2L_WPVIOLATION     0x20
#define R2L_ECCFAILED       0x10
#define R2L_CCERR           0x08
#define R2L_ERROR           0x04
#define R2L_WPES_LKFAIL     0x02
#define R2L_CARDLOCKED      0x01

/* Response type 3 (section 7.3.2.4 of PLSS):
 * This is the card's response to the READ_OCR command.
 * First byte is identical to R1, the remaining 4 bytes
 * are the card's OCR register.
 */
#define R3_RLEN             5
#define R3_STARTBIT         R1_STARTBIT
#define R3_PARAM_ERR        R1_PARAM_ERR
#define R3_ADDRESS_ERR      R1_ADDRESS_ERR
#define R3_ERASESEQ_ERR     R1_ERASESEQ_ERR
#define R3_CRC_ERR          R1_CRC_ERR
#define R3_ILLEGAL_CMD      R1_ILLEGAL_CMD
#define R3_ERASE_RESET      R1_ERASE_RESET
#define R3_IDLE             R1_IDLE

/* Reponsee R4 & R5 are reserved for IO mode.
 */

/* Response type 7 (section 7.3.2.6 of PLSS):
 * This is the card's response to the SEND_IF_COND command.
 * First bytes is identical to R1.
 */
#define R7_RLEN             5
#define R7_STARTBIT         R1_STARTBIT
#define R7_PARAM_ERR        R1_PARAM_ERR
#define R7_ADDRESS_ERR      R1_ADDRESS_ERR
#define R7_ERASESEQ_ERR     R1_ERASESEQ_ERR
#define R7_CRC_ERR          R1_CRC_ERR
#define R7_ILLEGAL_CMD      R1_ILLEGAL_CMD
#define R7_ERASE_RESET      R1_ERASE_RESET
#define R7_IDLE             R1_IDLE

#define R7_CMDVSNMSK        0xf8    // upper 5 bits of 2nd byte of response
#define R7_VAMSK            0x0f    // lower 4 bits of 3nd byte of response


/* Data response token (7.3.3.1 of PLSS):
 */
#define DRT_FIXEDMASK       0x11
#define DRT_FIXED           0x10
#define DRT_STATMASK        0x0e
#define DRT_STATACCEPTED    (0x2<<1)
#define DRT_STATCRCERR      (0x5<<1)
#define DRT_STATWRITEERR    (0x6<<1)


/* Data error token:
 */
#define DET_ERROR           0x01
#define DET_CCERR           0x02
#define DET_ECCFAILED       0x04
#define DET_OOR             0x08

/* Start block token:
 */
#define START_BLOCK_TOKEN   0xfe
#define START_BLKMBW_TOKEN  0xfc
#define STOP_TRAN_TOKEN     0xfd


#define R1  0x01
#define R1B 0x81
#define R2  0x02
#define R3  0x03
#define R7  0x07

#define APP                  0x1000
#define CMD_APP(cmd)        ((cmd & APP) ? 1 : 0)

/* SD-Card command macros used by SPI mode:
 * This definitions carry three pieces of information per macro:
 * Response lenght, response type & command index.
 */
#define GO_IDLE_STATE           0
#define SEND_OP_COND            1
#define SWITCH_FUNC             6
#define SEND_IF_COND            8
#define SEND_CSD                9
#define SEND_CID                10
#define STOP_TRANSMISSION       12
#define SEND_STATUS             13
#define SET_BLOCKLEN            16
#define READ_SINGLE_BLK         17
#define READ_MULTIPLE_BLK       18
#define WRITE_BLK               24
#define WRITE_MULTIPLE_BLK      25
#define PROGRAM_CSD             27
#define SET_WRITE_PROT          28
#define CLR_WRITE_PROT          29
#define SEND_WRITE_PROT         30
#define ERASE_WR_BLK_START_ADDR 32
#define ERASE_WR_BLK_END_ADDR   33
#define ERASE                   38
#define LOCK_UNLOCK             42
#define APP_CMD                 55
#define GEN_CMD                 56
#define READ_OCR                58
#define CRC_ON_OFF              59
#define SD_SEND_OP_COND         (41 | APP)


/* struct sdcmd:
 * Used to carry information about each of the commands handled
 * by this driver for the SD-Memory Card interface.
 */
struct sdcmd {
    int cmd;
    int rlen;
    int rtype;
};

/* struct sdinfo:
 * Used to maintain some information about each of the interfaces.
 * Note, in almost all cases, there will only be one SD-Card interface;
 * however there is a table of these structures anyway.
 */
struct sdinfo {
    char initialized;
    char cardversion;
    char highcapacity;
};

/* These two functions must be supplied by the port-specific code.
 */
extern char sdVerbose;
extern void sdio_init(void);
extern int  sdCardCmd(int interface, int cmd, unsigned long arg,
                      unsigned char *resp);
extern int  sdInit(int interface, int verbosity);
extern int  sdRead(int interface, char *buf, int blknum, int blkcount);
extern int  sdWrite(int interface, char *buf, int blknum, int blkcount);
extern int  sdInstalled(int interface);
extern int  sdPowerup(int tot);
extern int  sdGenericStartup(int interface);
extern int  sdCmdrlen(int);
extern int  sdCmdrtype(int);
extern void sdShowCID(uchar *cidbuf);
extern void sdShowCSD(uchar *csdbuf);
extern int  sdReadCxD(int interface, unsigned char *buf, int cmd);
extern unsigned char crc7(unsigned char seed, unsigned char *buf, int len);

extern struct sdinfo sdInfoTbl[];

