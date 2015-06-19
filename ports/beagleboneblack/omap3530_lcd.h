//==========================================================================
//
// omap3530_lcd.h
//
// Author(s):    Luis Torrico, Cogent Computer Systems, Inc.
// Contributors: 
// Date:         12/10/2008
// Description:  This file contains register offsets and bit defines
//				 for the OMAP3530 Cortex-A8 LCD Controller
//

#include "bits.h"

/* The DSS is designed to support video and graphics processing functions and to */
/* interface with video/still image sensors and displays. */

/*-------------------------------------------------------------------------------------*/
/* Display Interface Subsystem */ 
/*-------------------------------------------------------------------------------------*/
/* Module Name				Base Address		Size */
/* 
   DSI Protocol Engine		0x4804FC00			512 bytes
   DSI Complex I/O			0x4804FE00			64 bytes
   DSI PLL Controller		0x4804FF00			32 bytes
   DISS						0x48050000			512 byte
   DISPC					0x48050400			1K byte
   RFBI						0x48050800			256 bytes
   VENC						0x48050C00			256 bytes
*/
/*-------------------------------------------------------------------------------------*/
#define DSS_BASE_ADD		0x48050000 		// Display Subsystem Base Address
#define DISPC_BASE_ADD		0x48050400 		// Display Controller Base Address
#define DSS_REG(_x_)		*(vulong *)(DSS_BASE_ADD + _x_)
#define DISPC_REG(_x_)		*(vulong *)(DISPC_BASE_ADD + _x_)

// Display Subsystem Registers
#define DSS_SYSCONFIG		0x10 		// 
#define DSS_SYSSTATUS		0x14 		// 
#define DSS_IRQSTATUS		0x18 		// 
#define DSS_CONTROL			0x40 		// 
#define DSS_SDI_CONTROL		0x44 		// 
#define DSS_PLL_CONTROL		0x48 		// 
#define DSS_SDI_STATUS		0x5C 		// 

// Display Controller Registers
#define DISPC_SYSCONFIG			0x10 		// 
#define DISPC_SYSSTATUS			0x14 		// 
#define DISPC_IRQSTATUS			0x18 		// 
#define DISPC_IRQENABLE			0x1C 		// 
#define DISPC_CONTROL			0x40 		// 
#define DISPC_CONFIG			0x44 		// 
#define DISPC_CAPABLE			0x48 		// 
#define DISPC_DEFAULT_COLOR		0x4C 		// 
#define DISPC_TRANS_COLOR		0x54 		// 
#define DISPC_LINE_STATUS		0x5C 		// 
#define DISPC_LINE_NUMBER		0x60 		// 
#define DISPC_TIMING_H 			0x64 		// 
#define DISPC_TIMING_V 			0x68 		// 
#define DISPC_POL_FREQ			0x6C 		// 
#define DISPC_DIVISOR			0x70 		// 
#define DISPC_GLOBAL_ALPHA		0x74 		// 
#define DISPC_SIZE_DIG			0x78 		// 
#define DISPC_SIZE_LCD			0x7C 		// 
#define DISPC_GFX_BA			0x80 		// 
#define DISPC_GFX_POS			0x88 		// 
#define DISPC_GFX_SIZE			0x8C 		// 
#define DISPC_GFX_ATTR			0xA0 		// 
#define DISPC_GFX_FIFO_TH		0xA4 		// 
#define DISPC_GFX_FIFO_SS		0xA8 		// 
#define DISPC_GFX_ROW_INC		0xAC 		// 
#define DISPC_GFX_PIX_INC		0xB0 		// 
#define DISPC_GFX_WIN_SKIP		0xB4 		// 
#define DISPC_GFX_TABLE_BA		0xB8 		// 
#define DISPC_GFX_PRELOAD		0x62C 		// 

