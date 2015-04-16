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
 * vmprintf.c:
 *
 * Same as mprintf, but this uses stdarg.h.
 * Cleaner and simpler.  I have some targets using this and will probably
 * convert all targets to use this eventually.
 *
 * Note that this requires that the CFLAGS used in the building of this
 * file should omit the "-nostdinc" option, so that the standard header
 * (stdarg.h) can be pulled in.
 *
 * Public functions:
 * vsnprintf(), snprintf(), sprintf(), printf(), cprintf()
 *
 * Original author:     Ed Sutter (ed.sutter@alcatel-lucent.com)
 *
 */
#include <stdarg.h>
#include "stddefs.h"
#include "cli.h"

extern long abs(long);
extern void puts(char *);
extern int atoi(char *), putchar(char);
int printf(char *fmt, ...);

static char *hexdigits = "0123456789abcdef";
static char *Hexdigits = "0123456789ABCDEF";

#define MAX_NUMBER_SIZE 	18
#define HDRSIZE				4
#define NEGATIVE			1
#define POSITIVE			2
#define SCREENWIDTH			80

/* PROCCHARP(), PROCCHAR(), BUFDEC() & BUFINC():
 * Macros used to conveniently distinguish between buffer-bound
 * and console-bound characters during the formatting process.
 */
#define PROCCHARP(bp,eob,cp) 					\
	if (bp) {									\
		if (eob && (bp >= eob)) {				\
			puts("s_printf buffer overflow");	\
			return(-1);							\
		}										\
		*bp++ = *cp++;							\
	}											\
	else {										\
		putchar(*cp++);							\
	}											

#define PROCCHAR(bp,eob,c) 						\
	if (bp) {									\
		if (eob && (bp >= eob)) {				\
			puts("s_printf buffer overflow");	\
			return(-1);							\
		}										\
		*bp++ = c;								\
	}											\
	else {										\
		if (c)									\
			putchar(c);						\
	}

#define BUFDEC(bp,val)		\
	if (bp) {				\
		bp -= val;			\
	}		

#define BUFINC(bp,val)		\
	if (bp) {				\
		bp += val;			\
	}		


/* isdigit():
 * Copied here to avoid inclusion of ctype.h
 */
static int
isdigit(char ch)
{
  return ((ch >= '0') && (ch <= '9'));
}

/* proc_d_prefix():
 * Deal with the characters prior to the 'd' format designator.
 * For example, hdr would be '8' in the format %8d.
 */
static int
proc_d_prefix(char *hdr, char *lbp)
{
	char fill;
	int	minsize, i;

	minsize = 0;
	if (hdr[0]) {
		if (hdr[0] == '0')
			fill = '0';
		else
			fill = ' ';
		minsize = (short)atoi(hdr);
		for(i=0;i<minsize;i++)
			*lbp++ = fill;
	}
	return(minsize);
}


/* long_to_dec():
 * Convert an incoming long value to ASCII decimal notation.
 */
int
long_to_dec(long lval,char *buf,char *bufend,char *hdr)
{
	unsigned long	value;
	short	size, i, minsize;
	char	lbuf[MAX_NUMBER_SIZE], *lbp;

	lbp = lbuf;
	minsize = proc_d_prefix(hdr,lbuf);

	/* First determine how many ascii digits are needed. */
	value = abs(lval);
	size = 0;
	while (value > 0) {
		size++;
		value /= 10;
	}
	if (lval < 0)
		size++;
	if (minsize > size)
		size = minsize;
	lbp += size;

	/* Now build the string. */
	value = abs(lval);
	if (value == 0) {
		if (minsize==0) {
			lbuf[0] = '0';
			size = 1;
		}
		else 
			*--lbp = '0';
	}
	else {
		while (value > 0) {
			*--lbp = (char)((value % 10) + '0');
			value /= 10;
		}
	}
	if (lval < 0)
		*--lbp = '-';
	lbuf[size] = 0;

	/* At this point, lbuf[] contains the ascii decimal string
	 * so we can now pass it through PROCCHAR...
	 */
	for(i=0;i<size;i++) {
		PROCCHAR(buf,bufend,lbuf[i]);
	}
	return((int)size);
}

/* llong_to_dec():
 * Convert an incoming long long value to ASCII decimal notation.
 * This is essentially identical to long_to_dec, but we use longlong.
 */
static int
llong_to_dec(int64_t llval,char *buf,char *bufend,char *hdr)
{
	uint64_t value;
	short	size, i, minsize;
	char	lbuf[MAX_NUMBER_SIZE], *lbp;

	lbp = lbuf;
	minsize = proc_d_prefix(hdr, lbuf);

	/* First determine how many ascii digits are needed. */
	value = llval < 0 ? -llval : llval;
	size = 0;
	while (value > 0) {
		size++;
		value /= 10;
	}
	if (llval < 0)
		size++;
	if (minsize > size)
		size = minsize;
	lbp += size;

	/* Now build the string. */
	value = llval < 0 ? -llval : llval;
	if (value == 0) {
		if (minsize==0) {
			lbuf[0] = '0';
			size = 1;
		}
		else 
			*--lbp = '0';
	}
	else {
		while (value > 0) {
			*--lbp = (char)((value % 10) + '0');
			value /= 10;
		}
	}
	if (llval < 0)
		*--lbp = '-';
	lbuf[size] = 0;

	/* At this point, lbuf[] contains the ascii decimal string
	 * so we can now pass it through PROCCHAR...
	 */
	for(i=0;i<size;i++) {
		PROCCHAR(buf,bufend,lbuf[i]);
	}
	return((int)size);
}

/* long_to_ip():
 * Convert an incoming long value to an ascii IP formatted
 * string (i.e.  0x01020304 is converted to "1.2.3.4")
 */
static int
long_to_ip(long lval,char *buf,char *bufend,char *hdr)
{
	int	i, j, len;
	unsigned char *lp;

	len = 0;
	lp = (unsigned char *)&lval;
	for(j=0;j<4;j++) {
		i = long_to_dec(*lp++,buf,bufend,hdr);
		BUFINC(buf,i);
		if (j < 3) {
			PROCCHAR(buf,bufend,'.');
		}
		len += (i + 1);
	}
	BUFDEC(buf,1);
	len--;
	return(len);
}

/* proc_x_prefix():
 * Deal with the characters prior to the 'x' format designator.
 * For example, hdr would be '02' in the format %02x.
 */
static int
proc_x_prefix(char *hdr, char *lbp)
{
	int i, minsize;

	minsize = 0;
	if (hdr[0]) {
		if (hdr[1]) {
			minsize = (short)(hdr[1]&0xf);
			for(i=0;i<minsize;i++)
				*lbp++ = hdr[0];
		}
		else {
			minsize = (short)(hdr[0]&0xf);
			for(i=0;i<minsize;i++)
				*lbp++ = ' ';
		}
	}
	return(minsize);
}

/* long_to_hex():
 * Convert an incoming long value to ASCII hexadecimal notation.
 */
static int
long_to_hex(ulong lval,char *buf,char *bufend,char *hdr, char x)
{
	ulong	value;
	short	size, i, minsize;
	char	lbuf[MAX_NUMBER_SIZE], *lbp;

	lbp = lbuf;
	minsize = proc_x_prefix(hdr,lbuf);

	/* First determine how many ascii digits are needed. */
	value = lval;
	size = 0;
	while (value > 0) {
		size++;
		value /= 16;
	}
	if (minsize > size)
		size = minsize;
	lbp += size;

	/* Now build the string. */
	if (lval == 0) {
		if (size == 0)
			size = 1;
		else
			lbp--;
		*lbp = '0';
	}
	else {
		while (lval > 0) {
			if (x == 'X')
				*--lbp = Hexdigits[(int)(lval % 16)];
			else
				*--lbp = hexdigits[(int)(lval % 16)];
			lval /= 16;
		}
	}
	lbp[size] = 0;

	/* At this point, lbuf[] contains the ascii hex string
	 * so we can now pass it through PROCCHAR...
	 */
	for(i=0;i<size;i++) {
		PROCCHAR(buf,bufend,lbuf[i]);
	}

	return((int)size);
}

/* llong_to_hex():
 * Convert an incoming long long value to ASCII hexadecimal notation.
 */
static int
llong_to_hex(uint64_t llval,char *buf,char *bufend,char *hdr, char x)
{
	uint64_t	value;
	short	size, i, minsize;
	char	lbuf[MAX_NUMBER_SIZE], *lbp;

	lbp = lbuf;
	minsize = proc_x_prefix(hdr,lbuf);

	/* First determine how many ascii digits are needed. */
	value = llval;
	size = 0;
	while (value > 0) {
		size++;
		value /= 16;
	}
	if (minsize > size)
		size = minsize;
	lbp += size;

	/* Now build the string. */
	if (llval == 0) {
		if (size == 0)
			size = 1;
		else
			lbp--;
		*lbp = '0';
	}
	else {
		while (llval > 0) {
			if (x == 'X')
				*--lbp = Hexdigits[(int)(llval % 16)];
			else
				*--lbp = hexdigits[(int)(llval % 16)];
			llval /= 16;
		}
	}
	lbp[size] = 0;

	/* At this point, lbuf[] contains the ascii hex string
	 * so we can now pass it through PROCCHAR...
	 */
	for(i=0;i<size;i++) {
		PROCCHAR(buf,bufend,lbuf[i]);
	}

	return((int)size);
}

/* bin_to_mac():
 * Convert an incoming buffer to an ascii MAC formatted
 * string (i.e.  buf[] = 010203040506 is converted to "01:02:03:04:05:06")
 */
static int
bin_to_mac(uchar *ibin,char *buf,char *bufend)
{
	int	i, j, len;

	len = 0;
	for(j=0;j<6;j++) {
		i = long_to_hex(*ibin++,buf,bufend,"02",'x');
		BUFINC(buf,i);
		if (j < 5) {
			PROCCHAR(buf,bufend,':');
		}
		len += (i + 1);
	}
	BUFDEC(buf,1);
	len--;
	return(len);
}

/* build_string():
 * 	Build a string from 'src' to 'dest' based on the hdr and sign
 *	values.  Return the size of the string (may include left or right
 *	justified padding).
 */
static int
build_string(char *src,char *dest,char *bufend,char *hdr,int sign)
{
	char	*cp1;
	short	minsize, i, j;

	if (!src) {
		cp1 = "NULL_POINTER";
		while(*cp1) {
			PROCCHARP(dest,bufend,cp1);
		}
		return(12);
	}
	if (!*src)
		return(0);
	if (!hdr[0]) {
		j = 0;
		while(*src) {
			PROCCHARP(dest,bufend,src);
			j++;
		}
		return(j);
	}
	minsize = (short)atoi(hdr);
	i = 0;
	cp1 = (char *)src;
	while(*cp1) {
		i++;
		cp1++;
	}
	cp1 = (char *)src;
	j = 0;
	if (minsize > i) {
		if (sign == POSITIVE) {
			while(minsize > i) {
				j++;
				PROCCHAR(dest,bufend,' ');
				minsize--;
			}
			while(*cp1) {
				j++;
				PROCCHARP(dest,bufend,cp1);
			}
		}
		else {
			while(*cp1) {
				j++;
				PROCCHARP(dest,bufend,cp1);
			}
			while(minsize > i) {
				j++;
				PROCCHAR(dest,bufend,' ');
				minsize--;
			}
		}
	}
	else {
		while(*cp1) {
			j++;
			PROCCHARP(dest,bufend,cp1);
		}
	}
	return(j);
}

/* vsnprintf():
 *	Backend to all the others below it.
 *	Formats incoming argument list based on format string.
 *	Terminates population of buffer if it is to exceed the
 *	specified buffer size.
 */
int
vsnprintf(char *buf,int bsize, char *fmt, va_list argp)
{
	long	arg_l;
	long long arg_ll;
	int		i, sign, tot;
	char	*cp, hdr[HDRSIZE], *base, *bufend, arg_c, *arg_cp, ll;

	ll = 0;
	tot = 0;
	base = buf;

	if (bsize == 0)
		bufend = 0;
	else
		bufend = base+(bsize-1);

	cp = fmt;
	for(i=0;i<HDRSIZE;i++)
		hdr[i] = 0;
	while(*cp) {
		if (*cp != '%') {
			PROCCHARP(buf,bufend,cp);
			tot++;
			continue;
		}
		cp++;
		if (*cp == '%') {
			PROCCHARP(buf,bufend,cp);
			tot++;
			continue;
		}
		sign = POSITIVE;
		if (*cp == '-') {
			sign = NEGATIVE;
			cp++;
		}
		if (isdigit(*cp)) {
			for(i=0;i<(HDRSIZE-1);i++) {
				if (isdigit(*cp))
					hdr[i] = *cp++;
				else
					break;
			}
		}

		ll = 0;
		if (*cp == 'l') {			/* Ignore the 'long' designator */
			cp++;
			if (*cp == 'l') {		/* unless its the longlong designator */
				cp++;
				ll = 1;
			}
		}

		switch(*cp) {
			case 'c':			/* Character conversion */
				arg_c = (char)va_arg(argp,int);
				PROCCHAR(buf,bufend,arg_c);
				tot++;
				break;
			case 's':			/* String conversion */
				arg_cp = (char *)va_arg(argp,int);
				i = build_string(arg_cp,buf,bufend,hdr,sign);
				BUFINC(buf,i);
				tot += i;
				break;
			case 'M':			/* MAC address conversion */
				arg_cp = (char *)va_arg(argp,int);
				i = bin_to_mac((uchar *)arg_cp,buf,bufend);
				BUFINC(buf,i);
				tot += i;
				break;
			case 'I':			/* IP address conversion */
				arg_l = (long)va_arg(argp,int);
				i = long_to_ip(arg_l,buf,bufend,hdr);
				BUFINC(buf,i);
				tot += i;
				break;
			case 'd':			/* Decimal conversion */
			case 'u':
				if (ll) {
					arg_ll = (long long)va_arg(argp,long long);
					i = llong_to_dec(arg_ll,buf,bufend,hdr);
				}
				else {
					arg_l = (long)va_arg(argp,int);
					i = long_to_dec(arg_l,buf,bufend,hdr);
				}
				BUFINC(buf,i);
				tot += i;
				break;
			case 'p':			/* Hex conversion */
			case 'x':
			case 'X':
				if (*cp == 'p') {
					PROCCHAR(buf,bufend,'0');
					PROCCHAR(buf,bufend,'x');
				}
				if (ll) {
					arg_ll = (long long)va_arg(argp,long long);
					i = llong_to_hex(arg_ll,buf,bufend,hdr,*cp);
				}
				else {
					arg_l = (long)va_arg(argp,int);
					i = long_to_hex((ulong)arg_l,buf,bufend,hdr,*cp);
				}
				BUFINC(buf,i);
				tot += i;
				break;
			default:
				PROCCHARP(buf,bufend,cp);
				tot++;
				break;
		}
		cp++;

		if (hdr[0]) {
			for(i=0;i<HDRSIZE;i++)
				hdr[i] = 0;
		}
	}
	PROCCHAR(buf,bufend,0);
	return(tot);
}

/* snprintf(), sprintf(), printf() & cprintf():
 * All functions use vsnprintf() to format a string.
 * The string is either transferred to a buffer or transferred
 * directly to this target's console through putchar (refer to
 * the macros at the top of this file).
 *
 * - sprintf() formats to a buffer.
 * - printf() formats to stdio.
 * - cprintf() formats to a buffer, then centers the content of
 *		the buffer based on its size and a console screen with of
 *		SCREENWIDTH characters.
 */
int
snprintf(char *buf, int bsize, char *fmt, ...)
{
	int	tot;
	va_list	argp;

	va_start(argp,fmt);
	tot = vsnprintf(buf,bsize,fmt,argp);
	va_end(argp);
	return(tot);
}

int
sprintf(char *buf, char *fmt, ...)
{
	int	tot;
	va_list argp;

	va_start(argp,fmt);
	tot = vsnprintf(buf,0,fmt,argp);
	va_end(argp);
	return(tot);
}

int
printf(char *fmt, ...)
{
	int	tot;
	va_list argp;

	va_start(argp,fmt);
	tot = vsnprintf(0,0,fmt,argp);
	va_end(argp);
	return(tot);
}

int
cprintf(char *fmt, ...)
{
	int	i, tot, spaces;
	char	pbuf[CMDLINESIZE];
	va_list argp;

	va_start(argp,fmt);
	tot = vsnprintf(pbuf,CMDLINESIZE,fmt,argp);
	va_end(argp);

	if (tot < SCREENWIDTH) {
		spaces = (SCREENWIDTH-tot)/2;
		for(i=0;i<spaces;i++)
			putchar(' ');
	}
	else
		spaces = 0;

	for(i=0;i<tot;i++)
		putchar(pbuf[i]);

	return(tot+spaces);
}
