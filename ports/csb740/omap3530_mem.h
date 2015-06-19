//==========================================================================
//
// omap3530_mem.h
//
// Author(s):    Luis Torrico, Cogent Computer Systems, Inc.
// Contributors: 
// Date:         12/09/2008
// Description:  This file contains register base addresses and offsets
//				 and access macros for the OMAP3530 Cortex-A8 memory controller
//

#include "bits.h"

// The GPMC supports up to eight chip-select regions of programmable size, and programmable base
// addresses in a total address space of 1 Gbyte. 
// Chip Select mapping for CSB740 
// CS0 = NOR = 0x08000000 
// CS1 = N/A 
// CS2 = N/A 
// CS3 = NAND = 0x18000000 
// CS4 = Ethernet = 0x2C000000 
// CS5 = Expansion = 0x28000000 
// CS6 = N/A  
// CS7 = N/A  

//#define OMAP35XX_GPMC_BASE	0x6E000000
#define GPMC_REG(_x_)		*(vulong *)(OMAP35XX_GPMC_BASE + _x_)
#define MYGPMC_REG(_x_)		(vulong *)(OMAP35XX_GPMC_BASE + _x_)

// GPMC - General Purpose Memory Controller
#define GPMC_SYS_CONFIG		0x10 	// Chip Select 0 Upper Control Register
#define GPMC_SYS_STATUS		0x14 	// Chip Select 0 Lower Control Register
#define GPMC_IRQ_STATUS		0x18 	// Chip Select 0 Additional Control Register
#define GPMC_IRQ_EN			0x1C 	// Chip Select 1 Upper Control Register
#define GPMC_TIMEOUT		0x40 	// Chip Select 1 Lower Control Register
#define GPMC_ERR_ADD		0x44 	// Chip Select 1 Additional Control Register
#define GPMC_ERR_TYPE		0x48 	// Chip Select 2 Upper Control Register
#define GPMC_CONFIG			0x50 	// Chip Select 2 Lower Control Register
#define GPMC_STATUS			0x54 	// Chip Select 2 Additional Control Register

#define GPMC_CS0_CONFIG1	0x60 	// Chip Select 3 Upper Control Register
#define GPMC_CS0_CONFIG2	0x64 	// Chip Select 3 Lower Control Register
#define GPMC_CS0_CONFIG3	0x68 	// Chip Select 3 Additional Control Register
#define GPMC_CS0_CONFIG4	0x6C 	// Chip Select 4 Upper Control Register
#define GPMC_CS0_CONFIG5	0x70 	// Chip Select 4 Lower Control Register
#define GPMC_CS0_CONFIG6	0x74 	// Chip Select 4 Additional Control Register
#define GPMC_CS0_CONFIG7	0x78 	// Chip Select 5 Upper Control Register

#define GPMC_CS1_CONFIG1	0x90 	// Chip Select 5 Lower Control Register
#define GPMC_CS1_CONFIG2	0x94 	// Chip Select 5 Additional Control Register
#define GPMC_CS1_CONFIG3	0x98 	// Configuration Register
#define GPMC_CS1_CONFIG4	0x9C 	// Chip Select 5 Lower Control Register
#define GPMC_CS1_CONFIG5	0xA0 	// Chip Select 5 Additional Control Register
#define GPMC_CS1_CONFIG6	0xA4 	// Configuration Register
#define GPMC_CS1_CONFIG7	0xA8 	// Configuration Register

#define GPMC_CS2_CONFIG1	0xC0 	// Chip Select 5 Lower Control Register
#define GPMC_CS2_CONFIG2	0xC4 	// Chip Select 5 Additional Control Register
#define GPMC_CS2_CONFIG3	0xC8 	// Configuration Register
#define GPMC_CS2_CONFIG4	0xCC 	// Chip Select 5 Lower Control Register
#define GPMC_CS2_CONFIG5	0xD0 	// Chip Select 5 Additional Control Register
#define GPMC_CS2_CONFIG6	0xD4 	// Configuration Register
#define GPMC_CS2_CONFIG7	0xD8 	// Configuration Register

#define GPMC_CS3_CONFIG1	0xF0 	// Chip Select 5 Lower Control Register
#define GPMC_CS3_CONFIG2	0xF4 	// Chip Select 5 Additional Control Register
#define GPMC_CS3_CONFIG3	0xF8 	// Configuration Register
#define GPMC_CS3_CONFIG4	0xFC 	// Chip Select 5 Lower Control Register
#define GPMC_CS3_CONFIG5	0x100 	// Chip Select 5 Additional Control Register
#define GPMC_CS3_CONFIG6	0x104 	// Configuration Register
#define GPMC_CS3_CONFIG7	0x108 	// Configuration Register

#define GPMC_CS4_CONFIG1	0x120 	// Chip Select 5 Lower Control Register
#define GPMC_CS4_CONFIG2	0x124 	// Chip Select 5 Additional Control Register
#define GPMC_CS4_CONFIG3	0x128 	// Configuration Register
#define GPMC_CS4_CONFIG4	0x12C 	// Chip Select 5 Lower Control Register
#define GPMC_CS4_CONFIG5	0x130 	// Chip Select 5 Additional Control Register
#define GPMC_CS4_CONFIG6	0x134 	// Configuration Register
#define GPMC_CS4_CONFIG7	0x138 	// Configuration Register

#define GPMC_CS5_CONFIG1	0x150 	// Chip Select 5 Lower Control Register
#define GPMC_CS5_CONFIG2	0x154 	// Chip Select 5 Additional Control Register
#define GPMC_CS5_CONFIG3	0x158 	// Configuration Register
#define GPMC_CS5_CONFIG4	0x15C 	// Chip Select 5 Lower Control Register
#define GPMC_CS5_CONFIG5	0x160 	// Chip Select 5 Additional Control Register
#define GPMC_CS5_CONFIG6	0x164 	// Configuration Register
#define GPMC_CS5_CONFIG7	0x168 	// Configuration Register

#define GPMC_CS6_CONFIG1	0x180 	// Chip Select 5 Lower Control Register
#define GPMC_CS6_CONFIG2	0x184 	// Chip Select 5 Additional Control Register
#define GPMC_CS6_CONFIG3	0x188 	// Configuration Register
#define GPMC_CS6_CONFIG4	0x18C 	// Chip Select 5 Lower Control Register
#define GPMC_CS6_CONFIG5	0x190 	// Chip Select 5 Additional Control Register
#define GPMC_CS6_CONFIG6	0x194 	// Configuration Register
#define GPMC_CS6_CONFIG7	0x198 	// Configuration Register

#define GPMC_CS7_CONFIG1	0x1B0 	// Chip Select 5 Lower Control Register
#define GPMC_CS7_CONFIG2	0x1B4 	// Chip Select 5 Additional Control Register
#define GPMC_CS7_CONFIG3	0x1B8 	// Configuration Register
#define GPMC_CS7_CONFIG4	0x1BC 	// Chip Select 5 Lower Control Register
#define GPMC_CS7_CONFIG5	0x1C0 	// Chip Select 5 Additional Control Register
#define GPMC_CS7_CONFIG6	0x1C4 	// Configuration Register
#define GPMC_CS7_CONFIG7	0x1C8 	// Configuration Register

#define GPMC_PREFETCH_CONFIG1	0x1E0 	
#define GPMC_PREFETCH_CONFIG2	0x1E4 	
#define GPMC_PREFETCH_CONTROL	0x1EC 	
#define GPMC_PREFETCH_STATUS	0x1F0 	

// Bit Defines for OMAP3530 GPMC
// WEIM_CS0U to WEIM_CS5U - Chip Select Upper Control Register
#define WEIM_CSU_SP				BIT31					// Supervisor Protect, 0 = User mode accesses allowed, 1 = User mode accesses prohibited
#define WEIM_CSU_WP				BIT30					// Write Protect, 0 = Writes allowed, 1 = Writes prohibited
#define WEIM_CSU_BCD(_x_)		((_x_ & 0x03) << 28)	// Burst Clock Divisor, when EIM_CFG_BCM = 0
#define WEIM_CSU_BCS(_x_)		((_x_ & 0x03) << 24)	// Burst Clock Start, # of 1/2 cycles from LBA to BCLK high
#define WEIM_CSU_PSZ_4			(0 << 22)				// page size = 4 words
#define WEIM_CSU_PSZ_8			(1 << 22)				// page size = 8 words
#define WEIM_CSU_PSZ_16			(2 << 22)				// page size = 16 words
#define WEIM_CSU_PSZ_32			(3 << 22)				// page size = 32 words
#define WEIM_CSU_PME			BIT21					// 1 = Enables page mode emulation
#define WEIM_CSU_SYNC			BIT20					// 1 = Enables synchronous burst mode
#define WEIM_CSU_DOL(_x_)		((_x_ & 0x0f) << 16)	// # of clocks -1 before latching read data when SYNC = 1		
#define WEIM_CSU_CNC_0			(0 << 14)				// Hold CS negated after end of cycle for 0 clocks
#define WEIM_CSU_CNC_1			(1 << 14)				// Hold CS negated after end of cycle for 1 clock
#define WEIM_CSU_CNC_2			(2 << 14)				// Hold CS negated after end of cycle for 2 clocks
#define WEIM_CSU_CNC_3			(3 << 14)				// Hold CS negated after end of cycle for 3 clocks
#define WEIM_CSU_WSC(_x_)		((_x_ & 0x3f) << 8)		// Wait States, 0 = 2, 1 = 2, 2-62 = +1, 63 = dtack
#define WEIM_CSU_WSC_DTACK		(0x3f << 8)		        // Wait States, 0 = 2, 1 = 2, 2-62 = +1, 63 = dtack
#define WEIM_CSU_EW				BIT7					// Determines how WEIM supports the ECB input
#define WEIM_CSU_WWS(_x_)		((_x_ & 0x07) << 4)		// Additional wait states for write cycles
#define WEIM_CSU_EDC(_x_)		((_x_ & 0x0f) << 0)		// Dead Cycles after reads for bus turn around

// Bit Defines for MCIMX31
// WEIM_CS0L to WEIM_CS5L - Chip Select Lower Control Register
#define WEIM_CSL_OEA(_x_)		((_x_ & 0x0f) << 28)	// # of 1/2 cycles after CS asserts before OE asserts
#define WEIM_CSL_OEN(_x_)		((_x_ & 0x0f) << 24)	// # of 1/2 cycles OE negates before CS negates
#define WEIM_CSL_WEA(_x_)		((_x_ & 0x0f) << 20)	// # of 1/2 cycles EB0-3 assert before WE asserts
#define WEIM_CSL_WEN(_x_)		((_x_ & 0x0f) << 16)	// # of 1/2 cycles EB0-3 negate before WE negates 
#define WEIM_CSL_CSA(_x_)		((_x_ & 0x0f) << 12)	// # of clocks address is asserted before CS and held after CS
#define WEIM_CSL_EBC			BIT11					// 0 = assert EB0-3 for Reads & Writes, 1 = Writes only
#define WEIM_CSL_DSZ_BYTE_3		(0 << 8)				// device is 8-bits wide, located on byte 3 (D31-24)
#define WEIM_CSL_DSZ_BYTE_2		(1 << 8)				// device is 8-bits wide, located on byte 2 (D23-16)
#define WEIM_CSL_DSZ_BYTE_1		(2 << 8)				// device is 8-bits wide, located on byte 1 (D15-8)
#define WEIM_CSL_DSZ_BYTE_0		(3 << 8)				// device is 8-bits wide, located on byte 0 (D7-0)
#define WEIM_CSL_DSZ_HALF_1		(4 << 8)				// device is 16-bits wide, located on half word 1 (D31-16)
#define WEIM_CSL_DSZ_HALF_0		(5 << 8)				// device is 16-bits wide, located on half word 1 (D15-0)
#define WEIM_CSL_DSZ_WORD		(6 << 8)				// device is 32-bits wide, located on half word 1 (D15-0)
#define WEIM_CSL_CSN(_x_)		((_x_ & 0x0f) << 4)	  	// Chip Select Negate
#define WEIM_CSL_PSR			BIT3					// PSRAM Enable, 0 = disabled, 1 = enabled
#define WEIM_CSL_CRE			BIT2					// Control Register Enable
#define WEIM_CSL_WRAP			BIT1					// 0 = Memory in linear mode, 1 = Memory in wrap mode
#define WEIM_CSL_CSEN			BIT0					// 1 = Enable Chip Select

// Bit Defines for MCIMX31
// WEIM_CS0A to WEIM_CS5A - Chip Select Additional Control Register
#define WEIM_CSA_EBRA(_x_)		((_x_ & 0x0f) << 28)	// # of half AHB clock cycles before EB asserted.
#define WEIM_CSA_EBRN(_x_)		((_x_ & 0x0f) << 24)	// # of half AHB clock cycles between EB negation and end of access.
#define WEIM_CSA_RWA(_x_)		((_x_ & 0x0f) << 20)	// # of half AHB clock cycles RW delay
#define WEIM_CSA_RWN(_x_)		((_x_ & 0x0f) << 16)	// # of half AHB clock cycles between EB negation and end of access
#define WEIM_CSA_MUM			BIT15					// 1 = Muxed Mode
#define WEIM_CSA_LAH_0			(0 << 13)				// 0 AHB half clock cycles between LBA negation and address invalid
#define WEIM_CSA_LAH_1			(1 << 13)				// 1 AHB half clock cycles between LBA negation and address invalid
#define WEIM_CSA_LAH_2			(2 << 13)				// 2 AHB half clock cycles between LBA negation and address invalid
#define WEIM_CSA_LAH_3			(3 << 13)				// 3 AHB half clock cycles between LBA negation and address invalid
#define WEIM_CSA_LBN(_x_)		((_x_ & 0x07) << 10)		// This bit field determines when LBA is negated
#define WEIM_CSA_LBA_0			(0 << 8)				// 0 AHB half clock cycles between beginning of access and LBA assertion.
#define WEIM_CSA_LBA_1			(1 << 8)				// 1 AHB half clock cycles between beginning of access and LBA assertion.
#define WEIM_CSA_LBA_2			(2 << 8)				// 2 AHB half clock cycles between beginning of access and LBA assertion.
#define WEIM_CSA_LBA_3			(3 << 8)				// 3 AHB half clock cycles between beginning of access and LBA assertion.
#define WEIM_CSA_DWW(_x_)		((_x_ & 0x03) << 6)	  	// Decrease Write Wait State
#define WEIM_CSA_DCT_0			(0 << 4)				// 0 AHB clock cycles between CS assertion and first DTACK check.
#define WEIM_CSA_DCT_1			(1 << 4)				// 1 AHB clock cycles between CS assertion and first DTACK check.
#define WEIM_CSA_DCT_2			(2 << 4)				// 2 AHB clock cycles between CS assertion and first DTACK check.
#define WEIM_CSA_DCT_3			(3 << 4)				// 3 AHB clock cycles between CS assertion and first DTACK check.
#define WEIM_CSA_WWU			BIT3					// 1 = Allow wrap on write
#define WEIM_CSA_AGE			BIT2					// 1 = Enable glue logic
#define WEIM_CSA_CNC2			BIT1					// Chip Select Negation Clock Cycles
#define WEIM_CSA_FCE			BIT0					// 1 = Data captured using BCLK_FB

// WEIM_CFG - Configuration Register
#define WEIM_CFG_BCM			BIT2					// 1 = Burst Clock always on, 0 = when CS with SYNC = 1 is accessed
#define WEIM_CFG_MAS			BIT0					// 1 = Merged address space


