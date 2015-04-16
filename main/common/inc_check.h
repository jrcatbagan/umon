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
 * inc_check.h:
 *
 * DESCRIPTION HERE
 *
 * 
 *	This is an include file used to verify that all of the monitor macros
 *	are defined.  There is also some attempt to verify that	macro conflicts
 *	do not exist (for example, the flash file system cannot exist if the
 *	flash driver is not included); however, there are probably holes in	this
 *	check.
 * 
 *	Following is a brief description of what each macro includes if set to
 *	non-zero...
 *
 *	INCLUDE_MEMCMDS:
 *		Memory display, modification and test commands.  The included as a 
 *		result of setting this macro is primarily found in memcmds.c.
 *	INCLUDE_EDIT:
 *		An ascii file editor.  This is an on-board file editor that provides
 *		the system with the ability to edit files in TFS.  Refer to edit.c
 *		for details of this functionality.  This obviously assumes that TFS
 *		and the FLASH drivers are also enabled.
 *	INCLUDE_DISASSEMBLER:
 *		This pulls in a disassembler.
 *	INCLUDE_UNPACK:
 *	INCLUDE_UNZIP:
 *		These two macros define one of two different mechanisms for
 *		decompression of executables in TFS.  UNPACK is huffman and UNZIP
 *		is the public domain zlib stuff.  Huffman is a small addition to the
 *		monitor size but only provides about 15% compression.  The zlib stuff
 *		is a bit larger and needs more RAM for malloc() but it does a MUCH
 *		better job of compression.  I've seen as high as 75% of the file size
 *		compressed.  It is illegal to set both of these macros.
 *	INCLUDE_ETHERNET:
 *		This pulls in the basic ethernet drivers with ARP, and some of the
 *		lowest level ethernet interface code.
 *	INCLUDE_TFTP:
 *		This pulls in a TFTP client and server (obviously assumes ETHERNET
 *		is also pulled in).
 *	INCLUDE_DHCPBOOT:
 *		This pulls in a DHCP/BOOTP client (also assumes ETHERNET
 *		is also pulled in).
 *	INCLUDE_TFS:
 *	INCLUDE_TFSAPI:
 *	INCLUDE_TFSAUTODEFRAG:
 *	INCLUDE_TFSSYMTBL:
 *	INCLUDE_TFSSCRIPT:
 *	INCLUDE_TFSCLI:
 *		The above TFS macros allow the monitor to be built with varying degrees
 *		of FFS capability depending on the needs of the system.  INCLUDE_TFS
 *		is required by all others and is the minimum TFS hookup.  It provides
 *		the minimum set of TFS facilities such as autoboot, use of a monrc file
 *		for startup config, and defragmentation.  TFSAPI pulls in the TFS 
 *		code that allows an application to hook into TFS's capabilities in
 *		code-space.  TFSCLI pulls in the TFS command at the command line 
 *		interface.  TFSSAFEDEFRAG pulls in the power-safe defragmenation
 *		(otherwise a simpler, less robust mechanism is used).
 *		TFSSYMTBL pulls in the symbol-table functionality and TFSSCRIPT
 *		pulls in the CLI commands that are normally associated with scripts.
 *	INCLUDE_XMODEM:
 *		Pull in Xmodem.
 *	INCLUDE_LINEEDIT:
 *		Pull in a command line editor and history facility.
 *	INCLUDE_EE:
 *		Pull in an expression evaluator for the CLI.
 *		Note that this is not included with the public distribution.
 *	INCLUDE_FLASH:
 *		Pull in the flash drivers.
 *	INCLUDE_CRYPT:
 *		Pull in UNIX crypt() instead of a simpler encryption scheme I wrote.
 *		Note that this is not included with the public distribution.
 *	INCLUDE_GDB:
 *		Pull in the "gdb" command.  This is an incomplete facility that
 *		will eventually allow the monitor to hook up to a gdb debugger.
 *	INCLUDE_STRACE:
 *		Pull in the "strack trace" command. 
 *	INCLUDE_CAST:
 *		Pull in the "cast" command for complex structure display at the
 *		CLI.
 *	INCLUDE_REDIRECT:
 *		This pulls in the code that allows the monitor's CLI to redirect
 *		command output to a TFS file.
 *	INCLUDE_QUICKMEMCPY:
 *		This pulls in some faster memcpy stuff in genlib.c.
 *	INCLUDE_PROFILER:
 *		This pulls in some code and a CLI command that provides an
 *		application with some task and function level profiling.
 *	INCLUDE_BBC:
 *		This pulls in some code and a CLI command that provides an
 *		application with a basic ability to organize basic block coverage
 *		verification.
 *	INCLUDE_MEMTRACE:
 *		This pulls in a simple memory trace capability.  It's a simple
 *		printf()-like function with data logged to RAM instead of a UART.
 *	INCLUDE_STOREMAC:
 *		This pulls in a function that forces the user to establish the MAC 
 *		in the etheradd space allocated in reset.s.  It needs INCLUDE_FLASH
 *		because flash writes are done.  If not enabled the MAC address can
 *		still be stored in etheradd using the "ether mac" command.
 *	INCLUDE_SHELLVARS:
 *		This pulls in the monitor's ability to deal with shell variables.
 *		This includes the "set" command on the CLI, plus a lot of other
 *		subsections of the monitor depend on shell variables.
 *		This facility should only be disabled when there is a desperate
 *		need to shink the monitor's footprint.
 *	INCLUDE_MALLOC:
 *		This pulls in the monitor's malloc.  This includes the "heap" 
 *		command on the CLI, plus a few other factilies in the monitor
 *		assume malloc is included; so without it, they won't work either.
 *		This facility should only be disabled when there is a desperate
 *		need to shink the monitor's footprint.
 *	INCLUDE_HWTMR:
 *		If set, then the target port must supply a function called
 *	 	target_timer() which returns a 32-bit value representing a
 *		a hardware-resident clock whose rate is defined by the value
 *		specified by TIMER_TICKS_PER_MSEC.
 *	INCLUDE_VERBOSEHELP:
 *		If set, then full help text is built in; else only the usage
 *		and abstract is included.
 *	INCLUDE_PORTCMD:
 *		If set, then the mon_portcmd(int cmd, void *arg) API can be
 *		used to build port-specific API extensions.  
 *	INCLUDE_USRLVL:
 *		If set, then the code that incorporates user levels is included
 *		in the build.
 *	INCLUDE_STRUCT:
 *		If set, then the struct command is included in the build.  This
 *		requires that TFS be included also.
 *
 * Original author:     Ed Sutter (ed.sutter@alcatel-lucent.com)
 *
 */
#ifndef _INC_CHECK_H
#define _INC_CHECK_H

/***********************************************************************
 * Verify that the basic set of INCLUDE macros have been defined:
 */

#ifndef INCLUDE_ICMP
#error "INCLUDE_ICMP must be defined in config.h."
#endif

#ifndef INCLUDE_USRLVL
#error "INCLUDE_USRLVL must be defined in config.h."
#endif

#ifndef INCLUDE_PORTCMD
#error "INCLUDE_PORTCMD must be defined in config.h."
#endif

#ifndef INCLUDE_HWTMR
#error "INCLUDE_HWTMR must be defined in config.h."
#endif

#ifndef INCLUDE_VERBOSEHELP
#error "INCLUDE_VERBOSEHELP must be defined in config.h."
#endif

#ifndef INCLUDE_MEMTRACE
#error "INCLUDE_MEMTRACE must be defined in config.h."
#endif

#ifdef INCLUDE_TFSSAFEDEFRAG
#error "INCLUDE_TFSSAFEDEFRAG no longer needed."
#endif

#ifndef INCLUDE_MEMCMDS
#error "INCLUDE_MEMCMDS must be defined in config.h"
#endif

#ifndef INCLUDE_EDIT
#error "INCLUDE_EDIT must be defined in config.h"
#endif

#ifndef INCLUDE_DISASSEMBLER
#error "INCLUDE_DISASSEMBLER must be defined in config.h"
#endif

#ifndef INCLUDE_UNZIP
#error "INCLUDE_UNZIP must be defined in config.h"
#endif

#ifndef INCLUDE_ETHERNET
#error "INCLUDE_ETHERNET must be defined in config.h"
#endif

#ifndef INCLUDE_TFTP
#error "INCLUDE_TFTP must be defined in config.h"
#endif

#ifndef INCLUDE_DHCPBOOT
#error "INCLUDE_DHCPBOOT must be defined in config.h"
#endif

#ifndef INCLUDE_TFS
#error "INCLUDE_TFS	 must be defined in config.h"
#endif

#ifndef INCLUDE_TFSCLI
#error "INCLUDE_TFSCLI must be defined in config.h"
#endif

#ifndef INCLUDE_TFSAPI
#error "INCLUDE_TFSAPI must be defined in config.h"
#endif

#ifndef INCLUDE_TFSSCRIPT
#error "INCLUDE_TFSSCRIPT must be defined in config.h"
#endif

#ifndef INCLUDE_TFSSYMTBL
#error "INCLUDE_TFSSYMTBL must be defined in config.h"
#endif

#ifndef INCLUDE_XMODEM
#error "INCLUDE_XMODEM must be defined in config.h"
#endif

#ifndef INCLUDE_LINEEDIT
#error "INCLUDE_LINEEDIT must be defined in config.h"
#endif

#ifndef INCLUDE_EE
#error "INCLUDE_EE must be defined in config.h"
#endif

#ifndef INCLUDE_FLASH
#error "INCLUDE_FLASH must be defined in config.h"
#endif

#ifndef INCLUDE_STRACE
#error "INCLUDE_STRACE must be defined in config.h"
#endif

#ifndef INCLUDE_CAST
#error "INCLUDE_CAST must be defined in config.h"
#endif

#ifdef INCLUDE_SHELLVAR
#error "INCLUDE_SHELLVAR definition no longer needed in config.h"
#endif

//#ifdef INCLUDE_MALLOC
//#error "INCLUDE_MALLOC definition no longer needed in config.h"
//#endif

#ifndef INCLUDE_REDIRECT
#error "INCLUDE_REDIRECT must be defined in config.h"
#endif

#ifndef INCLUDE_QUICKMEMCPY
#error "INCLUDE_QUICKMEMCPY must be defined in config.h"
#endif

#ifndef INCLUDE_PROFILER
#error "INCLUDE_PROFILER must be defined in config.h"
#endif

#ifndef INCLUDE_BBC
#error "INCLUDE_BBC must be defined in config.h"
#endif

#ifndef INCLUDE_STOREMAC
#error "INCLUDE_STOREMAC must be defined in config.h"
#endif

#ifndef INCLUDE_MALLOC
#error "INCLUDE_MALLOC must be defined in config.h"
#endif

#ifndef INCLUDE_SHELLVARS
#error "INCLUDE_SHELLVARS must be defined in config.h"
#endif

#ifndef INCLUDE_STRUCT
#error "INCLUDE_STRUCT must be defined in config.h"
#endif


/***********************************************************************
 * The storemac facility needs flash to be enabled.
 */
#if INCLUDE_STOREMAC
#if !INCLUDE_FLASH
#error	"Can't include STOREMAC without FLASH"
#endif
#endif

/***********************************************************************
 * The hardware timer facility needs to know the tick rate.
 */
#if INCLUDE_HWTMR
#ifndef TIMER_TICKS_PER_MSEC
#error "Can't set INCLUDE_HWTMR without TIMER_TICKS_PER_MSEC."
#endif
#endif

/***********************************************************************
 * Certain pieces of the monitor cannot be enabled without basic TFS:
 */
#if !INCLUDE_TFS
#if INCLUDE_REDIRECT
#error	"Can't include REDIRECT without TFS"
#endif
#if INCLUDE_PROFILER
#error	"Can't include PROFILER without TFS"
#endif
#if INCLUDE_USRLVL
#error	"Can't include USRLVL without TFS"
#endif
#if INCLUDE_STRUCT
#error	"Can't include STRUCT without TFS"
#endif
#endif

/***********************************************************************
 * Certain pieces of the monitor cannot be enabled without TFS API:
 */

#if !INCLUDE_TFSAPI

#if INCLUDE_EDIT
#error	"Can't include EDIT without TFSAPI"
#endif
#if INCLUDE_STRACE
#error	"Can't include STRACE without TFSAPI"
#endif
#if INCLUDE_TFSSYMTBL
#error	"Can't include TFSSYMTBL without TFSAPI"
#endif

#endif

/***********************************************************************
 * Certain pieces of the monitor cannot be enabled without ETHERNET:
 */
#if !INCLUDE_ETHERNET

#if INCLUDE_TFTP
#error	"Can't include TFTP without ETHERNET"
#endif
#if INCLUDE_DHCPBOOT
#error	"Can't include DHCPBOOT without ETHERNET"
#endif
#if INCLUDE_SYSLOG
#error	"Can't include SYSLOG without ETHERNET"
#endif
#if INCLUDE_ICMP
#error	"Can't include ICMP without ETHERNET"
#endif

#endif

/***********************************************************************
 * Just history...
 */
#define DONT_INCLUDE_OLDSTYLE_FLAGCHECK

/***********************************************************************
 * Check for things removed as of 1.0...
 */
#ifdef INCLUDE_UNPACK
#error	"INCLUDE_UNPACK is unused as of uMon 1.0"
#endif
#ifdef INCLUDE_DEBUG
#error	"INCLUDE_DEBUG is unused as of uMon 1.0"
#endif
#ifdef INCLUDE_PIO
#error	"INCLUDE_PIO is unused as of uMon 1.0"
#endif
#ifdef INCLUDE_EXCTEST
#error "INCLUDE_EXCTEST is unused as of uMon 1.0"
#endif
#ifdef INCLUDE_IDEV
#error "INCLUDE_IDEV is unused as of uMon 1.0"
#endif
#ifdef INCLUDE_CRYPT
#error "INCLUDE_CRYPT is unused as of uMon 1.0"
#endif
#ifdef INCLUDE_TFSAUTODEFRAG
#error "INCLUDE_TFSAUTODEFRAG is unused as of uMon 1.0"
#endif
#ifdef FLASH_LOCK_SUPPORTED
#error "FLASH_LOCK_SUPPORT is unused as of uMon 1.0"
#endif
#ifdef INCLUDE_ARGV
#error "INCLUDE_ARGV is unused as of uMon 1.0"
#endif


#endif
