#include "config.h"
#include "stddefs.h"
#include "cpuio.h"
#include "genlib.h"
#include "cache.h"
#include "warmstart.h"
#include "timer.h"
#include "am335x.h"
#include "uart16550.h"
#include "cli.h"

int
getUartDivisor(int baud, unsigned char *hi, unsigned char *lo)
{
	*lo = ((48000000/16)/baud) & 0x00ff;
	*hi = (((48000000/16)/baud) & 0xff00) >> 8;
	return(0);
}

/* devInit():
 * As a bare minimum, initialize the console UART here using the
 * incoming 'baud' value as the baud rate.
 */
int
devInit(int baud)
{
        return(0);
}

/* ConsoleBaudSet():
 * Provide a means to change the baud rate of the running
 * console interface.  If the incoming value is not a valid
 * baud rate, then default to 9600.
 * In the early stages of a new port this can be left empty.
 * Return 0 if successful; else -1.
 */
/*int
ConsoleBaudSet(int baud)
{
	// ADD_CODE_HERE 
	return(0);
}*/

/* target_console_empty():
 * Target-specific portion of flush_console() in chario.c.
 * This function returns 1 if there are no characters waiting to
 * be put out on the UART; else return 0 indicating that the UART
 * is still busy outputting characters from its FIFO.
 * In the early stages of a new port this can simply return 1.
 */
/*int
target_console_empty(void)
{
	// if (UART_OUTPUT_BUFFER_IS_EMPTY())  <- FIX CODE HERE
		return(0);
	return(1);
}*/

/* intsoff():
 * Disable all system interrupts here and return a value that can
 * be used by intsrestore() (later) to restore the interrupt state.
 */
ulong
intsoff(void)
{
	ulong status = 0;

	/* ADD_CODE_HERE */
	return(status);
}

/* intsrestore():
 * Re-establish system interrupts here by using the status value
 * that was returned by an earlier call to intsoff().
 */
void
intsrestore(ulong status)
{
	/* ADD_CODE_HERE */
}

/* cacheInitForTarget():
 * Establish target specific function pointers and
 * enable i-cache...
 * Refer to $core/cache.c for a description of the function pointers.
 * NOTE:
 * If cache (either I or D or both) is enabled, then it is important
 * that the appropriate cacheflush/invalidate function be established.
 * This is very important because programs (i.e. cpu instructions) are
 * transferred to memory using data memory accesses and could
 * potentially result in cache coherency problems.
 */
void
cacheInitForTarget(void)
{
	/* ADD_CODE_HERE */
}

/* target_reset():
 * The default (absolute minimum) action to be taken by this function
 * is to call monrestart(INITIALIZE).  It would be better if there was
 * some target-specific function that would really cause the target
 * to reset...
 */
void
target_reset(void)
{
//	flushDcache(0,0);
//	disableDcache();
//	invalidateIcache(0,0);
//	disableIcache();
	monrestart(INITIALIZE);
}

/* Override the default exception handlers provided by the AM335x
 * internal ROM code with uMon's custom exception handlers
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

	*(ulong **)0x4030ce24 = &undefined_instruction;
	*(ulong **)0x4030ce28 = &software_interrupt;
	*(ulong **)0x4030ce2c = &abort_prefetch;
	*(ulong **)0x4030ce30 = &abort_data;
	*(ulong **)0x4030ce34 = &not_assigned;
	*(ulong **)0x4030ce38 = &interrupt_request;
	*(ulong **)0x4030ce3c = &fast_interrupt_request;
}

void
pinMuxInit(void)
{
	// Set pin mux configuration for UART0 RX/TX pins
	CNTL_MODULE_REG(CONF_UART0_RXD) = SLEWSLOW | RX_ON |
		PULL_OFF | MUXMODE_0;
	CNTL_MODULE_REG(CONF_UART0_TXD) = SLEWSLOW | RX_OFF |
		PULL_OFF | MUXMODE_0;

	// Configure GPIO pins tied to four USR LEDS...
	// GPIO1_21: USER0 LED (D2)
	CNTL_MODULE_REG(CONF_GPMC_A5) = SLEWSLOW | RX_ON |
		PULL_OFF | MUXMODE_7;
	// GPIO1_22: USER1 LED (D3)
	CNTL_MODULE_REG(CONF_GPMC_A6) = SLEWSLOW | RX_ON |
		PULL_OFF | MUXMODE_7;
	// GPIO1_23: USER2 LED (D4)
	CNTL_MODULE_REG(CONF_GPMC_A7) = SLEWSLOW | RX_ON |
		PULL_OFF | MUXMODE_7;
	// GPIO1_24: USER3 LED (D5)
	CNTL_MODULE_REG(CONF_GPMC_A8) = SLEWSLOW | RX_ON |
		PULL_OFF | MUXMODE_7;

	// Configure the pins for the MMC0 interface
	CNTL_MODULE_REG(CONF_MMC0_DAT0) = RX_ON | PULL_ON |
		PULLUP | MUXMODE_0;
	CNTL_MODULE_REG(CONF_MMC0_DAT1) = RX_ON | PULL_ON |
		PULLUP | MUXMODE_0;
	CNTL_MODULE_REG(CONF_MMC0_DAT2) = RX_ON | PULL_ON |
		PULLUP | MUXMODE_0;
	CNTL_MODULE_REG(CONF_MMC0_DAT3) = RX_ON | PULL_ON |
		PULLUP | MUXMODE_0;
	CNTL_MODULE_REG(CONF_MMC0_CLK) = RX_ON | PULL_OFF |
		MUXMODE_0;
	CNTL_MODULE_REG(CONF_MMC0_CMD) = RX_ON | PULL_ON |
		PULLUP | MUXMODE_0;
	CNTL_MODULE_REG(CONF_SPI0_CS1) = RX_ON | PULL_ON |
		PULLUP | MUXMODE_5;
}

void
InitGPIO1(void)
{
	// GPIO_CTRL: Enable GPIO1 module
	GPIO1_REG(0x130) = 0;

	// GPIO_OE: 25-24 are outputs...
	GPIO1_REG(0x134) &= ~(USR0_LED | USR1_LED | USR2_LED | USR3_LED);

	// All LEDs off...
	GPIO1_REG(0x13c) &= ~(USR0_LED | USR1_LED | USR2_LED | USR3_LED);
}

/* If any CPU IO wasn't initialized in reset.S, do it here...
 * This just provides a "C-level" IO init opportunity. 
 */
void
initCPUio(void)
{
	ram_vector_install();

	// Enable the control module:
	CM_WKUP_REG(CM_WKUP_CONTROL_CLKCTRL) |= 2;

	// Enable clock for UART0:
	CM_WKUP_REG(CM_WKUP_UART0_CLKCTRL) |= 2;

	// Enable clock for GPIO1:
	CM_PER_REG(CM_PER_GPIO1_CLKCTRL) |= 2;

	pinMuxInit();

	InitUART(DEFAULT_BAUD_RATE);
	InitGPIO1();

	// Set UART0 mode to 16x
	UART0_REG(UART_MDR1) &= ~7;
}

int
led(int num, int on)
{
	unsigned long bit;

	switch(num) {
		case 0:	// D0
			bit = USR0_LED;
			break;
		case 1:	// D1
			bit = USR1_LED;
			break;
		case 2:	// D2
			bit = USR2_LED;
			break;
		case 3:	// D3
			bit = USR3_LED;
			break;
		default:
			return(-1);
	}

	// GPIO21-24:
	if (on)
	    GPIO1_REG(0x13c) |= bit;
	else
	    GPIO1_REG(0x13c) &= ~bit;
	return(0);
}

void
target_blinkled(void)
{
#if INCLUDE_BLINKLED
	static uint8_t ledstate;
	static struct elapsed_tmr tmr;

#define STATLED_ON()	led(0,1)
#define STATLED_OFF()	led(0,0)
#ifndef BLINKON_MSEC
#define BLINKON_MSEC 10000
#define BLINKOFF_MSEC 10000
#endif

	switch(ledstate) {
		case 0:
			startElapsedTimer(&tmr,BLINKON_MSEC);
			STATLED_ON();
			ledstate = 1;
			break;
		case 1:
			if(msecElapsed(&tmr)) {
				STATLED_OFF();
				ledstate = 2;
				startElapsedTimer(&tmr,BLINKOFF_MSEC);
			}
			break;
		case 2:
			if(msecElapsed(&tmr)) {
				STATLED_ON();
				ledstate = 1;
				startElapsedTimer(&tmr,BLINKON_MSEC);
			}
			break;
	}
#endif
}

void
mpu_pll_init(void)
{
	uint32_t cm_clkmode_dpll_mpu;
	uint32_t cm_clksel_dpll_mpu;
	uint32_t cm_div_m2_dpll_mpu;

	// Put MPU PLL in MN Bypass mode
	cm_clkmode_dpll_mpu = CM_WKUP_REG(CM_CLKMODE_DPLL_MPU);
	cm_clkmode_dpll_mpu &= ~0x00000007;
	cm_clkmode_dpll_mpu |= 0x00000004;
	CM_WKUP_REG(CM_CLKMODE_DPLL_MPU) = cm_clkmode_dpll_mpu;
	// Wait for MPU PLL to enter MN Bypass mode
	while ((CM_WKUP_REG(CM_IDLEST_DPLL_MPU) & 0x00000101) != 0x00000100);

	// Set the ARM core frequency to 1 GHz
	cm_clksel_dpll_mpu = CM_WKUP_REG(CM_CLKSEL_DPLL_MPU);
	cm_clksel_dpll_mpu &= ~0x0007FF7F;
	cm_clksel_dpll_mpu |= 1000 << 8;
	cm_clksel_dpll_mpu |= 23;
	CM_WKUP_REG(CM_CLKSEL_DPLL_MPU) = cm_clksel_dpll_mpu;
	cm_div_m2_dpll_mpu = CM_WKUP_REG(CM_DIV_M2_DPLL_MPU);
	cm_div_m2_dpll_mpu &= ~0x0000001F;
	cm_div_m2_dpll_mpu |= 0x00000001;
	CM_WKUP_REG(CM_DIV_M2_DPLL_MPU) = cm_div_m2_dpll_mpu;

	// Lock MPU PLL
	cm_clkmode_dpll_mpu = CM_WKUP_REG(CM_CLKMODE_DPLL_MPU);
	cm_clkmode_dpll_mpu &= ~0x00000007;
	cm_clkmode_dpll_mpu |= 0x00000007;
	CM_WKUP_REG(CM_CLKMODE_DPLL_MPU) = cm_clkmode_dpll_mpu;
	// Wait for MPU PLL to lock
	while ((CM_WKUP_REG(CM_IDLEST_DPLL_MPU) & 0x00000001) != 0x00000001);
}

void
core_pll_init(void)
{
	uint32_t cm_clkmode_dpll_core;
	uint32_t cm_clksel_dpll_core;
	uint32_t cm_div_m4_dpll_core;
	uint32_t cm_div_m5_dpll_core;
	uint32_t cm_div_m6_dpll_core;

	// Put Core PLL in MN Bypass mode
	cm_clkmode_dpll_core = CM_WKUP_REG(CM_CLKMODE_DPLL_CORE);
	cm_clkmode_dpll_core &= ~0x00000007;
	cm_clkmode_dpll_core |= 0x00000004;
	CM_WKUP_REG(CM_CLKMODE_DPLL_CORE) = cm_clkmode_dpll_core;
	// Wait for Core PLL to enter MN Bypass mode
	while ((CM_WKUP_REG(CM_IDLEST_DPLL_CORE) & 0x00000101) != 0x00000100);

	// Configure the multiplier and divider
	cm_clksel_dpll_core = CM_WKUP_REG(CM_CLKSEL_DPLL_CORE);
	cm_clksel_dpll_core &= ~0x0007FF7F;
	cm_clksel_dpll_core |= 1000 << 8;
	cm_clksel_dpll_core |= 23;
	CM_WKUP_REG(CM_CLKSEL_DPLL_CORE) = cm_clksel_dpll_core;
	// Configure the M4, M5, and M6 dividers
	cm_div_m4_dpll_core = CM_WKUP_REG(CM_DIV_M4_DPLL_CORE);
	cm_div_m4_dpll_core &= ~0x0000001F;
	cm_div_m4_dpll_core |= 10;
	CM_WKUP_REG(CM_DIV_M4_DPLL_CORE) = cm_div_m4_dpll_core;
	cm_div_m5_dpll_core = CM_WKUP_REG(CM_DIV_M5_DPLL_CORE);
	cm_div_m5_dpll_core &= ~0x0000001F;
	cm_div_m5_dpll_core |= 8;
	CM_WKUP_REG(CM_DIV_M5_DPLL_CORE) = cm_div_m5_dpll_core;
	cm_div_m6_dpll_core = CM_WKUP_REG(CM_DIV_M6_DPLL_CORE);
	cm_div_m6_dpll_core &= ~0x0000001F;
	cm_div_m6_dpll_core |= 4;
	CM_WKUP_REG(CM_DIV_M6_DPLL_CORE) = cm_div_m6_dpll_core;

	// Lock Core PLL
	cm_clkmode_dpll_core = CM_WKUP_REG(CM_CLKMODE_DPLL_CORE);
	cm_clkmode_dpll_core &= ~0x00000007;
	cm_clkmode_dpll_core |= 0x00000007;
	CM_WKUP_REG(CM_CLKMODE_DPLL_CORE) = cm_clkmode_dpll_core;
	// Wait for Core PLL to lock
	while ((CM_WKUP_REG(CM_IDLEST_DPLL_CORE) & 0x00000001) != 0x00000001);
}

void
ddr_pll_init(void)
{
	uint32_t cm_clkmode_dpll_ddr;
	uint32_t cm_clksel_dpll_ddr;
	uint32_t cm_div_m2_dpll_ddr;

	// Put DDR PLL in MN Bypass mode
	cm_clkmode_dpll_ddr = CM_WKUP_REG(CM_CLKMODE_DPLL_DDR);
	cm_clkmode_dpll_ddr &= ~0x00000007;
	cm_clkmode_dpll_ddr |= 0x00000004;
	CM_WKUP_REG(CM_CLKMODE_DPLL_DDR) = cm_clkmode_dpll_ddr;
	// Wait for DDR PLL to enter MN Bypass mode
	while ((CM_WKUP_REG(CM_IDLEST_DPLL_DDR) & 0x00000101) != 0x00000100);

	// Set the DDR frequency to 400 MHz
	cm_clksel_dpll_ddr = CM_WKUP_REG(CM_CLKSEL_DPLL_DDR);
	cm_clksel_dpll_ddr &= ~0x0007FF7F;
	cm_clksel_dpll_ddr |= 400 << 8;
	cm_clksel_dpll_ddr |= 23;
	CM_WKUP_REG(CM_CLKSEL_DPLL_DDR) = cm_clksel_dpll_ddr;
	// Set M2 divider
	cm_div_m2_dpll_ddr = CM_WKUP_REG(CM_DIV_M2_DPLL_DDR);
	cm_div_m2_dpll_ddr &= ~0x0000001F;
	cm_div_m2_dpll_ddr |= 1;
	CM_WKUP_REG(CM_DIV_M2_DPLL_DDR) = cm_div_m2_dpll_ddr;

	// Lock the DDR PLL
	cm_clkmode_dpll_ddr = CM_WKUP_REG(CM_CLKMODE_DPLL_DDR);
	cm_clkmode_dpll_ddr &= ~0x00000007;
	cm_clkmode_dpll_ddr |= 0x00000007;
	CM_WKUP_REG(CM_CLKMODE_DPLL_DDR) = cm_clkmode_dpll_ddr;
	// Wait for DDR PLL to lock
	while ((CM_WKUP_REG(CM_IDLEST_DPLL_DDR) & 0x00000001) != 0x00000001);
}

void
per_pll_init(void)
{
	uint32_t cm_clkmode_dpll_per;
	uint32_t cm_clksel_dpll_per;
	uint32_t cm_div_m2_dpll_per;

	// Put Per PLL in MN Bypass mode
	cm_clkmode_dpll_per = CM_WKUP_REG(CM_CLKMODE_DPLL_PER);
	cm_clkmode_dpll_per &= ~0x00000007;
	cm_clkmode_dpll_per |= 0x00000004;
	CM_WKUP_REG(CM_CLKMODE_DPLL_PER) = cm_clkmode_dpll_per;
	// Wait for Per PLL to enter MN Bypass mode
	while ((CM_WKUP_REG(CM_IDLEST_DPLL_PER) & 0x00000101) != 0x00000100);

	// Configure the multiplier and divider
	cm_clksel_dpll_per = CM_WKUP_REG(CM_CLKSEL_DPLL_PER);
	cm_clksel_dpll_per &= ~0xFF0FFFFF;
	cm_clksel_dpll_per |= CM_CLKSEL_DPLL_PER_DPLL_SD_DIV | CM_CLKSEL_DPLL_PER_DPLL_MULT |
		CM_CLKSEL_DPLL_PER_DPLL_DIV;
	CM_WKUP_REG(CM_CLKSEL_DPLL_PER) = cm_clksel_dpll_per;
	// Set M2 divider
	cm_div_m2_dpll_per = CM_WKUP_REG(CM_DIV_M2_DPLL_PER);
	cm_div_m2_dpll_per &= ~0x0000007F;
	cm_div_m2_dpll_per |= 5;
	CM_WKUP_REG(CM_DIV_M2_DPLL_PER) = cm_div_m2_dpll_per;

	// Lock the Per PLL
	cm_clkmode_dpll_per = CM_WKUP_REG(CM_CLKMODE_DPLL_PER);
	cm_clkmode_dpll_per &= ~0x00000007;
	cm_clkmode_dpll_per |= 0x00000007;
	CM_WKUP_REG(CM_CLKMODE_DPLL_PER) = cm_clkmode_dpll_per;
	// Wait for Per PLL to lock
	while ((CM_WKUP_REG(CM_IDLEST_DPLL_PER) & 0x00000001) != 0x00000001);
}

void
pll_init(void)
{
	mpu_pll_init();
	core_pll_init();
	ddr_pll_init();
	per_pll_init();
}

void
ddr_init(void)
{
	uint32_t reg;

        // Enable the control module:
        CM_WKUP_REG(CM_WKUP_CONTROL_CLKCTRL) |= 2;

	// Enable EMIF module
	reg = CM_PER_REG(CM_PER_EMIF_CLKCTRL);
	reg &= ~3;
	reg |= 2;
	CM_PER_REG(CM_PER_EMIF_CLKCTRL) = reg;
	while ((CM_PER_REG(CM_PER_L3_CLKSTCTRL) & 0x00000004) != 0x00000004);

	// Configure VTP control
	CNTL_MODULE_REG(VTP_CTRL) |= 0x00000040;
	CNTL_MODULE_REG(VTP_CTRL) &= ~1;
	CNTL_MODULE_REG(VTP_CTRL) |= 1;
	// Wait for VTP control to be ready
	while ((CNTL_MODULE_REG(VTP_CTRL) & 0x00000020) != 0x00000020);

	// Configure the DDR PHY CMDx/DATAx registers
	DDR_PHY_REG(CMD0_REG_PHY_CTRL_SLAVE_RATIO_0) = 0x80;
	DDR_PHY_REG(CMD0_REG_PHY_INVERT_CLKOUT_0) = 0;
	DDR_PHY_REG(CMD1_REG_PHY_CTRL_SLAVE_RATIO_0) = 0x80;
	DDR_PHY_REG(CMD1_REG_PHY_INVERT_CLKOUT_0) = 0;
	DDR_PHY_REG(CMD2_REG_PHY_CTRL_SLAVE_RATIO_0) = 0x80;
	DDR_PHY_REG(CMD2_REG_PHY_INVERT_CLKOUT_0) = 0;

	DDR_PHY_REG(DATA0_REG_PHY_RD_DQS_SLAVE_RATIO_0) = 0x3A;
	DDR_PHY_REG(DATA0_REG_PHY_WR_DQS_SLAVE_RATIO_0) = 0x45;
	DDR_PHY_REG(DATA0_REG_PHY_WR_DATA_SLAVE_RATIO_0) = 0x7C;
	DDR_PHY_REG(DATA0_REG_PHY_FIFO_WE_SLAVE_RATIO_0) = 0x96;

	DDR_PHY_REG(DATA1_REG_PHY_RD_DQS_SLAVE_RATIO_0) = 0x3A;
	DDR_PHY_REG(DATA1_REG_PHY_WR_DQS_SLAVE_RATIO_0) = 0x45;
	DDR_PHY_REG(DATA1_REG_PHY_WR_DATA_SLAVE_RATIO_0) = 0x7C;
	DDR_PHY_REG(DATA1_REG_PHY_FIFO_WE_SLAVE_RATIO_0) = 0x96;

	CNTL_MODULE_REG(DDR_CMD0_IOCTRL) = 0x018B;
	CNTL_MODULE_REG(DDR_CMD1_IOCTRL) = 0x018B;
	CNTL_MODULE_REG(DDR_CMD2_IOCTRL) = 0x018B;
	CNTL_MODULE_REG(DDR_DATA0_IOCTRL) = 0x018B;
	CNTL_MODULE_REG(DDR_DATA1_IOCTRL) = 0x018B;

	CNTL_MODULE_REG(DDR_IO_CTRL) &= ~0x10000000;

	CNTL_MODULE_REG(DDR_CKE_CTRL) |= 0x00000001;

	// Enable dynamic power down when no read is being performed and set read latency
	// to CAS Latency + 2 - 1
	EMIF0_REG(DDR_PHY_CTRL_1) = 0x00100007;
	EMIF0_REG(DDR_PHY_CTRL_1_SHDW) = 0x00100007;

	// Configure the AC timing characteristics
	EMIF0_REG(SDRAM_TIM_1) = 0x0AAAD4DB;
	EMIF0_REG(SDRAM_TIM_1_SHDW) = 0x0AAAD4DB;
	EMIF0_REG(SDRAM_TIM_2) = 0x266B7FDA;
	EMIF0_REG(SDRAM_TIM_2_SHDW) = 0x266B7FDA;
	EMIF0_REG(SDRAM_TIM_3) = 0x501F867F;
	EMIF0_REG(SDRAM_TIM_3_SHDW) = 0x501F867F;

	// Set the refresh rate, 400,000,000 * 7.8 * 10^-6 = 3120 = 0x0C30
	EMIF0_REG(SDRAM_REF_CTRL) = 0x00000C30;
	// set the referesh rate shadow register to the same value as previous
	EMIF0_REG(SDRAM_REF_CTRL_SHDW) = 0x00000C30;

	// Configure the ZQ Calibration
	EMIF0_REG(ZQ_CONFIG) = 0x50074BE4;

	// Configure the SDRAM characteristics
	reg |= SDRAM_CONFIG_REG_SDRAM_TYPE_DDR3 | SDRAM_CONFIG_REG_IBANK_POS_0 |
		SDRAM_CONFIG_REG_DDR_TERM_DDR3_RZQ_4 | SDRAM_CONFIG_REG_DDR2_DDQS_DIFF_DQS |
		SDRAM_CONFIG_REG_DYN_ODT_RZQ_2 | SDRAM_CONFIG_REG_DDR_DISABLE_DLL_ENABLE |
		SDRAM_CONFIG_REG_SDRAM_DRIVE_RZQ_6 | SDRAM_CONFIG_REG_CAS_WR_LATENCY_5 |
		SDRAM_CONFIG_REG_NARROW_MODE_16BIT | SDRAM_CONFIG_REG_CAS_LATENCY_6 |
		SDRAM_CONFIG_REG_ROWSIZE_15BIT | SDRAM_CONFIG_REG_IBANK_8 |
		SDRAM_CONFIG_REG_EBANK_1 | SDRAM_CONFIG_REG_PAGESIZE_1024_WORD;
	EMIF0_REG(SDRAM_CONFIG) = reg;
	CNTL_MODULE_REG(CONTROL_EMIF_SDRAM_CONFIG) = reg;

	// Set the external bank position to 0
	EMIF0_REG(SDRAM_CONFIG_2) |= SDRAM_CONFIG_2_REG_EBANK_POS_0;
}
