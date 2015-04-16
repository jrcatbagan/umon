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
 * timestuff.c
 *
 * These functions support the monitor's ability to deal with elapsed
 * time on targets with or without hardware-timer support.
 *
 * The INCLUDE_HWTMR definition in config.h determines which mode
 * the timer will run in.
 *
 * The monitor does not require any hardware-assist for maintaining
 * elapsed time.  With no hardware assist (INCLUDE_HWTMR set to 0),
 * the value of LOOPS_PER_SECOND must be established in config.h, and
 * can be calibrated using the '-c' option in the sleep command.  Even
 * with this calibration the accuracy of this mechanism is limited;
 * however, since there is no code in the monitor that really requires
 * extremely accurate timing this may be ok.
 * 
 * On the other hand, it is preferrable to have accuracy, so on targets
 * that have a pollable clock,  the hooks can be put in place to allow
 * that clock to be used as the hardware assist for the monitor's 
 * elapsed time measurements.  The INCLUDE_HWTMR is set to 1, and 
 * TIMER_TICKS_PER_MSEC defines the number of ticks of the timer that
 * correspond to 1 millisecond of elapsed time.  The function target_timer() 
 * is assumed to be established and must return an unsigned long value
 * that is the content of the polled hardware timer.
 *
 * Regardless of the hardware-assist or not, the following interface is
 * used by the code in the monitor...
 *
 *	#include "timer.h"
 *
 *	struct elapsed_tmr tmr;
 *
 *	startElapsedTimer(&tmr,TIMEOUT):
 *	do {
 *		SOMETHING();
 *	} while(!msecElapsed(&tmr));
 *
 * Refer to the functions startElapsedTimer() and msecElapsed() below for
 * more details.
 * 
 * Original author:     Ed Sutter (ed.sutter@alcatel-lucent.com)
 *
 */

#include "config.h"
#include "stddefs.h"
#include "cli.h"
#include "genlib.h"
#include "ether.h"
#include "timer.h"
#include "cpuio.h"

/* startElapsedTimer() & msecElapsed():
 * The timer is started by loading the values timeout_low and timeout_high
 * with the number of ticks that must elapse for the timer to expire.
 *
 * In the case of the non-hardware-assisted timer, the expiration count
 * is based on the value of LoopsPerMillisecond (derived from the 
 * LOOPS_PER_SECOND definition in config.h).  Each time msecElapsed()
 * is called the elapsed count is incremented until it exceeds the timeout
 * timeout_low timeout_high values recorded by startElapsedTimer().
 * 
 * The case of the hardware-assisted timer is similar except that now
 * the number of ticks initialized in timeout_low and timeout_high are
 * based on the tick rate of the hardware timer (TIMER_TICKS_PER_MSEC).
 * This value is expected to be set in config.h.  Each time msecElapsed()
 * is called, it samples the timer and adds to the running total of ticks
 * until it matches or exceeds the timeout_low and timeout_high values.
 *
 * Notice that 64-bit values are used (high & low) because a 32-bit value
 * isn't large enough to deal with the tick rates (per second) of various
 * CPUs. 
 */
void
startElapsedTimer(struct elapsed_tmr *tmr, long milliseconds)
{
	unsigned long new_tm_low;
	unsigned long stepmsecs, stepticks, remainder;

#if INCLUDE_HWTMR
	tmr->tmrflags = HWTMR_ENABLED;
	tmr->tpm = (unsigned long)TIMER_TICKS_PER_MSEC;
#else
	tmr->tmrflags = 0;
	tmr->tpm = (unsigned long)LoopsPerMillisecond;
#endif
	
	tmr->elapsed_low = 0;
	tmr->elapsed_high = 0;
	tmr->timeout_high = 0;
	tmr->timeout_low = 0;
	
	/* Convert incoming timeout from a millisecond count to a
	 * tick count...
	 * Maximum number of milliseconds and ticks before 32-bit
	 * (tick counter) unsigned long overlaps
	 */
	stepmsecs = 0xffffffff / tmr->tpm;
	stepticks = stepmsecs * tmr->tpm;
	remainder = (milliseconds % stepmsecs);

	/* Take care of the step remainder
	 */
	tmr->timeout_low = remainder * tmr->tpm;
	milliseconds -= remainder;

	for (;milliseconds; milliseconds -= stepmsecs) {
		new_tm_low = tmr->timeout_low + stepticks;

		if (new_tm_low < tmr->timeout_low)
			tmr->timeout_high++;
		tmr->timeout_low = new_tm_low;
	}
	
#if INCLUDE_HWTMR
	tmr->tmrval = target_timer();
#else
	tmr->tmrval = 0;
#endif

}

int
msecElapsed(struct elapsed_tmr *tmr)
{
	ulong new_elapsed_low, new_tmrval, elapsed;

	/* If timeout has already occurred, then we can assume that this
	 * function being called without a matching startElapsedTimer() call.
	 */
	if (ELAPSED_TIMEOUT(tmr))
		return(1);

#if INCLUDE_HWTMR
	new_tmrval = target_timer();
#else
	new_tmrval = tmr->tmrval + 1;
#endif

	/* Record how many ticks elapsed since the last call to msecElapsed
	 * and add that value to the total number of ticks that have elapsed.
	 */
	elapsed = new_tmrval - tmr->tmrval;
	new_elapsed_low = tmr->elapsed_low + elapsed;

	if (new_elapsed_low < tmr->elapsed_low)
		tmr->elapsed_high++;

	/* If the total elapsed number of ticks exceeds the timeout number
	 * of ticks, then we can return 1 to indicate that the requested
	 * amount of time has elapsed.  Otherwise, we record the values and
	 * return 0.
	 */
	if ((tmr->elapsed_high >= tmr->timeout_high) &&
		(new_elapsed_low >= tmr->timeout_low)) {
		tmr->tmrflags |= TIMEOUT_OCCURRED;
		return(1);
	}
	
	tmr->tmrval = new_tmrval;
	tmr->elapsed_low = new_elapsed_low;
	return(0);
}

/* msecRemainging():
 * Used to query how many milliseconds were left (if any) in the timeout.
 */
ulong
msecRemaining(struct elapsed_tmr *tmr)
{
	ulong high, low, msectot, leftover, divisor;

	if (ELAPSED_TIMEOUT(tmr))
		return(0);
	
	high = tmr->timeout_high - tmr->elapsed_high;
	low = tmr->timeout_low - tmr->elapsed_low;

	msectot = leftover = 0;

#if INCLUDE_HWTMR
	divisor = (ulong)TIMER_TICKS_PER_MSEC;
#else
	divisor = (ulong)LoopsPerMillisecond;
#endif

	while(1) {
		while (low > divisor) {
			msectot++;
			low -= divisor;
		}
		leftover += low;
		if (high == 0)
			break;
		else {
			high--;
			low = 0xffffffff;
		}
	} 

	while(leftover > divisor) {
		msectot++;
		low -= divisor;
	}
	return(msectot);
}

/* monDelay():
 * Delay for specified number of milliseconds.
 * Refer to msecElapsed() description for a discussion on the
 * accuracy of this delay.
 */
void
monDelay(int milliseconds)
{
	struct elapsed_tmr tmr;

	startElapsedTimer(&tmr,milliseconds);
	while(!msecElapsed(&tmr)) {
		WATCHDOG_MACRO;
		pollethernet();
	}
}

/* monTimer():
 * Provide the API with the ability to start a millisecond-granularity
 * timer with some countdown value, and poll it waiting for completion.
 */
int
monTimer(int cmd, void *arg)
{
	int rc = 0;
	struct elapsed_tmr *tmr = (struct elapsed_tmr *)arg;

	switch(cmd) {
		case TIMER_START:
			startElapsedTimer(tmr,tmr->start);
			break;
		case TIMER_ELAPSED:
			msecElapsed(tmr);
			if (ELAPSED_TIMEOUT(tmr))
				rc = 1;
			break;
		case TIMER_QUERY:
#if INCLUDE_HWTMR
			tmr->tmrflags = HWTMR_ENABLED;
			tmr->tpm = (unsigned long)TIMER_TICKS_PER_MSEC;
			tmr->currenttmrval = target_timer();
#else
			tmr->tmrflags = 0;
			tmr->tpm = (unsigned long)LoopsPerMillisecond;
			tmr->currenttmrval = 0;
#endif
			break;
		default:
			rc = -1;
			break;
	}
	return(rc);
}

#if INCLUDE_TFSSCRIPT

/* Sleep():
 *	Simple delay loop accessible by the command line.
 *	This loop count is dependent on the underlying hardware.
 *	The LoopsPerMillisecond count is loaded with a default at startup, or
 *  it can be calibrated by the user via the -c option.
 *	Note that this LoopsPerMillisecond value is used in a few other places
 *	in the monitor also for time-dependent stuff.
 *	NOTES:
 *	- This is obviously not real accurate (not intended to be), but allows the
 *	  monitor to be independent of the underlying hardware.
 *	- The delay time is very dependent on ethernet activity, since the call
 *	  to pollethernet() part of the loop.
 */


char *SleepHelp[] = {
	"Second or msec delay (not precise)",
	"-[clmv:] {count}",
#if INCLUDE_VERBOSEHELP
	" -c  calibrate new LPS count",
	" -l  store LPS count",
	" -m  millisecond",
	" -v {LPSvarname}",
#endif
	0,
};

int
Sleep(int argc,char *argv[])
{
	int opt, calibrate, count, multiplier;

	multiplier = 1000;
	calibrate = 0;
	while ((opt=getopt(argc,argv,"clmv:")) != -1) {
		switch(opt) {
		case 'c':
			calibrate = 2;
			break;
		case 'l':
			calibrate = 1;
			break;
		case 'm':
			multiplier = 1;
			break;
		case 'v':
			shell_sprintf(optarg,"%d",LoopsPerMillisecond*1000);
			return(CMD_SUCCESS);
		default:
			return(CMD_PARAM_ERROR);
		}
	}

	/* If no args, just print the current LPS value and return... */
	if (argc == 1) {
#if INCLUDE_HWTMR
		printf("Hardware-based timer, LPS not applicable\n");
#else
		printf("Current LPS = %ld\n",LoopsPerMillisecond * 1000);
#endif
		return(CMD_SUCCESS);
	}

	/* For calibration, take in the count on the command line, then use
	 * it to put out 5 dots dot at the rate of the loop to allow the user
	 * to adjust it to be about 1 second.
	 */
	if (calibrate) {
#if INCLUDE_HWTMR
		printf("Hardware-based timer, doesn't calibrate\n");
#else
		long lps;

		if (argc != optind+1)
			return(CMD_PARAM_ERROR);

		printf("Current LPS: %ld\n",LoopsPerMillisecond * 1000);
		lps = strtol(argv[optind],0,0);
		LoopsPerMillisecond = lps/1000;
		printf("New LPS: %ld%s\n",LoopsPerMillisecond * 1000,
			lps % 1000 ? " (truncated by 1000)" : "");

		if (calibrate == 2) {
			count = 10;
			while(count-- > 0) {
				monDelay(1000);
				putstr(".\007");
			}
			putchar('\n');
		}
#endif
		return(CMD_SUCCESS);
	}

	if (argc == optind)
		count = 1;
	else
		count = strtol(argv[optind],(char **)0,0);

	monDelay(count * multiplier);
	return(CMD_SUCCESS);
}
#endif
