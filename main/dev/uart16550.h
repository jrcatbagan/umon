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
 * uart16550.h
 *
 * Defines for standard Dual Uart
 *
 * Original author:     Ed Sutter (ed.sutter@alcatel-lucent.com)
 *
 */

// If other than 1, SIO_STEP must be defined in config.h
#ifndef SIO_STEP
#define SIO_STEP 1
#endif

// register defines
#define SIO_RXD     (0 * SIO_STEP)  // receive data, read, dlab = 0
#define SIO_TXD     (0 * SIO_STEP)  // transmit data, write, dlab = 0
#define SIO_BAUDLO  (0 * SIO_STEP)  // baud divisor 0-7, read/write, dlab = 1
#define SIO_IEN     (1 * SIO_STEP)  // interrupt enable, read/write, dlab = 0
#define SIO_BAUDHI  (1 * SIO_STEP)  // baud divisor 8-15, read/write, dlab = 1
#define SIO_ISTAT   (2 * SIO_STEP)  // interrupt status, read, dlab = 0
#define SIO_FCTL    (2 * SIO_STEP)  // fifo control, write, dlab = 0
#define SIO_AFR     (2 * SIO_STEP)  // alt function reg, read/write, dlab = 1
#define SIO_LCTL    (3 * SIO_STEP)  // line control, read/write
#define SIO_MCTL    (4 * SIO_STEP)  // modem control read/write
#define SIO_LSTAT   (5 * SIO_STEP)  // line status read
#define SIO_MSTAT   (6 * SIO_STEP)  // modem status read
#define SIO_SPR     (7 * SIO_STEP)  // scratch pad register

// interrupt enable register bit defines
#define SIO_IEN_RXD     0x01    // enable int on rcv holding reg full
#define SIO_IEN_TXD     0x02    // enable int on xmt holding reg empty
#define SIO_IEN_LSTAT   0x04    // enable int on line status reg state change
#define SIO_IEN_MSTAT   0x08    // enable int on modem status reg state change

// interrupt status register bit defines
#define SIO_ISTAT_PEND  0x01    // 0 = interrupt pending
#define SIO_ISTAT_MASK  0x0e    // mask for interrupt ID bits
#define SIO_ISTAT_INT   0x0F    // bits 0-3 describe current highest priority
// interrupt pending.  They can be used to
// vector to the correct routine.
// The codes are as follows:
#define SIO_ISTAT_MI    0x00    // Modem Status Register interrupt
#define SIO_ISTAT_TI    0x02    // Transmit Holding Register Empty interrupt
#define SIO_ISTAT_RI    0x04    // Receive Holding Register Ready interrupt
#define SIO_ISTAT_LI    0x06    // Line Status Register interrupt
#define SIO_ISTAT_FI    0x0C    // Rcv Holding Register Fifo Timeout interrupt

// fifo control register bit defines
#define SIO_FCTL_FEN    0x01    // 1 = fifo operation enabled
#define SIO_FCTL_RXDC   0x02    // 1 = clear rxd fifo and reset counter,
//     returns to 0 automatically
#define SIO_FCTL_TXDC   0x04    // 1 = clear txd fifo and reset counter,
//     returns to 0 automatically
#define SIO_FCTL_MODE   0x08    // set txrdy and rxrdy modes, unused

// Bits 6 and 7 set the desired fifo trigger
// level as follows:
#define SIO_FCTL_T1     0x00    // 00 = trigger when 1 character in fifo
#define SIO_FCTL_T4     0x40    // 01 = trigger when 4 characters in fifo
#define SIO_FCTL_T8     0x80    // 10 = trigger when 8 characters in fifo
#define SIO_FCTL_T14    0xC0    // 11 = trigger when 14 characters in fifo

// line control register bit defines
// bits 0 and 1 set the word length as follows:
#define SIO_LCTL_W5     0x00    // 00 = 5
#define SIO_LCTL_W6     0x01    // 01 = 6
#define SIO_LCTL_W7     0x02    // 10 = 7
#define SIO_LCTL_W8     0x03    // 11 = 8

#define SIO_LCTL_STOP   0x04    // 0 = 1 stop bit, 1 = 1.5 stop bits if
//     word length is 5, 2 otherwise
#define SIO_LCTL_PAR    0x08    // 0 = no parity enabled, 1 = parity enabled
#define SIO_LCTL_EVEN   0x10    // 0 = odd parity, 1 = even parity,
//     if parity is enabled
#define SIO_LCTL_FRC    0x20    // 0 = force parity bit to 1,
// 1 = force to 0, if parity is enabled
#define SIO_LCTL_BRK    0x40    // 1 = send continuous break
#define SIO_LCTL_DLAB   0x80    // 1 = select baud and alt function registers

// modem control register bit defines
#define SIO_MCTL_DTR    0x01    // 0 = set DTR output = 0,
// 1 = set DTR = 1, unused
#define SIO_MCTL_RTS    0x02    // 0 = set RTS output = 0,
// 1 = set RTS = 1, channel A only
#define SIO_MCTL_OP2    0x08    // 0 = set OP2 output = 0,
// 1 = set OP2 = 1, unused
#define SIO_MCTL_LB     0x10    // 0 = normal operation,
// 1 = loopback mode

// line status register bit defines
#define SIO_LSTAT_RRDY  0x01    // 1 = receive register ready
#define SIO_LSTAT_OVER  0x02    // 1 = receive overrun error
#define SIO_LSTAT_PERR  0x04    // 1 = receive parity error
#define SIO_LSTAT_FRAM  0x08    // 1 = receive framing error
#define SIO_LSTAT_BRK   0x10    // 1 = receive break
#define SIO_LSTAT_TRDY  0x20    // 1 = transmit hold register empty
#define SIO_LSTAT_TEMTY 0x40    // 1 = transmit register empty
#define SIO_LSTAT_ERR   0x80    // 1 = any error condition

// modem status register bit defines
#define SIO_MSTAT_CTS   0x01    // 1 = CTS state has changed, channel A only
#define SIO_MSTAT_DSR   0x02    // 1 = DSR state has changed, unused
#define SIO_MSTAT_RI    0x04    // 1 = RI state has changed, unused
#define SIO_MSTAT_CD    0x08    // 1 = CD state has changed, unused
#define SIO_MSTAT_RTS   0x10    // reflects RTS bit during loopback, chA only
#define SIO_MSTAT_DTR   0x20    // reflects DTR bit during loopback, unused
#define SIO_MSTAT_OP1   0x40    // reflects OP1 bit during loopback, unused
#define SIO_MSTAT_OP2   0x80    // reflects OP2 bit during loopback, unused

extern void InitUART(int baud);
extern int ConsoleBaudSet(int baud);
extern int target_getchar(void);
extern int target_gotachar(void);
extern int target_putchar(char c);
extern int target_console_empty(void);
extern int getUartDivisor(int baud,unsigned char *hi, unsigned char *lo);

