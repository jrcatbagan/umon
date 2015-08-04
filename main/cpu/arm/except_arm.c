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
 * FILENAME_HERE
 *
 * DESCRIPTION_HERE
 *
 * Original author:     Ed Sutter (ed.sutter@alcatel-lucent.com)
 *
 */

#include "config.h"
#include "arm.h"
#include "stddefs.h"
#include "genlib.h"
#include "warmstart.h"

ulong   ExceptionAddr;
int ExceptionType;

/***********************************************************************
 *
 * umon_exception()
 * Default exception handler used by the low level code in vectors_arm.S.
 */
void
umon_exception(ulong addr, ulong type)
{
    ExceptionAddr = addr;
    ExceptionType = type;
    monrestart(EXCEPTION);
}

/***********************************************************************
 *
 * ExceptionType2String():
 * This function simply returns a string that verbosely describes
 * the incoming exception type (vector number).
 */
char *
ExceptionType2String(int type)
{
    char *string;

    switch(type) {
    case EXCTYPE_UNDEF:
        string = "Undefined instruction";
        break;
    case EXCTYPE_ABORTP:
        string = "Abort prefetch";
        break;
    case EXCTYPE_ABORTD:
        string = "Abort data";
        break;
    case EXCTYPE_IRQ:
        string = "IRQ";
        break;
    case EXCTYPE_FIRQ:
        string = "Fast IRQ";
        break;
    case EXCTYPE_NOTASSGN:
        string = "Not assigned";
        break;
    case EXCTYPE_SWI:
        string = "Software Interrupt";
        break;
    default:
        string = "???";
        break;
    }
    return(string);
}

void
vinit(void)
{
}
