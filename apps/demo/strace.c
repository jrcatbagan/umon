/* strace.c:
 * This file provides a simple, self-induced exception that can
 * be caught by MicroMonitor.  After catching the exception, the
 * monitor will not attempt any automatic restart because the
 * NO_EXCEPTION_RESTART shell variable is set at the top of
 * strace_demo() below (typically this environment variable would
 * be set during development in the monrc file).
 * 
 * After the exception has completed, at the uMON> prompt type the
 * "strace" command.  Assuming your monitor is built with INCLUDE_STRACE
 * set (refer to config.h for your monitor's port), then the output
 * should be something like this...
 *
 *  1:     uMON>app strace_demo
 *  2:     strace_demo
 *  3:     func1
 *  4:     func2
 *  5:     func3 exception now!
 *  6:
 *  7:     EXCEPTION: 'Syscall'
 *  8:                Occurred near 0xa0051240 (within func3)
 *  9:
 * 10:     uMON>strace
 * 11:        0xa0051240: func3() + 0x1c
 * 12:        0xa0051284: func2() + 0x28
 * 13:        0xa00512b4: func1() + 0x1c
 * 14:        0xa00512f4: strace_demo() + 0x30
 * 15:        0xa00511d0: main() + 0x1b4
 * 16:        0xa005120c: Cstart() + 0x34
 * 17:
 * 18:     uMON>
 *
 * Line 1      : shows the invocation of the application 'app'.
 * Lines 2-5   : show the output of the strace.c mon_printf calls. 
 * Lines 7-8   : show uMon catching the exception.
 * Line 10     : shows the invocation of the 'strace' command.
 * Lines 11-16 : show the output of strace giving the user the exact
 *               call-stack that caused the exception.

 * Note1:
 * The strace facility (INCLUDE_STRACE in config.h) in the
 * monitor requires that the symbol table facility (INCLUDE_SYMTBL in
 * the config.h file) also be enabled.
 * Note2:
 * Depending on the version of GCC you're using, you may have to specify
 * -fno-omit-frame-pointer on the command line; otherwise the stack trace
 * function will not work properly.
 */
#include "monlib.h"

#if CPU_IS_MIPS
#define EXCEPTION()		asm("syscall 99");	/* System call */
#elif CPU_IS_68K
#define EXCEPTION()		asm("trap #3");		/* Trap */
#elif CPU_IS_SH
#define EXCEPTION()		asm("trap #3");		/* Trap */
#elif CPU_IS_BFIN
#define EXCEPTION()		asm("excpt 5");		/* Force exception */
#elif CPU_IS_MICROBLAZE
#define EXCEPTION()		asm("bralid r15,8");	/* User exception */
#elif CPU_IS_PPC
#define EXCEPTION()		asm("sc");			/* System call */
#elif CPU_IS_ARM
#define EXCEPTION()		asm("swi");			/* Software interrupt */
#else
#error: Must specify CPU type for exception demo.
#endif
 
int
func3(int i)
{
	mon_printf("func3 exception now!\n");
	EXCEPTION();
	return(i+5);
}

int
func2(int i)
{
	mon_printf("func2\n");
	func3(i+3);
	return(99);
}

int
func1(void)
{
	mon_printf("func1\n");
	func2(3);
	return(88);
}

int
strace_demo(void)
{
	mon_printf("strace_demo\n");
	mon_setenv("NO_EXCEPTION_RESTART","TRUE");
	func1();
	return(77);
}
