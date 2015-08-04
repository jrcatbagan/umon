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
 * sbrk.c:
 *
 *  Used by malloc to get memory from "somewhere".
 *
 * Original author:     Ed Sutter (ed.sutter@alcatel-lucent.com)
 *
 */
#include "config.h"
#if INCLUDE_MALLOC
#include "genlib.h"
#define NULL 0

extern  int releaseExtendedHeap(int);

static long allocbuf[ALLOCSIZE/sizeof(long)];
static char *allocp1;
static char *ext_heapbase, *ext_heapspace, *ext_heapend;

/* GetMemory():
 *  This function is called by the guts of malloc when it needs to expand
 *  the size of the heap.  Initially, GetMemory() will allocate memory
 *  from a static array (allocbuf[]) that is allocated memory space when the
 *  monitor is built.   If the variable ext_heapbase is non-zero
 *  at the point when GetMemory() runs out of space in allocbuf[], it
 *  will start allocating memory from the block pointed to by ext_heapspase
 *  and ext_heapsize.
 *  WARNING: this feature can only be used if the malloc()/free() code
 *  can handle the fact that memory within its heap will be different
 *  blocks of non-contiguous space.
*/
char *
GetMemory(int n)
{
    if(!allocp1) {
        allocp1 = (char *)allocbuf;
    }

    /* First try to allocate from allocbuf[]... */
    if(allocp1 + n <= (char *)allocbuf + ALLOCSIZE) {
        allocp1 += n;
        return (allocp1 - n);
    }
    /* Else try to allocated from the extended heap (if one)... */
    else if(ext_heapbase) {
        if(ext_heapspace + n <= ext_heapend) {
            ext_heapspace += n;
            return(ext_heapspace - n);
        } else {
            return(NULL);
        }
    }
    /* Else, no space left to allocate from. */
    else {
        return (NULL);
    }
}

/* ExtendHeap():
 *  Called by the heap command to provide GetMemory() with more space.
 *  This function can be called through the monitor API.
 */
int
extendHeap(char *start, int size)
{
    /* If the size is -1, then assume this is a release request. */
    if(size == -1) {
        return(releaseExtendedHeap(0));
    }

    /* If extension is already loaded, then return -1 for failure. */
    if(ext_heapbase) {
        return(-1);
    }

    if(inUmonBssSpace(start,start+size-1)) {
        return(-2);
    }

    ext_heapbase = ext_heapspace = start;
    ext_heapend = start + size;
    return(0);
}

/* UnextendHeap():
 *  Called by the heap command to "undo" the memory extension.
 */
void
unExtendHeap(void)
{
    ext_heapbase = ext_heapspace = ext_heapend = 0;
}

char *
getExtHeapBase(void)
{
    return(ext_heapbase);
}

/* GetMemoryLeft():
 *  Return the amount of memory that has yet to be allocated from
 *  the static and extended heap (if one).
*/
int
GetMemoryLeft(void)
{
    int     spaceleft;

    if(!allocp1) {
        allocp1 = (char *)allocbuf;
    }

    spaceleft = ((char *)allocbuf + ALLOCSIZE) - allocp1;

    if(ext_heapbase) {
        spaceleft += (ext_heapend - ext_heapspace);
    }

    return(spaceleft);
}

#else

int
extendHeap(char *start, int size)
{
    return(-1);
}

#endif
