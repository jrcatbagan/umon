
//=============================================================================
//
//      cpu_gpio.h
//
//      CPU/Board Specific GPIO assignments
//
// Author(s):    Luis Torrico, Cogent Computer Systems, Inc.
// Date:         05/12/2008
// Description:  This file contains the GPIOIO usage for the CSB733
//
//
//=============================================================================

// PORT 1 - Virtual GPIO Bits 0-31
#define PORT1_VBIT      0
// Signal			    Bit		               Dir	Active	Usage

// Define initial direction (0 = output) and value
#define PORT1_OE		0xFFFFFFFF
// Define data value
#define PORT1_DR        0
       
// -----------------------------------------------------------------------------------------------
// PORT 2
#define PORT2_VBIT      32
// Signal			    Bit		               Dir	Active	Usage
#define	N_RDY_BIT	    BIT30	            // In   High	NAND Ready
#define	N_RDY		    PORT2_VBIT + 30
#define	LCD_BKL_BIT	    BIT26	            // Out  High	LCD Backlight Enable
#define	LCD_BKL		    PORT2_VBIT + 26
#define	EXP_INT_BIT	    BIT25	            // In   Low		Expansion Interrupt
#define	EXP_INT		    PORT2_VBIT + 25

// Define initial direction (0 = output) and value
#define PORT2_OE		~(LCD_BKL_BIT)
// Define data value
#define PORT2_DR        (LCD_BKL_BIT)

// -----------------------------------------------------------------------------------------------
// PORT 3
#define PORT3_VBIT      64

// Signal			    Bit		               Dir	Active	Usage
#define	GPIO7_BIT	    BIT28	            // In	High	Set as GPIO Input, pulled high.  Shared with SPI1_CS1
#define	GPIO7	        PORT3_VBIT + 28
#define	SPI3_CS0_BIT    BIT27	            // In	High	SPI3_CS0
#define	SPI3_CS0        PORT3_VBIT + 27
#define	E_INT_BIT	    BIT1	            // In	Low		LAN9211 Interrupt
#define	E_INT	        PORT3_VBIT + 1

// Define initial direction (0 = output) and value
#define PORT3_OE		~(SPI3_CS0_BIT)
// Define data value
#define PORT3_DR 		SPI3_CS0_BIT

// -----------------------------------------------------------------------------------------------
// PORT 4
#define PORT4_VBIT      96

// Signal			    Bit		               Dir	Active	Usage
#define	PIRQ_BIT	    BIT31	            // In	Low		Touch Interrupt from ADS7843
#define	PIRQ	        PORT4_VBIT + 31
#define	I2C_INT_BIT	    BIT30	            // In	Low		I2C Interrupt
#define	I2C_INT	        PORT4_VBIT + 30

// Define initial direction (0 = output) and value
#define PORT4_OE		0xFFFFFFFF
// Define data value
#define PORT4_DR 		0

// -----------------------------------------------------------------------------------------------
// PORT 5
#define PORT5_VBIT      128

// Signal			    Bit		               Dir	Active	Usage
#define	U_SEL_BIT	    BIT12	            // Out	N/A		Selects between USB Host or Device
#define	U_SEL	        PORT5_VBIT + 12
#define	GPIO1_BIT	    BIT1	            // In	High	Push button on CSB703
#define	GPIO1	        PORT5_VBIT + 1
#define	GPIO0_BIT	    BIT0	            // Out	N/A		LED on CSB703
#define	GPIO0	        PORT5_VBIT + 0

// Define initial direction (0 = output) and value
#define PORT5_OE		~(U_SEL_BIT | GPIO0_BIT)
// Define data value
#define PORT5_DR 		(U_SEL_BIT | GPIO0_BIT)

// -----------------------------------------------------------------------------------------------
// PORT 6
#define PORT6_VBIT      160

// Signal			    Bit		               Dir	Active	Usage
#define	GPIO9_BIT	    BIT25	            // In	High	Set as GPIO Input, pulled high.
#define	GPIO9	        PORT6_VBIT + 25
#define	GPIO8_BIT	    BIT24	            // In	High	Set as GPIO Input, pulled high.
#define	GPIO8	        PORT6_VBIT + 24
#define	GPIO2_BIT	    BIT22	            // In	High	Set as GPIO Input, pulled high.
#define	GPIO2	        PORT6_VBIT + 22
#define	GPIO3_BIT	    BIT21	            // In	High	Set as GPIO Input, pulled high.
#define	GPIO3	        PORT6_VBIT + 21
#define	GPIO4_BIT	    BIT20	            // In	High	Set as GPIO Input, pulled high.
#define	GPIO4	        PORT6_VBIT + 20
#define	GPIO5_BIT	    BIT19	            // In	High	Set as GPIO Input, pulled high.
#define	GPIO5	        PORT6_VBIT + 19
#define	GPIO6_BIT	    BIT17	            // In	High	Set as GPIO Input, pulled high.
#define	GPIO6	        PORT6_VBIT + 17

// Define initial direction (0 = output) and value
#define PORT6_OE		0xFFFFFFFF
// Define data value
#define PORT6_DR 		0

