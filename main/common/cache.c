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
 * cache.c:
 *  This code is the front-end to CPU-specific code that may or may not
 *  support basic I & D cache operations.  Various facilities in the
 *  monitor call the wrapper functions below for carrying out cpu-specific
 *  cache operations.  This code provides that wrapper allowing the other
 *  portions of the monitor to be unaware of what the specific CPU actually
 *  supports.
 *
 *  The cacheInit() function must be called at monitor startup, then
 *  cacheInit() calls cacheInitForTarget(), a function that must be provided
 *  by the target-specific code to fill in the CPU-specific functionality.
 *  This function should establish the cache as it is to be used in
 *  the monitor (typically i-cache enabled, d-cache disabled), plus, if
 *  applicable, the dcacheFlush and icacheInvalidate function pointers should
 *  be initialized to CPU-specific functions that match the API...
 *
 *      int (*dcacheDisable)(void);
 *      int (*icacheDisable)(void);
 *      int (*dcacheFlush)(char *base,int size);
 *      int (*icacheInvalidate)(char *base, int size);
 *
 *
 * Original author:     Ed Sutter (ed.sutter@alcatel-lucent.com)
 *
 */
#include "genlib.h"
#include "cache.h"

int (*dcacheFlush)(char *,int), (*icacheInvalidate)(char *, int);
int (*dcacheDisable)(void), (*icacheDisable)(void);

/* flushDcache():
 * Flush the address range specified, or if address and size are both
 * zero, flush the entire D-Cache.
 */
int
flushDcache(char *addr, int size)
{
    /* The dcacheFlush function pointer should be initialized in the
     * port-specific function "cacheInitForTarget".
     */
    if(dcacheFlush) {
        return(dcacheFlush(addr,size));
    }
    return(0);
}

/* disableDcache():
 * Disable data cache.
 */
int
disableDcache(void)
{
    /* The dcacheDisable function pointer should be initialized in the
     * port-specific function "cacheInitForTarget".
     */
    if(dcacheDisable) {
        return(dcacheDisable());
    }
    return(0);
}

/* invalidateIcache():
 * Invalidate the address range specified, or if address and size are both
 * zero, invalidate the entire I-Cache.
 */
int
invalidateIcache(char *addr, int size)
{
    /* The icacheInvalidate function pointer should be initialized in the
     * port-specific function "cacheInitForTarget".
     */
    if(icacheInvalidate) {
        return(icacheInvalidate(addr,size));
    }
    return(0);
}

/* disableIcache():
 * Disable instruction cache.
 */
int
disableIcache(void)
{
    /* The icacheDisable function pointer should be initialized in the
     * port-specific function "cacheInitForTarget".
     */
    if(icacheDisable) {
        return(icacheDisable());
    }
    return(0);
}

int
cacheInit()
{
    dcacheDisable = (int(*)(void))0;
    icacheDisable = (int(*)(void))0;
    dcacheFlush = (int(*)(char *,int))0;
    icacheInvalidate = (int(*)(char *,int))0;
    cacheInitForTarget();           /* Target-specific initialization. */
    return(0);
}
