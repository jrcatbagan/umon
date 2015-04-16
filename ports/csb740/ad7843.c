//==========================================================================
//
//      ad7843.c
//
// Author(s):   Michael Kelly - Cogent Computer Systems, Inc.
// Date:        03/06/03
// Description:	AD7843 Interface routines for CSB740
//				Modified from MC9328mxl version to use SPI1 
//
//==========================================================================

#include "config.h"
#include "cpuio.h"
#include "genlib.h"
#include "stddefs.h"
#include "omap3530.h"
#include "cpu_gpio.h"
#include "ad7843.h"
#include "cli.h"
#include "umongpio.h"

//--------------------------------------------------------------------------
// function prototypes
//
int ads_init(void);
int ads_rd(uchar ads_ctl);

extern void udelay(int delay);

#ifdef AD7843_GPIOMODE

// After several days trying to get the OMAP's SPI3 controller to interface
// to the AD7843, I gave up and implemented the protocol with GPIO...
//	SPI_CS:		GPIO_91
//	SPI_CLK:		GPIO_88
//	SPI_MOSI:	GPIO_89
//	SPI_MISO:	GPIO_90

#define clrSpiCs()		GPIO3_REG(GPIO_DATAOUT) &= ~BIT27
#define setSpiCs()		GPIO3_REG(GPIO_DATAOUT) |= BIT27
#define clrSpiClk()		GPIO3_REG(GPIO_DATAOUT) &= ~BIT24
#define setSpiClk()		GPIO3_REG(GPIO_DATAOUT) |= BIT24
#define clrSpiMosi()	GPIO3_REG(GPIO_DATAOUT) &= ~BIT25
#define setSpiMosi()	GPIO3_REG(GPIO_DATAOUT) |= BIT25
#define getSpiMiso()	(GPIO3_REG(GPIO_DATAIN) & BIT26) ? 1 : 0


// ads_rd():
// A bit-banged implementation of the SPI access for AD7843...
// Slow and steady gets the job done!
//
int
ads_rd(uchar ads_ctl)
{
	ushort mask, val;

	clrSpiClk();
	clrSpiCs();

	for(mask = 0x80;mask != 0;mask >>= 1) {
		if (ads_ctl & mask)
			setSpiMosi();
		else
			clrSpiMosi();

		setSpiClk();
		clrSpiClk();
	}

	val = 0;
	for(mask = 0x8000;mask != 0;mask >>= 1) {
		setSpiClk();
		if (getSpiMiso())
			val |= mask;
		clrSpiClk();
	}

	clrSpiClk();
	setSpiCs();
	return(val);
}

int
ads_init(void)
{
	GPIO3_REG(GPIO_OE) &= ~(BIT27 | BIT25 | BIT24);	// 0 = out
	GPIO3_REG(GPIO_OE) |= BIT26;
	setSpiCs();
	clrSpiClk();
	clrSpiMosi();
	return(0);
}

#else

//--------------------------------------------------------------------------
// ads_init()/ads_rd():
//
// This routine sets up the OMAP3530 SPI3 port. It also turns on
// the AD7843 pen interrupt via a dummy read. We are using CS0 on SPI3.  
// Can't get this to work.  Signals look good on scope, but we're not
// able to read the data back; hence the need for a GPIO version (above).

int
ads_rd(uchar ads_ctl)
{
	volatile ulong rxval;

	SPI3_REG(SPI_IRQSTATUS) = 0x0003777f;
	SPI3_REG(SPI_IRQENABLE) = 0x0000007f;
	SPI3_REG(SPI_CH0_CTRL) = 0x00000001;		// Enable SPI3 Channel 0

	// We have this OMAP3530 SPI ctrlr set up in 24-bit data mode, full
	// duplex transmit/receive.  So, put the byte to be transferred in the
	// upper 8 bits of the 24-bit word, then read back the next 16 bits...
	SPI3_REG(SPI_TXD0) = ads_ctl << 16;

	// Wait for the receive channel to be full...
	while (!(SPI3_REG(SPI_CH0_STAT) & SPI_CH_RX0_FULL));

	SPI3_REG(SPI_IRQSTATUS) = 0x0003777f;
	
	// Read the value...
	rxval = SPI3_REG(SPI_RXD0);

	SPI3_REG(SPI_CH0_CTRL) = 0x00000000;		// Disable SPI3 Channel 0

//	if (rxval)
//		printf("%08x\n",rxval);

	return (rxval);
}

int
ads_init()
{
	unsigned long conf;


	// Soft reset...
	SPI3_REG(SPI_SYSCONFIG) = 0x00000002;

	// Wait for reset done...
	while((SPI3_REG(SPI_SYSSTATUS) & 1) == 0);

	// Configure chan zero of SPI3...
	SPI3_REG(SPI_IRQSTATUS) = 0x0003777f;
	SPI3_REG(SPI_IRQENABLE) = 0x0000007f;
	SPI3_REG(SPI_MODULCTRL) = 0x00000000;	// Master, auto CS generation
	SPI3_REG(SPI_CH0_CTRL) = 0x00000001;	// Enable SPI3 Channel 0

	conf   	   = SPI_CH_CONF_DPE0
			   | SPI_CH_CONF_TRM_TR
		   	   | SPI_CH_CONF_WL(23)	 	// 24-bit data mode
		   	   | SPI_CH_CONF_EPOL	  	// CS is active low
		   	   | SPI_CH_CONF_SB_POL	  	// 
		   	   | SPI_CH_CONF_CLKD(9);	// divide by 512 = 93Khz

	SPI3_REG(SPI_CH0_CONF) = conf;

	// enable the AD7843 so it can generate a touch interrupt.
	// this consists of reading any channel, but setting the
	// power down mode in the control byte to 00b.  note we
	// flush the returned data

	if (ads_rd(AD7843_S | AD7843_ADD_DFR_Y | AD7843_PD_MOD0) == -1)
	{
		printf("Error returned from ads_rd(0x%02x)!\n", (AD7843_S | AD7843_ADD_DFR_Y | AD7843_PD_MOD0));
		return -1;
	}

	return 0;
}
#endif


//--------------------------------------------------------------------------
char *adsHelp[] = {
	"Screen touch demo using AD7843 and OMAP3530 SPI port.",
	"",
	"Detect screen touches and display the x,y values via",
	"the AD7843 and OMAP3530 SPI port.",
	0
};

int ads(int argc, char *argv[])
{
	uchar c;
	int pen, i;
	//int last_x, last_y;
	int sum_x, sum_y, average_x, average_y;

	// init the SPI and AD7843
	if (ads_init() == -1)
	{
		printf("Error intializing AD7843!\n");
		return (CMD_FAILURE);
	}

	printf("Waiting for Touch (Press \'X\' to end test)...\n");

	pen = 0;
	while (1)
	{
		if (gotachar())
		{
			c = getchar();
			if ((c == 'X') || (c == 'x')) goto done;
			if (c == 'L') {
				printf("In ads_rd loop...\n");
				while(1)
					ads_rd(AD7843_S | AD7843_PD_MOD0 | AD7843_ADD_DFR_X);
			}
		}
		if (GPIO_tst(PIRQ) == 0)
		{
			printf("Pen Down....\n");
			pen = 1;
			//last_x = last_y = 0;
			while(GPIO_tst(PIRQ) == 0) // keep reading until touch goes away
			{
				sum_x = sum_y = 0;

				// display every 4 samples
				for (i = 0; i < 64 ; i++)
				{
					sum_x += ads_rd(AD7843_S | AD7843_PD_MOD0 | AD7843_ADD_DFR_X);
					sum_y += ads_rd(AD7843_S | AD7843_PD_MOD0 | AD7843_ADD_DFR_Y);
				}
				average_x = sum_x/4;
				average_y = sum_y/4;

 				//if ((average_x != last_x) || (average_y != last_y))
 					printf("X = %04d, Y = %04d\n", average_x, average_y);
	
				//last_x = average_x;
				//last_y = average_y;

			} // while pen is down
		}
		if (pen)
		{
			printf("Pen Up....\n");
			pen = 0;
		}

	}

done:		
	return(CMD_SUCCESS);
}


/* The next four functions are required for the "scribble" feature in
 * the FBI command to work...
 */
#include "tsi.h"

#define TOUCH_MAX_YVAL	32760
#define TOUCH_MAX_XVAL	30752 

/* tsi_init():
 * Used to initialize the touch screen interface.
 */
int
tsi_init(void)
{
	return(ads_init());
}

/* tsi_active():
 * Return 1 if the screen is being touched, else 0.
 */
int
tsi_active(void)
{
	if (GPIO_tst(PIRQ) == 0)
		return(1);
	return(0);
}

/* tsi_getx()/tsi_gety():
 * Return the current 'x' or 'y' position detected by the touch screen.
 * Notice that these functions return a value that is relative to the
 * frame-buffer coordinates, not raw the coordinates generated by the
 * touch-screen hardware.
 * This requires not only that the incoming value from the AD7843 be
 * normalized to the range of the X/Y coordinates of the frame buffer,
 * but it also requires that the 'Y' coordinate be adjusted to be from
 * top-down, not bottom up.
 */
int
tsi_getx(void)
{
	int i, val, tot, stot, tmp;
	
	tot = stot = 0;

	/* Attempt 8 samples, then average...
	 */
	for(i=0;i<8;i++) {
		tmp = ads_rd(AD7843_S | AD7843_PD_MOD0 | AD7843_ADD_DFR_X);
		if ((tmp > 0) && (tmp < TOUCH_MAX_XVAL)) {
			tot += tmp;
			stot++;
		}
	}
	tot /= stot;

	val = tot / (TOUCH_MAX_XVAL/PIXELS_PER_ROW);
	return(val);
}

int
tsi_gety(void)
{
	int i, val, tot, stot, tmp;
	
	tot = stot = 0;

	/* Attempt 8 samples, then average...
	 */
	for(i=0;i<8;i++) {
		tmp = ads_rd(AD7843_S | AD7843_PD_MOD0 | AD7843_ADD_DFR_Y);
		if ((tmp > 0) && (tmp < TOUCH_MAX_YVAL)) {
			tot += tmp;
			stot++;
		}
	}
	tot /= stot;

	val = PIXELS_PER_COL - (tot/(TOUCH_MAX_YVAL/PIXELS_PER_COL));
	return(val);
}
