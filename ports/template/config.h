/* Template MicroMonitor configuration file.
 */
#define CPU_BE

/* PLATFORM_NAME:
 * Some string that briefly describes your target.
 */
#define PLATFORM_NAME       "Name of your Board here"

/* CPU_NAME:
 * The name of your CPU (i.e. "PowerPC 405", "Coldfire 5272" ,etc...). 
 */
#define CPU_NAME    "CPU_NAME_HERE"

/* LOOPS_PER_SECOND:
 * Approximately the size of a loop that will cause a 1-second delay.
 * This value can be adjusted by using the "sleep -c" command.
 */
#define LOOPS_PER_SECOND	23000

/* PRE_TFSAUTOBOOT_HOOK():
 * This is a port-specific function that will be called just prior to
 * uMon running any/each autobootable file (not including monrc).
#define PRE_TFSAUTOBOOT_HOOK()	func()
 */

/* PRE_COMMANDLOOP_HOOK():
 * This is a port-specific function that will be called just before
 * MicroMonitor turns over control to the endless command processing
 * loop in start() (start.c).
#define PRE_COMMANDLOOP_HOOK()	func()
 */

/* If a watchdog macro is needed, this is how you do it
 * (using target specific code in the macro of course)...
 * The remoteWatchDog() call is only needed if your appliation
 * will override the monitor's watchdog macro.  If that isn't
 * needed, then that logic can be omitted from the macro.

#define WATCHDOG_MACRO 										\
	{														\
		extern void (*remoteWatchDog)(void);				\
		if (remoteWatchDog) {								\
			remoteWatchdog()								\
		}													\
		else {												\
			*(unsigned long *)0xff000000 |= 0x00100000;		\
			*(unsigned long *)0xff000000 &= ~0x00100000;	\
		}
	}
 *
 */

/* If the ENET_LINK_IS_UP macro is defined as some function, then
 * that function will be called every half second for some number of
 * ticks.  The default tick count is set by the definition of
 * LINKUP_TICK_MAX also defined in that file.  This can be overridden
 * here if necessary.  Refer to the function EthernetWaitforLinkup() in
 * ethernet.c (umon_main/target/common/ethernet.c) for complete details.
 *
 * The function defined by ENET_LINK_IS_UP (shown below as phy_linkup())
 * is assumed to return 1 if the link is up; else 0. 
 *
#define ENET_LINK_IS_UP phy_linkup
#define LINKUP_TICK_MAX 10
 *
 * The purpose of this is to provide a common way to wait for the up-state
 * of the link prior to allowing other commands to execute from uMon at
 * startup.
 */


/* Flash bank configuration:
 * Basic information needed to configure the flash driver.
 * Fill in port specific values here.
 */
#define SINGLE_FLASH_DEVICE 	1
#define FLASH_COPY_TO_RAM 		1
#define FLASH_BANK0_BASE_ADDR  	0xFF800000
#define FLASH_PROTECT_RANGE  	"0-2"
#define FLASH_BANK0_WIDTH       2
#define FLASH_LARGEST_SECTOR    0x20000

/* If there is a need to have the range of protected sectors locked (and
 * the flash device driver supports it), then enable this macro...
#define LOCK_FLASH_PROTECT_RANGE
 */

/* TFS definitions:
 * Values that configure the flash space that is allocated to TFS.
 * Fill in port specific values here.
 */
#define TFSSPARESIZE    		FLASH_LARGEST_SECTOR
#define TFSSTART        		(FLASH_BANK0_BASE_ADDR+0x80000)
#define TFSEND          		0xFFFDFFFF
#define TFSSPARE        		(TFSEND+1)
#define TFSSECTORCOUNT			((TFSSPARE-TFSSTART)/0x20000)
#define TFS_EBIN_ELF    		1
#define TFS_VERBOSE_STARTUP		1
#define TFS_ALTDEVTBL_BASE		&alt_tfsdevtbl_base

/* FLASHRAM Parameters (not required):
 * Primarily used for configuring TFS on battery-backed RAM.
 * For a simple (volatile) RAM-based TFS device, use the
 * "tfs ramdev" command.
 *
 * Define a block of RAM space to be used as a TFS-device.
 * This is a block of RAM that TFS is fooled into treating like flash.
 * It essentially provides a RAM-based file-storage area.
 */
#define FLASHRAM_BASE			0x380000

#ifdef FLASHRAM_BASE
# define FLASHRAM_END			0x3fffff
# define FLASHRAM_SECTORSIZE	0x010000
# define FLASHRAM_SPARESIZE		FLASHRAM_SECTORSIZE
# define FLASHRAM_SECTORCOUNT	8
# define FLASHRAM_BANKNUM		1
# define FLASHBANKS				2
#else
# define FLASHBANKS				1
#endif

/* ALLOCSIZE:
 * Specify the size of the memory block (in monitor space) that
 * is to be allocated to malloc in the monitor.  Note that this
 * size can be dynamically increased using the heap command at
 * runtime.
 */
#define ALLOCSIZE 		(64*1024)

/* MONSTACKSIZE:
 * The amount of space allocated to the monitor's stack.
 */
#define MONSTACKSIZE	(8*1024)

/* INCLUDE_XXX Macros:
 * Set/clear the appropriate macros depending on what you want
 * to configure in your system.  The sanity of this list is tested
 * through the inclusion of "inc_check.h" at the bottom of this list...
 * When starting a port, set all but INCLUDE_MALLOC, INCLUDE_SHELLVARS,
 * INCLUDE_MEMCMDS and INCLUDE_XMODEM to zero.  Then in small steps
 * enable the following major features in the following order:
 *
 *	INCLUDE_FLASH to test the flash device drivers.
 *	INCLUDE_TFS* to overlay TFS onto the flash. 
 *  INCLUDE_ETHERNET, INCLUDE_TFTP to turn the ethernet interface.
 *
 * All other INCLUDE_XXX macros can be enabled as needed and for the
 * most part, they're hardware independent.
 * Note that for building a very small footprint, INCLUDE_MALLOC and
 * INCLUDE_SHELLVARS can be disabled.
 */
								
#define INCLUDE_MALLOC			1
#define INCLUDE_MEMCMDS         1
#define INCLUDE_SHELLVARS		1
#define INCLUDE_XMODEM          1
#define INCLUDE_EDIT            0
#define INCLUDE_DISASSEMBLER    0
#define INCLUDE_UNZIP           0
#define INCLUDE_ETHERNET        0
#define INCLUDE_ICMP			0
#define INCLUDE_TFTP            0
#define INCLUDE_TFS             0
#define INCLUDE_FLASH           0
#define INCLUDE_LINEEDIT        0
#define INCLUDE_DHCPBOOT        0
#define INCLUDE_TFSAPI          0
#define INCLUDE_TFSSYMTBL       0
#define INCLUDE_TFSSCRIPT       0
#define INCLUDE_TFSCLI          0
#define INCLUDE_EE              0
#define INCLUDE_GDB             0
#define INCLUDE_STRACE          0
#define INCLUDE_CAST            0
#define INCLUDE_STRUCT          0
#define INCLUDE_REDIRECT        0
#define INCLUDE_QUICKMEMCPY     0
#define INCLUDE_PROFILER        0
#define INCLUDE_BBC             0
#define INCLUDE_MEMTRACE        0
#define INCLUDE_STOREMAC        0
#define INCLUDE_VERBOSEHELP     0
#define INCLUDE_HWTMR	 	    0
#define INCLUDE_PORTCMD	 	    0
#define INCLUDE_USRLVL	 	    0

/* Some fine tuning (if needed)...
 * If these #defines are not in config.h, they default to '1' in
 * various other include files within uMon source; hence, they're
 * really only useful if you need to turn off ('0') the specific
 * facility or block of code.
 */
#define INCLUDE_TFTPSRVR		0
#define INCLUDE_ETHERVERBOSE	0
#define INCLUDE_MONCMD			0

#include "inc_check.h"
