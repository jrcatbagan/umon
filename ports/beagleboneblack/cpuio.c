#include "config.h"
#include "stddefs.h"
#include "cpuio.h"
#include "genlib.h"
#include "cache.h"
#include "warmstart.h"
#include "timer.h"
#include "am335x.h"
#include "uart16550.h"

int
getUartDivisor(int baud, unsigned char *hi, unsigned char *lo)
{
	*lo = ((48000000/16)/baud) & 0x00ff;
	*hi = (((48000000/16)/baud) & 0xff00) >> 8;
	return(0);
}

/* devInit():
 * As a bare minimum, initialize the console UART here using the
 * incoming 'baud' value as the baud rate.
 */
int
devInit(int baud)
{
        return(0);
}

/* ConsoleBaudSet():
 * Provide a means to change the baud rate of the running
 * console interface.  If the incoming value is not a valid
 * baud rate, then default to 9600.
 * In the early stages of a new port this can be left empty.
 * Return 0 if successful; else -1.
 */
/*int
ConsoleBaudSet(int baud)
{
	// ADD_CODE_HERE 
	return(0);
}*/

/* target_console_empty():
 * Target-specific portion of flush_console() in chario.c.
 * This function returns 1 if there are no characters waiting to
 * be put out on the UART; else return 0 indicating that the UART
 * is still busy outputting characters from its FIFO.
 * In the early stages of a new port this can simply return 1.
 */
/*int
target_console_empty(void)
{
	// if (UART_OUTPUT_BUFFER_IS_EMPTY())  <- FIX CODE HERE
		return(0);
	return(1);
}*/

/* intsoff():
 * Disable all system interrupts here and return a value that can
 * be used by intsrestore() (later) to restore the interrupt state.
 */
ulong
intsoff(void)
{
	ulong status = 0;

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
//	flushDcache(0,0);
//	disableDcache();
//	invalidateIcache(0,0);
//	disableIcache();
	monrestart(INITIALIZE);
}

/* Override the default exception handlers provided by the AM335x
 * internal ROM code with uMon's custom exception handlers
 */
void
ram_vector_install(void)
{
	extern unsigned long abort_data;
	extern unsigned long abort_prefetch;
	extern unsigned long undefined_instruction;
	extern unsigned long software_interrupt;
	extern unsigned long interrupt_request;
	extern unsigned long fast_interrupt_request;
	extern unsigned long not_assigned;

	*(ulong **)0x4030ce24 = &undefined_instruction;
	*(ulong **)0x4030ce28 = &software_interrupt;
	*(ulong **)0x4030ce2c = &abort_prefetch;
	*(ulong **)0x4030ce30 = &abort_data;
	*(ulong **)0x4030ce34 = &not_assigned;
	*(ulong **)0x4030ce38 = &interrupt_request;
	*(ulong **)0x4030ce3c = &fast_interrupt_request;
}

void
pinMuxInit(void)
{
        // Set pin mux configuration for UART0 RX/TX pins
        CNTL_MODULE_REG(CONF_UART0_RXD) = SLEWSLOW | RX_ON |
                PULL_OFF | MUXMODE_0;
        CNTL_MODULE_REG(CONF_UART0_TXD) = SLEWSLOW | RX_OFF |
                PULL_OFF | MUXMODE_0;
}

/* If any CPU IO wasn't initialized in reset.S, do it here...
 * This just provides a "C-level" IO init opportunity. 
 */
void
initCPUio(void)
{
	ram_vector_install();

        // Enable the control module:
        CM_WKUP_REG(CM_WKUP_CONTROL_CLKCTRL) |= 2;

        // Enable clock for UART0:
        CM_WKUP_REG(CM_WKUP_UART0_CLKCTRL) |= 2;

        pinMuxInit();

        InitUART(DEFAULT_BAUD_RATE);

        // Set UART0 mode to 16x
        UART0_REG(UART_MDR1) &= ~7;
}
