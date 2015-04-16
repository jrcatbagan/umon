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
 * alttfsdevtbl.S:
 *
 * The use of alt_tfsdevtbl_base within uMon allows the "tfs cfg"
 * command to be used by versions of uMon that are relocated to
 * RAM.  This applies to the "ramtst" build for a given port,
 * but more importantly, it applies to those ports that have to
 * relocate themselves to RAM just to successfully boot.
 *
 * The reason this is needed is because this block of empty flash
 * is placed at a fixed location in the memory space (usually
 * next to moncomptr and etheradd) so that other non-flash-based
 * versions can find this location through a tag in the linker
 * map file.  That tag must be set to point to the physical location
 * at which alt_tfsdevtbl_base is placed (similar to etheradd).
 * 
 * So, when building with the following line in config.h...
 *
 * #define TFS_ALTDEVTBL_BASE      &alt_tfsdevtbl_base
 *
 * the linker map file used by any run-from-ram version of that
 * port must contain the tag that initializes this base address
 * something like this (where ALTTFSDEVTBLBASE is replaced by
 * some fixed address)...
 *
 * alt_tfsdevtbl_base	= ALTTFSDEVTBLBASE;
 *
 * Original author:     Ed Sutter (ed.sutter@alcatel-lucent.com)
 *
 */

	.global	alt_tfsdevtbl_base

	.balign	0x10

alt_tfsdevtbl_base:
    .byte 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
    .byte 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
    .byte 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
    .byte 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
    .byte 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
    .byte 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
    .byte 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
    .byte 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
    .byte 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
    .byte 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
    .byte 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
alt_tfsdevtbl_end:
