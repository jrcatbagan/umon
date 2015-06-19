//==========================================================================
//
//      omap3530_lcd.c
//
// Author(s):   Luis Torrico - Cogent Computer Systems, Inc.
// Date:        12-10-2008
// Description:	Init Code for TI OMAP3530 LCD Controller
// NOTE 		Only 16-bit mode has been tested!
//
//==========================================================================

#include "config.h"
#include "cpuio.h"
#include "stddefs.h"
#include "genlib.h"
#include "omap3530.h"
#include "omap3530_lcd.h"
#include "cpu_gpio.h"
#include "vga_lookup.h"
#include "font8x16.h"
#include "fb_draw.h"
#include "warmstart.h"

#if INCLUDE_LCD

//--------------------------------------------------------------------------
// function prototypes and externs
//
void fbdev_init(void);

extern void udelay(int);
extern int GPIO_clr(int);
extern int GPIO_set(int);
extern int GPIO_tst(int);
extern int GPIO_out(int);
extern int GPIO_in(int);

// ADDED FOR WRITING CHARACTERS TO THE DISPLAY
void lcd_putchar(char c);
void lcd_writechar(uchar c);
void lcd_clr_row(int char_row);
void lcd_clr_scr(void);
void lcd_switch_buffer(void);

// globals to keep track of foreground, background colors and x,y position
int lcd_color_depth;		// 4, 8 or 16
int lcd_fg_color;			// 0 to 15, used as lookup into VGA color table
int lcd_bg_color;			// 0 to 15, used as lookup into VGA color table
int lcd_col;				// current column, 0 to COLS_PER_SCREEN - 1
int lcd_row;				// current row, 0 to (ROWS_PER_SCREEN * 2) - 1
int lcd_tst_mode = 0;
ulong lcd_fb_offset;	   	// current offset into frame buffer for lcd_putchar

//#define LCD_DBG
//--------------------------------------------------------------------------
// fbdev_init
//
// This function sets up the OMAP3530 LCD Controller, to be used
// as uMon's frame buffer device.
//
void
fbdev_init(void)
{
	//ushort temp16;

	if (StateOfMonitor != INITIALIZE)
		return;

	lcd_color_depth = 16;
	lcd_fg_color = vga_lookup[LCD_FG_DEF];
	lcd_bg_color = vga_lookup[LCD_BG_DEF];

	// Select DSS1_ALWON_FCLK (96MHz) as source for DSI and DISPC
    DSS_REG(DSS_CONTROL) = 0x30;

    // apply a soft reset to the display subsystem
    DISPC_REG(DISPC_SYSCONFIG) = 0x02;

	udelay(1000);
	
	// Set interface and functional clock to on during wakeup
	// and no standby or idle
    DISPC_REG(DISPC_SYSCONFIG) |= 0x2015;

	// Set up the DMA base address
    DISPC_REG(DISPC_GFX_BA) = 0x80200000;
 
	// Set up RGB 16 and disable the DMA for now
    DISPC_REG(DISPC_GFX_ATTR) = 0x0000000C;
 
	// Set preload based on equation in section 15.5.3.2 in RM 
    //DISPC_REG(DISPC_GFX_PRELOAD) = 0x60;
 
	// Set number of bytes to increment at end of row to 1 (default value) 
    DISPC_REG(DISPC_GFX_ROW_INC) = 0x0001;
 
	// Set number of bytes to increment between two pixels to 1 (default value)
    DISPC_REG(DISPC_GFX_PIX_INC) = 0x0001;
 
	// Set FIFO thresholds to defaults (hi = 1023, lo = 960)
    //DISPC_REG(DISPC_GFX_FIFO_TH) = 0x03FF03C0;
    DISPC_REG(DISPC_GFX_FIFO_TH) = 0x03FC03BC;
 
	// Set start position to 0 (frame buffer and active display area are the same)
    DISPC_REG(DISPC_GFX_POS) = 0x00000000;
 
	// Set frame buffer size, Y = PIXELS_PER_COL, X = PIXELS_PER_ROW
    DISPC_REG(DISPC_GFX_SIZE) = (((PIXELS_PER_COL -1) << 16) | (PIXELS_PER_ROW - 1));
 
	// Set the control register keep bit 5 (GOLCD) and bit 28 (LCDENABLESIGNAL) low
	// until shadow registers have all been written
    DISPC_REG(DISPC_CONTROL) = 0x38019209;
 
    // Disable all gating, pixel clock always toggles, frame data only
	// loaded every frame (palette/gamma table off)
    DISPC_REG(DISPC_CONFIG) = 0x00000004;
    //DISPC_REG(DISPC_CONFIG) = 0x00000000;
 
    // Disable all capabilities not used for LCD
    DISPC_REG(DISPC_CAPABLE) = 0x00000000;
 
	// Set horizontal timing
    DISPC_REG(DISPC_TIMING_H) = ((LCD_H_BACK << 20) | (LCD_H_FRONT << 8) | LCD_H_WIDTH);
 
	// Set vertical timing
    DISPC_REG(DISPC_TIMING_V) = ((LCD_V_BACK << 20) | (LCD_V_FRONT << 8) | LCD_V_WIDTH);
 
	// Set syncs low true and DE to hi true
    DISPC_REG(DISPC_POL_FREQ) = 0x00003000;
	
	// Set logic divisor to 1 and pixel divisor to 2
    DISPC_REG(DISPC_DIVISOR) = 0x00020001;
	
	// Set LCD size, lines per panel is , pixels per line is 
    DISPC_REG(DISPC_SIZE_LCD) = (((PIXELS_PER_COL -1) << 16) | (PIXELS_PER_ROW - 1));
	
	// Enable the DMA
    DISPC_REG(DISPC_GFX_ATTR) |= 0x00000001;
 
	// Set bit 5 (GOLCD) to enable LCD
    DISPC_REG(DISPC_CONTROL) |= 0x00000020;

	printf("OMAP3530 LCD Initialization Complete.\n");

	return;
}

/* fbdev_setstart():
 * Used by uMon's FBI interface to establish the starting address of
 * the frame buffer memory.
 */
void
fbdev_setstart(long offset)
{
	// Select DSS1_ALWON_FCLK (96MHz) as source for DSI and DISPC
    DSS_REG(DSS_CONTROL) = 0x30;

    // Set up the DMA base address
    DISPC_REG(DISPC_GFX_BA) = offset;

	// Enable the DMA
    DISPC_REG(DISPC_GFX_ATTR) |= 0x00000001;
 
	// Set bit 5 (GOLCD) to enable LCD
    DISPC_REG(DISPC_CONTROL) |= 0x00000020;

	return;
}

char *lcd_tstHelp[] = {
    "OMAP3530 LCD controller test",
    "-[n,x,d[4,8,16]]",
	"The user may set color depth to run the test at.",
	"The frame buffer R/W test will test all of the frame ",
	"buffer regardless of depth.",
    "Options...",
    "  -n    run test without keycheck - CAUTION: RESET SYSTEM TO STOP!",
	"  -d4   run test, force a depth of 4-bits/pixel",
	"  -d8   run test, force a depth of 8-bits/pixel",
	"  -d16  run test, force a depth of 16-bits/pixel",
	"  -x    init only, do not run frame buffer tests",
	"",
	"  No options, default to current mode and depth.",
	0
};

int lcd_tst(int argc,char *argv[])
{
	volatile ushort wr16, rd16;
	int i, x, opt;
	int no_wait = 0;
	int init_only = 0;
	char c;

	lcd_tst_mode = 1;

    while ((opt=getopt(argc,argv,"clnsxd:4,8,16")) != -1) {
        switch(opt) {
			case 'd':	// set the color depth
                switch(*optarg) {
            	    case '4':
	        	        lcd_color_depth = 4;
						printf("Forcing 4bpp Mode!\n");
        	            break;
    	            case '8':
            	        lcd_color_depth = 8;
						printf("Forcing 8bpp Mode!\n");
                    	break;
	                default:	// test with 16bpp
	                    lcd_color_depth = 16;
						printf("Forcing 16bpp Mode!\n");
						break;
                }
	            break;
	        case 'n':   // no waiting for keypress - fastest operation
				no_wait = 1;
				printf("No Keypress Mode, Must Reset System to Stop!\n");
				break;
	        case 'x':   // init only 
				no_wait = 1;
				printf("Initializing LCD, Skipping testsp!\n");
				init_only = 1;
				break;
			default:	// test with current mode
				break;
		}
	}

	// get the new parameters into the LCD controller
	fbdev_init();

	if (init_only) return 0;

	printf("Frame Buffer R/W...");
	// do an address=data read/write test on the frame buffer
	// PIXELS_PER_COL * PIXELS_PER_ROW is the highest pixel.  
	// Multiply by bits_per_pixel (sed_color_depth), then
	// divide by 8 to get the actual byte count.
	for (i = 0; i < LCD_FB_SIZE(lcd_color_depth) + LCD_ROW_SIZE(lcd_color_depth); i += 2){	
		LCD_BUF(i) = i & 0xffff;
		rd16 = LCD_BUF(i);
		if(rd16 != (i & 0xffff)){
			printf("Fail at 0x%08x, WR 0x%08x, RD 0x%04lx!\n",LCD_BUF_ADD + i, i, (ulong)rd16);
			return -1;
		}
	} 

	printf("OK!, Press key to continue.\n");

	c = getchar();

	printf("Frame Buffer Start: 0x%08x, End 0x%08x\n",LCD_BUF_ADD, 
			LCD_BUF_ADD + LCD_FB_SIZE(lcd_color_depth) + LCD_ROW_SIZE(lcd_color_depth));
	if (no_wait)
	{
		printf("Begin Full Screen Color Test.\n");
		while(1){
			// fill the frame buffer with incrementing color values
			for (x = 0; x < 16; x++){
				switch (lcd_color_depth){
					case 4:  wr16 = x | x << 4 | x << 8 | x << 12; break;
					case 8:  wr16 = x | x << 8; break;
					default: wr16 = vga_lookup[x]; break;	// 16-bits bypasses the lookup table
				}
				for (i = 0; i < LCD_FB_SIZE(lcd_color_depth); i += 2){	
					LCD_BUF(i) = wr16;
				}
			} // for x
		} // while
	} // no_wait
	else
	{
		printf("Begin Full Screen Color Test, Press any key to go to next color, \'x\' to end.\n");
		while(1){
			// fill the frame buffer with incrementing color values
			for (x = 0; x < 16; x++){
				switch (lcd_color_depth){
					case 4:  wr16 = x | x << 4 | x << 8 | x << 12; break;
					case 8:  wr16 = x | x << 8; break;
					default: wr16 = vga_lookup[x]; break;	// 16-bits bypasses the lookup table
				}
				for (i = 0; i < LCD_FB_SIZE(lcd_color_depth) + LCD_ROW_SIZE(lcd_color_depth); i += 2){	
					LCD_BUF(i) = wr16;
				}
				c = getchar();
				if (c == 'x') goto lcd_tst_next;
			} // for x
		} // while
	} // else no keycheck test

	lcd_tst_next:

	// write a one pixel border around the screen
	lcd_fg_color = 15;	// VGA Bright White
	fb_draw_line2(LEFT, TOP, RIGHT, TOP, lcd_fg_color);
	fb_draw_line2(LEFT, TOP, LEFT, BOTTOM, lcd_fg_color);
	fb_draw_line2(LEFT, BOTTOM, RIGHT, BOTTOM, lcd_fg_color);	// bottom left to bottom right
	fb_draw_line2(RIGHT, TOP, RIGHT, BOTTOM, lcd_fg_color);	// bottom right to top right
	// draw an x
	fb_draw_line2(LEFT, TOP, RIGHT, BOTTOM, lcd_fg_color);
	fb_draw_line2(LEFT, BOTTOM, RIGHT, TOP, lcd_fg_color);
	// draw 3 circles at the center of the screen, one inside the other
	lcd_fg_color = 12;	// VGA Bright Red
	fb_draw_circle2(CENTER_X, CENTER_Y, 100, lcd_fg_color);
	lcd_fg_color = 10;	// VGA Bright Green
	fb_draw_circle2(CENTER_X, CENTER_Y, 66, lcd_fg_color);
	lcd_fg_color = 9;	// VGA Bright Blue
	fb_draw_circle2(CENTER_X, CENTER_Y, 33, lcd_fg_color);

	return 0;
}

// fb_set_pixel sets a pixel to the specified color.
// This is target specific and is called from the generic
// fb_draw.c functions
void fb_set_pixel(ulong X, ulong Y, uchar color)
{
   	// Make sure the specified pixel is valid.
#if 0
   	if((X < 0) || (X >= PIXELS_PER_ROW) || (Y < 0) || (Y >= PIXELS_PER_COL))
   	{   
		printf("fb_set_pixel() bad X (%ld) or Y (%ld)!\n", X, Y);
		return;
   	}
#else
	if (X < 0)
		X = 0;
	else {
		if (X >= PIXELS_PER_ROW)
			X = PIXELS_PER_ROW - 1;
	}
	if (Y < 0)
		Y = 0;
	else {
		if (Y >= PIXELS_PER_COL)
			Y = PIXELS_PER_COL - 1;
	}
#endif

	LCD_BUF(LCD_GET_PIXEL_ADD(X, Y)) = vga_lookup[color];
}
#endif
