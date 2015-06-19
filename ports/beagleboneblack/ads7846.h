//==========================================================================
//
// ads7846.h
//
// Author(s):    Michael Kelly, Cogent Computer Systems, Inc.
// Contributors: 
// Date:         03/06/03
// Description:  This file contains register offsets and bit defines
//				 for the TI ADS7846 Touch Screen Controller
//

//
// Bit positions for ADS7846E Control byte
//
#define ADS7846E_S				0x80	    // Start bit, always 1
#define ADS7846E_8BIT			0x08	    // 0 = 12-bit conversion, 1 = 8-bits
#define ADS7846E_SER			0x04	    // 0 = Differential, 1 = Single ended

//
// Address select defines for single ended mode (or'ed with the single ended select bit)
//
// USE FOR ADS7846
//#define ADS7846E_ADD_SER_TEMP0	(ADS7846E_SER | (0x0 << 4))	// temperature measurement 1
//#define ADS7846E_ADD_SER_Y		(ADS7846E_SER | (0x1 << 4))	// Y position measurement
//#define ADS7846E_ADD_SER_BAT	(ADS7846E_SER | (0x2 << 4))	// battery input measurement
//#define ADS7846E_ADD_SER_Z1		(ADS7846E_SER | (0x3 << 4))	// pressure measurement 1
//#define ADS7846E_ADD_SER_Z2		(ADS7846E_SER | (0x4 << 4))	// pressure measurement 2
//#define ADS7846E_ADD_SER_X		(ADS7846E_SER | (0x5 << 4))	// X position measurement
//#define ADS7846E_ADD_SER_AUX	(ADS7846E_SER | (0x6 << 4))	// auxillary input measurement
//#define ADS7846E_ADD_SER_TEMP1	(ADS7846E_SER | (0x7 << 4))	// temperature measurement 2

// USE FOR ADS7843
//#define ADS7846E_ADD_SER_TEMP0	(ADS7846E_SER | (0x0 << 4))	// temperature measurement 1
#define ADS7846E_ADD_SER_Y		(ADS7846E_SER | (0x1 << 4))	// Y position measurement
//#define ADS7846E_ADD_SER_BAT	(ADS7846E_SER | (0x2 << 4))	// battery input measurement
//#define ADS7846E_ADD_SER_Z1		(ADS7846E_SER | (0x2 << 4))	// pressure measurement 1
//#define ADS7846E_ADD_SER_Z2		(ADS7846E_SER | (0x6 << 4))	// pressure measurement 2
#define ADS7846E_ADD_SER_X		(ADS7846E_SER | (0x5 << 4))	// X position measurement
//#define ADS7846E_ADD_SER_AUX	(ADS7846E_SER | (0x6 << 4))	// auxillary input measurement
//#define ADS7846E_ADD_SER_TEMP1	(ADS7846E_SER | (0x7 << 4))	// temperature measurement 2

//
// Address select defines for differential mode
//
#define ADS7846E_ADD_DFR_X		(0x1 << 4)	// Y position measurement
//#define ADS7846E_ADD_DFR_Z1		(0x2 << 4)	// pressure measurement 1
//#define ADS7846E_ADD_DFR_Z2		(0x6 << 4)	// pressure measurement 2
#define ADS7846E_ADD_DFR_Y		(0x5 << 4)	// X position measurement

//
// Power Down Modes
//
#define ADS7846E_PD_LPWR		0x0			// low-power mode, no power-up delay, *IRQ is enabled
#define ADS7846E_PD_REF			0x1			// 2.5V reference off, ADC on, requires delay before conversion
#define ADS7846E_PD_ADC			0x2			// ADC off, REF on, no delay required
#define ADS7846E_PD_IRQ			0x3			// device on, but *IRQ is disabled
