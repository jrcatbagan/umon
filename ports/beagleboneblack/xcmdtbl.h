/* xcmdtbl.h: */
/* This file must exist even if it is empty because it is #included in the */
/* common file cmdtbl.c.  The purpose is to keep the common comand table   */
/* file (common/cmdtbl.c) from being corrupted with non-generic commands   */
/* that may be target specific. */
/* It is the entry in the command table representing the new command being */
/* added to the cmdtbl[] array. */
/* For example:
    { "dummy",    dummycmd,   dummyHelp,    0 },
*/

{"mmc",     mmc,        mmcHelp,    0},
