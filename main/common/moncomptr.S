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
 * moncomptr.S:
 *
 * The tag moncomptr is a tag established at a point in the target's
 * memory map that is unlikely to change as a result of rebuilding the
 * monitor.   As a result, this file is included by most of the target
 * ports in the very beginning of the reset space.
 *
 * The address of moncom (moncomptr) is used by the monConnect()
 * function in application space when the application attempts to
 * connect to the monitor's API.
 *
 * Original author:     Ed Sutter (ed.sutter@alcatel-lucent.com)
 *
 */
.extern moncom
.global moncomptr

moncomptr:
.long   moncom
