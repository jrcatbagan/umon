/* NOTE:
 * THIS CODE IS NOT READY FOR USE YET!!!
 */
#include "config.h"
#include "cpuio.h"
#include "genlib.h"
#include "stddefs.h"
#include "timer.h"
#include "omap3530.h"
#include "sd.h"


#define MMCTMOUT	2000

/* This code is included here just for simulating the SD
 * interface (temporarily if a real one isn't ready.  In a real system,
 * the INCLUDE_SD_DUMMY_FUNCS definition would be off.
 */
int
xsdCmd(unsigned long cmd, unsigned short argh, unsigned short argl)
{
	vulong	stat, rsp;
	struct elapsed_tmr tmr;

	printf("sdCmd(0x%08lx) (cmd=%d)\n",cmd,(cmd & 0x3f000000) >> 24);

 	startElapsedTimer(&tmr,MMCTMOUT);		// Wait for command line not-in-use
	while(MMC1_REG(MMCHS_PSTATE) & CMDI) {
		if(msecElapsed(&tmr)) {
            printf("sdInit: CMDI timeout\n");
            return(-1);
        }
	}

	MMC1_REG(MMCHS_ARG) = ((argh << 16) | argl);
	MMC1_REG(MMCHS_IE) = 0xfffffeff;
	MMC1_REG(MMCHS_CMD) = cmd;

again:
	stat = MMC1_REG(MMCHS_STAT);
	if (stat & CTO) {
		if (stat & CCRC)
			printf("cmdline in use\n");
		else
			printf("CTO1 CCRC0\n");
		MMC1_REG(MMCHS_SYSCTL) |= SRC;
		startElapsedTimer(&tmr,MMCTMOUT);	
		while(MMC1_REG(MMCHS_SYSCTL) & SRC) {
			if(msecElapsed(&tmr))
				printf("sdInit: SRC timeout\n");
		}
		return(-1);
	}
	if ((stat & CC)  == 0)
		goto again;
		
	cmd = MMC1_REG(MMCHS_CMD);
	if ((cmd & RSPTYPE) == RSPTYPE_NONE) {
		printf("Success!\n");	
		return(0);
	}

	if ((cmd & RSPTYPE) == RSPTYPE_136) {
		rsp = MMC1_REG(MMCHS_RSP10);
		printf("RSP0: %04x, RSP1: %04x\n",
			rsp & 0xffff,(rsp & 0xffff0000) >> 16);
		rsp = MMC1_REG(MMCHS_RSP32);
		printf("RSP2: %04x, RSP3: %04x\n",
			rsp & 0xffff,(rsp & 0xffff0000) >> 16);
		rsp = MMC1_REG(MMCHS_RSP54);
		printf("RSP4: %04x, RSP5: %04x\n",
			rsp & 0xffff,(rsp & 0xffff0000) >> 16);
		rsp = MMC1_REG(MMCHS_RSP76);
		printf("RSP6: %04x, RSP7: %04x\n",
			rsp & 0xffff,(rsp & 0xffff0000) >> 16);
	}
	if ((cmd & RSPTYPE) == RSPTYPE_48) {
		rsp = MMC1_REG(MMCHS_RSP10);
		printf("RSP0: %04x, RSP1: %04x\n",
			rsp & 0xffff,(rsp & 0xffff0000) >> 16);
		rsp = MMC1_REG(MMCHS_RSP32);
		printf("RSP2: %04x, RSP3: %04x\n",
			rsp & 0xffff,(rsp & 0xffff0000) >> 16);
	}
	if ((cmd & RSPTYPE) == RSPTYPE_48BSY) {
		rsp = MMC1_REG(MMCHS_RSP10);
		printf("RSP0: %04x, RSP1: %04x\n",
			rsp & 0xffff,(rsp & 0xffff0000) >> 16);
		rsp = MMC1_REG(MMCHS_RSP32);
		printf("RSP2: %04x, RSP3: %04x\n",
			rsp & 0xffff,(rsp & 0xffff0000) >> 16);
	}

	return(0);
}

int
sdCmd(unsigned long cmd, unsigned short argh, unsigned short argl)
{
	vulong	stat, arg;
	struct elapsed_tmr tmr;

	printf("sdCmd(0x%08lx) (cmd=%d)\n",cmd,(cmd & 0x3f000000) >> 24);

	MMC1_REG(MMCHS_STAT) = 0xffffffff;
	MMC1_REG(MMCHS_BLK) =  NBLK(1) | BLEN(512);
	MMC1_REG(MMCHS_SYSCTL) &=  ~DTOMSK;
	MMC1_REG(MMCHS_SYSCTL) |= DTO(14);

 	startElapsedTimer(&tmr,MMCTMOUT);		// Wait for command line not-in-use
	while(MMC1_REG(MMCHS_PSTATE) & CMDI) {
		if(msecElapsed(&tmr)) {
            printf("sdCmd: CMDI timeout\n");
            return(-1);
        }
		monDelay(1);
	}

	arg = argh;
	arg <<= 16;
	arg |= argl;
	MMC1_REG(MMCHS_ARG) = arg;
	MMC1_REG(MMCHS_IE) = 0xfffffeff;
	MMC1_REG(MMCHS_CMD) = cmd;

 	startElapsedTimer(&tmr,MMCTMOUT);
	do {
		stat = MMC1_REG(MMCHS_STAT);
		if (stat & CTO) {
			if (stat & CCRC)
				printf("CCRC1\n");
			else
				printf("CTO1 CCRC0\n");
			MMC1_REG(MMCHS_SYSCTL) |= SRC;
			startElapsedTimer(&tmr,MMCTMOUT);	
			while(MMC1_REG(MMCHS_SYSCTL) & SRC) {
				if(msecElapsed(&tmr))
					printf("sdCmd: SRC timeout\n");
			}
			return(-1);
		}
		if(msecElapsed(&tmr)) {
            printf("sdCmd: CC timeout\n");
            return(-1);
        }
		monDelay(1);
	} while ((stat & CC) == 0);

	stat = MMC1_REG(MMCHS_STAT);
	if (stat & CCRC) 
		printf("Cmd crc\n");
	if (stat & DCRC) 
		printf("Data crc\n");
	if (stat & CERR)  {
		printf("Card error 0x%lx\n",stat);
		return(-1);
	}
	if (stat & CTO) {
		printf("CTO set!\n");
		return(-1);
	}
	if (stat & CC) {
		printf("Success!\n");	
		return(0);
	}
	else {
		printf("Didn't complete!\n");	
		return(-1);
	}
}

int
sdClkSet(int clkval)
{
	vulong reg;
	struct elapsed_tmr tmr;

	MMC1_REG(MMCHS_SYSCTL) &= ~CEN;
	reg = MMC1_REG(MMCHS_SYSCTL);
	reg &= ~CLKDMSK;
	reg |= CLKD(96000/clkval);
	MMC1_REG(MMCHS_SYSCTL) = reg;

 	startElapsedTimer(&tmr,MMCTMOUT);			// Wait for clock stable
	while((MMC1_REG(MMCHS_SYSCTL) & ICS) == 0) {
		if(msecElapsed(&tmr)) {
			printf("sdClkSet: ICS timeout\n");
			return(-1);
		}
		monDelay(1);
	}
	MMC1_REG(MMCHS_SYSCTL) |= CEN;
	startElapsedTimer(&tmr,MMCTMOUT);			// Wait for clock stable
	while((MMC1_REG(MMCHS_SYSCTL) & CEN) == 0) {
		if(msecElapsed(&tmr)) {
            printf("sdClkSet: ICS timeout\n");
            return(-1);
        }
		monDelay(1);
	}
	return(0);
}

/* sdInit():
 * This function is called by the "sd init" command on the command line.
 * Where applicable, the text refers to the section in the Sept 2008 
 * Technical Reference Manual (TRM) from which I got the code/functionality.
 */
int
sdInit(int interface, int verbose)
{
	int i, pbiasretry = 0;
	vulong reg;
	struct elapsed_tmr tmr;

	/* There's only one interface on the CSB740, so reject anything
	 * other than interface 0...
	 */
	if (interface != 0)
		return(-1);

	/*******************************
	 *
	 * Clock configuration:
	 * (TRM 22.5.1.1)
	 */
	*(vulong *)CM_ICLKEN1_CORE |= EN_MMC1;	// Configure interface and
	*(vulong *)CM_FCLKEN1_CORE |= EN_MMC1;	// functional clocks.

	/*******************************
	 *
	 * Not really sure what this is... apparently some kind of clock steering.
	 * I tried both setting the bit and clearing it.  Made no difference.
	 * In both cases the clock was present on the CLK pin.
	 */
	*(vulong *)CONTROL_DEVCONF0 |= MMCSDIO1ADPCLKISEL;

	/********************************
	 *
	 * Set up BIAS (this allows the pins to run at 1.8 or 3.0 volts I think).
	 * This is configured as 0606 in rom_reset.S (i don't think thats right).
	 * Note: The CSB703 ties this interface to 3.3 volts.
	 * TRM 22.5.3
	 * TRM 7.5.2 and flowchart in figure 7-24...
	 */
pbias_retry:
	*(vulong *)CONTROL_PBIAS_LITE = PBIAS_LITE_VMMC1_52MHZ;
	monDelay(100);
	*(vulong *)CONTROL_PBIAS_LITE |= MMC_PWR_STABLE;
	monDelay(100);
	if (*(vulong *)CONTROL_PBIAS_LITE & PBIAS_LITE_MMC1_ERROR) {
		*(vulong *)CONTROL_PBIAS_LITE &= (~MMC_PWR_STABLE);
		monDelay(100);
		if (pbiasretry++ < 3) {
			goto pbias_retry;
		}
		else {
            printf("sdInit: PBIAS timeout\n");
            return(-1);
		}
	}

#if 0
	/*******************************
	 *
	 * These registers are things I found when scouring the TRM for "MMC".
	 * I don't think they have any affect on basic startup of the interface
	 * so they are removed for now...
	 */
	*(vulong *)CM_AUTOIDLE1_CORE &= ~AUTO_MMC1;		// Disable auto clock enable
	*(vulong *)PM_WKEN1_CORE &= ~EN_MMC1;			// Disable wakeup event
	*(vulong *)PM_MPUGRPSEL1_CORE &= ~GRPSEL_MMC1;	// Disable mpu-group wakeup
	*(vulong *)PM_IVA2GRPSEL1_CORE &= ~GRPSEL_MMC1;	// Disable iva2-group wakeup
	*(vulong *)PM_WKST1_CORE &= ~EN_MMC1;			// Clear wakeup status
#endif

	/*******************************
	 *
	 * Issue soft reset and wait for completion...
	 * (TRM 22.5.1.2)
	 */
	MMC1_REG(MMCHS_SYSCONFIG) |= SRESET;			// Software reset
	if ((MMC1_REG(MMCHS_SYSSTATUS) & RESETDONE) == 0)
		printf("Good, RESETDONE is low here\n");

 	startElapsedTimer(&tmr,MMCTMOUT);					// Wait for completion
	while((MMC1_REG(MMCHS_SYSSTATUS) & RESETDONE) == 0) {
		if(msecElapsed(&tmr)) {
            printf("sdInit: SRST failed\n");
            return(-1);
        }
	}
	/********************************
	 *
	 * Set SRA bit, then wait for it to clear.
	 */
	MMC1_REG(MMCHS_SYSCTL) |= SRA;
 	startElapsedTimer(&tmr,MMCTMOUT);
	while((MMC1_REG(MMCHS_SYSCTL) & SRA)) {
		if(msecElapsed(&tmr)) {
            printf("sdInit: SRA timeout\n");
            return(-1);
        }
	}
	
 	startElapsedTimer(&tmr,MMCTMOUT);		// Wait for debounce stable.
	while((MMC1_REG(MMCHS_PSTATE) & DEBOUNCE) != DEBOUNCE) {
		if(msecElapsed(&tmr)) {
            printf("sdInit: DEBOUNCE timeout\n");
            return(-1);
        }
	}

	/*******************************
	 *
	 * Establish hardware capabilities:
	 * TRM 22.5.1.3
	 */
	reg = MMC1_REG(MMCHS_CAPA);
	reg &= ~(VS18 | VS30 | VS33);
	reg |= VS18;
	MMC1_REG(MMCHS_CAPA) = reg;

#if 0
	/********************************
	 *
	 * Enable wakeup mode (don't think I need this, tried both ways)
	 * TRM 22.5.1.4
	 */
	MMC1_REG(MMCHS_SYSCONFIG) |= ENWAKEUP;
	MMC1_REG(MMCHS_HCTL) |= IWE;
#endif
	
	/********************************
	 *
	 * MMC Host and Bus Configuration
	 * TRM 22.5.1.5
	 */
	//MMC1_REG(MMCHS_CON) =
	MMC1_REG(MMCHS_HCTL) &= ~SVDS;
	MMC1_REG(MMCHS_HCTL) |= SVDS18;
	monDelay(10);
	MMC1_REG(MMCHS_HCTL) |= SDBP;
	monDelay(100);
	
 	startElapsedTimer(&tmr,MMCTMOUT);			// Wait for SVDS verification
	while((MMC1_REG(MMCHS_HCTL) & SDBP) == 0) {
		if(msecElapsed(&tmr)) {
            printf("sdInit: SDBP timeout\n");
            return(-1);
        }
	}

	MMC1_REG(MMCHS_SYSCTL) |= ICE;			// Enable internal clock

	MMC1_REG(MMCHS_SYSCTL) &= ~CLKDMSK;		// Set clock divisor:
	MMC1_REG(MMCHS_SYSCTL) |= CLKD(960);	// (should be <= 80Khz initially)

 	startElapsedTimer(&tmr,MMCTMOUT);			// Wait for clock stable
	while((MMC1_REG(MMCHS_SYSCTL) & ICS) == 0) {
		if(msecElapsed(&tmr)) {
            printf("sdInit: ICS timeout\n");
            return(-1);
        }
	}

	/* I set these two bits with the hope that the clock will be
	 * active even if there is no card installed (so I atleast can
	 * see *some* activity).
	 */
	MMC1_REG(MMCHS_SYSCTL) |= CEN;			// External clock enable
#if 0
	MMC1_REG(MMCHS_CON) |= CLKEXTFREE;

	reg = MMC1_REG(MMCHS_SYSCONFIG);
	reg &= ~SIDLEMODEMSK;
	reg &= ~CLKACTIVITYMSK;
	reg &= ~AUTOIDLE;
	reg |= (SIDLEMODE(1) | CLKACTIVITY(3));
	MMC1_REG(MMCHS_SYSCONFIG) = reg;
#endif

	/********************************
	 *
	 * Set the INIT bit to send an initialization stream to the card...
	 * (top of left flowchart in TRM section 22.5.2.1)
	 */
	MMC1_REG(MMCHS_CON) |= MMCINIT;
	for(i=0;i<10;i++) {
		sdCmd(CMD(0) | RSPTYPE_NONE,0,0);
		monDelay(2);
	}
	MMC1_REG(MMCHS_CON) &= ~MMCINIT;
	MMC1_REG(MMCHS_STAT) = 0xffffffff;

	if (sdClkSet(400) != 0)
		return(-1);

	/* this is the get_card_type() function in the code from TI...
	 */
	if (sdCmd(CMD(55) | RSPTYPE_48,0,0) < 0) {
		printf("Card type = MMC\n");
		MMC1_REG(MMCHS_CON) |= ODE;
	}
	else {
		if ((MMC1_REG(MMCHS_RSP10) & 0xffff) == 0x0120) {
			printf("Card type = SD\n");
		}
		else {
			printf("Card type = MMC_CARD\n");
			MMC1_REG(MMCHS_CON) |= ODE;
		}
	}

	
#if 0
	/********************************
	 *
	 * Send Command 5
	 * (top of right flowchart in TRM section 22.5.2.1)
	 */
	sdCmd(CMD(5) | RSPTYPE_NONE,0,0);

 	startElapsedTimer(&tmr,MMCTMOUT);
	do {
		reg = MMC1_REG(MMCHS_STAT);
		if (reg & CC) {
			/* For now we assume only SD cards... */
			printf("SDIO detected!!!  Shouldn't be here!\n");
			return(-1);
		}
		if(msecElapsed(&tmr)) {
            printf("sdInit: CTO timeout1\n");
            return(-1);
        }
		
	} while((reg & CTO) == 0);

	/********************************
	 *
	 * Set SRC bit, then wait for it to clear.
	 * (midway down right flowchart in TRM section 22.5.2.1)
	 */
	MMC1_REG(MMCHS_SYSCTL) |= SRC;
 	startElapsedTimer(&tmr,MMCTMOUT);
	while((MMC1_REG(MMCHS_SYSCTL) & SRC)) {
		if(msecElapsed(&tmr)) {
            printf("sdInit: SRC timeout\n");
            return(-1);
        }
	}

	sdCmd(CMD(8) | RSPTYPE_NONE,0,0);

 	startElapsedTimer(&tmr,MMCTMOUT);
	do {
		reg = MMC1_REG(MMCHS_STAT);
		if (reg & CC) {
			/* For now we assume only SD cards... */
			printf("SD BINGO!!!  This is where we want to be!\n");
			return(0);
		}
		if(msecElapsed(&tmr)) {
            printf("sdInit: CTO timeout2\n");
            return(-1);
        }
		
	} while((reg & CTO) == 0);

	/* For now we assume only SD cards... */
	printf("MMC detected!!!  Shouldn't be here!\n");
#endif
	return(-1);
}

int
sdRead(int interface, char *buf, int blk, int blkcnt)
{
	char *from;
	int	size;

	if (interface != 0)
		return(-1);

	from = (char *)(blk * SD_BLKSIZE);
	size = blkcnt * SD_BLKSIZE;
	memcpy(buf,from,size);
	return(0);
}

int
sdWrite(int interface, char *buf, int blk, int blkcnt)
{
	char *to;
	int	size;

	if (interface != 0)
		return(-1);

	to = (char *)(blk * SD_BLKSIZE);
	size = blkcnt * SD_BLKSIZE;
	memcpy(to,buf,size);
	return(0);
}

