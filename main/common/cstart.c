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
 * cstart.c:
 * This is the first 'C' code exececuted by the processor after a reset if
 * the monitor is built to boot and copy itself into RAM.
 * 
 * This 2-stage monitor is done with two distinct images.
 * The first image (the real "boot" image) includes this cstart() code
 * in place of normal start().  The make process generates the umon.c file
 * included below.  This file is essentially a C-array that consists of
 * the binary image (second image) of a version of the monitor that is
 * built to run out of RAM.  The value of UMON_RAMBASE is the base address
 * of this image and the value of UMON_START is the entry point into this
 * image.
 *
 * Original author:     Ed Sutter (ed.sutter@alcatel-lucent.com)
 *
 */
#include "config.h"
#include "stddefs.h"
#include "cpu.h"

#ifdef ROM_MONSTACKSIZE
ulong   MonStack[ROM_MONSTACKSIZE/4];
#endif

/* The umon.c file is simply an array that contains the monitor
 * image and the necessary information needed to do the copy.
 */
#include "umon.c"

char *
memcpy(char *to,char *from,int count)
{
	char	*to_copy, *end;

	to_copy = to;

	/* If count is greater than 8, get fancy, else just do byte-copy... */
	if (count > 8) {
		/* Attempt to optimize the transfer here... */
		if (((int)to & 3) && ((int)from & 3)) {
			/* If from and to pointers are both unaligned to the
			 * same degree then we can do a few char copies to get them
			 * 4-byte aligned and then do a lot of 4-byte aligned copies.
			 */
			if (((int)to & 3) == ((int)from & 3)) {
				while((int)to & 3) {
					*to++ = *from++;
					count--;
				}
			}
			/* If from and to pointers are both odd, but different, then
			 * we can increment them both by 1 and do a bunch of 2-byte
			 * aligned copies...
			 */
			else if (((int)to & 1) && ((int)from & 1)) {
				*to++ = *from++;
				count--;
			}
		}
	
		/* If both pointers are now 4-byte aligned or 2-byte aligned,
		 * take advantage of that here...
		 */
		if (!((int)to & 3) && !((int)from & 3)) {
			end = to + (count & ~3);
			count = count & 3;
			while(to < end) {
				*(ulong *)to = *(ulong *)from;
				from += 4;
				to += 4;
			}
		}
		else if (!((int)to & 1) && !((int)from & 1)) {
			end = to + (count & ~1);
			count = count & 1;
			while(to < end) {
				*(ushort *)to = *(ushort *)from;
				from += 2;
				to += 2;
			}
		}
	}

	if (count) {
		end = to + count;
		while(to < end)
			*to++ = *from++;
	}
	return(to_copy);
}

void
cstart(void)
{
	register long *lp1, *lp2, *end2;
	void (*entry)();
	
	entry = (void(*)())UMON_START;

	/* Copy image from boot flash to RAM, then verify the copy.
	 * If it worked, then jump into that space; else reset and start
	 * over (not much else can be done!).
	 */
	memcpy((char *)UMON_RAMBASE,(char *)umon,(int)sizeof(umon));

	/* Verify the copy...
	 */
	lp1 = (long *)UMON_RAMBASE;
	lp2 = (long *)umon;
	end2 = lp2 + (int)sizeof(umon)/sizeof(long);
	while(lp2 < end2) {
		if (*lp1 != *lp2) {
#ifdef CSTART_ERROR_FUNCTION
			extern void CSTART_ERROR_FUNCTION();

			CSTART_ERROR_FUNCTION(lp1,*lp1,*lp2);
#endif
#ifdef INLINE_RESETFUNC
			INLINE_RESETFUNC();
#else
			entry = RESETFUNC();
#endif
			break;
		}
		lp1++; lp2++;
	}
	entry();
}
