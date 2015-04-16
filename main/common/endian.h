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
 * endian.h:
 *
 * macros used for endian conversion.
 * The intent is that the macros have nil effect on Big_Endian systems...
 *
 *   ecs: endian-convert short
 *   ecl: endian-convert long
 *
 * Original author:     Ed Sutter (ed.sutter@alcatel-lucent.com)
 *
 */
#ifndef _ENDIAN_H_
#define _ENDIAN_H_

#ifdef CPU_LE
#define ecs(a) (((a & 0x00ff) << 8) | ((a & 0xff00) >> 8))
#define ecl(a) (((a & 0x000000ff) << 24) | ((a & 0x0000ff00) << 8) | \
		  ((a & 0x00ff0000) >> 8) | ((a & 0xff000000) >> 24))
#define self_ecs(a)	(a = ecs(a))
#define self_ecl(a)	(a = ecl(a))
#define ntohl	ecl
#define ntohs	ecs
#define htonl	ecl
#define htons	ecs
#else
#ifdef CPU_BE
#define ecs(a)  a
#define ecl(a)  a
#define self_ecs(a)
#define self_ecl(a)
#define ntohl(a)	a
#define ntohs(a)	a
#define htonl(a)	a
#define htons(a)	a
#else
#error  You need to define CPU_BE or CPU_LE in config.h!
#endif	/* else ifdef CPU_BE */
#endif	/* ifdef CPU_LE */

/* just to be safe...
 */
#ifdef CPU_LE
#ifdef CPU_BE
#error You have both CPU_BE and CPU_LE defined.  Pick one!
#endif
#endif

#endif
