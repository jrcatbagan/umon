/**************************************************************************
 *
 * Copyright (c) 2013 Alcatel-Lucent
 * 
 * Alcatel Lucent licenses this file to You under the Apache License,
 * Version 2.0 (the "License"); you may not use this file except in
 * compliance with the License.  A copy of the License is contained the
 * file LICENSE at the top level of this repository.
 * You may also obtain a copy of the License at:
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 **************************************************************************
 *
 * fbi.c:
 *
 * Frame Buffer Interface...
 * This file started out as a simple means to transfer a .bmp formatted
 * file onto a framebuffer.  It has since expanded into what I think is a
 * pretty complete set of frame-buffer utilities to run in both pixel mode
 * (for images) and console mode (for text).
 * As of this writing, pixel mode allows the user to display raw frame-buffer
 * images or 24-bit RGB .bmp formatted images.  In console mode, the font
 * rendering supports variable size fonts (using one font file) and console
 * scrolling using a dual-buffer algorithm.
 * 
 *****
 *
 * Configuration:
 * If you're hardware has a frame buffer interface (typically, this will be
 * an LCD, but doesn't have to be), then do the following:
 * 
 *	- provide the basic initialization code for the framebuffer
 *	  device: void fbdev_init(void), which will be called by uMon
 *	  startup code automatically.
 *
 *	- establish these three macros as per your device's specifications...
 *	  PIXELS_PER_COL	(height of framebuffer device)
 *	  PIXELS_PER_ROW	(height of framebuffer device)
 *	  FRAME_BUFFER_BASE_ADDR	(starting point of the frame-buffer memory)
 *
 *  - establish one (and only one) of the following to be 1, with all
 *	  others being 0:
 *	  PIXFMT_IS_RGB565
 *	  PIXFMT_IS_RGB555
 *
 *	  (as of this writing only 16-bit color depth RGB565/RGB555 has been
 *	   tested).
 *
 *	- set INCLUDE_FBI to 1 in the config.h file;
 *	- add font.c and fbi.c to the common files list in your makefile;
 *  - to support more efficient consolemode screen scrolling, optionally
 *	  set FBDEV_SETSTART to the name of a function in the driver that
 *	  when called, will re-establish the base address of the frame
 *	  buffer memory using the 'addr' value provided.
 *  - the console mode feature includes a blinking cursor.  To build without
 *    the cursor, add...
 *       #define FBI_NO_CURSOR
 *    to your config.h file.
 *
 *****
 *
 * Startup:
 * At startup, the target can come up in console mode or pixel mode
 * based on the presence of a splash image.  The function fbi_splash()
 * is called and it will look for a few different filenames (see function
 * below).  If a splash image is found, it is dumped to the frame buffer
 * and the system will start up in pixel mode.  If a splash image is not
 * found, then the system will come up in console mode, meaning that all
 * characters typed at the serial port will be printed out the frame buffer
 * device.
 *
 *****
 *
 * Acknowledgements:
 * I got the majority of my information regarding the format of the BMP
 * file from Wikipedia (http://en.wikipedia.org/wiki/BMP_file_format).
 * I got the basic idea for the scrolling algorithm out of code from
 * one of Cogent's LCD drivers.  Also, the file font.c has one font that
 * I tediously created by hand (kinda ugly), and one that I got from Cogent.
 *
 * Original author:     Ed Sutter (ed.sutter@alcatel-lucent.com)
 *
 */

#include "config.h"
#if INCLUDE_FBI
#include "cpuio.h"
#include "genlib.h"
#include "cli.h"
#include "tfs.h"
#include "tfsprivate.h"
#include "warmstart.h"
#include "font.h"
#include "fbi.h"
#include "timer.h"

/* FBI_DEBUG:
 * Primarily used for debugging the fb_scroll() algorithm...
 */
//#define FBI_DEBUG

#ifdef FBDEV_SETSTART
extern void FBDEV_SETSTART(long addr);
#endif

/* PIXELS_PER_ROW, PIXELS_PER_COL and LCD_BUF_ADD are assumed to be
 * defined in cpuio.h...
 */
#define SCREEN_WIDTH	PIXELS_PER_ROW
#define SCREEN_HEIGHT	PIXELS_PER_COL
#define FB_BASE		((unsigned long)FRAME_BUFFER_BASE_ADDR)

#define RED(val) 	(char)((val & 0x00ff0000) >> 16)
#define GREEN(val) 	(char)((val & 0x0000ff00) >> 8)
#define BLUE(val) 	(char)((val & 0x000000ff))

#define FILLTYPE_BOX	1
#define FILLTYPE_COLOR	2

#define BOX_XSIZE 10
#define BOX_YSIZE 10

#define DEFAULT_FG_COLOR 0x00f0f0f0
#define DEFAULT_BG_COLOR 0x00101010

#if PIXFMT_IS_RGB565
#define fbtype		unsigned short
#define FB_GRAY		0x632c
#define FB_WHITE	0xffff
#define FB_BLACK	0x0821
#elif PIXFMT_IS_RGB555
# warning: pixel format RGB555 not yet tested
#define fbtype		unsigned short
#define FB_GRAY		0x318c
#define FB_WHITE	0xffff
#define FB_BLACK	0x0421
#else
# error: unknown pixel format
#endif

#define FB_SIZE		(SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(fbtype))
#define FB_END		(FB_BASE + FB_SIZE)

struct bmpstuff {
	unsigned long	size;
	unsigned long	offset;
	unsigned long	hdrsz;
	unsigned long	width;
	unsigned long	height;
	unsigned short	planes;
	unsigned short	bpp;
	unsigned long	cprs;
	unsigned long	rawsize;
	unsigned long	hrez;
	unsigned long	vrez;
	unsigned long	palettesiz;
	unsigned long	impcol;
};

/* Whenever color is represented where it will be stored in the frame buffer
 * but also queriable by the user, we store the RGB value and the FBTYPE
 * value because once converted to the FBTYPE, it doesn't necessarily 
 * convert directly back to the original RGB value it was derived from.
 */
struct fbicolor {
	fbtype	fbval;
	long	rgbval;
};

void fbi_cursor_clear(void);
void fbi_consolemode_enable(int enable, int fbinit);

static	struct bmpstuff bmpinfo;
static	struct fbicolor font_fgcolor, font_bgcolor;
static	int font_totalcharheight, font_totalcharwidth, font_linemodulo;
static	int font_rowsperscreen, font_colsperscreen, font_scrolltot;
static	int font_style, font_xscale, font_yscale, font_xpos, font_ypos;
static	fbtype *fbi_cursorpos;
static	char fbi_linewrap, fbi_cursorstate;
static	char fbi_consolemode_enabled, font_initialized, font_is_opaque;

/* ecl() & ecs:
 * The .bmp file is inherently Windows/Intel-ish.  As a result, all integers
 * in that file are little endian.  These functions automatically convert
 * as needed...
 *
 * If this host is little endian, just return the value; else do the
 * endian swap and return.
 */
unsigned long
ecl(unsigned long val)
{
	short tval = 1;
	char *tp;

	tp = (char *)&tval;
	if (*tp == 1) {
		return(val);
	}
	else {
		return((( val & 0x000000ff) << 24) |
				((val & 0x0000ff00) << 8) |
				((val & 0x00ff0000) >> 8) |
				((val & 0xff000000) >> 24));
	}
}

unsigned short
ecs(unsigned short val)
{
	short tval = 1;
	char *tp;

	tp = (char *)&tval;
	if (*tp == 1) {
		return(val);
	}
	else {
		return(((val & 0x00ff) << 8) | ((val & 0xff00) >> 8));
	}
}

/* rgb_to_fb() / fb_to_rgb():
 * Convert 24-bit RGB to (or from) whatever the frame buffer type is...
 */

#if PIXFMT_IS_RGB565
/* Assume incoming RGB values 8-bits each (24-bit RGB).
 * Convert that to 16-bit RGB565 format (5-bit red, 6-bit grn, 5-bit blue).
 */
fbtype
rgb_to_fb(long rgb)
{
	char red, grn, blue;

	red = RED(rgb);
	grn = GREEN(rgb);
	blue = BLUE(rgb);

	return(((short)(red >> 3) << 11) |
		   ((short)(grn >> 2) << 5) | (short)(blue >> 3)); 
}

#elif PIXFMT_IS_RGB555

/* Assume incoming RGB values 8-bits each (24-bit RGB).
 * Convert that to 16-bit RGB555 format (5-bit red, 5-bit grn, 5-bit blue).
 * (NOT READY YET)
 */
fbtype
rgb_to_fb(long rgb)
{
	unsigned long red, green, blue;

	red =   ((fbval & 0xf800) >> 11);
	green = ((fbval & 0x07e0) >> 5);
	blue =   (fbval & 0x001f);

	return(((short)(red >> 3) << 10) |
		   ((short)(grn >> 5) << 5) | (short)(blue >> 3)); 
}

#endif

/* fb_memset():
 * A version of memset that is written to efficiently populate the frame
 * buffer with a value of type fbtype.  The incoming total is assumed to
 * be in fbtype units.
 */
void
fb_memset(fbtype *dest, fbtype val, int tot)
{
	/* If the size of a framebuffer element is half the size of a long,
	 * then we can double up the memory fill by populating one long value
	 * with two framebuffer fill values (one being shifted left 16)...
	 */
	if (sizeof(fbtype) == sizeof(unsigned long)/2) {
		unsigned long l_val, *l_fp, *l_end;

		l_val = (unsigned long)val;
		l_val <<= 16;
		l_val |= (unsigned long)val;
		l_fp = (unsigned long *)dest;
		l_end = (unsigned long *)(dest+tot);
		while(l_fp < l_end)
			*l_fp++ = l_val;
		
	}
	/* Otherwise, we just do an fbtype-oriented memset...
	 */
	else {
		fbtype *end = dest+tot;

		while(dest < end)
			*dest++ = val;
	}
}

void
fb_memcpy(fbtype *dest, fbtype *src, int tot)
{
	/* If the size of a framebuffer element is half the size of a long,
	 * then we can double up the memory fill by populating one long value
	 * with two framebuffer fill values (one being shifted left 16)...
	 */
	if (sizeof(fbtype) == sizeof(unsigned long)/2) {
		unsigned long *l_dest, *l_src, *l_end;

		l_dest = (unsigned long *)dest;
		l_src = (unsigned long *)src;
		l_end = (unsigned long *)(dest+tot);
		while(l_dest < l_end)
			*l_dest++ = *l_src++;
		
	}
	/* Otherwise, we just do an fbtype-oriented memcpy...
	 */
	else {
		fbtype *end = dest+tot;

		while(dest < end)
			*dest++ = *src++;
	}
}

void
font_defaults(void)
{
	struct font *fontp;

	if (font_initialized)
		return;

	font_xpos = 0;
	font_ypos = 0;
	font_style = 1;
	font_xscale = 1;
	font_yscale = 1;
	font_scrolltot = 0;
	font_fgcolor.rgbval = DEFAULT_FG_COLOR;
	font_fgcolor.fbval = rgb_to_fb(font_fgcolor.rgbval);
	font_bgcolor.rgbval = DEFAULT_BG_COLOR;
	font_bgcolor.fbval = rgb_to_fb(font_bgcolor.rgbval);
	font_is_opaque = 1;
	fbi_linewrap = 0;
	fbi_cursorstate = 0;
	fbi_cursorpos = 0;

	fontp = &font_styles[font_style];
	font_totalcharheight = ((fontp->height + fontp->above + fontp->below) *
		font_yscale);
	font_totalcharwidth = ((fontp->width + fontp->between) * font_xscale);
	font_rowsperscreen = SCREEN_HEIGHT / font_totalcharheight;
	font_colsperscreen = SCREEN_WIDTH / font_totalcharwidth;
	font_linemodulo = SCREEN_HEIGHT % font_totalcharheight;
	font_initialized = 1;
}


/* fb_scroll():
 * This could be done with a simple memory-to-memory transfer which copies
 * row 2 to row 1, row 3 to row 2, etc... all the way down the screen.
 * However, this would be *really* slow, so an alternative solution is
 * used that takes advantage of the fact that we can adjust which portion
 * of memory is looked at by the DMA engine connected to the device that is
 * actually displaying the pixels (typically this will be an LCD).
 *
 * Consider the double buffer here, with the start of the second buffer
 * being adjacent to the end of the first buffer...
 *
 *             --------------------------- 
 *             |                        -|        *
 *             |                         |        *
 *             |                         |        *
 *             |                         |        *
 *          ___|                        -|___     *
 *             |                         |
 *             |                         |
 *             |                         |
 *             |                         |
 *             |                         |
 *             ---------------------------
 *
 * For the sake of this discussion, assume each buffer deals with five
 * lines of text.  Also, initially the visible portion of the frame 
 * buffer aligns with the top buffer (note the asterisks to the right).
 * As text is typed, each character is put into both buffers...
 * 
 *             --------------------------- 
 *             | text for line 1        -|        *
 *             | another line here       |        *
 *             | another line is here    |        *
 *             | this is the fourth line |        *
 *          ___| and this is line five  -|___     *
 *             | text for line 1         |
 *             | another line here       |
 *             | another line is here    |
 *             | this is the fourth line |
 *             | and this is line five   |
 *             ---------------------------
 *
 * Up to this point, the viewing window has not changed. 
 * The next line of text is simply placed at the top of both of the
 * buffers, just like a circular queue...
 * 
 *             --------------------------- 
 *             | this is now line #6     |      
 *             | another line here      -|        *
 *             | another line is here    |        *
 *             | this is the fourth line |        *
 *          ___| and this is line five   |___     *
 *             | this is now line #6    -|        *
 *             | another line here       |
 *             | another line is here    |
 *             | this is the fourth line |
 *             | and this is line five   |
 *             ---------------------------
 *
 * The only added trick now is that the viewing window is shifted down
 * one row; thus, giving the appearance of a single line scroll, but without
 * the memcpy overhead.
 * Then, continuing with this, when the viewing window gets to the bottom
 * of the second buffer as shown below...
 * 
 *             --------------------------- 
 *             | this is now line #6     |      
 *             | line seven              |
 *             | this would be line 8    |
 *             | and nine...             |
 *          ___| this is line ten        |___
 *             | this is now line #6    -|        *
 *             | line seven              |        *
 *             | this would be line 8    |        *
 *             | and nine...             |        *
 *             | this is line ten       -|        *
 *             ---------------------------
 *
 * the next "scroll" would simply put to one line from the top...
 * 
 *             --------------------------- 
 *             | eleventh line now       |
 *             | line seven             -|       *
 *             | this would be line 8    |       *
 *             | and nine...             |       *
 *          ___| this is line ten        |___    *
 *             | eleventh line now      -|       *
 *             | line seven              |
 *             | this would be line 8    |
 *             | and nine...             |
 *             | this is line ten        |
 *             ---------------------------
 *
 * One 'gotcha' that I discovered (and fixed) with this algorithm is that
 * it only works perfectly if the character height of the font divides
 * evenly into the height of the visible frame.  If this is not the case,
 * then as the lines are scrolling, there will be a larger gap between
 * one line and the next as each 'N' lines scroll by (where 'N' is the nuber
 * of lines that can fit on the screen.
 * To get around this problem, when the font is first established I determine
 * what the modulo is between the character height and the screen height.
 * Then for each character-oriented line generated I insert one extra pixel
 * oriented line until the modulo reaches zero.  This simply distributes the
 * extra space across each of the lines and only one pixel (at most) of 
 * difference is visible.
 * In short, the font_linemodulo value is the number of character-lines that
 * need to have one additional pixel added to their height so that the final
 * character-line is at the very bottom of the screen.
 */

void
fb_erase_line(int lno)
{
	int linesize, linesize1;
	fbtype	*dest, *end, *dest1;
#ifdef FBI_DEBUG
	int erasetype;
#endif

	linesize = SCREEN_WIDTH * font_totalcharheight;

	if (font_linemodulo == 0) {
		dest = (fbtype *)(FB_BASE + (lno * linesize * sizeof(fbtype)));
		end = dest + linesize;
#ifdef FBI_DEBUG
		erasetype = 0;
#endif
	}
	else {
		linesize1 = linesize + SCREEN_WIDTH;

		if (lno < font_linemodulo) {
			dest = (fbtype *)(FB_BASE + (lno * linesize1 * sizeof(fbtype)));
			end = dest + linesize1;
#ifdef FBI_DEBUG
			erasetype = 1;
#endif
		}
		else if (lno == font_linemodulo) {
			dest = (fbtype *)(FB_BASE + (lno * linesize1 * sizeof(fbtype)));
			end = dest + linesize1;
			end = dest + linesize;
#ifdef FBI_DEBUG
			erasetype = 2;
#endif
		}
		else {
			dest = (fbtype *)(FB_BASE + (font_linemodulo * linesize1 * sizeof(fbtype)));
			dest += ((lno-font_linemodulo) * linesize);
			end = dest + linesize;
#ifdef FBI_DEBUG
			erasetype = 3;
#endif
		}
	}

	dest1 = dest + FB_SIZE/sizeof(fbtype);
	fb_memset(dest,font_bgcolor.fbval,end-dest);
#ifdef FBDEV_SETSTART
	fb_memset(dest1,font_bgcolor.fbval,end-dest);
#endif

#ifdef FBI_DEBUG
	if (!fbi_consolemode_enabled)	/* don't print if console is enabled */
		printf("fb_erase_line_%d(%d)\n",erasetype,lno);
#endif

}

#ifdef FBDEV_SETSTART

/* fb_scroll() 
 * Called each time a 'newline' character is put to the screen.
 */
void
fb_scroll(void)
{
	int 	adjust, next_ypos;
	unsigned long fbstart;

	font_scrolltot++;

	if (font_ypos >= (font_rowsperscreen-1))
		next_ypos = 0;
	else
		next_ypos = font_ypos+1;

	if (font_scrolltot >= (font_rowsperscreen*2)) {
		fb_erase_line(next_ypos);

		fbstart = (FB_BASE +
			(SCREEN_WIDTH*font_totalcharheight * sizeof(fbtype)));

		if (font_linemodulo)
			fbstart += SCREEN_WIDTH * sizeof(fbtype);

		FBDEV_SETSTART(fbstart);

#ifdef FBI_DEBUG
		if (!fbi_consolemode_enabled) {	/* don't print if console is enabled */
			printf("fb_scroll_1: %d %d %d (fbstart=0x%lx)\n",
				font_ypos,font_scrolltot,font_rowsperscreen,fbstart);
		}
#endif
		font_scrolltot = font_rowsperscreen;
	}
	else if (font_scrolltot >= font_rowsperscreen) {
		int ldiff = font_scrolltot - font_rowsperscreen;

		fb_erase_line(ldiff);

		adjust =
			(SCREEN_WIDTH * font_totalcharheight * sizeof(fbtype));

		fbstart = (FB_BASE + ((ldiff+1) * adjust));

		if (font_linemodulo) {
			if (ldiff <= font_linemodulo)
				fbstart += ldiff * (SCREEN_WIDTH * sizeof(fbtype));
			else
				fbstart += font_linemodulo * (SCREEN_WIDTH * sizeof(fbtype));
		}

		FBDEV_SETSTART(fbstart);

#ifdef FBI_DEBUG
		if (!fbi_consolemode_enabled) {	/* don't print if console is enabled */
			printf("fb_scroll_2: %d %d %d %d (fbstart=0x%lx)\n",
				font_ypos,font_scrolltot,font_rowsperscreen,adjust,fbstart);
		}
#endif
	}

	font_ypos = next_ypos;
}

#else

void
fb_scroll(void)
{
	int i, tot;
	struct font *fp;
	fbtype *dest, *src;

	if (font_ypos >= (font_rowsperscreen-1)) {
		/* Starting at the point in the frame buffer where the second line
		 * starts, copy that to the top of the frame buffer.  Effectively 
		 * moving every line below the first line up by one.
		 * Then clear the newly created bottom line.
		 *
		 * NOTE: this is the *slow* way.  Ideally, the hardware has the ability
		 * to support the fbdev_setstart() functionality so the double-buffer
		 * algorithm (described above) can be used.
		 */
		fp = &font_styles[font_style];
		dest = (fbtype *)FB_BASE;
		src = (fbtype *)FB_BASE + (SCREEN_WIDTH * font_totalcharheight);
	
		tot = ((FB_END - FB_BASE)/2) - (src - dest);
	
		for(i=0;i<tot;i++)
			*dest++ = *src++;

		/* Clear the new bottom line...
		 */
		while(dest < (fbtype *)FB_END)
			*dest++ = font_bgcolor.fbval;
	}
	else
		font_ypos++;
}

#endif

/* fb_putchar():
 * Using the current font settings and frame buffer position, set the
 * pixels that correspond to the incoming character 'inchar'.
 * This function supports the double-buffer line scroll technique discussed
 * above; hence, the use of fp1 & fp2 below.
 */
static void
fb_putchar(char inchar)
{
	struct font *fp;
	int		i, j, k;
	char	*bmppos;
	unsigned char mask, endmask, fontchar;
	fbtype *fp1, *fp2, *position, *start, *end;

	if (!font_initialized)
		font_defaults();

	fp = &font_styles[font_style];

#ifndef FBI_NO_CURSOR
	fbi_cursor_clear();
#endif

	if (inchar == '\r') {
		font_xpos = 0;
		return;
	}
	else if (inchar == '\n') {
		fb_scroll();
		return;
	}
	else if (inchar == '\b') {
		if (font_xpos > 0) {
			font_xpos--;
			fb_putchar(' ');
			font_xpos--;
		}
		return;
	}

	/* Chop at end of screen...
	 */
	if (font_xpos >= font_colsperscreen) {
		if (fbi_linewrap) {
			font_xpos = 0;
			fb_scroll();
		}
		else
			return;
	}

	position = start = (fbtype *)FB_BASE +
		(font_ypos * font_totalcharheight * SCREEN_WIDTH) +
		(font_xpos * font_totalcharwidth);

	if (font_ypos < font_linemodulo) {
		position += font_ypos*SCREEN_WIDTH;
	}
	else {
		position += font_linemodulo*SCREEN_WIDTH;
	}

	bmppos = fp->bitmap + ((inchar - 0x20) * fp->height);
	for(i=0;i<fp->above * font_yscale;i++) {
		fp1 = (fbtype *)position;
		fp2 = fp1 + FB_SIZE/2;
		end = fp1 + (fp->width * font_xscale) + (fp->between * font_xscale);
		while(fp1 < end) {
			if (font_is_opaque) {
				*fp1++ = font_bgcolor.fbval;
				*fp2++ = font_bgcolor.fbval;
			}
			else {
				fp1++;
				fp2++;
			}
		}
		position += SCREEN_WIDTH;
	}

	/* Some fonts will only use the left-most 7 columns of pixels, so
	 * we support that here...
	 */
	if (fp->width == 7)
		endmask = 1;
	else
		endmask = 0;

	for(i=0;i<fp->height;i++) {
		fontchar = *bmppos++;

		for(k=0;k<font_yscale;k++) {
			fp1 = (fbtype *)position;
			fp2 = fp1 + FB_SIZE/2;
			for(mask = 0x80; mask != endmask; mask >>= 1) {
				if (fontchar & mask) {
					for(j=0;j<font_xscale;j++) {
						*fp1++ = font_fgcolor.fbval;
						*fp2++ = font_fgcolor.fbval;
					}
				}
				else if (font_is_opaque) {
					for(j=0;j<font_xscale;j++) {
						*fp1++ = font_bgcolor.fbval;
						*fp2++ = font_bgcolor.fbval;
					}
				}
				else {
					for(j=0;j<font_xscale;j++) {
						fp1++;
						fp2++;
					}
				}
			}
	
			end = fp1 + (fp->between * font_xscale);
			while(fp1 < end) {
				if (font_is_opaque) {
					*fp1++ = font_bgcolor.fbval;
					*fp2++ = font_bgcolor.fbval;
				}
				else {
					fp1++;
					fp2++;
				}
			}
			position += SCREEN_WIDTH;
		}
	}

	for(i=0;i<fp->below*font_yscale;i++) {
		fp1 = position;
		fp2 = fp1 + FB_SIZE/2;
		end = fp1 + (fp->width * font_xscale) + (fp->between * font_xscale);
		while(fp1 < end) {
			if (font_is_opaque) {
				*fp1++ = font_bgcolor.fbval;
				*fp2++ = font_bgcolor.fbval;
			}
			else {
				fp1++;
				fp2++;
			}
		}
		position += SCREEN_WIDTH;
	}

	font_xpos++;
}

static int
fbi_puts(char *str)
{
	int tot, slash;
	char nextchar;

	tot = slash = 0;
	while(*str) {
		if (slash) {
			switch(*str) {
				case 'n':
					nextchar = '\n';
					break;
				case 'r':
					nextchar = '\r';
					break;
				case 'b':
					nextchar = '\b';
					break;
				default:
					tot++;
					fb_putchar('\\');
					nextchar = *str;
					break;
			}
			str++;
		}
		else {
			if (*str  == '\\') {
				slash = 1;
				str++;
				continue;
			}
			nextchar = *str++;
		}
		tot++;
		fb_putchar(nextchar);
		slash = 0;
	}

	return(tot);
}

/* get_bmphdr_long():
 * Given an offset, copy 4 bytes of data to a temporary long
 * value, then endian-swap if necessary.
 */
unsigned long
get_bmphdr_long(char *offset, char *msg, int verbose)
{
	unsigned long tmp1, tmp2;

	memcpy((char *)&tmp1,(char *)offset,4);
	tmp2 = ecl(tmp1);

	if (verbose)
		printf(" %-20s: %ld\n",msg,tmp2);

	return(tmp2);
}

/* get_bmphdr_short():
 * Given an offset, copy 2 bytes of data to a temporary short
 * value, then endian-swap if necessary.
 */
unsigned short
get_bmphdr_short(char *offset, char *msg, int verbose)
{
	unsigned short tmp1, tmp2;

	memcpy((char *)&tmp1,(char *)offset,2);
	tmp2 = ecs(tmp1);

	if (verbose)
		printf(" %-20s: %d\n",msg,tmp2);

	return(tmp2);
}

/* get_bmpinfo():
 * Using the incoming base address as the pointer to the incoming data,
 * return 1 if the data is not BMP formatted
 * return 0 if the data is BMP formatted
 * return -1 if the data is BMP formatted but something is not compatible
 *		with this tool
 */
int
get_bmpinfo(char *base, int verbose)
{
	if ((base[0] != 'B') || (base[1] != 'M')) {
		return(1);
	}

	/* Retrieve the .bmp file's header info...
	 */
	bmpinfo.size = get_bmphdr_long(base+2,"Size",verbose);
	bmpinfo.offset = get_bmphdr_long(base+10,"Offset",verbose);
	bmpinfo.hdrsz = get_bmphdr_long(base+14,"Hdrsize",verbose);
	bmpinfo.width = get_bmphdr_long(base+18,"Width",verbose);
	bmpinfo.height = get_bmphdr_long(base+22,"Height",verbose);
	bmpinfo.planes = get_bmphdr_short(base+26,"Planes",verbose);
	bmpinfo.bpp = get_bmphdr_short(base+28,"Bpp",verbose);
	bmpinfo.cprs = get_bmphdr_long(base+30,"Cprs",verbose);
	bmpinfo.rawsize = get_bmphdr_long(base+34,"RawSize",verbose);
	bmpinfo.hrez = get_bmphdr_long(base+38,"Hrez",verbose);
	bmpinfo.vrez = get_bmphdr_long(base+42,"Vrez",verbose);
	bmpinfo.palettesiz = get_bmphdr_long(base+46,"Palettesiz",verbose);
	bmpinfo.impcol = get_bmphdr_long(base+50,"Important colors",verbose);

	if (bmpinfo.cprs != 0) {
		printf("Can't deal with non-zero compression method.\n");
		return(-1);
	}
	if (bmpinfo.bpp != 24) {
		printf("BMP file pixel format is not 24 bits per pixel.\n");
		return(-1);
	}
	return(0);
}

void
fbi_consolemode_enable(int enable, int fbinit)
{
	font_scrolltot = 0;
	font_ypos = font_xpos = 0;

	if (enable) {
		if (!font_initialized)
			font_defaults();
	}

	if (fbinit) {
		/* Regardless of whether we are enabling or disableing console
		 * mode we need to init the frame buffer.  We fill it with with the
		 * same color that is used by the font's background.
		 *
		 * If FBDEV_SETSTART is defined, then we assume that we're
		 * using the double-buffer technique (described above) for
		 * scrolling; hence, we need to initialize twice the space...
		 */
#ifdef FBDEV_SETSTART
		fb_memset((fbtype *)FB_BASE,font_bgcolor.fbval,
			((FB_END-FB_BASE)/sizeof(fbtype))*2);

		FBDEV_SETSTART(FB_BASE);
#else
		fb_memset((fbtype *)FB_BASE,font_bgcolor.fbval,
			(FB_END-FB_BASE)/sizeof(fbtype));
#endif
	}

	fbi_consolemode_enabled = (char)enable;
}

int
is_fbi_consolemode_enabled(void)
{
	if (fbi_consolemode_enabled)
		return(1);
	return(0);
}

void
fillbox(int x_offset,int y_offset,int type,fbtype *dest_base,fbtype *src_base)
{
	int x, y, offset;
	fbtype *src, *dest;

	offset = (y_offset * BOX_YSIZE * SCREEN_WIDTH) + (x_offset * BOX_XSIZE);
	dest = dest_base + offset;

	if (type & FILLTYPE_COLOR) {
		for(y=0;y<BOX_YSIZE;y++) {
			for(x=0;x<BOX_XSIZE;x++)
				dest[x] = *src_base;
	
			dest += SCREEN_WIDTH;
		}
	}
	else {
		src = src_base + offset;

		for(y=0;y<BOX_YSIZE;y++) {
			for(x=0;x<BOX_XSIZE;x++)
				dest[x] = src[x];
	
			dest += SCREEN_WIDTH;
			src += SCREEN_WIDTH;
		}
	}
}

/* copypixel():
 * Copy the frame buffer data that corresponds to the specified x/y
 * coordinate from one frame buffer block to another...
 * Note, if filltype is FILLTYPE_COLOR, then we assume that 'frm'
 * is pointing to a fixed color.
 */
void
copypixel(fbtype *to, fbtype *frm, int filltype, int x, int y)
{
	fbtype *dst, *src;

	dst = to + ((y * PIXELS_PER_ROW) + x);

	if (filltype & FILLTYPE_COLOR)
		src = frm;
	else
		src = frm + ((y * PIXELS_PER_ROW) + x);

	*dst = *src;
}

/* fillscreen():
 * Fill the screen from top to bottom in four scans, skipping around between
 * each scan just to improve the effect.  If 'box' is set, then the fill
 * area is a 10x10 box; otherwise its a pixel.
 */
void
fillscreen(fbtype *to,fbtype *frm,int filltype, int delay)
{
	int x, y;
	int xInit, yInit, xEnd, yEnd;

	yInit = xInit = 0;
	xEnd = SCREEN_WIDTH;
	yEnd = SCREEN_HEIGHT;
	if (filltype & FILLTYPE_BOX) {
		xEnd = xEnd/BOX_XSIZE;
		yEnd = yEnd/BOX_YSIZE;
	}

repeat:
	for(y=yInit;y<yEnd;y+=2) {
		for(x=xInit;x<xEnd;x+=2) {
			if (filltype & FILLTYPE_BOX) {
				fillbox(x,y,filltype,to,frm);
				if (delay)
					monDelay(delay);
			}
			else if (filltype & FILLTYPE_COLOR) {
				*(to + (y*SCREEN_WIDTH)+x) = *frm;
			}
			else {
				*(to + (y*SCREEN_WIDTH)+x) = *(frm + (y*SCREEN_WIDTH)+x);
			}
		}
		if (!(filltype & FILLTYPE_BOX)) {
			if (delay)
				monDelay(delay);
		}
	}
	if ((xInit == 0) && (yInit == 0)) {
		xInit = 1; yInit = 1;
		goto repeat;
	}
	if ((xInit == 1) && (yInit == 1)) {
		xInit = 0; yInit = 1;
		goto repeat;
	}
	if ((xInit == 0) && (yInit == 1)) {
		xInit = 1; yInit = 0;
		goto repeat;
	}
	/* If FILTYPE_BOX is set and the boxsize is not evenly
	 * divided into the actual screen size, then we need to
	 * fill in the edges (right & bottom).
	 */
	if (filltype & FILLTYPE_BOX) {
		int xmod, ymod;

		xmod = SCREEN_WIDTH % BOX_XSIZE;
		ymod = SCREEN_HEIGHT % BOX_YSIZE;
		if (xmod) {
			for(y=0;y<SCREEN_HEIGHT;y++) {
				for(x=SCREEN_WIDTH-xmod;x<SCREEN_WIDTH;x++)
					copypixel(to,frm,filltype,x,y);
			}
		}
		if (ymod) {
			for(y=SCREEN_HEIGHT-ymod;y<SCREEN_HEIGHT;y++) {
				for(x=0;x<SCREEN_WIDTH;x++)
					copypixel(to,frm,filltype,x,y);
			}
		}
	}
}

void
fill_centerout(fbtype *to,fbtype *frm,int filltype, int delay)
{
	int i, j, x, y, row, col, maxcol, maxrow;

	x = SCREEN_WIDTH/2;
	y = SCREEN_HEIGHT/2;
	copypixel(to,frm,filltype,x,y);
	for(i=0;x>=0 && y>=0;x--,y--,i+=2) {
		for(j=0,row=y,col=x;j<i;j++,col++)
			copypixel(to,frm,filltype,col,row);
		maxcol = col;
		maxrow = row;
		for(j=0,row=y;j<i;j++,row++)
			copypixel(to,frm,filltype,col,row);
		for(j=0;j<i;j++,col--)
			copypixel(to,frm,filltype,col,row);
		for(j=0;j<i;j++,row--)
			copypixel(to,frm,filltype,col,row);
	}
	if (x == -1) {
		while(y >= 0) {
			for(x=0;x<SCREEN_WIDTH;x++) {
				copypixel(to,frm,filltype,x,y);
				copypixel(to,frm,filltype,x,maxrow);
			}
			y--; maxrow++;
		}
	}
	else if (y == -1) {
		while(x >= 0) {
			for(y=0;y<SCREEN_HEIGHT;y++) {
				copypixel(to,frm,filltype,x,y);
				copypixel(to,frm,filltype,maxcol,y);
			}
			x--; maxcol++;
		}
	}
}

/* fb_setcolor():
 * Fill the frame buffer with the incoming RGB-24 formatted
 * color value...
 */
void
fb_setcolor(long rgb, int transition_type)
{
	fbtype fillval, *to, *from;
	
	to = (fbtype *)FB_BASE;
	from = &fillval;
	fillval = rgb_to_fb(rgb);

	switch(transition_type) {
		case 1:		/* Pixel fill */
			fillscreen(to,from,FILLTYPE_COLOR,0);
			break;
		case 2:		/* Box fill */
			fillscreen(to,from,FILLTYPE_BOX | FILLTYPE_COLOR,0);
			break;
		case 3:		/* Center-out fill */
			fill_centerout(to,from,FILLTYPE_COLOR,0);
			break;
		default:	/* Standard top-to-bottom fill */
			fb_memset(to,fillval,(FB_END-FB_BASE)/sizeof(fbtype));
			break;
	}
}

char *FbiHelp[] = {
	"Frame buffer interface",
	"-[d:f:o:t:v] {cmd} [args...]",
#if INCLUDE_VERBOSEHELP
	"Options:",
	" -d ##        delay the transition speed",
	" -f RGB       enable (and set) frame-buffer fill value",
	" -o X,Y       offset into frame buffer",
	" -t ##        apply transition when updating screen",
	"              (doesn't work with .bmp formated source data)",
	" -v           additive verbosity",
	"",
	"Commands:",
	" color {rgb-value}              # fill screen with specified color",
	" consolemode {on|wrap|off}      # enable/disable console mode",
	" fb2file {filename}             # copy frame-buffer to TFS file",
	" fill {filename | hex_address}  # fill screen with file or data",
	" font {style} {xscale} {yscale} {fgcolor} {bgcolor}",
	" print {text}                   # print to screen",
	" setpixel {x} {y} {size} {color}",
#ifdef FBDEV_SETSTART
	" setstart {hex address}         # establish base address of frame buffer",
#endif
#endif
	0,
};

int
FbiCmd(int argc,char *argv[])
{
	TFILE	*tfp;
	unsigned char *bmp;
	unsigned long fill_rgb;
	fbtype	*fp, fill_fbvalue;
	char	*cmd, *arg1, fill_is_set, *base, *comma;
	int		x, y, widthadjust, transition_type;
	int		data_is_raw, transition_delay, rowsize, verbose, opt;

	if (!font_initialized)
		font_defaults();

	verbose = 0;
	fill_rgb = 0;
	fill_fbvalue = 0;
	transition_delay = transition_type = fill_is_set = 0;
	while((opt=getopt(argc,argv,"d:f:o:t:v")) != -1) {
		switch(opt) {
		case 'd':
			transition_delay = (int)strtol(optarg,0,0);
			break;
		case 'f':
			fill_is_set = 1;
			fill_rgb = strtoul(optarg,0,0);
			break;
		case 'o':
			comma = strchr(optarg,',');
			if (!comma)
				return(CMD_PARAM_ERROR);
			font_xpos = strtol(optarg,0,0);
			font_ypos = strtol(comma+1,0,0);
			if ((font_xpos > SCREEN_WIDTH) || (font_ypos > SCREEN_HEIGHT)) {
				printf("Offset out of range\n");
				return(CMD_FAILURE);
			}
			break;
		case 't':
			transition_type = atoi(optarg);
			break;
		case 'v':
			verbose++;
			break;
		default:
			return(CMD_PARAM_ERROR);
		}
	}

	if (argc == optind) {
		printf("Frame buffer info...\n");
		printf("  Base addr:  0x%lx\n",FB_BASE);
		printf("  LCD width x height (pixels):  %dx%d\n",
			SCREEN_WIDTH,SCREEN_HEIGHT);
#if PIXFMT_IS_RGB565
		printf("  Pixel fmt:  RGB565 (16 bits-per-pixel)\n");
#elif PIXFMT_IS_RGB555
		printf("  Pixel fmt:  RGB555 (16 bits-per-pixel)\n");
#endif
		printf("Font (style=%d)...\n",font_style);
		printf(" Scale: x=%d, y=%d\n", font_xscale,font_yscale);
		printf(" Color: fg=0x%06x, bg=0x%06x\n",
			font_fgcolor.rgbval, font_bgcolor.rgbval);
		printf(" Total per-char font height: %d, width: %d\n",
			font_totalcharheight,font_totalcharwidth);
		printf(" Total rows: %d (mod %d), cols: %d\n",
			font_rowsperscreen, font_linemodulo, font_colsperscreen);
		printf(" Current position: x=%d, y=%d\n",font_xpos,font_ypos);

		shell_sprintf("FBIBASE","0x%x",FB_BASE);
		shell_sprintf("FBIROWS","%d",font_rowsperscreen);
		shell_sprintf("FBICOLS","%d",font_colsperscreen);
		return(CMD_SUCCESS);
	}
	if (argc < optind + 1)
		return(CMD_FAILURE);

	cmd = argv[optind];
	arg1 = argv[optind+1];

	if (strcmp(cmd,"fill") == 0) {
		if (argc != (optind+2))
			return(CMD_PARAM_ERROR);

		if ((arg1[0] == '0') && (arg1[1] == 'x')) {
			base = (char *)strtoul(arg1,0,0);
		}
		else {
			if ((tfp = tfsstat(arg1)) == (TFILE *)0) {
				printf("File '%s' not found\n",arg1);
				return(1);
			}
			base = TFS_BASE(tfp);
		}

		if ((data_is_raw = get_bmpinfo(base,verbose)) == -1)
			return(CMD_FAILURE);

		if (fill_is_set) {
			fill_fbvalue = rgb_to_fb(fill_rgb);
		}

		/* If the input data is raw, then just copy it directly to the frame
		 * buffer memory space and be done.
		 * Note, the transition_type set up with the -t option only works when
		 * we're pushing in raw data...
		 */
		if (data_is_raw) {
			fbtype *to = (fbtype *)FB_BASE;
			fbtype *from = (fbtype *)base;

			switch(transition_type) {
				case 1:		/* Pixel fill */
					fillscreen(to,from,0,transition_delay);
					break;
				case 2:		/* Box fill */
					fillscreen(to,from,FILLTYPE_BOX,transition_delay);
					break;
				case 3:		/* Center-out fill */
					fill_centerout(to,from,0,transition_delay);
					break;
				default:	/* Standard top-to-bottom fill */
					if (transition_delay) {
						while(to < (fbtype *)FB_END) {
							*to++ = *from++;
							if (((int)to & 0x1ff) == 0x1ff)
								monDelay(transition_delay);
						}
					}
					else {
						fb_memcpy(to,from,(fbtype *)FB_END-to);
					}
			}
			return(CMD_SUCCESS);
		}

		fp = (fbtype *)FB_BASE;

		/* Adust the starting point based on x & y offsets (if any)...
		 */
		fp += (font_ypos * SCREEN_WIDTH) + font_xpos;

		/* Each row of pixels is extended to a 4-byte boundary, filling
		 * with an unspecified value so that the next row will start on
		 * a multiple-of-four byte location in memory or in the file.
		 * This equation establishes a row size variable that is compliant
		 * with that rule.  The equation is taken directly out of the
		 * wikipedia page mentioned at the top of this file.
		 * 
		 *                      (n * width) + 31
		 *      rowsize = 4 * ( ---------------- )
		 *                             32
		 *
		 * (where 'n' is the number of bits per pixel).
		 */
		rowsize = ((((24*bmpinfo.width) + 31)/32) * 4);
		if (verbose > 1)
			printf("rowsize: %d\n",rowsize);
	
		widthadjust = 0;
		while ((bmpinfo.width * 3) % 4) {
			widthadjust++;
			bmpinfo.width++;
		}

		if ((verbose > 1) && widthadjust) {
			printf("%d-byte width adjustment: %ld -> %ld\n",
				widthadjust, bmpinfo.width-widthadjust, bmpinfo.width);
		}
	
		if (bmpinfo.rawsize == 0) {
			bmpinfo.rawsize = (bmpinfo.height * bmpinfo.width) * 3;
		}
		else {
			if ((verbose > 1) &&
				(bmpinfo.rawsize != (bmpinfo.height * bmpinfo.width) * 3)) {
				printf("size off: %d %d\n",
					bmpinfo.rawsize,(bmpinfo.height * bmpinfo.width) * 3);
			}
		}
	
		bmp = (unsigned char *)base + bmpinfo.offset + bmpinfo.rawsize;
	
		/* Below there are 4 distinct cases regarding the size of the .bmp
		 * image vs the size of the LCD screen...
		 * If the bitmap is larger than the LCD, then just fit the upper
		 * left corner of the bitmap into the LCD.
		 * If the LCD is larger than the bitmap, just put the bitmap in 
		 * the upper left corner of the LCD and allow portions to be cut off.
		 *
		 * - Pixels are stored "upside-down" with respect to normal image raster
		 *   scan order, starting in the lower left corner, going from left to
		 *   right, and then row by row from the bottom to the top of the image.
		 *   As a result, this code has to also deal with that.
		 */

		if (bmpinfo.width > SCREEN_WIDTH) {
			if (bmpinfo.height > SCREEN_HEIGHT) {
				if (verbose > 2)
					printf("case 1\n");
				for(y=0;y<SCREEN_HEIGHT;y++) {
					volatile unsigned char *bp;
	
					bmp -= rowsize;
					bp = bmp;
					for(x=0;x<SCREEN_WIDTH;x++,fp++) {
						*fp = ((short)(*bp++ & 0xf8) >> 3);		// Blue
						*fp |= ((short)(*bp++ & 0xfc) << 3);	// Green
						*fp |= ((short)(*bp++ & 0xf8) << 8);	// Red
					}
					if (transition_delay) monDelay(transition_delay);
				}
			}
			else {
				if (verbose > 2)
					printf("case 2\n");
				for(y=0;y<bmpinfo.height;y++) {
					volatile unsigned char *bp;
	
					bmp -= rowsize;
					bp = bmp;
					for(x=0;x<SCREEN_WIDTH;x++,fp++) {
						*fp = ((short)(*bp++ & 0xf8) >> 3);		// Blue
						*fp |= ((short)(*bp++ & 0xfc) << 3);	// Green
						*fp |= ((short)(*bp++ & 0xf8) << 8);	// Red
					}
					if (transition_delay) monDelay(transition_delay);
				}
				if (fill_is_set) {
					while(fp < (unsigned short *)FB_END)
						*fp++ = fill_fbvalue;
				}
			}
		}
		else {
			if (bmpinfo.height > SCREEN_HEIGHT) {
				if (verbose > 2)
					printf("case 3\n");
				for(y=0;y<SCREEN_HEIGHT;y++) {
					volatile unsigned char *bp;
	
					bmp -= rowsize;
					bp = bmp;
					for(x=0;x<bmpinfo.width;x++,fp++) {
						*fp = ((short)(*bp++ & 0xf8) >> 3);		// Blue
						*fp |= ((short)(*bp++ & 0xfc) << 3);	// Green
						*fp |= ((short)(*bp++ & 0xf8) << 8);	// Red
					}
					if (fill_is_set) {
						while(x++ < SCREEN_WIDTH) 
							*fp++ = fill_fbvalue;
					}
					else {
						fp += (SCREEN_WIDTH - x);
					}
					if (transition_delay) monDelay(transition_delay);
				}
			}
			else {
				if (verbose > 2)
					printf("case 4\n");
				for(y=0;y<bmpinfo.height;y++) {
					volatile unsigned char *bp;
	
					bmp -= rowsize;
					bp = bmp;
					for(x=0;x<bmpinfo.width;x++,fp++) {
						*fp = ((short)(*bp++ & 0xf8) >> 3);		// Blue
						*fp |= ((short)(*bp++ & 0xfc) << 3);	// Green
						*fp |= ((short)(*bp++ & 0xf8) << 8);	// Red
					}
					if (fill_is_set) { 
						while(x++ < SCREEN_WIDTH)
							*fp++ = fill_fbvalue;
					}
					else {
						fp += (SCREEN_WIDTH - x);
					}
					if (transition_delay) monDelay(transition_delay);
				}
				if (fill_is_set) {
					while(fp < (unsigned short *)FB_END)
						*fp++ = fill_fbvalue;
				}
			}
		}
	}
#ifdef FBDEV_SETSTART
	else if (strcmp(cmd,"setstart") == 0) {
		if (argc != (optind+2))
			return(CMD_PARAM_ERROR);

		FBDEV_SETSTART(strtol(arg1,0,0));
		return(CMD_SUCCESS);
	}
#endif
	else if (strcmp(cmd,"color") == 0) {
		if (argc != (optind+2))
			return(CMD_PARAM_ERROR);

		fb_setcolor(strtoul(arg1,0,0),transition_type);
		return(CMD_SUCCESS);
	}
	else if (strcmp(cmd,"consolemode") == 0) {
		if (argc == (optind+1)) {
			printf("Consolemode currently %sabled\n",
				fbi_consolemode_enabled ? "en" : "dis");
			return(CMD_SUCCESS);
		}

		if (argc != (optind+2))
			return(CMD_PARAM_ERROR);

		if (strcmp(arg1,"on") == 0) {
			fbi_consolemode_enable(1,1);
			fbi_linewrap = 0;
		}
		else if (strcmp(arg1,"wrap") == 0) {
			fbi_consolemode_enable(1,1);
			fbi_linewrap = 1;
		}
		else if (strcmp(arg1,"off") == 0) {
			fbi_consolemode_enable(0,1);
		}
		else {
			return(CMD_PARAM_ERROR);
		}

		return(CMD_SUCCESS);
	}
	else if (strcmp(cmd,"font") == 0) {
		int newstyle;
		struct font *fontp;

		if (argc != (optind+6))
			return(CMD_PARAM_ERROR);
		
		newstyle = atoi(argv[optind+1]);
		if ((newstyle < 0) || (newstyle >= font_style_total())) {
			printf("Invalid font style\n");
			return(CMD_FAILURE);
		}

		font_style = newstyle;
		font_xscale = atoi(argv[optind+2]);
		font_yscale = atoi(argv[optind+3]);
		if (strcmp(argv[optind+4],"--")) {
			font_fgcolor.rgbval = strtol(argv[optind+4],0,0);
			font_fgcolor.fbval = rgb_to_fb(font_fgcolor.rgbval);
		}
		

		if (strcmp(argv[optind+5],"transparent") == 0) {
			font_is_opaque = 0;
		}
		else if (strcmp(argv[optind+5],"--")) {
			font_is_opaque = 1;
			font_bgcolor.rgbval = strtol(argv[optind+5],0,0);
			font_bgcolor.fbval = rgb_to_fb(font_bgcolor.rgbval);
		}

		
		fontp = &font_styles[font_style];
		font_totalcharheight = ((fontp->height + fontp->above + fontp->below) *
			font_yscale);
		font_totalcharwidth = ((fontp->width + fontp->between) * font_xscale);

		font_rowsperscreen = SCREEN_HEIGHT / font_totalcharheight;
		font_colsperscreen = SCREEN_WIDTH / font_totalcharwidth;
		font_linemodulo = SCREEN_HEIGHT % font_totalcharheight;

		font_initialized = 1;
	}
	else if (strcmp(cmd,"setpixel") == 0) {
		long	color;
		int		x, y, xx, yy, width;

		if (argc != optind+5)
			return(CMD_PARAM_ERROR);

		x = strtol(argv[optind+1],0,0);
		y = strtol(argv[optind+2],0,0);
		width = strtol(argv[optind+3],0,0);
		color = strtol(argv[optind+4],0,0);

		for(xx = x-width;xx <= x+width;xx++) {
			for(yy = y-width;yy <= y+width;yy++)
				fbi_setpixel(xx,yy,color);
		}
	}
	else if (strcmp(cmd,"print") == 0) {
		if (argc != optind+2)
			return(CMD_PARAM_ERROR);

		if (verbose)
			printf("Print: <%s>\n",argv[optind+1]);

		fbi_puts(argv[optind+1]);
		return(CMD_SUCCESS);
	}
	else if (strcmp(cmd,"fb2file") == 0) {
		int err;

		if (argc != optind+2)
			return(CMD_PARAM_ERROR);

		if (verbose)
			printf("Copying frame buffer to '%s'...\n",arg1);

		err = tfsadd(arg1,0,0,(unsigned char *)FB_BASE,FB_SIZE);
		if (err != TFS_OKAY)
			printf("Failed: %s\n",(char *)tfsctrl(TFS_ERRMSG,err,0));
		return(CMD_SUCCESS);
	}
	else {
		return(CMD_PARAM_ERROR);
	}

	return(CMD_SUCCESS);
}

/* fbi_putchar():
 * This is the external putchar available to other parts of the code.
 */
void
fbi_putchar(char c)
{
	if (!fbi_consolemode_enabled)
		return;

	fb_putchar(c);
}

#ifndef FBI_NO_CURSOR
void
fbi_cursor(void)
{
	static char beenhere;
    static struct elapsed_tmr tmr;

	if (!fbi_consolemode_enabled)
		return;

	if (!beenhere) {
    	startElapsedTimer(&tmr,500);
		beenhere = 1;
		return;
	}

   	if(msecElapsed(&tmr)) {
		fbi_cursorpos = (fbtype *)FB_BASE +
			(font_ypos * font_totalcharheight * SCREEN_WIDTH) +
			(font_xpos * font_totalcharwidth);

		if (font_ypos < font_linemodulo) {
			fbi_cursorpos += font_ypos*SCREEN_WIDTH;
		}
		else {
			fbi_cursorpos += font_linemodulo*SCREEN_WIDTH;
		}
		fbi_cursorpos += (font_totalcharheight-3) * SCREEN_WIDTH;
		if (fbi_cursorstate) {
			fb_memset(fbi_cursorpos,font_bgcolor.fbval,font_totalcharwidth);
#ifdef FBDEV_SETSTART
			fb_memset(fbi_cursorpos + FB_SIZE/sizeof(fbtype),
				font_bgcolor.fbval,font_totalcharwidth);
#endif
			fbi_cursorstate = 0;
		}
		else {
			fb_memset(fbi_cursorpos,font_fgcolor.fbval,font_totalcharwidth);
#ifdef FBDEV_SETSTART
			fb_memset(fbi_cursorpos + FB_SIZE/sizeof(fbtype),
				font_fgcolor.fbval,font_totalcharwidth);
#endif
			fbi_cursorstate = 1;
		}
    	startElapsedTimer(&tmr,500);
	}
}

void
fbi_cursor_clear(void)
{
	if (fbi_cursorstate == 1) {
		fb_memset(fbi_cursorpos,font_bgcolor.fbval,font_totalcharwidth);
#ifdef FBDEV_SETSTART
		fb_memset(fbi_cursorpos + FB_SIZE/sizeof(fbtype),
			font_bgcolor.fbval,font_totalcharwidth);
#endif
		fbi_cursorstate = 0;
	}
}
#endif

/* fbi_splash():
 * This function should be called at system startup to push out a
 * "splash screen" if the appropriate file is installed in TFS.
 * The presence of this file also establishes the initial state of
 * the console...
 * If the file is present, then consolemode is disabled and the splash
 * screen is displayed.  If the file is not present, then just set
 * up the system with consolemode enabled.
 */
void
fbi_splash(void)
{
	TFILE *tfp;
	fbtype *fp;

#if INCLUDE_UNZIP
	tfp = tfsstat("splash.gz");
	if (tfp) {
		decompress((char *)TFS_BASE(tfp),(int)TFS_SIZE(tfp),(char *)FB_BASE);
		return;
	}
#endif

	tfp = tfsstat("splash.bin");
	fp = (fbtype *)FB_BASE;

	if (tfp) {
		fbtype *splash;

		splash = (fbtype *)TFS_BASE(tfp);
		fb_memcpy(fp,splash,(fbtype *)FB_END-fp);
		fbi_consolemode_enable(0,0);
	}
	else {
		fbi_consolemode_enable(1,1);
	}
}

void
fbi_setpixel(int x, int y, long rgbcolor)
{
	fbtype *fbp;
	
	if (x > PIXELS_PER_ROW)
		x = PIXELS_PER_ROW;
	else if (x < 0)
		x = 0;
	if (y > PIXELS_PER_COL)
		y = PIXELS_PER_COL;
	else if (y < 0)
		y = 0;

	fbp = (fbtype *)FB_BASE + ((y * PIXELS_PER_ROW) + x);
	*fbp = rgb_to_fb(rgbcolor);
}

#endif
