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
 * i2c.h:
 *
 * Header for I-Squared-C code.
 *
 * Original author:     Ed Sutter (ed.sutter@alcatel-lucent.com)
 *
 */
#ifndef _I2C_H_
#define _I2C_H_

extern int i2cVerbose;

/********************************************************************
 *
 * Commands used by i2cCtrl():
 */
#define I2CCTRL_INIT	1
#define I2CCTRL_SHOW	2
#define	I2CCTRL_START	3		/* Send START condition */
#define	I2CCTRL_STOP	4		/* Send STOP condition */
#define	I2CCTRL_WADD	5		/* Send Write address */
#define	I2CCTRL_RADD	6		/* Send Read address */
#define	I2CCTRL_RDAT	7		/* Read data */
#define	I2CCTRL_WDAT	8		/* Write data */
#define	I2CCTRL_SACK	9		/* Send ACK */
#define	I2CCTRL_WACK	10		/* WaitFor ACK */

/********************************************************************
 *
 * Upper bits of the bigaddr integer used for special case read/write
 */
#define REPEATED_START	0x40000000
#define OMIT_STOP		0x20000000

/********************************************************************
 *
 * Functions that must be provided from some target-specific code.
 */


/* i2cCtrl()
 * Parameters:
 *	int interface-
 *		This parameter supports the case where the target hardware has more
 *		than one i2c controller.  The interface number would correspond to
 *		one of potentially several different controllers.
 *	int cmd-
 *		Command to be carried out by the control function.
 *	ulong arg1-
 *		First command-specific argument.
 *	ulong arg2-
 *		Second command-specific argument.
 */
extern int i2cCtrl(int interface,int cmd,unsigned long arg1,unsigned long arg2);

/* i2cRead()
 * Parameters:
 *	int interface-
 *		See description under i2cCtrl.
 *	int bigaddr-
 *		The device address on the I2C bus.  Referred to here as "big" because
 *		it is an "int" so it supports 10-bit addresses (if needed).
 *	uchar *data-
 *		Pointer to the data buffer into which the data is to be placed.
 *	int len-
 *		Number of bytes to be read from the I2C device.  This must not be
 *		larger than the size of the data buffer.
 * Return:
 *	int
 *		The function should return the number of bytes read.  If everything
 *		went well, then the return value should equal the len parameter.
 *		Negative represents some failure.
 */
extern int i2cRead(int interface,int bigaddr,unsigned char *data,int len);

/* i2cWrite()
 * Parameters:
 *	int interface-
 *		See description under i2cCtrl.
 *	int bigaddr-
 *		See description under i2cRead.
 *	uchar *data-
 *		Buffer from which the data is to be taken and put into the specified
 *		I2C device.
 *	int len-
 *		Number of bytes to be written.
 * Return:
 *	int
 *		The function should return the number of bytes written.  If everything
 *		went well, then the return value should equal the len parameter.
 *		Negative represents some failure.
 */
extern int i2cWrite(int interface,int bigaddr,unsigned char *data,int len);

/* i2cShow()
 * Parameters:
 *	int interface-
 *		See description under i2cCtrl.
 * Return:
 *	void
 *		The function should be part of target-specific code that simply
 *		prints out a list of the device names and their address on the 
 *		I-Squared-C bus of the specfied interface.
 */
extern void i2cShow(int interface);

#endif
