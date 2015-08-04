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
 * monbuilt.c:
 *
 *  This file contains all of the monitor code that constructs
 *  the time and date of the build.
 *
 * Original author:     Ed Sutter (ed.sutter@alcatel-lucent.com)
 *
 */
/* whatbuild:
 * A 'what' string to allow the 'what' tool to query the binary
 * image built with this source code.
 */
#define WHAT_PREFIX "@(#)"

char *whatbuild = WHAT_PREFIX __DATE__ " @ " __TIME__;

/* monBuilt():
 * Return a pointer to a string that is a concatenation of the
 * build date and build time.
 */

char *
monBuilt(void)
{
    return(whatbuild+sizeof(WHAT_PREFIX)-1);
}
