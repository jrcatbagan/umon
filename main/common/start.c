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
 * start.c:
 *
 * This is typically the first 'C' code executed by the processor after a
 * reset.
 * 
 * Original author:     Ed Sutter (ed.sutter@alcatel-lucent.com)
 *
 */
#include "config.h"
#include "cpu.h"
#include "cpuio.h"
#include "stddefs.h"
#include "genlib.h"
#include "devices.h"
#include "ether.h"
#include "warmstart.h"
#include "monlib.h"
#include "monflags.h"
#include "boardinfo.h"
#include "cli.h"
#include "tfsprivate.h"
#include "fbi.h"

#ifdef PRE_COMMANDLOOP_HOOK
extern void PRE_COMMANDLOOP_HOOK();
#endif

#ifndef EXCEPTION_HEADING
#define EXCEPTION_HEADING "EXCEPTION"
#endif

extern	void vinit(void);
extern	int addcommand(struct monCommand *cmdlist, char *cmdlvl);

/* StateOfMonitor:
 * The StateOfMonitor variable is used throughout the code to determine
 * how the monitor started up.  In general, the three most common 
 * startup types are: INITIALIZE, APP_EXIT and EXCEPTION.
 */
int		StateOfMonitor;

/* MonStack:
 * The monitor's stack is declared within the monitor's own .bss space.
 * This keeps the memory map simple, the only thing that needs to be
 * accounted for is that in the bss init loop, this section should not
 * be initialized.  MONSTACKSIZE must be defined in config.h.  
 */
ulong	MonStack[MONSTACKSIZE/4];

/* APPLICATION_RAMSTART:
 * Loaded with the address after which the application can be loaded.
 */
ulong	APPLICATION_RAMSTART;

/* BOOTROM_BASE:
 * Loaded with the address considered to be the base address at which
 * the monitor is burned into flash.
 */
ulong	BOOTROM_BASE;

/* LoopsPerMillisecond:
 * Loaded with a count (established in config.h) that is an estimation of
 * one second of elapsed time.  It is NOT an accurate value, but serves the
 * purpose of providing a hardware-independent mechanism for establishing 
 * a reasonable estimation of one second's worth of elapsed time.
 */
ulong	LoopsPerMillisecond;

/* init0():
 * The code in this function was originally part of start().  It has been
 * moved out so that this portion of the initialization can be re-used by
 * AppWarmStart().  See notes in AppWarmStart() for more details on this.
 */
void
init0(void)
{
    /* Store the base address of memory that is used by application.
     * Start it off on the next 4K boundary after the end of monitor ram.
	 * Usually APPRAMBASE and BOOTROMBASE can be retrieved from bss_end and
	 * boot_base; but they can be overridden with values specified in the
	 * target-specific config.h file.
	 */
#ifdef APPRAMBASE_OVERRIDE
	APPLICATION_RAMSTART = APPRAMBASE_OVERRIDE;
#else
	APPLICATION_RAMSTART = (((ulong)&bss_end) | 0xfff) + 1;
#endif

#ifdef BOOTROMBASE_OVERRIDE
	BOOTROM_BASE = BOOTROMBASE_OVERRIDE;
#else
	BOOTROM_BASE = (ulong)&boot_base;
#endif

	/* Load bottom of stack with 0xdeaddead to be used by stkchk().
	 */
	MonStack[0] = 0xdeaddead;

	/* Set up default loops-per-millisecond count.
	 */
	LoopsPerMillisecond = LOOPS_PER_SECOND/1000;

	/* Clear any user-installed command list.
	 */
	addcommand(0,0);
}

/* init1():
 * This is the first level of initialization (aside from the most fundamental
 * stuff that must be done in reset.s) that is done after the system resets.
 */
void
init1(void)
{
	initCPUio();		/* Configure all chip-selects, parallel IO
						 * direction registers, etc...  This may have
						 * been done already in reset.s
						 */
	vinit();			/* Initialize the CPU's vector table. */
	InitRemoteIO();		/* Initialize all application-settable IO functions */

	if (ConsoleBaudRate == 0)
		ConsoleBaudRate = DEFAULT_BAUD_RATE;

	devInit(ConsoleBaudRate);

}

/* init2():
 */
void
init2(void)
{
#if INCLUDE_FLASH | INCLUDE_TFS
	int rc = 0;
#endif

#if INCLUDE_FBI
	if (StateOfMonitor == INITIALIZE)
		fbdev_init();	/* Initialize the frame-buffer device */
#endif

#if INCLUDE_FLASH
	if (StateOfMonitor == INITIALIZE)
    	rc = FlashInit();	/* Init flashop data structures and (possibly) */
							/* the relocatable functions.  This MUST be */
#endif						/* done prior to turning on cache!!! */

    cacheInit();        /* Initialize cache. */

#if INCLUDE_ETHERNET
    enreset();          /* Clear the ethernet interface. */
#endif

#if INCLUDE_TFS
	if (rc != -1)		/* Start up TFS as long as flash */
	    tfsstartup();	/* initialization didn't fail. */
#endif
}

/* init3():
 *	As each target boots up, it calls init1() and init2() to do multiple
 *	phases of hardware initialization.  This third initialization phase
 *	is always common to all targets...
 *  It is possible that this is being called by AppWarmStart().  In this
 *	case, the user can specify which portions of the monitor code are
 *	to be initialized by specifying them in the startmode mask.
 */
static void
_init3(ulong startmode)
{
	char *baud;

#if INCLUDE_LINEEDIT
	/* Initialize command line history table. */
	historyinit();
#endif

#if INCLUDE_SHELLVARS
	/* Init shell variable table. */
	ShellVarInit();
#endif

#if INCLUDE_USRLVL
    /* Set user level to its max, then allow monrc file to adjust it. */
    initUsrLvl(MAXUSRLEVEL);
#endif

#if INCLUDE_BOARDINFO
	/* Check for board-specific-information in some otherwise unused
	 * sector.  Also, establish shell variables from this information
	 * based on the content of the boardinfo structure.
	 * Note that the second half of the board-specific information setup
	 * (the call to BoardInfoEnvInit()) must be done AFTER ShellVarInit()
	 * because shell variables are established here.
	 */
	if (startmode & WARMSTART_BOARDINFO) {
		BoardInfoInit();
		BoardInfoEnvInit();
	}
#endif

#ifdef MONCOMPTR
	/* If MONCOMPTR is defined, then verify that the definition matches
	 * the location of the actual moncomptr value.  The definition in
	 * config.h is provided so that outside applications can include that
	 * header file so that the value passed into monConnect() is defined
	 * formally. The REAL value of moncomptr is based on the location of
	 * the pointer in monitor memory space, determined only after the final
	 * link has been done.  As a result, this check is used to simply verify
	 * that the definition matches the actual value.
	 */
	{
	if (MONCOMPTR != (ulong)&moncomptr)
		printf("\nMONCOMPTR WARNING: runtime != definition\n"); 
	}
#endif

#if INCLUDE_TFS
	/* After basic initialization, if the monitor's run-control file
	 * exists, run it prior to EthernetStartup() so that the
	 * MAC/IP addresses can be configured based on shell variables
	 * that would be loaded by the rc file.
	 */
	if (startmode & WARMSTART_RUNMONRC)
		tfsrunrcfile();
#endif

	/* After MONRC is run, establish monitor flags...
	 */
	InitMonitorFlags();

	/* We've run the monrc file at this point, so check for the
	 * presence of the CONSOLEBAUD shell variable.  If it's set,
	 * then use the value to re-establish the console baud rate.
	 * If it isn't set, then establish the CONSOLEBAUD shell variable
	 * using the default, pre-configured baud rate.
	 */
	baud = getenv("CONSOLEBAUD");
	if (baud) 
		ChangeConsoleBaudrate(atoi(baud));
	ConsoleBaudEnvSet();

#if INCLUDE_ETHERNET
#if INCLUDE_STOREMAC
	storeMac(0);
#endif
	if (startmode & WARMSTART_IOINIT)
		EthernetStartup(0,1);
#endif

	/* Now that all has been initialized, display the monitor header. */
#ifndef NO_UMON_STARTUP_HDR
	if (startmode & WARMSTART_MONHEADER) {
		if (!MFLAGS_NOMONHEADER())
		    monHeader(1);
	}
#endif

#if INCLUDE_FBI
	if (StateOfMonitor == INITIALIZE)
		fbi_splash();
#endif

#if INCLUDE_TFS
	if (startmode & WARMSTART_TFSAUTOBOOT)
		tfsrunboot();
#endif
}

void
init3(void)
{
	_init3(WARMSTART_ALL);
}

/* umonBssInit():
 * All of the monitor-owned ram is within it's bss space as defined
 * by the locations of bss_start and bss_end in the memory map
 * definition file.  This includes the the monitor's stack and
 * heap.  To support a generic, C-based BSS initialzation loop,
 * this loop must skip over the space allocated to the stack because
 * the stack is likely to be in use during this loop...
 */
void
umonBssInit(void)
{
	int *tmp;
	volatile ulong *bssptr;

	tmp = &bss_start;
	bssptr = (ulong *)tmp;
	while(bssptr < MonStack)
		*bssptr++ = 0;
	bssptr = (ulong *)MonStack+(MONSTACKSIZE/4);
	tmp = &bss_end;
	while(bssptr < (ulong *)tmp)
		*bssptr++ = 0;
}

/* AppWarmStart():
 * This function is provided to allow the application to partially
 * restart the monitor.  It supports the situation where an application
 * run but the monitor has not already been initialized.  This would be
 * the case if some type of debugger (JTAG or BDM probably) was attached
 * to the target and it was used to download and run the application.
 * If this type of debug connection does not support the ability to run
 * after the monitor runs, then the application must be able to somehow
 * get the monitor initialized without having it go through a complete
 * warmstart.
 * This will only run through the stack space provided by the application.
 */
void
AppWarmStart(ulong mask)
{
	/* First initialize monitor bss space (skipping over MonStack[]): */
	if (mask & WARMSTART_BSSINIT) 
		umonBssInit();

	StateOfMonitor = INITIALIZE;

	/* Initialize some of the real fundamental stuff regardless of
	 * the incoming mask.
	 */
	init0();

	/* Subset of init1(): */
	if (mask & WARMSTART_IOINIT) {
    	initCPUio();
		InitRemoteIO();
		if (ConsoleBaudRate == 0)
			ConsoleBaudRate = DEFAULT_BAUD_RATE;
    	devInit(ConsoleBaudRate);
	}
	init2();
	_init3(mask);
}

/* start():
 * Called at the end of reset.s as the first C function after processor
 * bootup.  It is passed a state that is used to determine whether or not
 * the CPU is restarting as a result of a warmstart or a coldstart.  If
 * the restart is a coldstart, then state will be INITIALIZE.  if the 
 * restart is a warmstart, then there are a few typical values of state,
 * the most common of which are APP_EXIT and EXCEPTION.
 *
 * The bss_start and bss_end variables are usually defined in the
 * target-specific linker memory-map definition file.  They symbolically
 * identify the beginning and end of the .bss section.  Many compilers
 * support intrinsic tags for this; however, since it is apparently not
 * consistent from one toolset to the next; I chose to make up my own
 * tags so that this file never changes from one toolset to the next.
 *
 * The FORCE_BSS_INIT definition can be established in the target-specific
 * config.h file to force .bss space initialization regardless of warm/cold
 * start.
 */
void
start(int state)
{
    char    buf[48];

#ifdef FORCE_BSS_INIT
	state = INITIALIZE;
#endif

	/* Based on the incoming value of 'state' we may or may not initialize
	 * monitor-owned ram.  Ideally, we only want to initialize
	 * monitor-owned ram when 'state' is INITIALIZE (power-up or reset);
	 * however, to support the case where the incoming state variable may
	 * be corrupted, we also initialize monitor-owned ram when 'state'
	 * is anything unexpected...
	 */
	switch(state) {
		case EXCEPTION:
		case APP_EXIT:
			break;
		case INITIALIZE:
		default:
			umonBssInit();
			break;
	}

	/* Now that the BSS clear loop has been done, we can copy the
	 * value of state (either a register or on stack) to the global
	 * variable (in BSS) StateOfMonitor...
	 */
	StateOfMonitor = state;

	/* Step through different phases of startup...
	 */
	init0();
	init1();
	init2();

	/* Depending on the type of startup, alert the console and do
	 * further initialization as needed...
	 */
    switch(StateOfMonitor) {
	    case INITIALIZE:
	        reginit();     
	        init3();       
	        break;
	    case APP_EXIT:
			EthernetStartup(0,0);  
			if (!MFLAGS_NOEXITSTATUS()) {
	        	printf("\nApplication Exit Status: %d (0x%x)\n",
					AppExitStatus,AppExitStatus);
			}
	        break;
	    case EXCEPTION:
			EthernetStartup(0,0);  
	        printf("\n%s: '%s'\n",EXCEPTION_HEADING,
				ExceptionType2String(ExceptionType));
	        printf("           Occurred near 0x%lx",ExceptionAddr);
	        if (AddrToSym(-1,ExceptionAddr,buf,0))
	            printf(" (within %s)",buf);
	        printf("\n\n");
	        exceptionAutoRestart(INITIALIZE);
	        break;
	    default:
	        printf("Unexpected monitor state: 0x%x (sp @ 0x%x)\n",
				StateOfMonitor, buf);
			/* To attempt to recover from the bad state, just do
			 * what INITIALIZE would do...
			 */
	        reginit();     
	        init3();       
	        break;
    }

#ifdef LOCK_FLASH_PROTECT_RANGE
	/* Issue the command that will cause the range of sectors
	 * designated by FLASH_PROTECT_RANGE to be locked.  This only
	 * works if the flash device is capable of being locked.
	 */
	sprintf(buf,"flash lock %s",FLASH_PROTECT_RANGE);
	docommand(buf,0);
#endif

#ifdef PRE_COMMANDLOOP_HOOK
	PRE_COMMANDLOOP_HOOK();
#endif

    /* Enter the endless loop of command processing: */
    CommandLoop();

	printf("ERROR: CommandLoop() returned\n");
	monrestart(INITIALIZE);
}

/* __eabi():
 * Called intrinsically by main for some PowerPc builds.
 * I'm not sure what the purpose of it is, but EABI is Embedded Application
 * Binary Interface, and is documented on Motorola's web site at
 *    http://www.mcu.motsps.com/lit/manuals/eabispec.html#536421
 */
void
__eabi(void)
{
}

/* __main() or __gccmain(): called intrinsically by main.  One or the other
 * is typically used to run constructors and destructors for C++.  If this
 * function is not created here, then the compiler will automatically create
 * one and with it, will come other unnecessary baggage.
 */
void
__main(void)
{
}

void
__gccmain(void)
{
}
