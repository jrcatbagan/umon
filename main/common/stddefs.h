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
 *  stddefs.h:
 *
 * Original author:     Ed Sutter (ed.sutter@alcatel-lucent.com)
 *
 */
#ifndef _STDDEFS_H_
#define _STDDEFS_H_

typedef unsigned char       uint8_t;
typedef unsigned short      uint16_t;
typedef unsigned long       uint32_t;
typedef unsigned long long  uint64_t;

typedef signed char         int8_t;
typedef signed short        int16_t;
typedef signed long         int32_t;
typedef signed long long    int64_t;

typedef volatile unsigned char      vuint8_t;
typedef volatile unsigned short     vuint16_t;
typedef volatile unsigned long      vuint32_t;
typedef volatile unsigned long long vuint64_t;

typedef volatile signed char        vint8_t;
typedef volatile signed short       vint16_t;
typedef volatile signed long        vint32_t;
typedef volatile signed long long   vint64_t;


typedef unsigned char       uchar;
typedef unsigned short      ushort;
typedef unsigned long       ulong;
typedef unsigned int        uint;

typedef volatile unsigned char  vuchar;
typedef volatile unsigned short vushort;
typedef volatile unsigned long  vulong;
typedef volatile unsigned int   vuint;
typedef volatile int            vint;


#endif
