//=============================================================================
//
//      cpuio.c
//
//      CPU/Board Specific IO
//
// Author(s):    Luis Torrico, Cogent Computer Systems, Inc.
// Date:         12/04/2008
// Description:  This file contains the IO functions required by Micro Monitor
//				 that are unique to the CSB740
//
//
//=============================================================================

#include "config.h"
#include "cpuio.h"
#include "genlib.h"
#include "ether.h"
#include "stddefs.h"
#include "warmstart.h"
#include "omap3530.h"
#include "omap3530_mem.h"
#include "cpu_gpio.h"
#include "fb_draw.h"
#include "uart16550.h"
#include "umongpio.h"
#include "ad7843.h"

#define __raw_readl(a)    (*(volatile unsigned int *)(a))

extern ulong i2c_init(void);
extern ulong getpsr(void);
extern void putpsr(ulong);

uchar bcolor=0;		// vga black

/******************************************************
// Delay for some usecs. - Not accurate, assumes ROM mode
// and no Cache
 ******************************************************/
void udelay(int delay)
{
  volatile int i;
  for ( i = LOOPS_PER_USEC * delay; i ; i--);
}

/******************************************************
 * Routine: wait_for_command_complete
 * Description: Wait for posting to finish on watchdog
 ******************************************************/
void wait_for_command_complete(unsigned int wd_base)
{
	int pending = 1;
	do {
		pending = __raw_readl(wd_base + WD_WWPS);
	} while (pending);
}

/******************************************************
// getUARTDivisor is called from UART16550.c
 ******************************************************/
int
getUartDivisor(int baud, unsigned char *hi, unsigned char *lo)
{
	*lo = ((48000000/16)/baud) & 0x00ff;
	*hi = (((48000000/16)/baud) & 0xff00) >> 8;
	return(0);
}

/******************************************************
// Set pads (pins) to correct mode.  Refer to section 7.4.4
	in TI omap35xx_tech_ref_manual for bit defines.
 ******************************************************/
void pads_init()
{
	// Set up chip selects
	SCM_REG(PADCONFS_GPMC_NCS3) = 0x00180018;	// NCS3[15:0], NCS4[31:16]
	SCM_REG(PADCONFS_GPMC_NCS5) = 0x011C0018;	// NCS5[15:0], EXP_INTX[31:16]
	
	// Set LCD_BKL_X to output, pullup enabled, mode 4
	// Set LCLK to output, no pull-type and disabled, mode 0
	SCM_REG(PADCONFS_GPMC_NCS7) = 0x0000001C; 	// LCD_BKL_X[15:0], LCLK(or GPIO_59)[31:16]
	
	// Set LCD pads to outputs, pull-type = up, pullud disabled, mode 0
	SCM_REG(PADCONFS_DSS_PCLK) = 0x00100010;	// LCD_PCLK_X[15:0], LCD_HS_X[31:16]
	SCM_REG(PADCONFS_DSS_VSYNC) = 0x00100010;	// LCD_VS_X[15:0], LCD_OE_X[31:16]
	SCM_REG(PADCONFS_DSS_DATA0) = 0x00100010;	// LCD_B0_X[15:0], LCD_B1_X[31:16]
	SCM_REG(PADCONFS_DSS_DATA2) = 0x00100010;	// LCD_B2_X[15:0], LCD_B3_X[31:16]
	SCM_REG(PADCONFS_DSS_DATA4) = 0x00100010; 	// LCD_B4_X[15:0], LCD_B5_X[31:16]
	SCM_REG(PADCONFS_DSS_DATA6) = 0x00100010;	// LCD_G0_X[15:0], LCD_G1_X[31:16]
	SCM_REG(PADCONFS_DSS_DATA8) = 0x00100010;	// LCD_G2_X[15:0], LCD_G3_X[31:16]
	SCM_REG(PADCONFS_DSS_DATA10) = 0x00100010; 	// LCD_G4_X[15:0], LCD_G5_X[31:16]
	SCM_REG(PADCONFS_DSS_DATA12) = 0x00100010;	// LCD_R0_X[15:0], LCD_R1_X[31:16]
	SCM_REG(PADCONFS_DSS_DATA14) = 0x00100010;	// LCD_R2_X[15:0], LCD_R3_X[31:16]
	SCM_REG(PADCONFS_DSS_DATA16) = 0x00100010;	// LCD_R4_X[15:0], LCD_R5_X[31:16]
	
	// Set D_TXD for output and D_RXD for input.  Set both to pullup enabled and mode 1
	SCM_REG(PADCONFS_MCBSP3_CLKX) = 0x01190019;	// D_TXD[15:0], D_RXD[31:16]
	
#ifdef AD7843_GPIOMODE
	// Depending on AD7843_GPIOMODE setting, we either configure the SPI 
	// interface to the AD7843 as a real SPI device using the OMAP's SPI
	// controller, or we set it up with GPIO bits... 
	SCM_REG(PADCONFS_DSS_DATA18) = 0x00040004; 
	SCM_REG(PADCONFS_DSS_DATA20) = 0x00040104;
#else
	SCM_REG(PADCONFS_DSS_DATA18) = 0x0002011A; 	// SPI1_CLK_X[15:0], SPI1_MOSI_X[31:16]
	SCM_REG(PADCONFS_DSS_DATA20) = 0x001A011A; 	// SPI1_MISO_X[15:0], *SPI1_CS0_X[31:16]
#endif

	// Set PIRQ for ADS7843 touch interrupt.  Set both to pullup enabled and mode 4
	SCM_REG(PADCONFS_MMC1_DAT4) = 0x01040104; 	// *I2C_INT_X[15:0], *PIRQ_X[31:16]
	
	// GPIO1 is the push button on CSB703(set as input), GPIO0 is is LED on CSB703(set as output)
	SCM_REG(PADCONFS_MMC1_DAT6) = 0x01040004; 	// GPIO0_X[15:0], GPIO1_X[31:16]
	
	// Set E_INT* to be an input in GPIO mode
	SCM_REG(PADCONFS_GPMC_WAIT2) = 0x011C011f; 	// NA[15:0], E_INTX[31:16]
	
  	// Set SYS_CLKOUT1 for USB_CLK
  	SCM_REG(PADCONFS_SYS_OFF_MODE) = 0x0000010f;   // OFF_MODE_X[15:0], SYS_CLKOUT1[31:16]

  	// Set SYS_CLKOUT2 for debug purposes
  	SCM_REG(PADCONFS_SYS_NIRQ) = 0x0000011f;   // FIQ[15:0], SYS_CLK2[31:16]
  
}

int
devInit(int baud)
{
	// Initialize pads
    pads_init();	

	// Disable MPU Watchdog WDT2
	WD2_REG(WD_WSPR) = 0x0000aaaa;
	wait_for_command_complete(WD2_BASE_ADD);
	WD2_REG(WD_WSPR) = 0x00005555;

	// Initialize GPIO
	GPIO_init();	

	// Setup UART pins to UART mode before calling InitUART from uart16550.c
	UART2_REG(UART_MDR1) = 0x0000;
    // Initialize the UART
    InitUART(baud);

	// Setup CS0 for 110ns Spansion Flash
	GPMC_REG(GPMC_CS0_CONFIG7) = 0x00000c48;	// Base addr 0x08000000, 64M
	GPMC_REG(GPMC_CS0_CONFIG1) = 0x00001210;
	//GPMC_REG(GPMC_CS0_CONFIG5) = 0x00080808;	// Config5
 
	// Setup CS4 for LAN9211, on the CSB740 it is refered to as CS2
	// and mapped to E_CS
	GPMC_REG(GPMC_CS4_CONFIG7) = 0x00000F6C;	// Base addr 0x2C000000, 16M
	GPMC_REG(GPMC_CS4_CONFIG1) = 0x00001200;

	return 0;
}

/* Referring to table 25-10 of the TRM, install
 * the RAM exception vectors...
 */
void
ram_vector_install(void)
{
	extern unsigned long abort_data;
	extern unsigned long abort_prefetch;
	extern unsigned long undefined_instruction;
	extern unsigned long software_interrupt;
	extern unsigned long interrupt_request;
	extern unsigned long fast_interrupt_request;
	extern unsigned long not_assigned;

	*(ulong **)0x4020ffe4 = &undefined_instruction;
	*(ulong **)0x4020ffe8 = &software_interrupt;
	*(ulong **)0x4020ffec = &abort_prefetch;
	*(ulong **)0x4020fff0 = &abort_data;
	*(ulong **)0x4020fff4 = &not_assigned;
	*(ulong **)0x4020fff8 = &interrupt_request;
	*(ulong **)0x4020fffc = &fast_interrupt_request;
}

void
initCPUio()
{
	volatile unsigned register cntens;
	volatile unsigned register usren;
	volatile unsigned register pmnc;

	ram_vector_install();

	/* Do this stuff to enable the cycle counter
	 * (for use by target_timer)...
	 */
	/* Allow user mode to have access to performance monitor registers:
	 */
   	asm volatile ("   MRC p15, 0, %0, c9, c14, 0" : "=r" (usren));
	usren |= 1;
   	asm volatile ("   MCR p15, 0, %0, c9, c14, 0" : : "r" (usren));

	/* Enable all counters, and reset Cycle counter...
	 */
	asm volatile ("   MRC p15, 0, %0, c9, c12, 0" : "=r" (pmnc));
	pmnc |= 5;
	asm volatile ("   MCR p15, 0, %0, c9, c12, 0" : : "r" (pmnc));

	/* Enable all performance counter registers...
	 */
	asm volatile ("   MRC p15, 0, %0, c9, c12, 1" : "=r" (cntens));
	cntens |= 0x8000000f;
	asm volatile ("   MCR p15, 0, %0, c9, c12, 1" : : "r" (cntens));
}

/* target_reset():
 * Set the counter to 16 ticks before trigger, then enable the
 * watchdog timer (WDT2) and wait...
 */
void
target_reset(void)
{
	// Preload the count-up register...
	WD2_REG(WD_WCRR) = 0xfffffff0;

	// Start MPU Watchdog WDT2
	WD2_REG(WD_WSPR) = 0x0000bbbb;
	wait_for_command_complete(WD2_BASE_ADD);
	WD2_REG(WD_WSPR) = 0x00004444;

	// Now just wait...
	while(1);
}

void
intsrestore(psr)
ulong     psr;
{
    putpsr(psr);
}

/*
 * Read the program status register (CPSR)
 * and set the FIQ and IRQ bits.
 */
ulong
intsoff(void)
{
    ulong  psr;

    psr = getpsr();

    /*
     * Set bit 6, bit 7 to disable interrupts.
     */
    putpsr(psr | 0x000000c0);
    return(psr);
}

/* show_revision():
 * Called when the system banner is printed...
 */
void
show_revision(int center)
{
	int	(*pfunc)(char *, ...);
	volatile unsigned register main_id;
	volatile unsigned register silicon_id;

	if (center)
		pfunc = cprintf;
	else
		pfunc = printf;

	asm("   MRC p15, 0, %0, c0, c0, 0" : "=r" (main_id));
	asm("   MRC p15, 1, %0, c0, c0, 7" : "=r" (silicon_id));

	pfunc("Silicon ID: %d.%d\n",
		((silicon_id & 0xf0)>>4),(silicon_id & 0xf));

	pfunc("CPU Rev: %d, Variant: %d\n",
		main_id & 0xf,(main_id & 0x00f00000) >> 20);

	pfunc("CM Rev: %d.%d, PRM Rev: %d.%d\n",
		CM_REV_MAJ(),CM_REV_MIN(),PRM_REV_MAJ(),PRM_REV_MIN());
}

/* target_timer():
 * Used in conjunction with INCLUDE_HWTMR and TIMER_TICKS_PER_MSEC
 * to set up a hardware based time base.
 */
unsigned long
target_timer(void)
{
	volatile unsigned register ccr;

   	asm("   MRC p15, 0, %0, c9, c13, 0" : "=r" (ccr));

	return(ccr);
}

/* cacheInitForTarget():
    Enable instruction cache only...
*/
void
cacheInitForTarget()
{
    asm("   MRC p15, 0, r0, c1, c0, 0");
    asm("   ORR r0, r0, #0x1000");  /* bit 12 is ICACHE enable*/
    asm("   MCR p15, 0, r0, c1, c0, 0");

    /* Flush instruction cache */
    asm("   MCR p15, 0, r0, c7, c5, 0");
}

