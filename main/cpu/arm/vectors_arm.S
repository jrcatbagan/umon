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
 * vectors_arm.S
 *
 * Minimalist exception handlers...
 * Catch the exception, call generic handler with exception-specific id.
 *
 * Original author:     Ed Sutter (ed.sutter@alcatel-lucent.com)
 *
 */

#include "arm.h"

.global undefined_instruction
undefined_instruction:
mov r1, #EXCTYPE_UNDEF
b umon_exception

.global software_interrupt
software_interrupt:
mov r1, #EXCTYPE_SWI
b umon_exception

.global abort_prefetch
abort_prefetch:
mov r1, #EXCTYPE_ABORTP
b umon_exception

.global abort_data
abort_data:
mov r1, #EXCTYPE_ABORTD
b umon_exception

.global not_assigned
not_assigned:
mov r1, #EXCTYPE_NOTASSGN
b umon_exception

.global interrupt_request
interrupt_request:
mov r1, #EXCTYPE_IRQ
b umon_exception

.global fast_interrupt_request
fast_interrupt_request:
mov r1, #EXCTYPE_FIRQ
b umon_exception
