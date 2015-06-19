/* target_version.h:
 * Initial version for all ports is zero.  As the TARGET_VERSION incrments
 * as a result of changes made to the target-specific code, this file should
 * be used as an informal log of those changes for easy reference by others.
 *
 * 0: UART/DRAM/FLASH/TFS working through BDI2000
 * 0->1: Boots from flash (without bdi2000), sleep delay adjusted.
 * 1->2: Ethernet added.
 * 2->3: LCD interface added.
 * 3->4: Flash driver fix, enabled INCLUDE_HWTMR, and added show_version(),
 *		 and the splash screen is loaded from TFS.
 * 4->5: Added support for the FBI (frame buffer) interface, and hard-reset.
 * 5->6: Speedup (clock configuration) provided by Luis; also changed
 *       cpuio.c so that SPI-mode of the touch-screen interface now works.
 */

#define TARGET_VERSION 6
