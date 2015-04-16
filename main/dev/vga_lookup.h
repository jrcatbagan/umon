//==========================================================================
//
// vga_lookup.h
//
// Author(s):    Michael Kelly, Cogent Computer Systems, Inc.
// Contributors: 
// Date:         03/06/2005
// Description:  This file contains vga color lookup

// 16-bit pixels are RGB 565 - LSB of RED and BLUE are tied low at the 
// LCD Interface, while the LSB of GREEN is loaded as 0
#define RED_SUBPIXEL(n)		(n & 0x1f) << 11
#define GREEN_SUBPIXEL(n)	(n & 0x1f) << 6
#define BLUE_SUBPIXEL(n)	(n & 0x1f) << 0

// define a simple VGA style 16-color pallette
#define	LU_BLACK		RED_SUBPIXEL(0x00) | GREEN_SUBPIXEL(0x00) | BLUE_SUBPIXEL(0x00)
#define	LU_BLUE			RED_SUBPIXEL(0x0f) | GREEN_SUBPIXEL(0x00) | BLUE_SUBPIXEL(0x00)
#define	LU_GREEN		RED_SUBPIXEL(0x00) | GREEN_SUBPIXEL(0x0f) | BLUE_SUBPIXEL(0x00)
#define	LU_CYAN			RED_SUBPIXEL(0x00) | GREEN_SUBPIXEL(0x0f) | BLUE_SUBPIXEL(0x0f)
#define	LU_RED			RED_SUBPIXEL(0x0f) | GREEN_SUBPIXEL(0x00) | BLUE_SUBPIXEL(0x00)
#define	LU_VIOLET		RED_SUBPIXEL(0x0f) | GREEN_SUBPIXEL(0x00) | BLUE_SUBPIXEL(0x0f)
#define	LU_YELLOW		RED_SUBPIXEL(0x0f) | GREEN_SUBPIXEL(0x0f) | BLUE_SUBPIXEL(0x00)
#define	LU_GREY			RED_SUBPIXEL(0x0f) | GREEN_SUBPIXEL(0x0f) | BLUE_SUBPIXEL(0x0f)
#define	LU_WHITE		RED_SUBPIXEL(0x17) | GREEN_SUBPIXEL(0x17) | BLUE_SUBPIXEL(0x17)
#define	LU_BRT_BLUE		RED_SUBPIXEL(0x00) | GREEN_SUBPIXEL(0x00) | BLUE_SUBPIXEL(0x1f)
#define	LU_BRT_GREEN	RED_SUBPIXEL(0x00) | GREEN_SUBPIXEL(0x1f) | BLUE_SUBPIXEL(0x00)
#define	LU_BRT_CYAN		RED_SUBPIXEL(0x00) | GREEN_SUBPIXEL(0x1f) | BLUE_SUBPIXEL(0x1f)
#define	LU_BRT_RED		RED_SUBPIXEL(0x1f) | GREEN_SUBPIXEL(0x00) | BLUE_SUBPIXEL(0x00)
#define	LU_BRT_VIOLET	RED_SUBPIXEL(0x1f) | GREEN_SUBPIXEL(0x00) | BLUE_SUBPIXEL(0x1f)
#define	LU_BRT_YELLOW	RED_SUBPIXEL(0x00) | GREEN_SUBPIXEL(0x1f) | BLUE_SUBPIXEL(0x1f)
#define	LU_BRT_WHITE	RED_SUBPIXEL(0x1f) | GREEN_SUBPIXEL(0x1f) | BLUE_SUBPIXEL(0x1f)

const static unsigned short vga_lookup[] = {
LU_BLACK,		
LU_BLUE,			
LU_GREEN,		
LU_CYAN,			
LU_RED,			
LU_VIOLET,		
LU_YELLOW,		
LU_GREY,			
LU_WHITE,		
LU_BRT_BLUE,		
LU_BRT_GREEN,	
LU_BRT_CYAN,		
LU_BRT_RED,		
LU_BRT_VIOLET,
LU_BRT_YELLOW,
LU_BRT_WHITE	
};
