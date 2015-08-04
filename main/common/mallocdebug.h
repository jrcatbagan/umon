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
 * FN:
 *
 * This file should be included in config.h if the monitor is
 * to be built with malloc-debug enabled.  The malloc-debug feature
 * simply adds the filename and file line number to the mhdr structure.
 *
 * This makes it easier to find a memory leak because the location of
 * the violating malloc call will be dumped by "heap -v".
 *
 * Original author:     Ed Sutter (ed.sutter@alcatel-lucent.com)
 *
 */
#ifndef _MALLOCDEBUG_H_
#define _MALLOCDEBUG_H_

#ifndef ASSEMBLY_ONLY

#define MALLOC_DEBUG
extern  char *malloc(int, char *, int);
extern  char *realloc(char *, int, char *, int);

#define malloc(a) malloc(a,__FILE__,__LINE__)
#define realloc(a,b) realloc(a,b,__FILE__,__LINE__)

#endif  /* ASSEMBLY_ONLY */

#endif
