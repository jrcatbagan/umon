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
 *  date.c:
 *
 *  A command front-end and utilities for time-of-day-clock hardware.
 *  Some of this is taken from the older file dallasdate.c which *should*
 *  be moved to the dev directory (if not already there).
 *  This code expects a few HW-specific functions to be available...
 *
 *  getTimeAndDate(struct todinfo *);
 *  setTimeAndDate(struct todinfo *);
 *
 *  If setTimeAndDate() is passed '0', then it is this code's attempt
 *  to disable the TOD oscillator (to reduce battery drain).
 *
 *
 * Original author:     Ed Sutter (ed.sutter@alcatel-lucent.com)
 *
 */
#include "config.h"
#include "stddefs.h"
#include "genlib.h"
#include "cli.h"
#include "date.h"

/* rangecheck():
    Return 0 if value is outside the range of high and low; else 1.
*/
static int
rangecheck(int value, char *name, int low, int high)
{
    if((value < low) || (value > high)) {
        printf("%s outside valid range of %d - %d\n",
               name,low,high);
        return(0);
    }
    return(1);
}

int
showDate(int center)
{
    int (*pfunc)(char *, ...);
    struct todinfo now;

    getTimeAndDate(&now);

    if(center) {
        pfunc = cprintf;
    } else {
        pfunc = printf;
    }

    pfunc("%02d/%02d/%04d @ %02d:%02d:%02d\n",
          now.month,now.date,now.year,
          now.hour,now.minute,now.second);

    return(0);
}

char *DateHelp[] = {
    "Display (mm/dd/yyyy@hh:mm:ss) or modify time and date.",
    "[{day date month year hour min sec}]",
#if INCLUDE_VERBOSEHELP
    "Where...",
    " day:   1-7 (sun=1)",
    " date:  1-31",
    " month: 1-12",
    " year:  full number (i.e. 2006)",
    " hour:  0-23",
    " min:   0-59",
    " sec:   0-59",
    "Note: 'date off' disables the oscillator (if HW supports this)",
#endif
    0
};

int
DateCmd(int argc,char *argv[])
{
    int     rngchk;
    struct  todinfo now;

    if(argc == 1) {
        showDate(0);
        return(CMD_SUCCESS);
    } else if((argc == 2) && !strcmp(argv[1],"off")) {
        if(setTimeAndDate(0) < 0) {
            printf("Clock can't be disabled\n");
            return(CMD_FAILURE);
        }
        return(CMD_SUCCESS);
    } else if(argc != 8) {
        return(CMD_PARAM_ERROR);
    }

    now.day = (char)atoi(argv[1]);
    now.date = (char)atoi(argv[2]);
    now.month = (char)atoi(argv[3]);
    now.year = atoi(argv[4]);
    now.hour = (char)atoi(argv[5]);
    now.minute = (char)atoi(argv[6]);
    now.second = (char)atoi(argv[7]);

    rngchk = 0;
    rngchk += rangecheck(now.day,"day",1,7);
    rngchk += rangecheck(now.date,"date",1,31);
    rngchk += rangecheck(now.month,"month",1,12);
    rngchk += rangecheck(now.year,"year",0,9999);
    rngchk += rangecheck(now.hour,"hour",0,23);
    rngchk += rangecheck(now.minute,"minute",0,59);
    rngchk += rangecheck(now.second,"second",0,59);

    if(rngchk != 7) {
        return(CMD_PARAM_ERROR);
    }

    setTimeAndDate(&now);
    return(CMD_SUCCESS);
}

int
timeofday(int cmd, void *arg)
{
    int rc;
    struct todinfo *now;

    rc = 0;
    now = (struct todinfo *)arg;

    switch(cmd) {
    case TOD_ON:
    case TOD_OFF:
        break;
    case TOD_SET:
        rc = setTimeAndDate(now);
        break;
    case TOD_GET:
        rc = getTimeAndDate(now);
        break;
    }
    return(rc);
}
