/* reset.s:
 *
 * First bit of boot code run by the processor.
 *
 */
    .file   "reset.s"

	.extern start
	.extern MonStack
	.global reset
	.global warmstart
	.global coldstart

#include "warmstart.h"

/* Depending on the CPU, these entries may have to be shuffled
 * around a bit.  The position of coldstart is very CPU specific
 * and the location of moncomptr should be in a place where it is
 * unlikely to be moved if other monitor functionality is changed.
 */

/*********************************************************************
 *
 * coldstart:
 * The reset point.
 */
reset:
coldstart:
	/* - Invalidate and disable caches
 	 * - Load INITIALIZE (defined in warmstart.h) into whereever it
	 *   needs to be put so that warmstart sees it as an argument
	 *   passed to it.
	 * 
	 * - Jump to warmstart
	 */
	
/*********************************************************************
 *
 * moncomptr:
 * Pointer to the moncom function, used to link application to monitor.
 * Refer to umon_main/target/core/moncomptr.S
 */
#include "moncomptr.S"

/*********************************************************************
 *
 * etheraddr:
 * Location that could be used to store a fixed MAC address.
 * Refer to umon_main/target/core/etheraddr.S.
 * NOTE:
 * This should only be included in flash-resident code, then
 * the address in flash should be accessible to ram-based versions of
 * uMon via tags in the linker map file.
 */
#include "etheraddr.S"

/*********************************************************************
 *
 * alttfsdevtbl.S:
 * Location that could be used to store an "alternate" TFS device
 * table for use by the "tfs cfg" command.
 * Refer to umon_main/target/core/alttfsdevtbl.S.
 * NOTE:
 * This should only be included in flash-resident code, then
 * the address in flash should be accessible to ram-based versions of
 * uMon via tags in the linker map file.
 */
#include "alttfsdevtbl.S"
	
/*********************************************************************
 *
 * warmstart:
 * A point callable by C, as warmstart(int type), where
 * 'type' is one of the values defined in warmstart.h.
 */
warmstart:
	/* Minimal CPU/IO Initialization (i.e. chip selects) here.
	 * Stay in assembler here if possible.
	 */
    
/*********************************************************************
 *
 * SPInit:
 * Establish a stack frame.
 */
SPInit:
	/* First initialize stack to point to MonStack+MONSTACKSIZE+64
	 * (an address that is outside the MonStack array), and then
	 * call stkinit().  This loads the MonStack[] array with a known
	 * pattern that allows uMon to later analyze the running stack usage.
	 */
    
	/* Next, re-initialize stack to point to MonStack+MONSTACKSIZE...
	 * This is important because other portions of the code
	 * assume this is where the stack resides.
	 */

/*********************************************************************
 *
 * gotoC:
 * This code jumps to the start() function in the monitor
 * and should never return.
 */
gotoC:
	/* - Retrieve the variable passed to warmstart and place it
	 *   whereever it needs to be so that the start() function in
	 *   'C' sees it as an argument.  Note that if FORCE_BSS_INIT
	 *   is defined, then start() will ignore this argument.
	 * - Branch to start().
	 */
