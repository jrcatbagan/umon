/*
 * Monitor configuration file for the Beaglebone Black
 *
 * Adapted by Jarielle Catbagan
 * email: jcatbagan93@gmail.com
 *
 *
 * General notice:
 * This code is part of a boot-monitor package developed as a generic base
 * platform for embedded system designs.  As such, it is likely to be
 * distributed to various projects beyond the control of the original
 * author.  Please notify the author of any enhancements made or bugs found
 * so that all may benefit from the changes.  In addition, notification back
 * to the author will allow the new user to pick up changes that may have
 * been made by other users after this version of the code was distributed.
 *
 * Author:  Ed Sutter
 * email:   esutter@lucent.com
 * phone:   908-582-2351
 */

#include "am335x.h"

/*
 * The target_putchar() function also drops the character at the
 * LCD...
 */
//#define MORE_PUTCHAR lcd_putchar
//#define CONSOLE_UART_BASE (OMAP35XX_L4_IO_BASE+0x6C000)
#define CONSOLE_UART_BASE UART0_BASE

#define SIO_STEP 4
#define IEN_DEFAULT 0x40
#define MCTL_DEFAULT 0x01

#define TIMER_TICKS_PER_MSEC	545000

/* DEFAULT_ETHERADD & DEFAULT_IPADD:
 * Refer to notes in ethernet.c function EthernetStartup() for details
 * regarding the use of these definitions.
 * DEFAULT_IPADD could be set to "DHCP" or "BOOTP" as well.
 */
#define	DEFAULT_ETHERADD "00:30:23:40:00:"  	// Cogent Block
#define DEFAULT_IPADD    "192.168.254.110"		

#define CPU_LE

// Establish a user defined function to be called when uMon
// prints out the startup banner...
// If this is defined, then the output similar to the following will
// be printed just above the uMon header...
//		Silicon ID: 1.0
//		CPU Rev: 2, Variant: 1
//		CM Rev: 1.0, PRM Rev: 1.0
// #define USR_HEADER_FUNC show_revision

/* Defining DONT_CENTER_MONHEADER eliminates the automatic centering
 * of the monitor's startup banner...
 */
#define DONT_CENTER_MONHEADER

/* XBUFCNT & RBUFCNT:
 *  Number of transmit and receive buffers allocated to ethernet.
 *  The total of XBUFCNT+RBUFCNT should not exceed MAXEBDS
 */
#define XBUFCNT 	8
#define RBUFCNT 	8
#define XBUFSIZE	2048
#define RBUFSIZE	2048

/* LOOPS_PER_SECOND:
 * Approximately the size of a loop that will cause a 1-second delay.
 * This can be guestimated or modified with the sleep -c command at the
 * monitor command line.
 */
#define LOOPS_PER_SECOND    15000

#define INCLUDE_NANDCMD			0

#if INCLUDE_NANDCMD
/* Needed for NAND to work with TFSRAM:
 */
#define NAND_TFS_BASE			0x10000		// base of TFS in NAND

#define FLASHRAM_BASE			0x80300000
#define FLASHRAM_END			0x8037ffff
#define FLASHRAM_SECTORSIZE		0x00010000
#define FLASHRAM_SPARESIZE		FLASHRAM_SECTORSIZE
#define FLASHRAM_BANKNUM		1
#define FLASHRAM_SECTORCOUNT	8
#endif

/* Flash bank configuration:
 */
#ifdef FLASHRAM_BASE
#define FLASHBANKS				2
#else
#define FLASHBANKS				1
#endif
#define SINGLE_FLASH_DEVICE 	1
#define FLASH_COPY_TO_RAM 		1
#define FLASH_BANK0_BASE_ADDR  	0x08000000
#define FLASH_PROTECT_RANGE  	"0-2"
#define FLASH_BANK0_WIDTH       2
#define FLASH_LARGEST_SECTOR    0x20000
#define FLASH_LOOP_TIMEOUT		10000000
#define BUFFERED_WRITE

/* TFS definitions:
 *  TFSSTART:       Base address in FLASH at which TFS starts.
 *  TFSEND:         End address of TFS in FLASH.
 *  TFSSPARE:       Location of sector that is used as the spare sector
 *                  by TFS for defragmentation.
 *  TFSSPARESIZE:   Size of the spare sector used by TFS for defragmentation.
 *  TFSSECTORCOUNT: Number of eraseable sectors that TFS covers, not including
 *                  the TFSSPARE sector.
 */
#define TFSSPARESIZE    		FLASH_LARGEST_SECTOR
#define TFS_DEVTOT      		1
#define TFSSTART        		(FLASH_BANK0_BASE_ADDR+0x060000)
//#define TFSEND          		(FLASH_BANK0_BASE_ADDR+0x007dffff)	// 8MB Flash
#define TFSEND          		(FLASH_BANK0_BASE_ADDR+0x00edffff)	// 16MB Flash
//#define TFSEND          		(FLASH_BANK0_BASE_ADDR+0x03dfffff)	// 64MB Flash
#define TFSSPARE        		(TFSEND+1)
#define TFSSECTORCOUNT			((TFSSPARE-TFSSTART)/0x20000)
#define TFS_EBIN_ELFMSBIN		1
#define TFS_VERBOSE_STARTUP		1
#define TFS_ALTDEVTBL_BASE		&alt_tfsdevtbl_base

/* Specify CPU/PLATFORM type and name so that common code can be used
 * for a similar cpu, on different platforms.
 * The 'TYPE' definition is used for ifdefs in the code and the 'NAME'
 * is used for printfs in the code.
 */
#define CPU_TYPE        AM3358
#define CPU_NAME        "TI AM3358 Cortex-A8"
#define PLATFORM_TYPE   BEAGLEBONEBLACK
#define PLATFORM_NAME   "Beaglebone Black"

/* Specify the size of the memory block (in monitor space) that is to be
 * allocated to malloc in the monitor.  Note that this size can be dynamically
 * increased using the heap extension option in the heap command.
 */
#define ALLOCSIZE	1024 	// (64*1024)
#define MONSTACKSIZE	(16*1024)


/* Specify inclusion of subsystems within the monitor here.
 * Refer to comments in common/monitor/inc_check.h for details on
 * each of these macros.
 */
						
#define INCLUDE_MEMTRACE	0
#define INCLUDE_MEMCMDS         0
#define INCLUDE_DM				1
#define INCLUDE_PM				1
#define INCLUDE_EDIT            0
#define INCLUDE_DISASSEMBLER    0
#define INCLUDE_UNZIP           0
#define INCLUDE_ETHERNET        0
#define INCLUDE_ICMP		0
#define INCLUDE_TFTP            0
#define INCLUDE_DHCPBOOT        0
#define INCLUDE_TFS             0
#define INCLUDE_TFSCLI          0
#define INCLUDE_TFSAPI          0
#define INCLUDE_TFSSCRIPT       0
#define INCLUDE_TFSSYMTBL       0
#define INCLUDE_XMODEM          0
#define INCLUDE_LINEEDIT        0
#define INCLUDE_EE              0
#define INCLUDE_FLASH           0
#define INCLUDE_STRACE          0
#define INCLUDE_CAST            0
#define INCLUDE_STRUCT          0
#define INCLUDE_REDIRECT        0
#define INCLUDE_QUICKMEMCPY     0
#define INCLUDE_PROFILER        0
#define INCLUDE_BBC             0
#define INCLUDE_STOREMAC        0
#define INCLUDE_SHELLVARS	0
#define INCLUDE_MALLOC		0
#define INCLUDE_PORTCMD	        0
#define INCLUDE_SYSLOG	        0
#define INCLUDE_HWTMR	        0
#define INCLUDE_VERBOSEHELP     0
#define INCLUDE_GDB		0
#define INCLUDE_USRLVL		0
#define INCLUDE_JFFS2		0
#define INCLUDE_JFFS2ZLIB	0
#define INCLUDE_FBI		0
#define INCLUDE_TSI		0
#define INCLUDE_SD		0
#define INCLUDE_DNS		0
#define INCLUDE_BLINKLED	1
#define TARGET_BLINKLED target_blinkled

/* Inclusion of this next file will make sure that all of the above
 * inclusions are legal; and warn/adjust where necessary.
 */
#include "inc_check.h"
