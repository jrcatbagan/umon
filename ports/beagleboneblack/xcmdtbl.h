/*
 * xcmdtbl.h
 *
 * This file must exist even if it is empty because it is #included in the
 * common file cmdtbl.c.  The purpose is to keep the common comand table
 * file (common/cmdtbl.c) from being corrupted with non-generic commands
 * that may be target specific.
 * It is the entry in the command table representing the new command being
 * added to the cmdtbl[] array.
 * For example:
 *      { "dummy",    dummycmd,   dummyHelp,    0 },
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
 *
 *
 * Adapted by Jarielle Catbagan for the Beaglebone Black
 * email: jcatbagan93@gmail.com
 */

{"mmc",     mmc,        mmcHelp,    0},
