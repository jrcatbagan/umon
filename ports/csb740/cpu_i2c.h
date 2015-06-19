//==========================================================================
//
//      cpu_i2c.c
//
// Author(s):   Michael Kelly - Cogent Computer Systems, Inc.
// Date:        03/30/2002
// Description:	CSB272 - 405GP Single Board IIC Routines 
//
//==========================================================================

#include "config.h"
#include "cpuio.h"
#include "genlib.h"
#include "stddefs.h"

//--------------------------------------------------------------------------
// some 405GP I2C register and bit defines
//
#define I2C_XCTL				*(vulong *)(0xef60050f)	// extended control register
#define I2C_XCTL_SRST			0x01000000	// Soft Reset Bit - must be set 
											// to 1 to use direct control

#define I2C_DCTL				*(vulong *)(0xef600510)	// direct control of IIC bits
#define I2C_DCTL_SDA_OUT		0x08000000	// SDA Out, 0 = drive low, 1 = tri-state 
#define I2C_DCTL_SCL_OUT		0x04000000	// SCL Out, 0 = drive low, 1 = tri-state 
#define I2C_DCTL_SDA_IN			0x02000000	// SDA In, Direct Read
#define I2C_DCTL_SCL_IN			0x01000000	// SCL In, Direct Read

//--------------------------------------------------------------------------
// Low level I2C Macros
//
#define I2C_SCL_CLR				I2C_DCTL &= ~(I2C_DCTL_SCL_OUT)
#define I2C_SCL_SET				I2C_DCTL |= (I2C_DCTL_SCL_OUT) 	

#define I2C_SDA_CLR				I2C_DCTL &= ~(I2C_DCTL_SDA_OUT)		
#define I2C_SDA_SET				I2C_DCTL |= (I2C_DCTL_SDA_OUT) 		

#define I2C_SDA_RD				I2C_DCTL & I2C_DCTL_SDA_IN
#define I2C_SCL_RD				I2C_DCTL & I2C_DCTL_SCL_IN

#define I2C_DELAY 				udelay(100)

//--------------------------------------------------------------------------
// function prototypes
//
ulong i2c_init(void);
ulong i2c_wr_device(uchar dev, uchar reg, uchar data);
ulong i2c_rd_device(uchar dev, uchar reg, uchar *data);
ulong i2c_wr_byte(uchar data);
uchar i2c_rd_byte(void);
void i2c_start(void);
void i2c_stop(void);

extern void udelay(int delay);

extern ulong sed_disp_mode;

//--------------------------------------------------------------------------
// i2c_init()
//
// Initialize the I2C registers for direct I2C control
ulong i2c_init()
{
	// place the automatic I2C logic in reset
	I2C_XCTL |= I2C_XCTL_SRST;

	// Set the SCL and SDA outputs into tristate
	I2C_DCTL |= (I2C_DCTL_SDA_OUT | I2C_DCTL_SCL_OUT);

	return 0;

}

//--------------------------------------------------------------------------
// i2c_wr_device()
//
// This function writes an 8-bit value to the I2C device at the requested
// register.
//
ulong i2c_wr_device(uchar dev, uchar reg, uchar data)
{

	// issue a start command
	i2c_start();

	// write the 7-bit device address with write = 0
	if(i2c_wr_byte((dev << 1) & 0xfe)){
		return -1;
	}
	// Write the 8-bit register address
	if(i2c_wr_byte(reg)){
		return -1;
	}
	// Write the 8-bit data value
	if(i2c_wr_byte(data)){
		return -1;
	}

	// issue a stop
	i2c_stop();
	
	return 0;
}

//--------------------------------------------------------------------------
// i2c_rd_device()
//
// This function reads an 8-bit value from the I2C device at the requested
// register.
//
ulong i2c_rd_device(uchar dev, uchar reg, uchar *data)
{

	// issue a start command
	i2c_start();

	// write the 7-bit device address with write = 0
	if(i2c_wr_byte((dev << 1) & 0xfe)){
		return -1;
	}
	// Write the 8-bit register address
	if(i2c_wr_byte(reg)){
		return -1;
	}
	// repeat the start command
	i2c_start();
	// write the 7-bit device address again plus data direction (read = 1)
	if(i2c_wr_byte((dev << 1) | 0x01)){
		return -1;
	}
	*data = i2c_rd_byte();

	// issue a stop
	i2c_stop();

	return 0;
}

//--------------------------------------------------------------------------
// i2c_wr_byte()
//
// This function writes an 8-bit value to the I2C bus, MSB first.
// Data is written by changing SDA during SCL low, then bringing
// SCL high.  SCL is returned low to setup for the next transition.
//
ulong i2c_wr_byte(uchar data)
{

	int i;

	for (i = 0; i < 8; i++){
		if (data & 0x80) {
			// write a 1 bit
			I2C_SDA_SET;
			I2C_DELAY;
			I2C_SCL_SET;
			I2C_DELAY;
			I2C_SCL_CLR;
			I2C_DELAY;
		}
		else {
			// write a 0 bit
			I2C_SDA_CLR;
			I2C_DELAY;
			I2C_SCL_SET;
			I2C_DELAY;
			I2C_SCL_CLR;
			I2C_DELAY;
		}
		data = data << 1;
	}
	// Release SDA, bring SCL high, then read SDA.
	// A low indicates an acknowledge.
	I2C_SDA_SET;
	I2C_DELAY;
   	I2C_SCL_SET;
	I2C_DELAY;
	if(I2C_SDA_RD){	// a high means no ack
		// re-enable SDA for output
	   	I2C_SCL_CLR;
		I2C_DELAY;
		return -1;
	}

   	I2C_SCL_CLR;
	I2C_DELAY;

	return 0;
}

//--------------------------------------------------------------------------
// i2c_rd_byte()
//
// This function reads an 8-bit data value from the I2C bus, MSB first. 
// Data is read from SDA after each low to high SCL transition.
//
uchar i2c_rd_byte()
{

	int i;
	uchar volatile data;

	data = 0;

	for (i = 0; i < 8; i++){
		data = data << 1;
		data = data & 0xfe;
		// clock the data out of the slave
		I2C_SCL_SET;
		I2C_DELAY;
		// check it
		if (I2C_SDA_RD){
			data = data | 0x01;
		}
		I2C_SCL_CLR;
		I2C_DELAY;
	}
	// generate an extra SCL transition
	// The slave generates no acknowledge for reads.
	I2C_SCL_SET;
	I2C_DELAY;
	I2C_SCL_CLR;
	I2C_DELAY;

	return data;
}


//--------------------------------------------------------------------------
// i2c_start()
//
// This function issues an I2C start command which is a high to low
// transition on SDA while SCL is high.
//
void i2c_start()
{

	I2C_SDA_SET;
	I2C_DELAY;
   	I2C_SCL_SET;
	I2C_DELAY;
   	I2C_SDA_CLR;
	I2C_DELAY;
   	I2C_SCL_CLR;
	I2C_DELAY;
	I2C_SDA_SET;
	I2C_DELAY;
}

//--------------------------------------------------------------------------
// i2c_stop()
//
// This function issues an I2C stop command which is a low to high
// transition on SDA while SCL is high.
//
void i2c_stop()
{

	I2C_SDA_CLR;
	I2C_DELAY;
	I2C_SCL_SET;
	I2C_DELAY;
	I2C_SDA_SET;
	I2C_DELAY;
	I2C_SCL_CLR;
	I2C_DELAY;
}


