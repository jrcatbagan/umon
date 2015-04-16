/* This file is included by the common file reg_cache.c.
 * The file included below is the table of register names.
 * The purpose behind this multiple level of file inclusion is to allow
 * the common file "reg_cache.c" to include a file called "regnames.c"
 * which will have a target-specific register table without the target-
 * specific filename.
 * If the file specified below isn't correct, then check main/cpu/CPU for
 * others.
 */
#include "regs_403.c"
