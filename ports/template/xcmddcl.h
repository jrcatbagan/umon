/* xcmdtbl.h:
 * This file must exist even if it is empty because it is #included in the
 * common file cmdtbl.c.  The purpose is to keep the common comand table
 * file (common/cmdtbl.c) from being corrupted with non-generic commands
 * that may be target specific.
 * This is the declaration portion of the code that must be at the top of
 * the cmdtbl[] array.
 *
 * For example...

extern int date();
extern char *dateHelp[];

 */
