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
 * cache.h:
 *
 * Refer to cache.c for comments.
 *
 * Original author:     Ed Sutter (ed.sutter@alcatel-lucent.com)
 *
 */
#ifndef _CACHE_H_
#define _CACHE_H_

/* dcacheFlush, icacheInvalidate, dcacheDisable & icacheDisable:
 * Function pointers that should be initialized by target-specific code
 * to point to target-specific functions that can be accessed by the
 * generic monitor source.
 */
extern int (*dcacheFlush)(char *,int), (*icacheInvalidate)(char *,int);
extern int (*dcacheDisable)(void), (*icacheDisable)(void);

/* flushDcache():
 * Wrapper function for the target-specific d-cache flushing operation
 * (if one is appropriate).  If addr and size are both zero, then flush
 * the entire D-cache.
 */
extern int flushDcache(char *addr, int size);

/* invalidateIcache():
 * Wrapper function for the target-specific i-cache invalidation operation
 * (if one is appropriate).  If addr and size are both zero, then invalidate
 * the entire I-cache.
 */
extern int invalidateIcache(char *addr, int size);

/* disableDcache() & disableIcache():
 * Wrapper functions to call target-specific cache disable functions
 * (if available).
 */
extern int disableDcache(void);
extern int disableIcache(void);

/* cacheInit():
 * Called at startup.  This function calls cacheInitForTarget() which
 * establishes the cache configuration and initializes the above
 * function pointers (if applicable).
 */
extern int cacheInit(void);
#endif
