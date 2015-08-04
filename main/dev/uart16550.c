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
 * FILENAME_HERE
 *
 * Generic (hopefully) 16550 uart code for use as a MicroMonitor
 * console serial port driver.
 *
 * The baud-rate divisor can be derived in one of two ways:
 * If BRD_115200 is defined, then this code assumes that the full
 * set of BRD_XXX definitions are defined (in config.h) and they
 * are used.  If BRD_115200 is not defined, then this driver assumes
 * the getUartDivisor() function is externally provided by the
 * port-specific code.
 *
 * The base address of the UART used as the console port must be
 * defined as CONSOLE_UART_BASE (also in config.h).
 *
 * Also, if the uart is configured in the memory map such that the
 * gap between registers is not 1, then set SIO_STEP (see uart16550.h)
 * to 2 or 4 appropriately.
 *
 * Original author:     Ed Sutter (ed.sutter@alcatel-lucent.com)
 *
 */
#include "config.h"
#include "uart16550.h"


#define CONSOLE(offset) *(volatile unsigned char *)(CONSOLE_UART_BASE+offset)

#ifdef MORE_GOTACHAR
extern void MORE_GOTACHAR();
#endif
#ifdef MORE_GETCHAR
extern void MORE_GETCHAR();
#endif
#ifdef MORE_PUTCHAR
extern void MORE_PUTCHAR();
#endif

/* Establish default initialization values that can be overridden
 * in config.h:
 */
#ifndef MCTL_DEFAULT
#define MCTL_DEFAULT    (SIO_MCTL_DTR | SIO_MCTL_RTS)
#endif
#ifndef FCTL_DEFAULT
#define FCTL_DEFAULT    (SIO_FCTL_FEN | SIO_FCTL_RXDC | SIO_FCTL_TXDC)
#endif
#ifndef SPR_DEFAULT
#define SPR_DEFAULT     0x00        // Clear scratchpad
#endif
#ifndef IEN_DEFAULT
#define IEN_DEFAULT     0x00        // Ints used
#endif
#ifndef LCTL_DEFAULT
#define LCTL_DEFAULT    SIO_LCTL_W8
#endif

/* getDlLsb():
 * Populate hi & lo, default to 9600...
 */
#ifdef BRD_115200
int
getUartDivisor(int baud, unsigned char *hi, unsigned char *lo)
{
    switch(baud) {
    case 115200:
        *lo = BRD_115200;
        break;
    case 57600:
        *lo = BRD_57600;
        break;
    case 38400:
        *lo = BRD_38400;
        break;
    case 19200:
        *lo = BRD_19200;
        break;
    case 9600:
        *lo = BRD_9600;
        break;
    default:
        return(-1);
    }
    *hi = 0;
    return(0);
}
#endif

int
ConsoleBaudSet(int baud)
{
    unsigned char   tmp, hi, lo;

    /* If either getDivisor returns -1 or both hi & lo are zero we
     * return failure to indicate that the requested baud rate could
     * not be established.
     */
    if(getUartDivisor(baud,&hi,&lo) == -1) {
        return(-1);
    }
    if((hi == 0) && (lo == 0)) {
        return(-1);
    }

    tmp = CONSOLE(SIO_LCTL);    /* Save linectl reg */
    CONSOLE(SIO_LCTL) = SIO_LCTL_DLAB;
    CONSOLE(SIO_BAUDLO) = lo;   /* Set baud */
    CONSOLE(SIO_BAUDHI) = hi;
    CONSOLE(SIO_LCTL) = tmp;    /* Restore linectl reg */
    return(0);
}


void
InitUART(int baud)
{
    unsigned char   tmp, hi, lo;

    getUartDivisor(baud,&hi,&lo);

    CONSOLE(SIO_LCTL) = SIO_LCTL_DLAB;
    CONSOLE(SIO_BAUDLO) = lo;           /* Set baud */
    CONSOLE(SIO_BAUDHI) = hi;
    CONSOLE(SIO_LCTL) = LCTL_DEFAULT;   /* 8-bits, no parity */
    CONSOLE(SIO_MCTL) = MCTL_DEFAULT;
    tmp = CONSOLE(SIO_LSTAT);           /* clear line stat */
    tmp = CONSOLE(SIO_RXD);             /* read receive buffer */
    tmp = tmp;                          /* eliminate unused warning */
    CONSOLE(SIO_IEN) = IEN_DEFAULT;
    CONSOLE(SIO_FCTL) = FCTL_DEFAULT;
    CONSOLE(SIO_SPR) = SPR_DEFAULT;
}

int
target_console_empty(void)
{
    if(CONSOLE(SIO_LSTAT) & SIO_LSTAT_TRDY) {
        return(0);
    }
    return(1);
}

int
target_putchar(char c)
{
    while(!(CONSOLE(SIO_LSTAT) & SIO_LSTAT_TRDY));
    CONSOLE(SIO_TXD) = c;
#ifdef MORE_PUTCHAR
    MORE_PUTCHAR(c);
#endif
    return((int)c);
}

int
target_getchar(void)
{
    char c;

#ifdef MORE_GETCHAR
    if(MORE_GOTACHAR()) {
        c = MORE_GETCHAR();
    } else
#endif
        c = CONSOLE(SIO_RXD);
    return((int)c);
}

int
target_gotachar(void)
{
#if INCLUDE_BLINKLED
    extern void TARGET_BLINKLED(void);

    TARGET_BLINKLED();
#endif
    if(CONSOLE(SIO_LSTAT) & SIO_LSTAT_RRDY) {
        return(1);
    }
#ifdef MORE_GOTACHAR
    if(MORE_GOTACHAR()) {
        return(1);
    }
#endif
    return(0);
}
