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
 * dallasdate.c:
 *
 * Code to support the DALLAS DS1743W Timekeeping RAM plus the hooks to
 * allow TFS to timestamp files.  The function tfsTimeEnable() must be
 * called to setup the connection between TFS and the DS1743W.
 *
 * Applicable to DS1743W and DS1746WP.  Probably also works with other
 * Dallas timekeepers, but the above two are the only ones I've tested 
 * it with.  This code is assumed to be compiled with the DATEREGBASE
 * value defined elsewhere.  This value is the address at which the code
 * is to access the 8-byte register map that contains the time/date
 * information.
 *
 *
 * Original author:     Ed Sutter (ed.sutter@alcatel-lucent.com)
 *
 */
#include "config.h"
#include "stddefs.h"
#include "genlib.h"
#include "tfs.h"
#include "tfsprivate.h"
#include "cli.h"

#ifndef DATEREGBASE
#error DATEREGBASE not defined
#endif

#define WRITE	0x80		/* part of 'cent' member */
#define READ	0x40		/* part of 'cent' member */
#define FT		0x40		/* part of 'wday' member */
#define OSC_OFF	0x80		/* part of 'sec' member */

/* Masks for various portions of the time/date... */
#define YEAR10_MASK		0xf0
#define MONTH10_MASK	0x10
#define DATE10_MASK		0x30
#define DAY10_MASK		0x00
#define HOUR10_MASK		0x30
#define MINUTE10_MASK	0x70
#define SECOND10_MASK	0x70
#define CENTURY10_MASK	0x30
#define YEAR_MASK		0x0f	/* Year is 0-99 */
#define MONTH_MASK		0x0f	/* Month is 1-12 */
#define DATE_MASK		0x0f	/* Date is 1-31 */
#define DAY_MASK		0x07	/* Day is 1-7 */
#define HOUR_MASK		0x0f	/* Hour is 0-23 */
#define MINUTE_MASK		0x0f	/* Minutes is 0-59 */
#define SECOND_MASK		0x0f	/* Seconds is 0-59 */
#define CENTURY_MASK	0x0f	/* Century is 0-39 */

struct dsdate {
	uchar cent;		/* B7=W, B6=R, B5-4=10cent, B3-0=cent (00-39) */
	uchar sec;		/* B7=OSC; B6-4=10secs; B3-0=sec (0-59) */
	uchar min;		/* B6-4=10mins; B3-0=min (0-59) */
	uchar hour;		/* B4-5=10hour; B3-0=hour (0-23) */
	uchar wday;		/* B6=FT B2-0=day (1-7) */
	uchar mday;		/* B4-5=10date; B3-0=date (1-31) */
	uchar month;	/* B4=10mo; B3-0=month (1-12) */
	uchar year;		/* MS-nibble=10Year; LS-nibble=year (0-99) */
};

#define DS1743REGS 		((struct dsdate *)DATEREGBASE)
#define DS_century		(DS1743REGS->cent)
#define DS_second		(DS1743REGS->sec)
#define DS_minute		(DS1743REGS->min)
#define DS_hour			(DS1743REGS->hour)
#define DS_wday			(DS1743REGS->wday)
#define DS_mday			(DS1743REGS->mday)
#define DS_month		(DS1743REGS->month)
#define DS_year			(DS1743REGS->year)

#define SECONDS_IN_DAY	86400

static char
DaysInMonth[] = {
    31, 28, 31, 30, 31, 30,     /* Jan, Feb, Mar, Apr, May, Jun */
    31, 31, 30, 31, 30, 31		/* Jul, Aug, Sep, Oct, Nov, Dec */
};

/* rangecheck():
	Return 0 if value is outside the range of high and low; else 1.
*/
static int
rangecheck(int value, char *name, int low, int high)
{
	if ((value < low) || (value > high)) {
		printf("%s outside valid range of %d - %d\n",
			name,low,high);
		return(0);
	}
	return(1);
}

/* to_dsdatefmt():
 * Take an incoming integer and convert it to the dallas time date
 * format using a 10-multiplier for the upper nibble.
 */
static unsigned char
to_dsdatefmt(int value)
{
	int	tens;

	tens = 0;
	while (value >= 10) {
		tens++;
		value -= 10;
	}
	return((tens << 4) | value);
}

static int
from_dsdatefmt(unsigned char value, unsigned char mask10)
{
	int	newvalue;
	
	newvalue = value & 0x0f;
	newvalue += 10 * ((value & mask10) >> 4);
	return(newvalue);
}

int
showDate(int center)
{
	int	(*pfunc)(char *, ...);
	int	day, date, month, year, hour, minute, second, century;

	DS_century |= READ;	/* Set READ bit */

	century = from_dsdatefmt(DS_century,CENTURY10_MASK);
	second = from_dsdatefmt(DS_second,SECOND10_MASK);
	minute = from_dsdatefmt(DS_minute,MINUTE10_MASK);
	hour = from_dsdatefmt(DS_hour,HOUR10_MASK);
	day = from_dsdatefmt(DS_wday,DAY10_MASK);
	date = from_dsdatefmt(DS_mday,DATE10_MASK);
	month = from_dsdatefmt(DS_month,MONTH10_MASK);
	year = (((DS_year & 0xf0) >> 4) * 10) + (DS_year & 0x0f);

	DS_century &= ~READ;	/* Clear READ bit */

	if (center)
		pfunc = cprintf;
	else
		pfunc = printf;

	pfunc("%02d/%02d/%02d%02d @ %02d:%02d:%02d\n",
		month,date,century,year,hour,minute,second);

	return(0);
}

#if INCLUDE_TFS

static int
YearIsLeap(int year)
{
    if (year % 4)		/* if year not divisible by 4... */
        return(0);		/* it's not leap */
    if (year < 1582)	/* all years divisible by 4 were */
        return(1);		/* leap prior to 1582            */
    if (year % 100)		/* if year divisible by 4, */
        return(1);		/* but not by 100, it's leap */
    if (year % 400)		/* if year divisible by 100, */
        return(0);		/* but not by 400, it's not leap */
    else
        return(1);		/* if divisible by 400, it's leap */
}

/* GetAtime():
 *	Build a string based on the incoming long value as described in 
 *	GetLtime()...
 *	Used by TFS to keep track of file time.
 */
static char *
GetAtime(long tval, char *buf, int bsize)
{
	int	year;		/* Actual year */
	int	doy;		/* Day of year */
	int	sid;		/* Seconds in day */
	int	leapyear;	/* Set if year is leap */
	int	month, hour, minute;

	if ((bsize < 18)  || (tval == TIME_UNDEFINED)) {
		*buf = 0;
		return(buf);
	}

	/* Break down the basic bitfields: */
	year = 2000 + ((tval >> 26) & 0x3f);
	leapyear = YearIsLeap(year);
	doy = ((tval >> 17) & 0x1ff);
	sid = tval & 0x1ffff;

	/* Now build the date from the bit fields: */
	hour = sid / 3600;
	sid -= (hour*3600);
	minute = sid/60;
	sid -= (minute*60);

	month = 0;
	while(doy > DaysInMonth[month]) {
		doy -= DaysInMonth[month]; 
		if ((month == 1) && (leapyear))
			doy--;
		month++;
	}
	sprintf(buf,"%02d/%02d/%02d@%02d:%02d:%02d",
		month+1,doy,year,hour,minute,sid);
	return(buf);
}

/* GetLtime():
 *	Build a long with the following format....
 *  
 *	B31-26		year since 2000		(0-127 yep, this breaks in 2128)
 *	B25-17		day of year			(1-365)
 *	B16-0		seconds in day		(0-86400)
 *
 *	Used by TFS to keep track of file time.
 */
static long
GetLtime(void)
{
	long	tval;
	int		year, month, mday, seconds, doy, leapyear;

	DS_century |= READ;	/* Set READ bit */

	year = from_dsdatefmt(DS_year,YEAR10_MASK);			/* 00=2000 */
	month = from_dsdatefmt(DS_month,MONTH10_MASK)-1;		/* 0-11 */
	mday = from_dsdatefmt(DS_mday,DATE10_MASK);			/* 1-31 */
	leapyear = YearIsLeap(year+2000);

	if ((month > 11) || (month < 0) || (mday < 1) || (mday > 31))
		return(TIME_UNDEFINED);

	/* Determine current day of year... */
	doy = mday;
	while(month > 0) {
		doy += DaysInMonth[month-1];
		if ((month == 1) && leapyear)
			doy++;
		month--;
	}

	/* Determine current second of day... */
	seconds = (from_dsdatefmt(DS_hour,HOUR10_MASK) * 3600);
	seconds += (from_dsdatefmt(DS_minute,MINUTE10_MASK) * 60);
	seconds += from_dsdatefmt(DS_second,SECOND10_MASK);

	DS_century &= ~READ;	/* Clear READ bit */

	tval = (((year & 0x3f) << 26) |
			((doy & 0x1ff) << 17) |
			(seconds & 0x1ffff));
	return(tval);
}

/* tfsTimeEnable():
 *	Hook the file timestamping in TFS to the DS1743 device...
 */
void
tfsTimeEnable(void)
{
	tfsctrl(TFS_TIMEFUNCS,(long)GetLtime,(long)GetAtime);
}
#endif

char *DateHelp[] = {
	"Display (mm/dd/yyyy@hh:mm:ss) or modify time and date.",
	"[{day date month year hour min sec}]",
#if INCLUDE_VERBOSEHELP
	"Where...",
	" day:   1-7 (sun=1)",
	" date:  1-31",
	" month: 1-12", 
	" year:  0-3899", 
	" hour:  0-23", 
	" min:   0-59", 
	" sec:   0-59", 
	"Note: 'date off' disables the oscillator",
#endif
	0
};

int
Date(int argc,char *argv[])
{
	int	day, date, month, year, hour, minute, second, century, rngchk;

	if (argc == 1) {
		if (DS_second & OSC_OFF)
			printf("Warning: oscillator disabled.\n");
		showDate(0);
		return(CMD_SUCCESS);
	}
	else if ((argc == 2) && !strcmp(argv[1],"off")) {
		DS_century |= WRITE;			/* Disable the oscillator */
		DS_second = OSC_OFF;			/* to save battery life. */
		DS_century &= ~WRITE;
		return(CMD_SUCCESS);
	}
	else if (argc != 8)
		return(CMD_PARAM_ERROR);

	day = atoi(argv[1]);
	date = atoi(argv[2]);
	month = atoi(argv[3]);
	year = atoi(argv[4]);
	hour = atoi(argv[5]);
	minute = atoi(argv[6]);
	second = atoi(argv[7]);

	rngchk = 0;
	rngchk += rangecheck(day,"day",1,7);
	rngchk += rangecheck(date,"date",1,31);
	rngchk += rangecheck(month,"month",1,12);
	rngchk += rangecheck(year,"year",0,3899);
	rngchk += rangecheck(hour,"hour",0,23);
	rngchk += rangecheck(minute,"minute",0,59);
	rngchk += rangecheck(second,"second",0,59);

	if (rngchk != 7)
		return(CMD_PARAM_ERROR);
	
	DS_century = WRITE;		/* Set WRITE bit */
	DS_second = to_dsdatefmt(second) & ~OSC_OFF;
	DS_minute = to_dsdatefmt(minute);
	DS_hour = to_dsdatefmt(hour);
	DS_wday = day;
	DS_mday = to_dsdatefmt(date);
	DS_month = to_dsdatefmt(month);
	century = year / 100;
	year = year % 100;
	DS_year = to_dsdatefmt(year);
	DS_century = (to_dsdatefmt(century)&(CENTURY_MASK|CENTURY10_MASK))|WRITE;

	DS_century &= ~WRITE;		/* Clear WRITE bit */
	return(CMD_SUCCESS);
}


