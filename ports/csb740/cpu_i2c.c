//==========================================================================
//
//      cpu_i2c.c
//
// Author(s):   Michael Kelly - Cogent Computer Systems, Inc.
// Date:        03/26/2003
// Description:	Generic IIC Routines - requires I2C_SCL and I2C_SDA to
//				be defined in cpu_gpio.h
//
//==========================================================================

#include "config.h"
#include "cpuio.h"
#include "genlib.h"
#include "stddefs.h"
#include "cli.h"
#include "cpu_gpio.h"
#include "cpu_gpio.h"
#include "umongpio.h"

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
int i2c(int argc,char *argv[]);

extern void udelay(int delay);

//--------------------------------------------------------------------------
// I2C Macros
//
#define I2C_SCL_CLR             GPIO_out(I2C_SCL)				
#define I2C_SCL_SET             GPIO_in(I2C_SCL)				 	

#define I2C_SDA_CLR				GPIO_out(I2C_SDA)		
#define I2C_SDA_SET				GPIO_in(I2C_SDA) 		

#define I2C_SCL_RD				GPIO_tst(I2C_SCL)
#define I2C_SDA_RD				GPIO_tst(I2C_SDA)

#define I2C_DELAY 				udelay(1000)

//--------------------------------------------------------------------------
// i2c_init()
//
// I2C is a shared bus.  We drive a low by setting the SCL/SDA GPIO as
// an output.  We must preset a 0 in the GPIO output bit so the line will
// go low whenever we make it an output.  For a high, we make the GPIO an
// input, thus letting the external pullup to pull the line high.
//
ulong i2c_init()
{
    GPIO_out(I2C_SCL);
    GPIO_clr(I2C_SCL);
    GPIO_in(I2C_SCL);
	
    GPIO_out(I2C_SDA);
    GPIO_clr(I2C_SDA);
    GPIO_in(I2C_SDA);
	
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

char *i2cHelp[] = {
	" This command allows the user to read ",
	" and write devices on the i2c bus. \n ",
    " Usage:",
    " i2c -[w] {device} {register} {val/count}",
    " Options...",
    " -w write val to device/register",
	" no options, read from device/register up to count",
	0
};

int i2c(int argc,char *argv[])
{
	int i, opt;
	int write = 0;
	uchar dev, reg, data, count;

    while ((opt=getopt(argc,argv,"w")) != -1) {
    if (opt == 'w') write = 1;
	}

	// make sure we have the right number of paramters
	if (argc < (optind+3))
		return(CMD_PARAM_ERROR);

	dev = (uchar) strtoul(argv[optind],(char **)0,0);
	reg = (uchar) strtoul(argv[optind+1],(char **)0,0);

	// 3rd arg is the data value if it's a write, count if it's a read
	data = (uchar) strtoul(argv[optind+2],(char **)0,0);
	count = data;
	// do it
	if (write)
	{
		printf("Writing 0x%02x to Device 0x%02x @ Register 0x%02x.\n", data, dev, reg);
    	if(i2c_wr_device(dev, reg, data))
    	{
			printf("I2C Bus Failure - Check Paramters!\n");
    		return (CMD_FAILURE);
		}
	}
	else
	{
		for (i = 0; i < count; i++)
    	{
			printf("Read Device 0x%02x, Register 0x%02x = ", dev, reg + i);
			if(i2c_rd_device(dev, reg + i, &data))
			{
				printf("I2C Bus Failure - Check Paramters!\n");
    			return (CMD_FAILURE);
			}
			printf("0x%02x.\n", data);
		}
	}
	return(CMD_SUCCESS);
}

