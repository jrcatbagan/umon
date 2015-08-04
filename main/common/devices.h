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
 * devices.h
 *
 * device structure:
 * This structure defines the basic information needed for device
 * control.  The table of devices is initialized at compile time
 * with each device assigned an integer device descriptor.
 *
 * Original author:     Ed Sutter (ed.sutter@alcatel-lucent.com)
 *
 */
#ifndef _DEVICES_H_
#define _DEVICES_H_

struct device {
    int (*init)(unsigned long);
    int (*ctrl)(int,unsigned long,unsigned long);
    int (*open)(unsigned long,unsigned long);
    int (*close)(int);
    int (*read)(char *,int);
    int (*write)(char *,int);
    char    *name;
};

extern struct device    devtbl[];

/* Generic driver functions: */
extern  int open(int, unsigned long, unsigned long);
extern  int close(int);
extern  int read(int, char *, int);
extern  int write(int, char *, int);
extern  int init(int, unsigned long);
extern  int ioctl(int, int, unsigned long, unsigned long);
extern  int devInit(int);
extern  int isbaddev(int);
extern  void devtbldump(void);

/* Common ioctl definitions: */
#define INIT        1
#define GOTACHAR    2
#define SETBAUD     3

#define nodriver    0

extern int ConsoleDevice;
#endif
