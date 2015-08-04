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
 * version.h:
 *
 * The version number for MicroMonitor is 3 'dot' separated numbers.
 * Each number can be as large as is needed.
 *
 *  MAJOR_VERSION.MINOR_VERSION.TARGET_VERSION
 *
 * MAJOR & MINOR apply to the common code applicable to all targets.
 * TARGET_VERSION applies to the target-specific code.
 *
 * Original author:     Ed Sutter (ed.sutter@alcatel-lucent.com)
 *
 */
#ifndef _VERSION_H_
#define _VERSION_H_

/* MAJOR_VERSION:
 * Incremented as a result of a major change or enhancement to the core
 * monitor code or as a means of dropping the MINOR_VERSION back to zero;
 * hence, simply identifying a significant set of MINOR changes or some
 * big change.
 * As of June 2015, Micromonitor is hosted by RTEMS; is based on a subset
 * of the uMon1.19 code and is licensed under APACHE.
 * To signify this, the MAJOR.MINOR number starts at 3.0.
 */
#define MAJOR_VERSION   3

/* MINOR_VERSION:
 */
#define MINOR_VERSION   0

/* TARGET_VERSION:
 * Incremented as a result of a bug fix or change made to the
 * target-specific (i.e. port) portion of the code.
 *
 * To keep a "cvs-like" log of the changes made to a port that is
 * not under CVS, it is recommended that the target_version.h file be
 * used as the log to keep track of changes in one place.
 */
#include "target_version.h"

#endif
