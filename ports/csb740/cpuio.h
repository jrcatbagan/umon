//=============================================================================
//
//      cpuio.h
//
//      CPU/Board Specific IO
//
// Author(s):    Luis Torrico, Cogent Computer Systems, Inc.
// Date:         05-16-2008
// Description:  This file contains the IO functions required by Micro Monitor
//				 that are unique to each CPU/Board combination
//
// 
// cpuio.h for the CSB740 OMAP3530 Cortex-A8
//
//=============================================================================

// board specific defines for micro monitor
#define DEFAULT_BAUD_RATE   	38400
#define MON_CPU_CLOCK       	400000000

#define LOOPS_PER_USEC			5

#define BASE_OF_NAND			0x1000
#define SIZE_OF_NAND			0x100000

// SMSC LAN9211 Ethernet 
#define SMSC911X_BASE_ADDRESS	0x2C000000	// CS4 on OMAP3530 but we call it CS2

// LCD Defines
//
// The LCD frame buffer is fixed at 0x80200000, which is 2Mbyte from the
// beginning of SDRAM space.  Note that we access it 16-bits at a time.

#define LCD_BUF_ADD				0x80200000	
#define LCD_BUF(_x_)			*(vushort *)(LCD_BUF_ADD + _x_)	// Frame Buffer
#define USE_FONT8X8
#define LCD_GET_PIXEL_ADD(_X_, _Y_)	(((_Y_ * PIXELS_PER_ROW) + _X_)*2)

// defines for the display geometry - OSD043TN24 480x272 TFT
// (some of these defines are also used by uMon's frame buffer interface)
#define PIXELS_PER_ROW			480		//
#define PIXELS_PER_COL			272		//
#define BITS_PER_PIXEL			16		//
#define PIXFMT_IS_RGB565		1
#define FBDEV_SETSTART			fbdev_setstart
#define FRAME_BUFFER_BASE_ADDR	LCD_BUF_ADD
#define LCD_H_WIDTH 			41		// pulse width in pixels
#define LCD_H_FRONT 			2		// front porch (sync to enable)
#define LCD_H_BACK 				2		// back porch (enable to sync)
#define LCD_V_WIDTH 			10		// pulse width in lines
#define LCD_V_FRONT 			2		// front porch (sync to enable)
#define LCD_V_BACK 				2		// back porch (enable to sync)
//#define LCD_PCD					2		// LCD PERCLK3 = 32Mhz/PCD +1 = Pixel Clock ~ 4Mhz

#define TOP						1
#define BOTTOM					(PIXELS_PER_COL-1)
#define LEFT					0
#define RIGHT					(PIXELS_PER_ROW-1)
#define CENTER_X				(PIXELS_PER_ROW/2)
#define CENTER_Y				(PIXELS_PER_COL/2)

#define ROWS_PER_SCREEN			17
#define COLS_PER_SCREEN			60

#define LCD_BG_DEF				9
#define LCD_FG_DEF				15

#define LCD_FB_SIZE(_depth_)				(((PIXELS_PER_COL * PIXELS_PER_ROW) * _depth_) / 8)

#define LCD_ROW_SIZE(_depth_)				((PIXELS_PER_ROW * _depth_) / 8)

#define LCD_GET_ADD(_row_, _col_, _depth_)	(((((_row_ * PIXELS_PER_ROW) * FONT_HEIGHT) \
											  + (_col_ * FONT_WIDTH)) \
											  * _depth_) / 8)


