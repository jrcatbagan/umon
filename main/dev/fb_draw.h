extern void fb_draw_line2(int x0, int y0, int x1, int y1, uchar color);
extern void fb_draw_circle(ulong x, ulong y, ulong radius, uchar color);
extern void fb_draw_circle2(int xpos, int ypos, int radius, uchar color);
extern void fb_fill_circle(ulong x, ulong y, ulong radius, uchar color);
extern void fb_print_char(uchar cchar, ulong x, ulong y, uchar color);
extern void fb_print_string(uchar *pcbuffer, ulong x, ulong y, uchar color);
extern void fb_print_charx2(char cchar, ulong x, ulong y, uchar color);
extern void fb_print_stringx2(char *pcbuffer, ulong x, ulong y, uchar color);
