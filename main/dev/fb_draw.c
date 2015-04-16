//==========================================================================
//
// fb_draw.c
//
// Author(s):   Michael Kelly - Cogent Computer Systems, Inc.
// Date:        10/18/03
// Description:	generic frame buffer drawing routines - based on
//			    lcd.c (c) 1999 Cirrus Logic.
//
// These routines all end up calling fb_set_pixel().  That routine must
// be provided in the target specific code.
//
//==========================================================================
#include "config.h"
#include "cpuio.h"
#include "genlib.h"
#include "stddefs.h"
#include "fb_fonts.h"
#include "vga_lookup.h"

#if INCLUDE_LCD
extern uchar bcolor;
extern void fb_set_pixel(int X, int Y, uchar color);

void fb_draw_line2(int x0, int y0, int x1, int y1, uchar color)
 {
        int i;
        int steep = 1;
        int sx, sy;  /* step positive or negative (1 or -1) */
        int dx, dy;  /* delta (difference in X and Y between points) */
        int e;

        /*
         * optimize for vertical and horizontal lines here
         */

        dx = (x1 > x0) ? (x1 - x0) : x0 - x1;
        sx = ((x1 - x0) > 0) ? 1 : -1;
        dy = (y1 > y0) ? (y1 - y0) : y0 - y1;
        sy = ((y1 - y0) > 0) ? 1 : -1;
        e = (dy << 1) - dx;
        for (i = 0; i < dx; i++) {
                if (steep) {
                        fb_set_pixel(x0,y0,color);
                } else {
                        fb_set_pixel(y0,x0,color);
                }
                while (e >= 0) {
                        y0 += sy;
                        e -= (dx << 1);
                }
                x0 += sx;
                e += (dy << 1);
        }
 }

// ------------------------------------------------------------------
// fb_draw_circle draws a circle at the specified location based on
// bresenham's circle drawing algorithm
void fb_draw_circle(ulong x, ulong y, ulong radius, uchar color)
{
    ulong xpos, ypos, d;

    xpos = x;
    ypos = radius;

    d = 3 - (2 * ypos);

    while(ypos >= xpos)
    {
		fb_set_pixel(x + xpos, y + ypos, color); // point in octant 1 
		fb_set_pixel(x - xpos, y + ypos, color); // point in octant 4 
		fb_set_pixel(x - xpos, y - ypos, color); // point in octant 5 
		fb_set_pixel(x + xpos, y - ypos, color); // point in octant 8 
		fb_set_pixel(x + ypos, y + xpos, color); // point in octant 2 
		fb_set_pixel(x - ypos, y + xpos, color); // point in octant 3 
		fb_set_pixel(x - ypos, y - xpos, color); // point in octant 6 
		fb_set_pixel(x + ypos, y - xpos, color); // point in octant 7 

		if (d < 0)
		{
		     d = (d + (4 * xpos)) + 6;
		}
		else
		{
			d = (d + (4 * (xpos - ypos))) + 10;
			ypos = ypos - 1;
		}

        xpos++;
    }
}

void fb_draw_circle2(ulong xpos, ulong ypos, ulong radius, uchar color)
{
	int x;
	int y = radius;
	int d = -radius;
	for(x = 1; x < (radius/1.414); x++)
	{  d += (2 * x) - 1;
		if (d >= 0) 
			{  y--;
			   d -= (2 * y); /* Must do this AFTER y-- */
			}
	//	   fb_set_pixel(x + xpos, y + ypos, color);
		fb_set_pixel(xpos + x, ypos + y, color); // point in octant 1 
		fb_set_pixel(xpos - x, ypos + y, color); // point in octant 4 
		fb_set_pixel(xpos - x, ypos - y, color); // point in octant 5 
		fb_set_pixel(xpos + x, ypos - y, color); // point in octant 8 
		fb_set_pixel(xpos + y, ypos + x, color); // point in octant 2 
		fb_set_pixel(xpos - y, ypos + x, color); // point in octant 3 
		fb_set_pixel(xpos - y, ypos - x, color); // point in octant 6 
		fb_set_pixel(xpos + y, ypos - x, color); // point in octant 7 
	}    
}

// ------------------------------------------------------------------
// fb_draw_circle draws a filled circle at the specified location based on
// bresenham's circle drawing algorithm
//
void fb_fill_circle(ulong x, ulong y, ulong radius, uchar color)
{
    ulong xpos, ypos, d;

    xpos = x;
    ypos = radius;

    d = 3 - (2 * ypos);

    while(ypos >= xpos)
    {
        fb_draw_line2(x - xpos, y + ypos, x + xpos, y + ypos, color);
        fb_draw_line2(x - ypos, y + xpos, x + ypos, y + xpos, color);
        fb_draw_line2(x - xpos, y - ypos, x + xpos, y - ypos, color);
        fb_draw_line2(x - ypos, y - xpos, x + ypos, y - xpos, color);

        if(d < 0)
        {
            d += (4 * xpos) + 6;
        }
        else
        {
            d += 10 + (4 * (xpos - ypos));
            ypos--;
        }

        xpos++;
    }
}

// ------------------------------------------------------------------
// fb_print_char prints a character at the specified location.
//
void fb_print_char(uchar cchar, ulong x, ulong y, uchar color)
{
    ulong idx1, idx2;
    uchar font_line, fcolor;

	// printable characters only
	if ((cchar < FIRST_CHAR) || (cchar > LAST_CHAR))
		return;

    // loop through each row of the font.
    for(idx1 = 0; idx1 < FONT_HEIGHT; idx1++)
    {
        // read the byte which describes this row of the font.
        font_line = fb_font_data[(cchar - FIRST_CHAR) * FONT_STEP][idx1];

        // loop through each column of this row.
        for(idx2 = 0; idx2 < 8; idx2++)
        {
            // set the pixel based on the value of this bit of the font, 
            // using the requested or the backround color.
            fcolor = font_line & 1 << ((FONT_WIDTH - 1) - idx2) ? color : bcolor;

            fb_set_pixel(x + idx2, y + idx1, fcolor);
        }
    }
}

// ------------------------------------------------------------------
// fb_print_string prints a string at the specified location.
//
void
fb_print_string(uchar *pcbuffer, ulong x, ulong y, uchar color)
{
    // loop through each character in the string.
    while(*pcbuffer)
    {
        // print this character.
        fb_print_char(*pcbuffer++, x, y, color);

        // advance horizontaly past this character.
        x += FONT_WIDTH;
    }
}

// ------------------------------------------------------------------
// fb_print_charx2 prints a double-sized character at the specified location.
//
void fb_print_charx2(char cchar, ulong x, ulong y, uchar color)
{
    ulong idx1, idx2;
    ushort font_line;
    uchar fcolor;

	// printable characters only
	if (((unsigned char)cchar < FIRST_CHAR) ||
		((unsigned char)cchar > LAST_CHAR))
		return;

    // loop through each row of the font.
    for(idx1 = 0; idx1 < FONT_HEIGHT; idx1++)
    {
        // read the byte which describes this row of the font.
        font_line = (fb_font_data[(cchar - FIRST_CHAR) * FONT_STEP][idx1]) << 8;
		if (FONT_WIDTH > 8)	// get the second byte if present
		{
	        font_line = fb_font_data[(cchar - FIRST_CHAR) * FONT_STEP][idx1 + 1];
		}

        // loop through each column of this row.
        for(idx2 = 0; idx2 < FONT_WIDTH; idx2++)
        {
            // determine the color of this pixel block.
            fcolor = font_line & (1 << ((FONT_WIDTH - 1) - idx2)) ? color : bcolor;

            // set the pixel block (2x2) based on the value of this bit of
            // the font, using the requested or the backround color.
            fb_set_pixel(x + (idx2 << 1), y + (idx1 << 1), fcolor);
            fb_set_pixel(x + (idx2 << 1) + 1, y + (idx1 << 1), fcolor);
            fb_set_pixel(x + (idx2 << 1), y + (idx1 << 1) + 1, fcolor);
            fb_set_pixel(x + (idx2 << 1) + 1, y + (idx1 << 1) + 1, fcolor);
        }
    }
}

// ------------------------------------------------------------------
// fb_print_stringx2 prints a string of double-sized characters at the
// specified location.
//
void fb_print_stringx2(char *pcbuffer, ulong x, ulong y, uchar color)
{
    //
    // loop through each character in the string.
    //
    while(*pcbuffer)
    {
        //
        // print this character.
        //
        fb_print_charx2(*pcbuffer++, x, y, color);

        //
        // advance horizontaly past this character.
        //
        x += 16;
    }
}
#endif

