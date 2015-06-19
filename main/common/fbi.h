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
 * fbi.h:
 *
 * Frame Buffer Interface...
 *
 * Original author:     Ed Sutter (ed.sutter@alcatel-lucent.com)
 *
 */
#include "config.h"

#if	INCLUDE_FBI

#define fbi_putchar(a)	fbi_putchar(a)
extern void fbi_putchar(char c);

#ifdef FBI_NO_CURSOR
#define fbi_cursor()
#else
#define fbi_cursor(a)	fbi_cursor(a)
extern void fbi_cursor(void);
#endif

#else

#define fbi_putchar(a)
#define fbi_cursor()

#endif

extern void fbi_setpixel(int x, int y, long rgbcolor);
extern void fbi_splash(void);
extern void fbdev_init(void);
