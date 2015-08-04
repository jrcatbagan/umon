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
 * term.h:
 *
 * See term.c for details.
 *
 * Original author:     Ed Sutter (ed.sutter@alcatel-lucent.com)
 */
#ifndef _TERM_H_
#define _TERM_H_

#define TERMTYPE_UNDEFINED  0
#define TERMTYPE_VT100      1

/* Font attributes:
 */
#define ATTR_RESET      0
#define ATTR_BRIGHT     1
#define ATTR_DIM        2
#define ATTR_UNDERLINE  3
#define ATTR_BLINK      4
#define ATTR_REVERSE    7
#define ATTR_HIDDEN     8

/* Font colors:
 */
#define BLACK       0
#define RED         1
#define GREEN       2
#define YELLOW      3
#define BLUE        4
#define MAGENTA     5
#define CYAN        6
#define WHITE       7

extern int term_settextcolor(int fg);
extern int term_setbgcolor(int bg);
extern int term_resettext(void);
extern int term_getsize(int *row, int *col);

#endif


