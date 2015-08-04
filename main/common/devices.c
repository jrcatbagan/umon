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
 * devices.c:
 *
 * This code provides the monitor with the ability to treat the peripherals
 * as an entry in a device table so that each device can be accessed through
 * read, write, ioctl, init, open and close.
 * The device table header file is defined as part of the target-specific
 * code and contains all of the target-specific device entries.
 *
 * Original author:     Ed Sutter (ed.sutter@alcatel-lucent.com)
 *
 */
#include "devtbl.h"
#include "devices.h"
#include "genlib.h"
#include "stddefs.h"

/* devtbldump():
 * Dump the contents of the device table.
 */
void
devtbldump()
{
    int i;

    /* Dump the device table and add an asterisk next to the console device. */
    for(i=0; i<DEVTOT; i++) {
        printf("Dev %d: %s%s\n",i,devtbl[i].name,
               ConsoleDevice == i ? " (console)" : "");
    }
}

/* isbaddev():
 * Return the result of a basic range check of the device number.
 */
int
isbaddev(int dev)
{
    if((dev < 0) || (dev >= DEVTOT)) {
        printf("Invalid device: %d\n",dev);
        return(1);
    }
    return(0);
}

/* Upper half device driver interfaces: */

int
init(int dev,ulong arg)
{
    if(isbaddev(dev) || !devtbl[dev].init) {
        return(-1);
    }
    return (devtbl[dev].init(arg));
}

int
open(int dev,ulong arg1,ulong arg2)
{
    if(isbaddev(dev) || !devtbl[dev].open) {
        return(-1);
    }
    return (devtbl[dev].open(arg1,arg2));
}

int
close(int dev)
{
    if(isbaddev(dev) || !devtbl[dev].close) {
        return(-1);
    }
    return (devtbl[dev].close(dev));
}

int
ioctl(int dev,int func,ulong data1,ulong data2)
{
    if(isbaddev(dev) || !devtbl[dev].ctrl) {
        return(-1);
    }
    return (devtbl[dev].ctrl(func,data1,data2));
}

int
read(int dev,char *buf,int cnt)
{
    if(isbaddev(dev) || !devtbl[dev].read) {
        return(-1);
    }
    return (devtbl[dev].read(buf,cnt));
}

int
write(int dev,char *buf,int cnt)
{
    if(isbaddev(dev) || !devtbl[dev].read) {
        return(-1);
    }
    return (devtbl[dev].write(buf,cnt));
}

/* devInit():
 * Step through the device table and execute each devices' initialization
 * function (if any).  If any device initialization fails, print out an
 * error message and immediately branch to the montor's command loop.
 * Since it is through this point that the monitor's UARTs are first
 * initialized, we pass in a baudrate that is then handed to each of the
 * device initialization routines (although it is only used by serial
 * devices).
 */
int
devInit(int baud)
{
    int fd;

    if(baud == 0) {
        baud = ConsoleBaudRate;
    }

    /* Keep track of the last baud rate, so that it can be used if the
     * incoming baudrate is NULL.
     */
    ConsoleBaudRate = baud;

    fd = 0;
    for(fd=0; fd<DEVTOT; fd++) {
        /* If device table does not have an init function, skip over it. */
        if(!devtbl[fd].init) {
            continue;
        }
        if(devtbl[fd].init((ulong)baud) != 0) {
            printf("devInit() <%s> FAILED\n",devtbl[fd].name);
            CommandLoop();
        }
    }
    return(0);
}
