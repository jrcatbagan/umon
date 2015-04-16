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
 * version.h:
 *
 * MicroMonitor started using a version number as of December 2004.
 * Since it has been around for quite some time, the initial version
 * number is: 1.0.1.1
 * The version number for MicroMonitor is 4 'dot' separated numbers.
 * Each number can be as large as is needed.
 *
 *	MAJOR_VERSION.MINOR_VERSION.BUILD_NUMBER.TARGET_VERSION 

 * MAJOR, MINOR & BUILD apply to the common code applicable to all targets.
 * TARGET applies to the target-specific code.
 *
 * Original author:     Ed Sutter (ed.sutter@alcatel-lucent.com)
 *
 */
#ifndef _VERSION_H_
#define _VERSION_H_

/* MAJOR_VERSION:
 * Incremented as a result of a major change or enhancement to the core
 * monitor code or as a means of dropping the MINOR_VERSION back to zero;
 * hence, simply identifying a significant set of MINOR changes or some
 * big change.
 */
#define MAJOR_VERSION		1

/* MINOR_VERSION:
 * Incremented as a result of a new command or feature, or as a result
 * of a bug fix to the core monitor code.
 * When MAJOR_VERSION is incremented, MINOR_VERSION is reset to 0.
 * 0->1:
 *   Formalize the uMon1.0 transition.  Needed to do this because of the
 *   amount of churn in 1.0.
 * 1->2:
 *   - New 'call -A' option
 *   - Work on flash internals to reduce the need for callers to know
 *     the flash bank pointer.
 *   - New tfs 'qclean' subcommand.
 *   - Bug fix: file in "tfs ramdev" space could not be marked stale.
 * 2->3:
 *   - Bug fix: uMonInRam() re-write.
 *   - Bug fix: "tfs ramdev" device would be lost after mon_appexit().
 *   - Bug fix: "tfs ramdev" naming conflict could occur between device
 *		and file.
 * 3->4:
 *   - The tfscheck() function accepts a NULL input TDEV pointer to signify
 *     a request to check all TFS devices (instead of just one named device).
 *   - The address used by xmodem -B for determining the last sector burned
 *     had to be decremented by 1.
 * 4->5:
 *   - The "flash erase" command takes addresses as well as sector numbers.
 *   - The "flash info" and "tfs stat" populate shellvars with their info.
 *   - Bug fix: tftp get would turn on the server, now fixed so that if 
 *		server was off, it stays off.
 *   - Bug fix: if destination file received by tftp server started with
 *		a $, but the shell variable didn't exist, the server would create
 *		a file with the $.  This will now generate an error.
 * 5->6:
 *	 - Added more configurability so that uMon's footprint can be smaller.
 *	 - Broke up memcmds.c into individually configurable commands using
 *	   INCLUDE_DM, INCLUDE_PM, etc.
 *	 - Added support to configure USRLVL, ICMP, and ICMPTIME in or out.
 *	 - TFS now supports the option of being built without FLASH.
 *	 - New read options: -p -n.
 *   - New pm options: -a -o -x.
 *   - New PRE_TFSAUTOBOOT_HOOK() macro.
 *   - Converted genlib.c to a library.
 *   - New api: mon_portcmd().
 * 6->7:
 *   - New JFFS2 command.
 *   - New TFSERR_DSIMAX error checking in tfsmemuse() and tfsadd().
 *   - Eliminated the -x option in tfs command.
 *   - The tfs command now returns CMD_FAILURE if tfsadd fails.
 *   - Moncmd server will process a leading '.' as indication that the
 *     command is to be executed immediately rather than after the
 *     incoming packet queue is empty.
 * 7->8:
 *   - New TFS_ALTDEVTBL_BASE code to support an alternat TFS device table
 *	   that is outside uMon's text/data space.
 *   - Fixed bug in JFFS2 related to file truncation.
 * 8->9:
 *   - New DOSFS/FATFS/CF facility (much help from Graham Henderson).
 *   - CodeWarrior-specific code cleanup (submitted by Arun Biyani).
 *   - Atmel NIOS port (submitted by Graham Henderson).
 * 9->10:
 *   - New 'struct' command to hopefully eliminate 'lboot' and 'ldatags'.
 * 10->11:
 *   - Fixed problems with packet transfer interface.
 *   - Updated the umon_apps/udp application.
 *   - New Microblaze port (as3dev).
 * 11->12:
 *   - Added the 'to' side of the ARP request in ethernet verbosity.
 *   - Fixed bugs in tcpstuff.c that were only seen on little-endian CPUs.
 *	 - Added the ability to load an elf file from raw memory space.  This
 *	   introduces the notion of a 'fake' tfs file header to tfs, using the
 *	   first reserved entry in the header as a pointer to the data portion
 *	   of the file.
 * 12->14:
 *   - Added new DHCP shell variable ROOTPATH (reflects option 17).
 *   - New DHCP variable: DHCPDONTBOOT.  Tells DHCP not to do anything with
 *     the incoming DHCP transaction (except store away the info in the
 *     shell variables); thus, allowing a script to do what it wants to do.
 *   - Change in TFTP server: if an out-of-sequence block number is received,
 *     it is now just ignored, the transaction doesn't terminate with an error.
 *   - Added inUmonBssSpace() check to the heap extension code.
 *   - Fixed bug in "tfs ramdev" command... If partition didn't exist and
 *     a size of zero was specified, TFS incorrectly attempted to create a
 *     zero-size ramdev partition.
 *   - Fixed bug in multiple-command-line-processing that occurs if a
 *     shell variable is expanded within one of the commands.  See CVS
 *     log for docmd.c for more info.
 * 14->15:
 *   - Fixed a bug in TFTP packet reception that was causing all incoming
 *     file downloads greater than 32Mg to fail because that is the point
 *     at which the block number will wrap. 
 *   - Updates/cleanups made to keep the build warning-free with GCC 4.2
 *     from Microcross.
 * 15->16:
 *   - Added lwip user application.
 *   - Added better exception handling to blackfin.
 *   - Added mon_timer() api.
 * 16->17:
 *   - Lotta new stuff, refer to user manual for complete list...
 *   - Lwipapp: httpget, telnet client. 
 *   - Tested support for nor-less system (booting from SPI flash).
 *   - TSI, FBI, mDNS, LLAD, etc...
 * 17->18:
 *   - Refer to user manual for complete list...
 *   - SPI-SD support for BF537.
 *   - New FATFS
 *   - SPI-resident TFS support.
 *   - JFFS2 extended by B.Gatliff
 * 18->19:
 *	 - Refer to user manual for complete list...
 *   - TFS defrag bug fixes.
 */
#define MINOR_VERSION	19

/* TARGET_VERSION:
 * Incremented as a result of a bug fix or change made to the
 * target-specific (i.e. port) portion of the code.
 * 
 * To keep a "cvs-like" log of the changes made to a port that is
 * not under CVS, it is recommended that the target_version.h file be
 * used as the log to keep track of changes in one place.
 */
#include "target_version.h"

#endif
