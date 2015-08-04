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
 * flashram.c
 *
 * Original author:     Ed Sutter (ed.sutter@alcatel-lucent.com)
 *
 */

#include "config.h"

#ifdef FLASHRAM_BASE
/* As of Jan 2004, this module is no longer needed.
 * I added an option to FlashRamInit() so that if the incoming sector
 * size table is NULL, then all sectors are initialized to the same
 * size.  For cases where this table was used, the call to FlashRamInit
 * should have the ssizes argument changed to zero.  See FlashRamInit()
 * for more on this.
 */
#if 0
#include "flash.h"

/* Generic Flash RAM configuration information...
 * The assumption is a 16-element array (FLASHRAM_SECTORCOUNT = 16).
 *
 * This can be included in a monitor build if the build has
 * a block of RAM dedicated to TFS file storage.  If some other
 * configuration is required, then copy this to target-specific space
 * and modify a local version.
 */
int
ramSectors[FLASHRAM_SECTORCOUNT] = {
    FLASHRAM_SECTORSIZE, FLASHRAM_SECTORSIZE,
    FLASHRAM_SECTORSIZE, FLASHRAM_SECTORSIZE,
    FLASHRAM_SECTORSIZE, FLASHRAM_SECTORSIZE,
    FLASHRAM_SECTORSIZE, FLASHRAM_SECTORSIZE,
    FLASHRAM_SECTORSIZE, FLASHRAM_SECTORSIZE,
    FLASHRAM_SECTORSIZE, FLASHRAM_SECTORSIZE,
    FLASHRAM_SECTORSIZE, FLASHRAM_SECTORSIZE,
    FLASHRAM_SECTORSIZE, FLASHRAM_SECTORSIZE,
};
#endif
#endif
