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
 * boardinfo.c:
 *
 * When the boot monitor is updated (using newmon), only the sectors that
 * are used by the monitor are re-written.  If a sector  in the flash is
 * not being used by TFS, and is not being used by the monitor, then the
 * strategy of the code in this file may be useful...
 *
 * The monrc file is used by the monitor to store various initialization
 * parameters.  At a minimum (on systems with ethernet), this will include
 * an IP address, MAC address, Gateway IP address and Network Mask.  It
 * may also include some board/target-specific information. This is convenient
 * because it is a file that can be updated on a per-board basis; however,
 * the convenience can also be a pain in the butt.  The user can easily
 * modify the monrc file and change the IP address and MAC address (things
 * that are usually not changed very often, especially the MAC address). 
 * To make some of this stuff a bit less convenient to change, these
 * values (strings) can be put into some other portion of flash that is
 * not used by the monitor or TFS.  Hence, even if TFS is initialized or
 * the monitor is updated, this information will remain in place.
 * 
 * This file, in conjunction with a target-specific table called 
 * boardinfotbl[] supports the ability to store this data in a sector
 * that is not being used by TFS or the monitor binary.  When the target
 * first comes up, it will check to see if this space is erased.  If yes,
 * then it will query the user for values to be loaded and they will then
 * be stored away in a sector of flash that is not used by TFS or uMon.
 * They will become shell variables at startup, and will not be changeable
 * unless that sector is manually erased.
 *
 * Note that an inefficiency of this is that it assumes it has a sector
 * dedicated to it.  This "inefficiency" however; is what makes it less
 * likely to be mistakenly erased.
 *
 * Following is an example boardinfotbl[]...
 *
    extern unsigned char etheraddr[], boardtag[];

    struct boardinfo boardinfotbl[] = {
        { etheraddr,	32,	"ETHERADD",	(char *)0,		"MAC addr" },
        { boardtag,		32,	"BOARDTAG",	(char *)0,		"Board tag" },
        { 0,0,0,0 }
     };
  
 *
 * The externs "etheraddr" and "boardtag" are actually pointers into some
 * portion of the flash sector that is to be used for this storage.  These
 * values can be placed in the linker map file or be hard-coded addresses
 * in this file.
 * 
 * Then, in config.h, the following two definitions must be established...
 *

	#define BOARDINFO_SECTOR 7	
	#define BOARDINFO_BUFMAX 32

 *
 * The value assigned to BOARDINFO_SECTOR is the sector number that is 
 * used for the storage and BOARDINFO_BUFMAX is the size of buf[] within
 * the BoardInfoInit function below.
 *
 * Original author:     Ed Sutter (ed.sutter@alcatel-lucent.com)
 *
 */
#include "config.h"
#include "stddefs.h"
#include "genlib.h"
#include "boardinfo.h"
#include "cli.h"

#if INCLUDE_FLASH & INCLUDE_BOARDINFO

#ifdef BOARDINFO_DEBUG
#define BINFOPRINT(a)	printf a 
#else
#define BINFOPRINT(a)
#endif

/* boardinfo_error:
 * Set if there was any kind of read error from the board info structure.
 */
int boardinfo_error;

void
BoardInfoInit(void)
{
	short	len;
	ushort	crc;
	struct	boardinfo *bip;
	int		maxpromptsize, erased;
	struct	boardinfoverify *bipv;
	uchar	buf[BOARDINFO_BUFMAX];
	char	c, snum[8], prfmt[16], *prefill;

	sprintf(snum,"%d",BOARDINFO_SECTOR);

	/* Step through each bip entry and see if the data area is either
	 * empty or the crc16 passed.  If either is true, then allow
	 * If every entry in the board-information area is either empty
	 * or the crc test passes, we can just return...
	 */
	erased = 0;
	maxpromptsize = 0;
	boardinfo_error = 0;
	bip = boardinfotbl;
	while((bip->array) && (!boardinfo_error)) {
		BINFOPRINT(("Boardinfo item: %s (%s)",bip->varname,bip->prompt));
		if (bip->array[0] != 0xff) {
			BINFOPRINT((" not"));
			bipv = (struct boardinfoverify *)&bip->array[bip->size-BIVSIZE];

			/* If len and crc are set, then use those fields to sanity
			 * check the data...
			 */
			if ((bipv->len != 0xffff) && (bipv->crc16 != 0xffff)) {
			if ((bipv->len > bip->size) ||
				(xcrc16(bip->array,bipv->len) != bipv->crc16)) {
					boardinfo_error = 1;
				}
			}
		}
		else
			erased++;
		BINFOPRINT((" erased\n"));

		/* Gather data to determine the largest prompt... */
		len = strlen(bip->prompt);
		if (len > maxpromptsize)
			maxpromptsize = len;
		bip++;
	}

	/* If there was no error, and the board info area is not erased,
	 * we return here assuming the data is valid.
	 */
	if ((boardinfo_error == 0) && (erased == 0)) {
		sectorProtect(snum,1);
		return;
	}

	/* If there was some kind of error reading any of the fields in the
	 * board-info area, then either return or clear the error and interact
	 * with the user to re-load the data...
	 */
	if (boardinfo_error != 0) {
#ifdef BOARDINFO_REENTER_ON_ERROR
		printf("\nError reading board-info data, re-enter data...");
		boardinfo_error = 0;
#else
		printf("\nError reading board-info data in sector %d.\n",
			BOARDINFO_SECTOR);
		return;
#endif
	}

	sprintf(prfmt,"%%%ds : ",maxpromptsize);

	printf("%s: board information field initialization...\n",PLATFORM_NAME);

	/* Un-protect the sector used for board information...
	 */
	sectorProtect(snum,0);

	do {
		/* Erase the sector used for board information...
		 */
		if (AppFlashErase(BOARDINFO_SECTOR) < 0) {
			boardinfo_error = 1;
			break;
		}

		/* Step through each entry in the boardinfo table and query
		 * the user for input to be stored...
		 */
		bip = boardinfotbl;
		while(bip->array) {
			bipv = 	(struct boardinfoverify *)&bip->array[bip->size-BIVSIZE];

			if (bip->def) 
				prefill = bip->def;
			else 
				prefill = 0;

			printf(prfmt,bip->prompt);
			len = 0;
			if (getline_p((char *)buf,bip->size-BIVSIZE,0,prefill) != 0) {
				len = strlen(buf)+1;
				if (AppFlashWrite(bip->array,buf,len) < 0) {
					boardinfo_error = 1;
					break;
				}
				crc = xcrc16(buf,len);
			}
			if (len) {
				if (AppFlashWrite((uchar *)(&bipv->len),
						(uchar *)&len,sizeof(len)) < 0) {
					boardinfo_error = 1;
					break;
				}
				if (AppFlashWrite((uchar *)(&bipv->crc16),
						(uchar *)&crc,sizeof(crc)) < 0) {
					boardinfo_error = 1;
					break;
				}
			}
			bip++;
		}
		if (boardinfo_error)
			break;	
	
		bip = boardinfotbl;
		printf("\nNew system settings:\n\n");
		while(bip->array) {
#ifdef BOARDINFO_SHOW_RAW
			printMem(bip->array,bip->size,1);
#else
			printf("%24s: %s\n",bip->prompt,
				bip->array[0] == 0xff ? "" : (char *)bip->array);
#endif
			bip++;
		}
		putstr("\nHit ENTER to accept, any other key to re-enter...");
		c = getchar();
		putchar('\n');

	} while((c != '\r') && (c != '\n'));

	sectorProtect(snum,1);
}

void
BoardInfoEnvInit(void)
{
	struct boardinfo *bip;

	/* If an error was detected by BoardInfoInit() then we don't
	 * want to do any environmental initialization here...
	 */
	if (boardinfo_error != 0) {
		printf("Board info environment not initialized\n");
		return;
	}

	bip = boardinfotbl;
	while(bip->array) {
		if ((bip->varname) && (bip->array[0] != 0xff))
			setenv(bip->varname,bip->array);
		bip++;
	}
}

/* BoardInfoVar():
 * Return 1 if the incoming variable name is a variable
 * that is established by the BoardInfo facility; else zero.
 */
int
BoardInfoVar(char *varname)
{
	struct boardinfo *bip; 

	bip = boardinfotbl;
	while(bip->array) {
		if (strcmp(varname,bip->varname) == 0)
			return(1);
		bip++;
	}
	return(0);
}

char *BinfoHelp[] = {
	"Dump board-specific information",
	"",
#if INCLUDE_VERBOSEHELP
#endif
	0,
};

int
BinfoCmd(int argc, char *argv[])
{
	int i;
	char *array;
	struct boardinfo *bip;

	i = 1;
	bip = boardinfotbl;
	printf("Boardinfo sector: %d\n",BOARDINFO_SECTOR);
	printf("  #  ADDRESS     VALUE              VARNAME         DESCRIPTION\n");
	while(bip->array) {
		if (bip->array[0] == 0xff)
			array = "-empty-";
		else
			array = bip->array;
		printf(" %2d: 0x%08lx  %-18s %-15s %s\n",i,
			(long)bip->array,array,bip->varname,bip->prompt);
		bip++;
		i++;
	}
	return(CMD_SUCCESS);
}
#endif
