/* 
 * Board/device specific connection between the generic
 * nand command and the CSB740's NAND memory access through
 * the OMAP3530.
 *
 * The CSB740 uses K9K4GO8UOM NAND (512Mx8bit) device from Samsung.
 * It is connected to the OMAP3530's CS3 through the CPLD.
 * The device ID for this part number should be: 0xECDC109554
 *
 *  Maker code:		0xEC
 *	Device code:	0xDC
 *  Third cycle:	0x10
 *  Fourth cycle:	0x95
 *  Fifth cycle:	0x54
 *
 * Blocksize: 128K	(smallest eraseable chunk)
 * Pagesize:  2K  (64 pages per block)
 *
 * Taken from the NAND datasheet...
 *   The 528M byte physical space requires 30 addresses, thereby
 *   requiring five cycles for addressing :
 *   2 cycles of column address, 3 cycles of row address, in that
 *   order. Page Read and Page Program need the same five address
 *   cycles following the required command input. In Block Erase
 *   operation, however, only the three row address cycles are used.
 *   Device operations are selected by writing specific commands into
 *   the command register.
 *
 * Taken from ONFI Spec:
 *   The address is comprised of a row address and a column address.
 *   The row address identifies the page, block, and LUN to be accessed.
 *   The column address identifies the byte or word within a page to access.
 *
 * Note:
 *	Apparently this Sansung device is not ONFI compliant.  There was an 
 *  earlier version of the CSB740 that had a Micron part on it and it was
 *  ONFI compliant (ONFI=Open Nand Flash Interface specification).
 * 
 */
#include "config.h"
#if INCLUDE_NANDCMD
#include "stddefs.h"
#include "genlib.h"
#include "omap3530.h"
#include "omap3530_mem.h"
#include "nand.h"

static int pgSiz;
static int blkSiz;
static char onfi;

#define NANDSTAT_FAIL	0x01
#define NANDSTAT_READY	0x40
#define NANDSTAT_NOTWP	0x80

#define NAND_CMD(cs) (vuchar *)(0x6e00007C + (0x30 * cs))
#define NAND_ADR(cs) (vuchar *)(0x6e000080 + (0x30 * cs))
#define NAND_DAT(cs) (vuchar *)(0x6e000084 + (0x30 * cs))

#define PREFETCH_READ_MODE()	GPMC_REG(GPMC_PREFETCH_CONFIG1) &= ~1
#define WRITE_POSTING_MODE()	GPMC_REG(GPMC_PREFETCH_CONFIG1) |= 1

#define NAND_CS_BASEADDR	0x18000000

/* some discussion to follow:
 * http://e2e.ti.com/support/dsp/omap_applications_processors/f/42/p/29683/103192.aspx
 * Address is "page/block/column"... 
 */

/* nandBusyWait():
 * Poll the WAIT3 bit of the gmpc-status register, waiting for
 * it to go active.
 * The R/B (ready/busy) pin of the NAND device is low when busy.
 * It is tied to WAIT3 of the CPU.  
 */
void
nandHardBusyWait(void)
{
	while((GPMC_REG(GPMC_STATUS) & 0x800) == 0x800);
	//while((GPMC_REG(GPMC_STATUS) & 0x800) == 0);
}

void
nandSoftBusyWait(void)
{
	do {
		*NAND_CMD(3) = 0x70;
	} while((*NAND_DAT(3) & NANDSTAT_READY) == 0);
}

void
nandSetColAddr(unsigned long addr)
{
	// Column1 (A07|A06|A05|A04|A03|A02|A01|A00):
	*NAND_ADR(3) = ((addr & 0x000000ff));

	// Column2 (---|---|---|---|A11|A10|A09|A08):
	*NAND_ADR(3) = ((addr & 0x00000f00) >> 8);
}

void
nandSetRowAddr(unsigned long addr)
{
	// Row1 (A19|A18|A17|A16|A15|A14|A13|A12):
	*NAND_ADR(3) = ((addr & 0x000ff000) >> 12);

	// Row2 (A27|A26|A25|A24|A23|A22|A21|A20):
	*NAND_ADR(3) = ((addr & 0x0ff00000) >> 20);

	// Row3 (---|---|---|---|---|---|A29|A28):
	*NAND_ADR(3) = ((addr & 0x30000000) >> 28);
}

/* nandReadChunk():
 * Transfer some chunk of memory from NAND to a destination.
 */
int
nandReadChunk(char *src, char *dest, int len)
{
	unsigned long addr = (long)src;

	PREFETCH_READ_MODE();

	if (nandVerbose)
		printf("nandReadChunk(src=0x%x,dest=0x%x,len=%d)\n",src,dest,len);

	while(len > 0) {
		int tot;

		*NAND_CMD(3) = 0x00;
		nandSetColAddr(addr);
		nandSetRowAddr(addr);
		*NAND_CMD(3) = 0x30;

		nandHardBusyWait();

		tot = len > pgSiz ? pgSiz : len;
		memcpy(dest,(char *)NAND_CS_BASEADDR,tot);

		len -= tot;
		dest += tot;
	}
	return(0);
}

int
nandWriteChunk(char *dest, char *src, int len)
{
	unsigned long addr = (long)dest;

	WRITE_POSTING_MODE();

	if (nandVerbose)
		printf("nandWriteBlock(dest=0x%x,src=0x%x,len=%d)\n",dest,src,len);

	*NAND_CMD(3) = 0x80;
	nandSetColAddr(addr);
	nandSetRowAddr(addr);
	memcpy((char *)NAND_CS_BASEADDR,src,len);
	*NAND_CMD(3) = 0x10;
	
	nandSoftBusyWait();

	return(0);
}

int
nandEraseChunk(char *base, int len)
{
	unsigned long addr = (long)base;

	if (nandVerbose)
		printf("nandEraseChunk(addr=0x%x,len=%d)\n",addr,len);

	*NAND_CMD(3) = 0x60;
	nandSetRowAddr(addr);
	*NAND_CMD(3) = 0xd0;
	
	nandSoftBusyWait();

	return(0);
}

void
nandId(void)
{
	uchar d[5];
	uchar d1[4];

	*NAND_CMD(3) = 0x90;
	*NAND_ADR(3) = 0x00;
	d[0] = *NAND_DAT(3);
	d[1] = *NAND_DAT(3);
	d[2] = *NAND_DAT(3);
	d[3] = *NAND_DAT(3);
	d[4] = *NAND_DAT(3);

	switch(d[3] & 3) {
		case 0:
			pgSiz = 1024;
			break;
		case 1:
			pgSiz = 1024*2;
			break;
		case 2:
			pgSiz = 1024*4;
			break;
		case 3:
			pgSiz = 1024*8;
			break;
	}
	switch((d[3] & 0x30) >> 4) {
		case 0:
			blkSiz = 1024*64;
			break;
		case 1:
			blkSiz = 1024*128;
			break;
		case 2:
			blkSiz = 1024*256;
			break;
		case 3:
			blkSiz = 1024*512;
			break;
	}

	*NAND_CMD(3) = 0x90;
	*NAND_ADR(3) = 0x20;
	d1[0] = *NAND_DAT(3);
	d1[1] = *NAND_DAT(3);
	d1[2] = *NAND_DAT(3);
	d1[3] = *NAND_DAT(3);
	if (memcmp((char *)d1,"ONFI",4) == 0)
		onfi = 1;
	else
		onfi = 0;

	if (nandVerbose) {
		printf("nandID():  %02x%02x%02x%02x%02x\n",d[0],d[1],d[2],d[3],d[4]);
		printf("nandID+():  %02x%02x%02x%02x%02x\n",d1[0],d1[1],d1[2],d1[3]);
		printf("Page size:  0x%x\n",pgSiz);
		printf("Block size: 0x%x\n",blkSiz);
		printf("%sONFI compliant\n",onfi ? "" : "Not ");
	}
}

int
nandInit(void)
{
	vulong cfgreg;

#if 0
	printf("CFG1: dm -4 0x%08x 1 = %08x\n",
		MYGPMC_REG(GPMC_CS3_CONFIG1),GPMC_REG(GPMC_CS3_CONFIG1));
	printf("CFG2: dm -4 0x%08x 1 = %08x\n",
		MYGPMC_REG(GPMC_CS3_CONFIG2),GPMC_REG(GPMC_CS3_CONFIG2));
	printf("CFG3: dm -4 0x%08x 1 = %08x\n",
		MYGPMC_REG(GPMC_CS3_CONFIG3),GPMC_REG(GPMC_CS3_CONFIG3));
	printf("CFG4: dm -4 0x%08x 1 = %08x\n",
		MYGPMC_REG(GPMC_CS3_CONFIG4),GPMC_REG(GPMC_CS3_CONFIG4));
	printf("CFG5: dm -4 0x%08x 1 = %08x\n",
		MYGPMC_REG(GPMC_CS3_CONFIG5),GPMC_REG(GPMC_CS3_CONFIG5));
	printf("CFG6: dm -4 0x%08x 1 = %08x\n",
		MYGPMC_REG(GPMC_CS3_CONFIG6),GPMC_REG(GPMC_CS3_CONFIG6));
	printf("CFG7: dm -4 0x%08x 1 = %08x\n",
		MYGPMC_REG(GPMC_CS3_CONFIG7),GPMC_REG(GPMC_CS3_CONFIG7));
#endif


	/* WAIT3 of the CPU is tied to the NAND's READY pin.  The NAND
	 * is on CS3.  Referring to section 11.1.7.2.10 of the OMAP3530 TRM,
	 * the GPMC_CONFIGX registers must be programmed...
	 */
	GPMC_REG(GPMC_CONFIG) |= 0x1;				// Force posted write.
	GPMC_REG(GPMC_CONFIG) &= ~0x800;			// WAIT3 active low.
	GPMC_REG(GPMC_CS3_CONFIG7) = 0x00000858; 	// Base addr 0x18000000
	GPMC_REG(GPMC_CS3_CONFIG1) = 0x00030800; 	// 8-bit NAND, WAIT3
 	GPMC_REG(GPMC_CS3_CONFIG2) = 0x00000000;	// Chipselect timing
 	GPMC_REG(GPMC_CS3_CONFIG3) |= 0x7;
 	GPMC_REG(GPMC_CS3_CONFIG6) = 0x8f0307c0;

	// Set drive strength of BE0/CLE
	*(vulong *)0x48002444 |= 0x00000020;

	/***********************************************************
	 *
	 * ALE and CLE on the NAND are both active high, so we want
	 * them to be pulled low...
	 *
	 * ALE config is the low half of this config register, so we only
	 * touch the bottom half...
	 */
	cfgreg = SCM_REG(PADCONFS_GPMC_NADV_ALE);	// NOE[31:16], NADV_ALE[15:0]
	cfgreg &= 0xffff0000;
	cfgreg |= 0x00000008;
	SCM_REG(PADCONFS_GPMC_NADV_ALE) = cfgreg;
	/*
	 * CLE config is the upper half of this config register, so we only
	 * touch the upper half...
	 */
	cfgreg = SCM_REG(PADCONFS_GPMC_NWE);		// NBE0_CLE[31:16], NWE[15:0]
	cfgreg &= 0x0000ffff;
	cfgreg |= 0x00080000;
	SCM_REG(PADCONFS_GPMC_NWE) = cfgreg;


	/* WAIT3 of CPU is tied to R/B pin of NAND...
	 * So, we configure that pin to run as WAIT3.
	 * NOTE:
	 *  There is some confusion between this and what is in cpuio.c.  
	 *  The PADCONFS_GPMC_WAIT2 register sets this pin as GPIO-65 there;
	 *  however the comments are confusing.
	 */
	cfgreg = SCM_REG(PADCONFS_GPMC_WAIT2);		// WAIT3[31:16], WAIT2[15:0]
	cfgreg &= 0x0000ffff;
	cfgreg |= 0x00080000;
	SCM_REG(PADCONFS_GPMC_NWE) = cfgreg;

	nandId();
	return(0);
}

int
nandInfo(void)
{
	nandId();
	return(0);
}


#endif


