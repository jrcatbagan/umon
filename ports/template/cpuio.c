#include "config.h"
#include "stddefs.h"
#include "cpuio.h"
#include "genlib.h"
#include "cache.h"
#include "warmstart.h"
#include "timer.h"

/* devInit():
 * As a bare minimum, initialize the console UART here using the
 * incoming 'baud' value as the baud rate.
 */
void
devInit(int baud)
{
	/* ADD_CODE_HERE */
}

/* ConsoleBaudSet():
 * Provide a means to change the baud rate of the running
 * console interface.  If the incoming value is not a valid
 * baud rate, then default to 9600.
 * In the early stages of a new port this can be left empty.
 * Return 0 if successful; else -1.
 */
int
ConsoleBaudSet(int baud)
{
	/* ADD_CODE_HERE */
	return(0);
}

/* target_console_empty():
 * Target-specific portion of flush_console() in chario.c.
 * This function returns 1 if there are no characters waiting to
 * be put out on the UART; else return 0 indicating that the UART
 * is still busy outputting characters from its FIFO.
 * In the early stages of a new port this can simply return 1.
 */
int
target_console_empty(void)
{
	/* if (UART_OUTPUT_BUFFER_IS_EMPTY())  <- FIX CODE HERE */
		return(0);
	return(1);
}

/* target_putchar():
 * When buffer has space available, load the incoming character
 * into the UART.
 */
int
target_putchar(char c)
{
    /* Wait for transmit ready bit */
    while(1) {
        /* if (UART_IS_READY_FOR_CHARACTER())	 <- FIX CODE HERE */
            break;
    }

	/* ADD_CODE_HERE
	 * Put character into UART buffer here
	 */

    return((int)c);
}

/* target_gotachar():
 * Return 0 if no char is avaialable at UART rcv fifo; else 1.
 * Do NOT pull character out of fifo, just return status. 
 *
 * Define INCLUDE_BLINKLED in config.h and add STATLED_ON()
 * and STATLED_OFF() macros (or functions)  so that uMon will
 * run and blink a target LED at a configured (or default 500msec)
 * interval...
 */
int
target_gotachar(void)
{
#if INCLUDE_BLINKLED
	static uint8_t ledstate;
	static struct elapsed_tmr tmr;

#ifndef BLINKON_MSEC
#define BLINKON_MSEC 500
#define BLINKOFF_MSEC 500
#endif

	switch(ledstate) {
		case 0:
			startElapsedTimer(&tmr,BLINKON_MSEC);
			ledstate = 1;
			STATLED_ON();
			break;
		case 1:
			if(msecElapsed(&tmr)) {
				STATLED_OFF();
				ledstate = 2;
				startElapsedTimer(&tmr,BLINKOFF_MSEC);
			}
			break;
		case 2:
			if(msecElapsed(&tmr)) {
				STATLED_ON();
				ledstate = 1;
				startElapsedTimer(&tmr,BLINKON_MSEC);
			}
			break;
	}
#endif
	/* if (UART_INPUT_BUFFER_IS_NOT_EMPTY())  <- FIX CODE HERE */
        return(1);
    return(0);
}

/* target_getchar():
 * Assume there is a character in the UART's input buffer and just
 * pull it out and return it.
 */
int 
target_getchar(void)
{
    char    c;

    /* c = GET_CHARACTER_FROM_UART();	 <- FIX CODE HERE */
    return((int)c);
}

/* intsoff():
 * Disable all system interrupts here and return a value that can
 * be used by intsrestore() (later) to restore the interrupt state.
 */
ulong
intsoff(void)
{
	ulong status;

	/* ADD_CODE_HERE */
	return(status);
}

/* intsrestore():
 * Re-establish system interrupts here by using the status value
 * that was returned by an earlier call to intsoff().
 */
void
intsrestore(ulong status)
{
	/* ADD_CODE_HERE */
}

/* cacheInitForTarget():
 * Establish target specific function pointers and
 * enable i-cache...
 * Refer to $core/cache.c for a description of the function pointers.
 * NOTE:
 * If cache (either I or D or both) is enabled, then it is important
 * that the appropriate cacheflush/invalidate function be established.
 * This is very important because programs (i.e. cpu instructions) are
 * transferred to memory using data memory accesses and could
 * potentially result in cache coherency problems.
 */
void
cacheInitForTarget(void)
{
	/* ADD_CODE_HERE */
}

/* target_reset():
 * The default (absolute minimum) action to be taken by this function
 * is to call monrestart(INITIALIZE).  It would be better if there was
 * some target-specific function that would really cause the target
 * to reset...
 */
void
target_reset(void)
{
	flushDcache(0,0);
	disableDcache();
	invalidateIcache(0,0);
	disableIcache();
	monrestart(INITIALIZE);
}

/* If any CPU IO wasn't initialized in reset.S, do it here...
 * This just provides a "C-level" IO init opportunity. 
 */
void
initCPUio(void)
{
	/* ADD_CODE_HERE */
}
