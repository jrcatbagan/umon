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
 * boardinfo.h:
 *
 * Structures and data used by boardinfo facility.
 *
 * Original author:     Ed Sutter (ed.sutter@alcatel-lucent.com)
 *
 */
#ifndef _BOARDINFO_H_
#define _BOARDINFO_H_

struct boardinfo {
	unsigned char *array;
	short size;
	char *varname;
	char *def;
	char *prompt;
};

struct boardinfoverify {
	unsigned short len;
	unsigned short crc16;
}; 

#define BISIZE		sizeof(struct boardinfo)
#define BIVSIZE		sizeof(struct boardinfoverify)

/* boardinfotbl[]:
 * If the "boardinfo" facility in the monitor is to be used, then
 * this table must be included in the target-specific portion of
 * the monitor build.
 */
extern struct boardinfo boardinfotbl[];

extern void BoardInfoInit(void), BoardInfoEnvInit(void);
extern int BoardInfoVar(char *);
#endif
