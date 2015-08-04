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
 * font.h
 *
 * Original author:     Ed Sutter (ed.sutter@alcatel-lucent.com)
 *
 */
struct font {
    char  *bitmap;      /* Pointer to font bitmap array. */
    int width;          /* Width of font in array. */
    int height;         /* Height of font in array. */
    int above;          /* Number of pixels of empty space above. */
    int below;          /* Number of pixels of empty space blow. */
    int between;        /* Number of pixels of separation between each char. */
};

extern char font8x12[];
extern struct font font_styles[];
extern int font_style_total(void);

#define MAX_FONT_WIDTH      8
#define MIN_FONT_WIDTH      8

#define FONT_WHITE          0x00ffffff
#define FONT_BLACK          0x00000000
#define FONT_TRANSPARENT    0x01000000
#define FONT_INVERT         0x02000000

#define OPAQUE_BACKGROUND(x)    ((x & FONT_TRANSPARENT) == 0)
