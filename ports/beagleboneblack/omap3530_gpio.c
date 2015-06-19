//==========================================================================
//
// mx31_gpio.c
//
//
// Author(s):    Luis Torrico, Cogent Computer Systems, Inc.
// Contributors: 
// Date:         05/09/2008
// Description:  This file contains code to intialize the MCIMX31 GPIO
//               section as well as functions for manipulating the GPIO 
//               bits
//
//--------------------------------------------------------------------------

#include "config.h"
#include "cpuio.h"
#include "stddefs.h"
#include "genlib.h"
#include "omap3530.h"
#include "cpu_gpio.h"    // pull in target board specific header

//#define GPIO_DBG

//--------------------------------------------------------
// GPIO_init()
//
// This function sets the startup state for the MCIMX31 GPIO 
// registers as used on the target CPU.  Refer to cpu_gpio.h 
// for a description of the default values.  Here we just put
// them into the chip in the following order:
// 1. Port x DR	   - Data Register
// 2. Port x DIR   - Direction Register
// 3. Port x PSTAT - Pad Status Register
// 4. Port x ICR1  - GPIO Interrupt Configuration Register 1
// 5. Port x ICR2  - GPIO Interrupt Configuration Register 2
// 6. Port x IMASK - GPIO Interrupt Mask Register
// 7. Port x ISTAT - GPIO Interrupt Status Register

void GPIO_init()
{
	// Port 1
	GPIO1_REG(GPIO_OE)				= PORT1_OE;
	GPIO1_REG(GPIO_DATAOUT)			= PORT1_DR;
	//GPIO1_REG(GPIO_CLEARDATAOUT)	= 0;
	//GPIO1_REG(GPIO_SETDATAOUT)		= 0;

	// Port 2
	//GPIO2_REG(GPIO_OE)			= 0xFEFFFFFF;
	GPIO2_REG(GPIO_OE)				= PORT2_OE;
	GPIO2_REG(GPIO_DATAOUT)			= PORT2_DR;
	//GPIO2_REG(GPIO_CLEARDATAOUT)	= 0;
	//GPIO2_REG(GPIO_SETDATAOUT)		= 0;

	// Port 3
	GPIO3_REG(GPIO_OE)				= PORT3_OE;
	GPIO3_REG(GPIO_DATAOUT)			= PORT3_DR;
	//GPIO3_REG(GPIO_CLEARDATAOUT)	= 0;
	//GPIO3_REG(GPIO_SETDATAOUT)		= 0;

	// Port 4
	GPIO4_REG(GPIO_OE)				= PORT4_OE;
	GPIO4_REG(GPIO_DATAOUT)			= PORT4_DR;
	//GPIO4_REG(GPIO_CLEARDATAOUT)	= 0;
	//GPIO4_REG(GPIO_SETDATAOUT)		= 0;

	// Port 5
	GPIO5_REG(GPIO_OE)				= PORT5_OE;
	GPIO5_REG(GPIO_DATAOUT)			= PORT5_DR;
	//GPIO5_REG(GPIO_CLEARDATAOUT)	= 0;
	//GPIO5_REG(GPIO_SETDATAOUT)		= 0;

	// Port 6
	GPIO6_REG(GPIO_OE)				= PORT6_OE;
	GPIO6_REG(GPIO_DATAOUT)			= PORT6_DR;
	//GPIO6_REG(GPIO_CLEARDATAOUT)	= 0;
	//GPIO6_REG(GPIO_SETDATAOUT)		= 0;
}

//--------------------------------------------------------
// GPIO_set()
//
// This function sets the desired bit passed in.
// NOTE: We do not test to see if setting the bit
// would screw up any alternate functions.  Use 
// this function with caution!
//

int GPIO_set(int gpio_bit)
{
	// quick sanity test
#ifdef GPIO_DBG
	printf("GPIO_set %d.\n", gpio_bit);
#endif
	if (gpio_bit > 191) return -1;

	if (gpio_bit < 32)
	{
		// Port 1
		GPIO1_REG(GPIO_DATAOUT) |= (1 << (gpio_bit - 0));
	}
	else if (gpio_bit < 64)
	{
		// Port 2
		GPIO2_REG(GPIO_DATAOUT) |= (1 << (gpio_bit - 32));
	}
	else if (gpio_bit < 96)
	{
		// Port 3
		GPIO3_REG(GPIO_DATAOUT) |= (1 << (gpio_bit - 64));
	}
	else if (gpio_bit < 128)
	{
		// Port 4
		GPIO4_REG(GPIO_DATAOUT) |= (1 << (gpio_bit - 96));
	}
	else if (gpio_bit < 160)
	{
		// Port 5
		GPIO5_REG(GPIO_DATAOUT) |= (1 << (gpio_bit - 128));
	}
	else
	{
		// Port 6
		GPIO6_REG(GPIO_DATAOUT) |= (1 << (gpio_bit - 160));
	}
	return 0;
}

//--------------------------------------------------------
// GPIO_clr()
//
// This function clears the desired bit passed in.
//

int GPIO_clr(int gpio_bit)
{
#ifdef GPIO_DBG
	printf("GPIO_clr %d.\n", gpio_bit);
#endif
 	// quick sanity test
	if (gpio_bit > 191) return -1;

	if (gpio_bit < 32)
	{
		// Port 1
		GPIO1_REG(GPIO_DATAOUT) &= ~(1 << (gpio_bit - 0));
	}
	else if (gpio_bit < 64)
	{
		// Port 2
		GPIO2_REG(GPIO_DATAOUT) &= ~(1 << (gpio_bit - 32));
	}
	else if (gpio_bit < 96)
	{
		// Port 3
		GPIO3_REG(GPIO_DATAOUT) &= ~(1 << (gpio_bit - 64));
	}
	else if (gpio_bit < 128)
	{
		// Port 4
		GPIO4_REG(GPIO_DATAOUT) &= ~(1 << (gpio_bit - 96));
	}
	else if (gpio_bit < 160)
	{
		// Port 5
		GPIO5_REG(GPIO_DATAOUT) &= ~(1 << (gpio_bit - 128));
	}
	else
	{
		// Port 6
		GPIO6_REG(GPIO_DATAOUT) &= ~(1 << (gpio_bit - 160));
	}
    return 0;
}
//--------------------------------------------------------
// GPIO_tst()
//
// This function returns the state of desired bit passed in.
// It does not test to see if it's an input or output and thus
// can be used to verify if an output set/clr has taken place
// as well as for testing an input state.
//

int GPIO_tst(int gpio_bit)
{
#ifdef GPIO_DBG
	printf("GPIO_tst %d.\n", gpio_bit);
#endif
 	// quick sanity test
	if (gpio_bit > 191) return -1;

	if (gpio_bit < 32)
	{
		// Port 1
		if (GPIO1_REG(GPIO_DATAIN) & (1 << (gpio_bit - 0))) return 1;
	}
	else if (gpio_bit < 64)
	{
		// Port 2
		if (GPIO2_REG(GPIO_DATAIN) & (1 << (gpio_bit - 32))) return 1;
	}
	else if (gpio_bit < 96)
	{
		// Port 3
		if (GPIO3_REG(GPIO_DATAIN) & (1 << (gpio_bit - 64))) return 1;
	}
	else if (gpio_bit < 128)
	{
		// Port 4
		if (GPIO4_REG(GPIO_DATAIN) & (1 << (gpio_bit - 96))) return 1;
	}
	else if (gpio_bit < 160)
	{
		// Port 5
		if (GPIO5_REG(GPIO_DATAIN) & (1 << (gpio_bit - 128))) return 1;
	}
	else
	{
		// Port 6
		if (GPIO6_REG(GPIO_DATAIN) & (1 << (gpio_bit - 160))) return 1;
	}
	return 0; // bit was not set
}

//--------------------------------------------------------
// GPIO_out()
//
// This function changes the direction of the desired bit 
// to output.  NOTE: We do not test to see if changing the
// direction of the bit would screw up anything.  Use this
// function with caution!
//
// This only worlks if the GPIO has been defined as a GPIO
// during init.  It will not override the init setting, only
// change the direction bit

int GPIO_out(int gpio_bit)
{
#ifdef GPIO_DBG
	printf("GPIO_out %d.\n", gpio_bit);
#endif
	// quick sanity test
	if (gpio_bit > 191) return -1;

	if (gpio_bit < 32)
	{
		// Port 1
		GPIO1_REG(GPIO_OE) &= ~(1 << (gpio_bit - 0));
	}
	else if (gpio_bit < 64)
	{
		// Port 2
		GPIO2_REG(GPIO_OE) &= ~(1 << (gpio_bit - 32));
	}
	else if (gpio_bit < 96)
	{
		// Port 3
		GPIO3_REG(GPIO_OE) &= ~(1 << (gpio_bit - 64));
	}
	else if (gpio_bit < 128)
	{
		// Port 4
		GPIO4_REG(GPIO_OE) &= ~(1 << (gpio_bit - 96));
	}
	else if (gpio_bit < 160)
	{
		// Port 5
		GPIO5_REG(GPIO_OE) &= ~(1 << (gpio_bit - 128));
	}
	else
	{
		// Port 6
		GPIO6_REG(GPIO_OE) &= ~(1 << (gpio_bit - 160));
	}
    return 0;
}

//--------------------------------------------------------
// GPIO_in()
//
// This function changes the direction of the desired bit 
// to input.  NOTE: We do not test to see if changing the
// direction of the bit would screw up anything.  Use this
// function with caution!
//
// This only worlks if the GPIO has been defined as a GPIO
// during init.  It will not override the init setting, only
// change the direction bit
int GPIO_in(int gpio_bit)
{
#ifdef GPIO_DBG
	printf("GPIO_in %d.\n", gpio_bit);
#endif
	// quick sanity test
	if (gpio_bit > 191) return -1;

	if (gpio_bit < 32)
	{
		// Port 1
		GPIO1_REG(GPIO_OE) |= (1 << (gpio_bit - 0));
	}
	else if (gpio_bit < 64)
	{
		// Port 2
		GPIO2_REG(GPIO_OE) |= (1 << (gpio_bit - 32));
	}
	else if (gpio_bit < 96)
	{
		// Port 3
		GPIO3_REG(GPIO_OE) |= (1 << (gpio_bit - 64));
	}
	else if (gpio_bit < 128)
	{
		// Port 4
		GPIO4_REG(GPIO_OE) |= (1 << (gpio_bit - 96));
	}
	else if (gpio_bit < 160)
	{
		// Port 5
		GPIO5_REG(GPIO_OE) |= (1 << (gpio_bit - 128));
	}
	else
	{
		// Port 6
		GPIO6_REG(GPIO_OE) |= (1 << (gpio_bit - 160));
	}
	return 0;
}

