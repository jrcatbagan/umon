//==========================================================================
//
// ad7843.h
//
// Author(s):    Michael Kelly, Cogent Computer Systems, Inc.
// Contributors: 
// Date:         03/06/03
// Description:  This file contains register offsets and bit defines
//				 for the Analog Devices AD7843 Touch Screen Controller
//

//
// Bit positions for AD7843 Control byte
//
#define AD7843_S			0x80	    // Start bit, always 1
#define AD7843_8BIT			0x08	    // 0 = 12-bit conversion, 1 = 8-bits
#define AD7843_SER			0x04	    // 0 = Differential, 1 = Single ended

// Address select defines for Single-Ended mode
#define AD7843_ADD_SER_Y	(AD7843_SER | (0x1 << 4))	// Y position measurement
#define AD7843_ADD_SER_IN3	(AD7843_SER | (0x2 << 4))	// auxillary input 1 measurement
#define AD7843_ADD_SER_X	(AD7843_SER | (0x5 << 4))	// X position measurement
#define AD7843_ADD_SER_IN4	(AD7843_SER | (0x6 << 4))	// auxillary input 2 measurement

// Address select defines for Differential mode
#define AD7843_ADD_DFR_Y		(0x1 << 4)	// Y position measurement
#define AD7843_ADD_DFR_X		(0x5 << 4)	// X position measurement

// Power Down Modes
#define AD7843_PD_MOD0		0x0			// low-power mode, no power-up delay, *IRQ is enabled
#define AD7843_PD_MOD1		0x1			// same as low-power mode, except *IRQ is disabled
#define AD7843_PD_MOD2		0x2			// device on, *IRQ is enabled
#define AD7843_PD_MOD3		0x3			// device on, *IRQ is disabled

//#define AD7843_GPIOMODE	
