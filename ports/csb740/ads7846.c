//==========================================================================
//
//      ads7846.c
//
// Author(s):   Michael Kelly - Cogent Computer Systems, Inc.
// Date:        03/06/03
// Description:	ADS7846 Interface routines for CSB740
//				Modified from MC9328mxl version to use SPI1 
//
//==========================================================================

#include "config.h"
#include "cpuio.h"
#include "genlib.h"
#include "stddefs.h"
#include "omap3530.h"
#include "cpu_gpio.h"
#include "ads7846.h"
#include "cli.h"
#include "umongpio.h"

//--------------------------------------------------------------------------
// function prototypes
//
int ads_init(void);
int ads_rd(uchar ads_ctl);

extern void udelay(int delay);

//--------------------------------------------------------------------------
// ads_init()
//
// This routine sets up the OMAP3530 SPI3 port in Microwire mode
// (8 bit control, 16 bit data, 24 bit total).  It also turns on
// the ADS7843 pen interrupt via a dummy read.  Note that we assume
// that PERCLK2 is 12Mhz (HCLK/4). We are using CS0 on SPI3.  
//
int ads_init()
{
	volatile uchar temp;


	SPI3_REG(SPI_CH0_CTRL) = 0x00;				// Disable SPI3 Channel 0

	SPI3_REG(SPI_CH0_CONF) = SPI_CH_CONF_CLKG
					   | SPI_CH_CONF_DPE0	// no transmission on SPI3_MISO
					   | SPI_CH_CONF_WL(7)	 	// 8-bit data mode
					   | SPI_CH_CONF_EPOL	  	// CS is active low
//					   | SPI_CH_CONF_SB_POL	  	// 
//					   | SPI_CH_CONF_SBE	  	// 
					   | SPI_CH_CONF_CLKD(9);	// divide by 512 = 93Khz
//					   | SPI_CH_CONF_PHA;		// Data is latched on even numbered edges
//					   | SPI_CH_CONF_POL;	  	// SPI clock is active low

	SPI3_REG(SPI_MODULCTRL) = 0x0;				// Functional mode, Master, and auto CS generation

	SPI3_REG(SPI_CH0_CTRL) = 0x01;				// Enable SPI3 Channel 0

	// enable the ADS7846 so it can generate a touch interrupt.
	// this consists of reading any channel, but setting the
	// power down mode in the control byte to 00b.  note we
	// flush the returned data
	//ads_rd(ADS7846E_S | ADS7846E_PD_ADC | ADS7846E_ADD_DFR_X);
	ads_rd(ADS7846E_S | ADS7846E_PD_ADC | ADS7846E_8BIT | ADS7846E_ADD_DFR_Y);
//	ads_rd(ADS7846E_S | ADS7846E_PD_ADC | ADS7846E_ADD_DFR_Y);
	temp = SPI3_REG(SPI_RXD0);

	return 0;
}

//--------------------------------------------------------------------------
int ads_rd(uchar ads_ctl)
{
	int timeout = 100;
	volatile uchar temp0, temp1;

	// the OMAP3530 only handles up to 16-bits per transfer
	// so we send 8-bits at a time to get our total of 24

//	SPI3_REG(SPI_TXD0) = ads_ctl;
//	udelay(10);
//	while (!(SPI3_REG(SPI_IRQSTATUS) & SPI_TX0_EMPTY));
//	while (!(SPI3_REG(SPI_IRQSTATUS) & SPI_RX0_FULL));
//	temp0 = SPI3_REG(SPI_RXD0);
//
//	SPI3_REG(SPI_TXD0) = 0;
//	udelay(10);
//	while (!(SPI3_REG(SPI_IRQSTATUS) & SPI_TX0_EMPTY));
//	while (!(SPI3_REG(SPI_IRQSTATUS) & SPI_RX0_FULL));
//	temp0 = SPI3_REG(SPI_RXD0);
//
//	SPI3_REG(SPI_TXD0) = 0;
//	udelay(10);
//	while (!(SPI3_REG(SPI_IRQSTATUS) & SPI_TX0_EMPTY));
//	while (!(SPI3_REG(SPI_IRQSTATUS) & SPI_RX0_FULL));
//	temp1 = SPI3_REG(SPI_RXD0);

	SPI3_REG(SPI_TXD0) = ads_ctl;
	udelay(10);
	while (!(SPI3_REG(SPI_CH0_STAT) & SPI_CH_TX0_EMPTY));
	while (!(SPI3_REG(SPI_CH0_STAT) & SPI_CH_RX0_FULL));
	temp0 = SPI3_REG(SPI_RXD0);

	SPI3_REG(SPI_TXD0) = 0;
	udelay(10);
	while (!(SPI3_REG(SPI_CH0_STAT) & SPI_CH_TX0_EMPTY));
	while (!(SPI3_REG(SPI_CH0_STAT) & SPI_CH_RX0_FULL));
	temp0 = SPI3_REG(SPI_RXD0);

//	SPI3_REG(SPI_TXD0) = 0;
//	udelay(10);
//	while (!(SPI3_REG(SPI_CH0_STAT) & SPI_CH_TX0_EMPTY));
//	while (!(SPI3_REG(SPI_CH0_STAT) & SPI_CH_RX0_FULL));
//	temp1 = SPI3_REG(SPI_RXD0);
	temp1 = 0;
	return ((temp0 << 8) | temp1);
}

//--------------------------------------------------------------------------
char *adsHelp[] = {
	"Screen touch demo using ADS7846 and OMAP3530 SPI port.",
	"",
	0
};

int ads(int argc, char *argv[])
{
	uchar c;
	int pen, i;
	int average_x, average_y;

	// init the SPI and ADS7846
	if (ads_init() == -1)
	{
		printf("Error intializing ADS7846!\n");
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
		}
		if (GPIO_tst(PIRQ) == 0)
		{
			printf("Pen Down....\n");
			pen = 1;
			while(GPIO_tst(PIRQ) == 0) // keep reading until touch goes away
			{
				average_x = 0;
				average_y = 0;

				// display every 256 samples (less if pen is down a short time)
				for (i = 0; i < 256 ; i++)
				{
//					average_x += ads_rd(ADS7846E_S | ADS7846E_PD_ADC | ADS7846E_ADD_DFR_X);
//					average_y += ads_rd(ADS7846E_S | ADS7846E_PD_ADC | ADS7846E_ADD_DFR_Y);
					average_x += ads_rd(ADS7846E_S | ADS7846E_PD_ADC | ADS7846E_8BIT | ADS7846E_ADD_DFR_X);
					average_y += ads_rd(ADS7846E_S | ADS7846E_PD_ADC | ADS7846E_8BIT | ADS7846E_ADD_DFR_Y);
				}
				printf("X = %04d, Y = %04d\n", average_x/i, average_y/i);

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
