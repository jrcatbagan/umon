/* except_template.c:
 *	This code handles exceptions that are caught by the exception vectors
 *	that have been installed by the monitor through vinit().  It is likely
 *	that there is already a version of this function available for the CPU
 *	being ported to, so check the cpu directory prior to porting this to a
 *	new target.
 *
 *	General notice:
 *	This code is part of a boot-monitor package developed as a generic base
 *	platform for embedded system designs.  As such, it is likely to be
 *	distributed to various projects beyond the control of the original
 *	author.  Please notify the author of any enhancements made or bugs found
 *	so that all may benefit from the changes.  In addition, notification back
 *	to the author will allow the new user to pick up changes that may have
 *	been made by other users after this version of the code was distributed.
 *
 *	Author:	Ed Sutter
 *	email:	esutter@lucent.com
 *	phone:	908-582-2351		
 */
#include "config.h"
#include "cpu.h"
#include "cpuio.h"
#include "genlib.h"
#include "stddefs.h"
#include "warmstart.h"

ulong	ExceptionAddr;
int		ExceptionType;

/* exception():
 * This is the first 'C' function called out of a monitor-installed
 * exception handler. 
 */
void
exception(void)
{
	/* ADD_CODE_HERE */

	/* Populating these two values is target specific.
	 * Refer to other target-specific examples for details.
 	 * In some cases, these values are extracted from registers
	 * already put into the register cache by the lower-level
	 * portion of the exception handler in vectors_template.s
	 */
	ExceptionAddr = 0;
	ExceptionType = 0;

	/* Allow the console uart fifo to empty...
	 */
	flush_console_out();
	monrestart(EXCEPTION);
}

/* vinit():
 * This function is called by init1() at startup of the monitor to
 * install the monitor-based vector table.  The actual functions are
 * in vectors.s.
 */
void
vinit()
{
	/* ADD_CODE_HERE */
}

/* ExceptionType2String():
 * This function simply returns a string that verbosely describes
 * the incoming exception type (vector number).
 */
char *
ExceptionType2String(int type)
{
	char *string;

	/* ADD_CODE_HERE */
	return(string);
}

