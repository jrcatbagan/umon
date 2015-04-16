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
 * cache_arm.c
 *
 * ARM's definition of "flush" and "clean" (as taken from the
 * "ARM System Developer's Guide") ...
 *
 * To "flush a cache" is to clear it of any stored data.  Flushing clears
 * the valid bit in the affected cache line... The term "invalidate"
 * is sometimes used in place of the term "flush".
 *
 * To "clean a cache" is to force a write of the dirty cache lines
 * from the cache out to main memory and clear the dirty bits in the
 * cache line.
 * 
 * This conflicts with uMon's general use of the terms "flush" and
 * "invalidate".  For uMon, "flush" refers to  what ARM calls "clean"
 * and "invalidate" refers to what ARM calls "flush".  ARRGGHH!!
 *
 * Original author:     Ed Sutter (ed.sutter@alcatel-lucent.com)
 *
 */
#include "cache.h"

int
arm_cleanDcache(char *base, int size)
{
	return(0);
}

int
arm_flushIcache(char *base, int size)
{
	/* Flush (i.e. "invlidate in uMon terminology) entire instruction
	 * cache (ignore incoming args).
	 */
	asm("   MOV r0, #0");
	asm("   MCR p15, 0, r0, c7, c5, 0");
	return(0);
}

/* cacheInitForTarget():
	Enable instruction cache only...
*/
void
cacheInitForTarget()
{
	asm("   MRC p15, 0, r0, c1, c0, 0");
	asm("   ORR r0, r0, #0x1000");  /* bit 12 is ICACHE enable*/
	asm("   MCR p15, 0, r0, c1, c0, 0");

	/* Flush instruction cache */
	arm_flushIcache(0,0);

	dcacheFlush = arm_cleanDcache;
	icacheInvalidate = arm_flushIcache;
}

/* MRC/MCR assembler syntax (for ARM general):
 *
 * <MCR|MRC>{cond} p#,<expression1>,Rd,cn,cm{,<expression2>}
 *
 * Where:
 *	- MRC move from coprocessor to ARM register (L=1)
 *	- MCR move from ARM register to coprocessor (L=0)
 *	- {cond} two character condition mnemonic (see list below)
 *	- p# the unique number of the required coprocessor
 *	- <expression1> evaluated to a constant and placed in the CP Opc field
 *	- Rd is an expression evaluating to a valid ARM processor register
 *		number
 *	- cn and cm are expressions evaluating to the valid coprocessor register
 *		numbers CRn and CRm respectively
 *	- <expression2> where present is evaluated to a constant and placed in
 *		the CP field
 *
 * Examples
 *	- MRC 2,5,R3,c5,c6	;request coproc 2 to perform operation 5
 *				;on c5 and c6, and transfer the (single
 *				;32-bit word) result back to R3
 *	- MCR 6,0,R4,c6		;request coproc 6 to perform operation 0
 *				;on R4 and place the result in c6
 *	- MRCEQ 3,9,R3,c5,c6,2	;conditionally request coproc 2 to
 *				;perform
 *				;operation 9 (type 2) on c5 and c6, and
 *				;transfer the result back to R3
 *
 * Condition codes:
 *	EQ (equal)				- Z set
 *	NE (not equal)			- Z clear
 *	CS (unsigned higher or same) - C set
 *	CC (unsigned lower)		- C clear
 *	MI (negative)			- N set
 *	PL (positive or zero)	- N clear
 *	VS (overflow)			- V set
 *	VC (no overflow)		- V clear
 *	HI (unsigned higher)	- C set and Z clear
 *	LS (unsigned lower or same) - C clear or Z set
 *	GE (greater or equal)	- N set and V set, or N clear and V clear
 *	LT (less than)			- N set and V clear, or N clear and V set
 *	GT (greater than)		- Z clear, and either N set and Vset,
 *							  or N clear and V clear
 *	LE (less than or equal) - Z set, or N set and V clear,
 *							  or N clear and V set
 *	AL - always
 *	NV - never
 */
