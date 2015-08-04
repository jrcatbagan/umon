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
 * except_template.c:
 *
 * Template for cpu-specific code that handles exceptions that are caught
 * by the exception vectors that have been installed by the monitor through
 * vinit().
 *
 * Original author:     Ed Sutter (ed.sutter@alcatel-lucent.com)
 *
 */
#include "config.h"
#include "cpu.h"
#include "cpuio.h"
#include "genlib.h"
#include "stddefs.h"
#include "warmstart.h"

ulong   ExceptionAddr;
int     ExceptionType;

/* exception():
 * This is the first 'C' function called out of a monitor-installed
 * exception handler.
 */
void
exception(void)
{
    /* ADD_CODE_HERE */

    /* Populating these two values is target specific.
     * Refer to other target-specific examples for details.
     * In some cases, these values are extracted from registers
     * already put into the register cache by the lower-level
     * portion of the exception handler in vectors_template.s
     */
    ExceptionAddr = 0;
    ExceptionType = 0;

    /* Allow the console uart fifo to empty...
     */
    flushconsole();
    monrestart(EXCEPTION);
}

/* vinit():
 * This function is called by init1() at startup of the monitor to
 * install the monitor-based vector table.  The actual functions are
 * in vectors.s.
 */
void
vinit()
{
    /* ADD_CODE_HERE */
}

/* ExceptionType2String():
 * This function simply returns a string that verbosely describes
 * the incoming exception type (vector number).
 */
char *
ExceptionType2String(int type)
{
    char *string;

    /* ADD_CODE_HERE */
    return(string);
}

