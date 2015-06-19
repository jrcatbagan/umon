/* extcmdtbl.h: */
/* This file must exist even if it is empty because it is #included in the */
/* common file cmdtbl.c.  The purpose is to keep the common comand table   */
/* file (common/cmdtbl.c) from being corrupted with non-generic commands   */
/* that may be target specific. */
/* It is the entry in the command table representing the new command being */
/* added to the cmdtbl[] array. */
/* For example:
    "dummy",    dummycmd,   dummyHelp,
*/
{"ads",			ads,		adsHelp,},
//{"i2c",			i2c,		i2cHelp,},
#if INCLUDE_LCD
{"lcd_tst",		lcd_tst,	lcd_tstHelp,},
#endif
{"ldatags",		ldatags,	ldatagsHelp,},
#if INCLUDE_NANDCMD
{"nand",		nandCmd,	nandHelp,},
#endif
