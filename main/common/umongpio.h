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
 * gpio.h
 * DESCRIPTION HERE
 * A common way of looking at general purpose IO.
 *
 * This header is included by all code that uses uMon's general purpose
 * GPIO interface mechanism.  The idea is as follows:
 * Each bit has two values associated with it:
 *
 * bit:
 *  The bit number within the register it is assigned (starting from 0).
 *
 * vbit:
 *  The bit number within a device which may have multiple registers of GPIO.
 *  For example, if a device has three 32-bit registers for GPIO, then there
 *  would be a total of 96 port numbers where 0-31 represent the bits of
 *  register 0, 32-63 represetnt the bits of register 1 and 64
 *
 * For each bit, the GPIO(vbit,bit) macro uses fields within a 32-bit value to
 * define this information; hence, one 32-bit value contains both the register
 * and device specific bit position.
 *
 * Original author:     Ed Sutter (ed.sutter@alcatel-lucent.com)
 *
 */
#ifndef _UMONGPIO_H_
#define _UMONGPIO_H_

#define GPIO_BITNUM_MASK    0x000000ff
#define GPIO_VBITNUM_MASK   0xffff0000
#define GPIO(vbit,bit)      (((vbit << 16) & GPIO_VBITNUM_MASK) | \
                             (bit & GPIO_BITNUM_MASK))
#define GPIOVBIT(val)       ((val & GPIO_VBITNUM_MASK) >> 16)
#define GPIOBIT(val)        (1 << (val & GPIO_BITNUM_MASK))

extern void GPIO_init(void);

/* Each of these functions uses the "virtual" bit number...
 */
extern int GPIO_set(int vbit);
extern int GPIO_clr(int vbit);
extern int GPIO_tst(int vbit);
extern int GPIO_in(int  vbit);
extern int GPIO_out(int vbit);

#endif
