/* cpu.h:
    General notice:
    This code is part of a boot-monitor package developed as a generic base
    platform for embedded system designs.  As such, it is likely to be
    distributed to various projects beyond the control of the original
    author.  Please notify the author of any enhancements made or bugs found
    so that all may benefit from the changes.  In addition, notification back
    to the author will allow the new user to pick up changes that may have
    been made by other users after this version of the code was distributed.

    Author: Ed Sutter
    email:  esutter@lucent.com
    phone:  908-582-2351

*/
#include "omap3530.h"

#define RESETMACRO() \
{																\
	WD2_REG(WD_WCRR) = 0xfffffff0; 								\
	WD2_REG(WD_WSPR) = 0x0000bbbb; 								\
	while(*(volatile unsigned long *)(WD2_BASE_ADD + WD_WWPS)); \
	WD2_REG(WD_WSPR) = 0x00004444; 								\
	while(1); 													\
}
