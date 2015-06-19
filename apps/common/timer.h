#ifndef _TIMER_H_
#define _TIMER_H_

struct elapsed_tmr {
	unsigned long start;			// Start value of elapsed timeout.
	unsigned long tmrval;			// Running timer value
									// (used by elapsed timeout)
	unsigned long currenttmrval;	// Current timer value
									// (not used by elapsed timeout)
	unsigned long tpm;				// Ticks per millisecond 

	unsigned long elapsed_low;
	unsigned long elapsed_high;

	unsigned long timeout_low;
	unsigned long timeout_high;

	unsigned long tmrflags;
};

/* Timer flags:
 */
#define HWTMR_ENABLED		(1 << 0)
#define TIMEOUT_OCCURRED	(1 << 1)

/* Timer macros:
 */
#define HWRTMR_IS_ENABLED(tmr)			\
	((tmr)->tmrflags & HWTMR_ENABLED)	

#define ELAPSED_TIMEOUT(tmr)			\
	((tmr)->tmrflags & TIMEOUT_OCCURRED)

/* uMon API timer commands:
 */
#define TIMER_START		1
#define TIMER_ELAPSED	2
#define TIMER_QUERY		3

extern unsigned long target_timer(void);
extern void startElapsedTimer(struct elapsed_tmr *tmr,long timeout);
extern int msecElapsed(struct elapsed_tmr *tmr);
extern unsigned long msecRemaining(struct elapsed_tmr *tmr);
extern int monTimer(int cmd, void *arg);

#endif
