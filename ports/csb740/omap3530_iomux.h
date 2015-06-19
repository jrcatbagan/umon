//==========================================================================
//
// mx31_gpio.h
//
// Author(s):    Luis Torrico, Cogent Computer Systems, Inc.
// Contributors: 
// Date:         05/02/2008
// Description:  This file contains offsets and bit defines
//				 for the MCIMX31 Software Multiplexor Control Register
//

#include "omap3530.h"
#include "bits.h"

// Base address for Software Multiplexor Control Register (SW_MUX_CTL).
// The SW_MUX_CTL has 82 seperate registers each of which contains 4
// seperate Control Signals.  So we have to write to a total of 328
// different signals in order to set-up the MX31 to run on the Cogent platform.
#define IOMUXC_BASE_ADDR        0x43FAC000
#define IOMUX_CTL_REG(_x_)		*(vulong *)(IOMUXC_BASE_ADDR + _x_)

#define GENERAL_REGISTER			0x008
#define SW_MUX_CTL_REGISTER1	    0x00C	// Contains cspi3_miso, cspi3_sclk, sspi3_spi_rdy, and ttm_pad
#define SW_MUX_CTL_REGISTER2        0x010	// Contains ata_reset_b, ce_control, clkss, and cspi3_mosi
#define SW_MUX_CTL_REGISTER3        0x014	// Contains ata_cs1, ata_dior, ata_diow, and ata_dmack
#define SW_MUX_CTL_REGISTER4        0x018	// Contains sd1_data1, sd1_data2, sd1_data3, and ata_cs0
#define SW_MUX_CTL_REGISTER5        0x01C	// Contains d3_spl, sd1_cmd, sd1_clk, and sd1_data0
#define SW_MUX_CTL_REGISTER6        0x020	// Contains vsync3, contrast, d3_rev, and d3_cls
#define SW_MUX_CTL_REGISTER7        0x024	// Contains ser_rs, par_rs, write, and read
#define SW_MUX_CTL_REGISTER8        0x028	// Contains sd_d_io, sd_d_clk, lcs0, and lcs1
#define SW_MUX_CTL_REGISTER9        0x02C	// Contains hsync, fpshift, drdy0, and sd_d_i
#define SW_MUX_CTL_REGISTER10       0x030	// Contains ld15, ld16, ld17, and vsync0
#define SW_MUX_CTL_REGISTER11       0x034
#define SW_MUX_CTL_REGISTER12       0x038
#define SW_MUX_CTL_REGISTER13       0x03C
#define SW_MUX_CTL_REGISTER14       0x040
#define SW_MUX_CTL_REGISTER15       0x044
#define SW_MUX_CTL_REGISTER16       0x048
#define SW_MUX_CTL_REGISTER17       0x04C
#define SW_MUX_CTL_REGISTER18       0x050
#define SW_MUX_CTL_REGISTER19       0x054
#define SW_MUX_CTL_REGISTER20       0x058
#define SW_MUX_CTL_REGISTER21       0x05C
#define SW_MUX_CTL_REGISTER22       0x060
#define SW_MUX_CTL_REGISTER23       0x064
#define SW_MUX_CTL_REGISTER24       0x068
#define SW_MUX_CTL_REGISTER25       0x06C
#define SW_MUX_CTL_REGISTER26       0x070
#define SW_MUX_CTL_REGISTER27       0x074
#define SW_MUX_CTL_REGISTER28       0x078
#define SW_MUX_CTL_REGISTER29       0x07C
#define SW_MUX_CTL_REGISTER30       0x080
#define SW_MUX_CTL_REGISTER31       0x084
#define SW_MUX_CTL_REGISTER32       0x088
#define SW_MUX_CTL_REGISTER33       0x08C
#define SW_MUX_CTL_REGISTER34       0x090
#define SW_MUX_CTL_REGISTER35       0x094
#define SW_MUX_CTL_REGISTER36       0x098
#define SW_MUX_CTL_REGISTER37       0x09C
#define SW_MUX_CTL_REGISTER38       0x0A0
#define SW_MUX_CTL_REGISTER39       0x0A4
#define SW_MUX_CTL_REGISTER40       0x0A8
#define SW_MUX_CTL_REGISTER41       0x0AC
#define SW_MUX_CTL_REGISTER42       0x0B0
#define SW_MUX_CTL_REGISTER43       0x0B4
#define SW_MUX_CTL_REGISTER44       0x0B8
#define SW_MUX_CTL_REGISTER45       0x0BC
#define SW_MUX_CTL_REGISTER46       0x0C0
#define SW_MUX_CTL_REGISTER47       0x0C4
#define SW_MUX_CTL_REGISTER48       0x0C8
#define SW_MUX_CTL_REGISTER49       0x0CC
#define SW_MUX_CTL_REGISTER50       0x0D0
#define SW_MUX_CTL_REGISTER51       0x0D4
#define SW_MUX_CTL_REGISTER52       0x0D8
#define SW_MUX_CTL_REGISTER53       0x0DC
#define SW_MUX_CTL_REGISTER54       0x0E0
#define SW_MUX_CTL_REGISTER55       0x0E4
#define SW_MUX_CTL_REGISTER56       0x0E8
#define SW_MUX_CTL_REGISTER57       0x0EC
#define SW_MUX_CTL_REGISTER58       0x0F0
#define SW_MUX_CTL_REGISTER59       0x0F4
#define SW_MUX_CTL_REGISTER60       0x0F8
#define SW_MUX_CTL_REGISTER61       0x0FC
#define SW_MUX_CTL_REGISTER62       0x100
#define SW_MUX_CTL_REGISTER63       0x104
#define SW_MUX_CTL_REGISTER64       0x108
#define SW_MUX_CTL_REGISTER65       0x10C
#define SW_MUX_CTL_REGISTER66       0x110
#define SW_MUX_CTL_REGISTER67       0x114
#define SW_MUX_CTL_REGISTER68       0x118
#define SW_MUX_CTL_REGISTER69       0x11C
#define SW_MUX_CTL_REGISTER70       0x120
#define SW_MUX_CTL_REGISTER71       0x124
#define SW_MUX_CTL_REGISTER72       0x128
#define SW_MUX_CTL_REGISTER73       0x12C
#define SW_MUX_CTL_REGISTER74       0x130
#define SW_MUX_CTL_REGISTER75       0x134
#define SW_MUX_CTL_REGISTER76       0x138
#define SW_MUX_CTL_REGISTER77       0x13C
#define SW_MUX_CTL_REGISTER78       0x140
#define SW_MUX_CTL_REGISTER79       0x144
#define SW_MUX_CTL_REGISTER80       0x148
#define SW_MUX_CTL_REGISTER81       0x14C
#define SW_MUX_CTL_REGISTER82       0x150

// General Purpose Register Bit Defines
#define DDR_MODE_ON_CLK0_EN		BIT31		// 1 = Enable DDR mode on CLK0 contact
#define USBH2_LOOPBACK_EN		BIT30		// 1 = Turn on sw_input_on (loopback) on some USBH2 contacts
#define USBH1_LOOPBACK_EN		BIT29		// 1 = Turn on sw_input_on (loopback) on some USBH1 contacts
#define USBOTG_LOOPBACK_EN		BIT28		// 1 = Turn on sw_input_on (loopback) on some USBOTG contacts
#define USBH1_SUS_ON_SFS6_EN	BIT27		// 1 = Enable USBH1_SUSPEND signal on SFS6 contact
#define ATA_ON_KEYPAD_EN		BIT26		// 1 = Enable ATA signals on Keypad Group contacts
#define UART5_DMA_REQ_EN		BIT25		// Selects either CSPI3 or UART5 DMA requests for events 10 and 11, 1 = UART5, 0 = CSPI1
#define	SLEW_RATE_SEL			BIT24		// 1 = Fast Slew Rate, 0 = Slow Slew Rate
#define DRIVE_STRENGTH_SEL		BIT23		// 1 = Maximum drive strength, 0 = standard or high drive strength
#define UPLL_ON_GPIO3_1_EN		BIT22		// 1 = Enable UPLL clock bypass through GPIO3_1 contact
#define SPLL_ON_GPIO3_0_EN		BIT21		// 1 = Enable SPLL clock bypass through GPIO3_0 contact
#define MSHC2_DMA_REQ_EN		BIT20		// Selects either SDHC2 or MSHC2 DMA requests, 1 = MSCHC2, 0 = SDHC2
#define MSHC1_DMA_REQ_EN		BIT19		// Selects either SDHC1 or MSHC1 DMA requests, 1 = MSCHC1, 0 = SDHC1
#define OTG_DATA_ON_UART_EN		BIT18		// 1 = Enable USBOTG_DATA[5:3] on Full UART Group contacts
#define OTG_D4_ON_DSR_DCE1_EN	BIT17		// 1 = Enable USBOTG_DATA4 on DSR_DCE1 contact
#define TAMPER_DETECT_EN		BIT16		// 1 = Enable Tamper detect logic
#define MBX_DMA_REQ_EN			BIT15		// Selects either External or MBX DMA requests, 1 = MDX, 0 = External
#define UART_DMA_REQ_EN			BIT14		// Selects either CSPI1 or UART3 DMA requests, 1 = UART3, 0 = CSPI1
#define WEIM_ON_CS3_EN			BIT13		// Selects either CSD1 or WEIM on EMI CS3 contact, 1 = CSD1, 0 = WEIM
#define WEIM_ON_CS2_EN			BIT12		// Selects either CSD0 or WEIM on EMI CS2 contact, 1 = CSD0, 0 = WEIM
#define USBH2_ON_AUDIO_EN		BIT11		// 1 = Enable USBH2 signals on AudioPort 3 and AudioPort6
#define ATA_SIG_ON_CSPI1_EN		BIT10		// 1 = Enable ATA signals on CSPI1 Group contacts
#define ATA_DATA_ON_CSPI1_EN	BIT9		// 1 = Enable ATA DATA14-15 on Timer Group contacts and DATA0-6 on CSPI1 Group contacts
#define ATA_DATA_ON_AUDIO_EN	BIT8		// 1 = Enable DATA7-10 signals of ATA on AudioPort3 and DATA11-13 on AudioPort6
#define ATA_DATA_ON_IPU_EN		BIT7		// 1 = Enable DATA0-13 signals of ATA on IPU (CSI) and DATA14-15 on I2C
#define ATA_SIG_ON_NANDF_EN		BIT6		// 1 = Enable ATA signals on NANF contacts
#define ATA_DATA_ON_NANDF_EN	BIT5		// 1 = Enable ATA DATA7-13 on NANDF contacts
#define ATA_ON_USBH2_EN			BIT4		// 1 = Enable ATA signals on USBH2 contacts
#define PWMO_ON_ATA_IORDY_EN	BIT3		// 1 = Enable ATA IORDY signal on PWMO contact
#define CSPI1_ON_UART_EN		BIT2		// 1 = Replaces Full UART Group with CSPI1 signals
#define DDR_MODE_EN				BIT1		// 1 = Forces DDR type I/O contacts to DDR mode
#define FIR_DMA_REQ_EN			BIT0		// Selects FIR or UART2 SDMA events, 1 = FIR, 0 = UART2

// Initialization of GPR for CSB733
//IOMUX_CTL_REG(GENERAL_REGISTER) = WEIM_ON_CS3_EN | CSPI1_ON_UART_EN;

// various IOMUX output functions
#define	OUTPUTCONFIG_GPIO	0x00	// used as GPIO
#define	OUTPUTCONFIG_FUNC	0x10	// output used as function
#define	OUTPUTCONFIG_ALT1	0x20	// output used as alternate function 1
#define	OUTPUTCONFIG_ALT2	0x30	// output used as alternate function 2
#define	OUTPUTCONFIG_ALT3	0x40	// output used as alternate function 3
#define	OUTPUTCONFIG_ALT4	0x50	// output used as alternate function 4
#define	OUTPUTCONFIG_ALT5	0x60	// output used as alternate function 5
#define	OUTPUTCONFIG_ALT6	0x70	// output used as alternate function 6

// various IOMUX input functions
#define	INPUTCONFIG_NONE	0x00	// not configured for input
#define	INPUTCONFIG_GPIO	0x01	// input used as GPIO
#define	INPUTCONFIG_FUNC	0x02	// input used as function
#define	INPUTCONFIG_ALT1	0x04	// input used as alternate function 1
#define	INPUTCONFIG_ALT2	0x08	// input used as alternate function 2

// Software Mux Control Signal Defines (SW_MUX_CTL_SIGNAL 1-4)
#define	MX31_PIN_CSPI3_MISO(_x_)	((_x_ & 0xff) << 24)
#define	MX31_PIN_CSPI3_SCLK(_x_)	((_x_ & 0xff) << 16)
#define	MX31_PIN_CSPI3_SPI_RDY(_x_)	((_x_ & 0xff) << 8)
#define	MX31_PIN_TTM_PAD(_x_)		((_x_ & 0xff) << 0)

#define	MX31_PIN_ATA_RESET_B(_x_)	((_x_ & 0xff) << 24)
#define	MX31_PIN_CE_CONTROL(_x_)	((_x_ & 0xff) << 16)
#define	MX31_PIN_CLKSS(_x_)			((_x_ & 0xff) << 8)
#define	MX31_PIN_CSPI3_MOSI(_x_)	((_x_ & 0xff) << 0)

#define	MX31_PIN_ATA_CS1(_x_)		((_x_ & 0xff) << 24)
#define	MX31_PIN_ATA_DIOR(_x_)		((_x_ & 0xff) << 16)
#define	MX31_PIN_ATA_DIOW(_x_)		((_x_ & 0xff) << 8)
#define	MX31_PIN_ATA_DMACK(_x_)		((_x_ & 0xff) << 0)

#define	MX31_PIN_SD1_DATA1(_x_)		((_x_ & 0xff) << 24)
#define	MX31_PIN_SD1_DATA2(_x_)		((_x_ & 0xff) << 16)
#define	MX31_PIN_SD1_DATA3(_x_)		((_x_ & 0xff) << 8)
#define	MX31_PIN_ATA_CS0(_x_)		((_x_ & 0xff) << 0)

#define	MX31_PIN_D3_SPL(_x_)		((_x_ & 0xff) << 24)
#define	MX31_PIN_SD1_CMD(_x_)		((_x_ & 0xff) << 16)
#define	MX31_PIN_SD1_CLK(_x_)		((_x_ & 0xff) << 8)
#define	MX31_PIN_SD1_DATA0(_x_)		((_x_ & 0xff) << 0)

#define	MX31_PIN_VSYNC3(_x_)		((_x_ & 0xff) << 24)
#define	MX31_PIN_CONTRAST(_x_)		((_x_ & 0xff) << 16)
#define	MX31_PIN_D3_REV(_x_)		((_x_ & 0xff) << 8)
#define	MX31_PIN_D3_CLS(_x_)		((_x_ & 0xff) << 0)

#define	MX31_PIN_SER_RS(_x_)		((_x_ & 0xff) << 24)
#define	MX31_PIN_PAR_RS(_x_)		((_x_ & 0xff) << 16)
#define	MX31_PIN_WRITE(_x_)			((_x_ & 0xff) << 8)
#define	MX31_PIN_READ(_x_)			((_x_ & 0xff) << 0)

#define	MX31_PIN_SD_D_IO(_x_)		((_x_ & 0xff) << 24)
#define	MX31_PIN_SD_D_CLK(_x_)		((_x_ & 0xff) << 16)
#define	MX31_PIN_LCS0(_x_)			((_x_ & 0xff) << 8)
#define	MX31_PIN_LCS1(_x_)			((_x_ & 0xff) << 0)

#define	MX31_PIN_HSYNC(_x_)			((_x_ & 0xff) << 24)
#define	MX31_PIN_FPSHIFT(_x_)		((_x_ & 0xff) << 16)
#define	MX31_PIN_DRDY0(_x_)			((_x_ & 0xff) << 8)
#define	MX31_PIN_SD_D_I(_x_)		((_x_ & 0xff) << 0)

#define	MX31_PIN_LD15(_x_)			((_x_ & 0xff) << 24)
#define	MX31_PIN_LD16(_x_)			((_x_ & 0xff) << 16)
#define	MX31_PIN_LD17(_x_)			((_x_ & 0xff) << 8)
#define	MX31_PIN_VSYNC0(_x_)		((_x_ & 0xff) << 0)

#define	MX31_PIN_LD11(_x_)			((_x_ & 0xff) << 24)
#define	MX31_PIN_LD12(_x_)			((_x_ & 0xff) << 16)
#define	MX31_PIN_LD13(_x_)			((_x_ & 0xff) << 8)
#define	MX31_PIN_LD14(_x_)			((_x_ & 0xff) << 0)

#define	MX31_PIN_LD7(_x_)			((_x_ & 0xff) << 24)
#define	MX31_PIN_LD8(_x_)			((_x_ & 0xff) << 16)
#define	MX31_PIN_LD9(_x_)			((_x_ & 0xff) << 8)
#define	MX31_PIN_LD10(_x_)			((_x_ & 0xff) << 0)

#define	MX31_PIN_LD3(_x_)			((_x_ & 0xff) << 24)
#define	MX31_PIN_LD4(_x_)			((_x_ & 0xff) << 16)
#define	MX31_PIN_LD5(_x_)			((_x_ & 0xff) << 8)
#define	MX31_PIN_LD6(_x_)			((_x_ & 0xff) << 0)

#define	MX31_PIN_USBH2_DATA1(_x_)	((_x_ & 0xff) << 24)
#define	MX31_PIN_LD0(_x_)			((_x_ & 0xff) << 16)
#define	MX31_PIN_LD1(_x_)			((_x_ & 0xff) << 8)
#define	MX31_PIN_LD2(_x_)			((_x_ & 0xff) << 0)

#define	MX31_PIN_USBH2_DIR(_x_)		((_x_ & 0xff) << 24)
#define	MX31_PIN_USBH2_STP(_x_)		((_x_ & 0xff) << 16)
#define	MX31_PIN_USBH2_NXT(_x_)		((_x_ & 0xff) << 8)
#define	MX31_PIN_USBH2_DATA0(_x_)	((_x_ & 0xff) << 0)

#define	MX31_PIN_USBOTG_DATA5(_x_)	((_x_ & 0xff) << 24)
#define	MX31_PIN_USBOTG_DATA6(_x_)	((_x_ & 0xff) << 16)
#define	MX31_PIN_USBOTG_DATA7(_x_)	((_x_ & 0xff) << 8)
#define	MX31_PIN_USBH2_CLK(_x_)		((_x_ & 0xff) << 0)

#define	MX31_PIN_USBOTG_DATA1(_x_)	((_x_ & 0xff) << 24)
#define	MX31_PIN_USBOTG_DATA2(_x_)	((_x_ & 0xff) << 16)
#define	MX31_PIN_USBOTG_DATA3(_x_)	((_x_ & 0xff) << 8)
#define	MX31_PIN_USBOTG_DATA4(_x_)	((_x_ & 0xff) << 0)

#define	MX31_PIN_USBOTG_DIR(_x_)	((_x_ & 0xff) << 24)
#define	MX31_PIN_USBOTG_STP(_x_)	((_x_ & 0xff) << 16)
#define	MX31_PIN_USBOTG_NXT(_x_)	((_x_ & 0xff) << 8)
#define	MX31_PIN_USBOTG_DATA0(_x_)	((_x_ & 0xff) << 0)

#define	MX31_PIN_USB_PWR(_x_)		((_x_ & 0xff) << 24)
#define	MX31_PIN_USB_OC(_x_)		((_x_ & 0xff) << 16)
#define	MX31_PIN_USB_BYP(_x_)		((_x_ & 0xff) << 8)
#define	MX31_PIN_USBOTG_CLK(_x_)	((_x_ & 0xff) << 0)

#define	MX31_PIN_TDO(_x_)			((_x_ & 0xff) << 24)
#define	MX31_PIN_TRSTB(_x_)			((_x_ & 0xff) << 16)
#define	MX31_PIN_DE_B(_x_)			((_x_ & 0xff) << 8)
#define	MX31_PIN_SJC_MOD(_x_)		((_x_ & 0xff) << 0)

#define	MX31_PIN_RTCK(_x_)			((_x_ & 0xff) << 24)
#define	MX31_PIN_TCK(_x_)			((_x_ & 0xff) << 16)
#define	MX31_PIN_TMS(_x_)			((_x_ & 0xff) << 8)
#define	MX31_PIN_TDI(_x_)			((_x_ & 0xff) << 0)

#define	MX31_PIN_KEY_COL4(_x_)		((_x_ & 0xff) << 24)
#define	MX31_PIN_KEY_COL5(_x_)		((_x_ & 0xff) << 16)
#define	MX31_PIN_KEY_COL6(_x_)		((_x_ & 0xff) << 8)
#define	MX31_PIN_KEY_COL7(_x_)		((_x_ & 0xff) << 0)

#define	MX31_PIN_KEY_COL0(_x_)		((_x_ & 0xff) << 24)
#define	MX31_PIN_KEY_COL1(_x_)		((_x_ & 0xff) << 16)
#define	MX31_PIN_KEY_COL2(_x_)		((_x_ & 0xff) << 8)
#define	MX31_PIN_KEY_COL3(_x_)		((_x_ & 0xff) << 0)

#define	MX31_PIN_KEY_ROW4(_x_)		((_x_ & 0xff) << 24)
#define	MX31_PIN_KEY_ROW5(_x_)		((_x_ & 0xff) << 16)
#define	MX31_PIN_KEY_ROW6(_x_)		((_x_ & 0xff) << 8)
#define	MX31_PIN_KEY_ROW7(_x_)		((_x_ & 0xff) << 0)

#define	MX31_PIN_KEY_ROW0(_x_)		((_x_ & 0xff) << 24)
#define	MX31_PIN_KEY_ROW1(_x_)		((_x_ & 0xff) << 16)
#define	MX31_PIN_KEY_ROW2(_x_)		((_x_ & 0xff) << 8)
#define	MX31_PIN_KEY_ROW3(_x_)		((_x_ & 0xff) << 0)

#define	MX31_PIN_TXD2(_x_)			((_x_ & 0xff) << 24)
#define	MX31_PIN_RTS2(_x_)			((_x_ & 0xff) << 16)
#define	MX31_PIN_CTS2(_x_)			((_x_ & 0xff) << 8)
#define	MX31_PIN_BATT_LINE(_x_)		((_x_ & 0xff) << 0)

#define	MX31_PIN_RI_DTE1(_x_)		((_x_ & 0xff) << 24)
#define	MX31_PIN_DCD_DTE1(_x_)		((_x_ & 0xff) << 16)
#define	MX31_PIN_DTR_DCE2(_x_)		((_x_ & 0xff) << 8)
#define	MX31_PIN_RXD2(_x_)			((_x_ & 0xff) << 0)

#define	MX31_PIN_RI_DCE1(_x_)		((_x_ & 0xff) << 24)
#define	MX31_PIN_DCD_DCE1(_x_)		((_x_ & 0xff) << 16)
#define	MX31_PIN_DTR_DTE1(_x_)		((_x_ & 0xff) << 8)
#define	MX31_PIN_DSR_DTE1(_x_)		((_x_ & 0xff) << 0)

#define	MX31_PIN_RTS1(_x_)			((_x_ & 0xff) << 24)
#define	MX31_PIN_CTS1(_x_)			((_x_ & 0xff) << 16)
#define	MX31_PIN_DTR_DCE1(_x_)		((_x_ & 0xff) << 8)
#define	MX31_PIN_DSR_DCE1(_x_)		((_x_ & 0xff) << 0)

#define	MX31_PIN_CSPI2_SCLK(_x_)	((_x_ & 0xff) << 24)
#define	MX31_PIN_CSPI2_SPI_RDY(_x_)	((_x_ & 0xff) << 16)
#define	MX31_PIN_RXD1(_x_)			((_x_ & 0xff) << 8)
#define	MX31_PIN_TXD1(_x_)			((_x_ & 0xff) << 0)

#define	MX31_PIN_CSPI2_MISO(_x_)	((_x_ & 0xff) << 24)
#define	MX31_PIN_CSPI2_SS0(_x_)		((_x_ & 0xff) << 16)
#define	MX31_PIN_CSPI2_SS1(_x_)		((_x_ & 0xff) << 8)
#define	MX31_PIN_CSPI2_SS2(_x_)		((_x_ & 0xff) << 0)

#define	MX31_PIN_CSPI1_SS2(_x_)		((_x_ & 0xff) << 24)
#define	MX31_PIN_CSPI1_SCLK(_x_)	((_x_ & 0xff) << 16)
#define	MX31_PIN_CSPI1_SPI_RDY(_x_)	((_x_ & 0xff) << 8)
#define	MX31_PIN_CSPI2_MOSI(_x_)	((_x_ & 0xff) << 0)

#define	MX31_PIN_CSPI1_MOSI(_x_)	((_x_ & 0xff) << 24)
#define	MX31_PIN_CSPI1_MISO(_x_)	((_x_ & 0xff) << 16)
#define	MX31_PIN_CSPI1_SS0(_x_)		((_x_ & 0xff) << 8)
#define	MX31_PIN_CSPI1_SS1(_x_)		((_x_ & 0xff) << 0)

#define	MX31_PIN_STXD6(_x_)			((_x_ & 0xff) << 24)
#define	MX31_PIN_SRXD6(_x_)			((_x_ & 0xff) << 16)
#define	MX31_PIN_SCK6(_x_)			((_x_ & 0xff) << 8)
#define	MX31_PIN_SFS6(_x_)			((_x_ & 0xff) << 0)

#define	MX31_PIN_STXD5(_x_)			((_x_ & 0xff) << 24)
#define	MX31_PIN_SRXD5(_x_)			((_x_ & 0xff) << 16)
#define	MX31_PIN_SCK5(_x_)			((_x_ & 0xff) << 8)
#define	MX31_PIN_SFS5(_x_)			((_x_ & 0xff) << 0)

#define	MX31_PIN_STXD4(_x_)			((_x_ & 0xff) << 24)
#define	MX31_PIN_SRXD4(_x_)			((_x_ & 0xff) << 16)
#define	MX31_PIN_SCK4(_x_)			((_x_ & 0xff) << 8)
#define	MX31_PIN_SFS4(_x_)			((_x_ & 0xff) << 0)

#define	MX31_PIN_STXD3(_x_)			((_x_ & 0xff) << 24)
#define	MX31_PIN_SRXD3(_x_)			((_x_ & 0xff) << 16)
#define	MX31_PIN_SCK3(_x_)			((_x_ & 0xff) << 8)
#define	MX31_PIN_SFS3(_x_)			((_x_ & 0xff) << 0)

#define	MX31_PIN_CSI_HSYNC(_x_)		((_x_ & 0xff) << 24)
#define	MX31_PIN_CSI_PIXCLK(_x_)	((_x_ & 0xff) << 16)
#define	MX31_PIN_I2C_CLK(_x_)		((_x_ & 0xff) << 8)
#define	MX31_PIN_I2C_DAT(_x_)		((_x_ & 0xff) << 0)

#define	MX31_PIN_CSI_D14(_x_)		((_x_ & 0xff) << 24)
#define	MX31_PIN_CSI_D15(_x_)		((_x_ & 0xff) << 16)
#define	MX31_PIN_CSI_MCLK(_x_)		((_x_ & 0xff) << 8)
#define	MX31_PIN_CSI_VSYNC(_x_)		((_x_ & 0xff) << 0)

#define	MX31_PIN_CSI_D10(_x_)		((_x_ & 0xff) << 24)
#define	MX31_PIN_CSI_D11(_x_)		((_x_ & 0xff) << 16)
#define	MX31_PIN_CSI_D12(_x_)		((_x_ & 0xff) << 8)
#define	MX31_PIN_CSI_D13(_x_)		((_x_ & 0xff) << 0)

#define	MX31_PIN_CSI_D6(_x_)		((_x_ & 0xff) << 24)
#define	MX31_PIN_CSI_D7(_x_)		((_x_ & 0xff) << 16)
#define	MX31_PIN_CSI_D8(_x_)		((_x_ & 0xff) << 8)
#define	MX31_PIN_CSI_D9(_x_)		((_x_ & 0xff) << 0)

#define	MX31_PIN_M_REQUEST(_x_)		((_x_ & 0xff) << 24)
#define	MX31_PIN_M_GRANT(_x_)		((_x_ & 0xff) << 16)
#define	MX31_PIN_CSI_D4(_x_)		((_x_ & 0xff) << 8)
#define	MX31_PIN_CSI_D5(_x_)		((_x_ & 0xff) << 0)

#define	MX31_PIN_PC_RST(_x_)		((_x_ & 0xff) << 24)
#define	MX31_PIN_IOIS16(_x_)		((_x_ & 0xff) << 16)
#define	MX31_PIN_PC_RW_B(_x_)		((_x_ & 0xff) << 8)
#define	MX31_PIN_PC_POE(_x_)		((_x_ & 0xff) << 0)

#define	MX31_PIN_PC_VS1(_x_)		((_x_ & 0xff) << 24)
#define	MX31_PIN_PC_VS2(_x_)		((_x_ & 0xff) << 16)
#define	MX31_PIN_PC_BVD1(_x_)		((_x_ & 0xff) << 8)
#define	MX31_PIN_PC_BVD2(_x_)		((_x_ & 0xff) << 0)

#define	MX31_PIN_PC_CD2_B(_x_)		((_x_ & 0xff) << 24)
#define	MX31_PIN_PC_WAIT_B(_x_)		((_x_ & 0xff) << 16)
#define	MX31_PIN_PC_READY(_x_)		((_x_ & 0xff) << 8)
#define	MX31_PIN_PC_PWRON(_x_)		((_x_ & 0xff) << 0)

#define	MX31_PIN_D2(_x_)			((_x_ & 0xff) << 24)
#define	MX31_PIN_D1(_x_)			((_x_ & 0xff) << 16)
#define	MX31_PIN_D0(_x_)			((_x_ & 0xff) << 8)
#define	MX31_PIN_PC_CD1_B(_x_)		((_x_ & 0xff) << 0)

#define	MX31_PIN_D6(_x_)			((_x_ & 0xff) << 24)
#define	MX31_PIN_D5(_x_)			((_x_ & 0xff) << 16)
#define	MX31_PIN_D4(_x_)			((_x_ & 0xff) << 8)
#define	MX31_PIN_D3(_x_)			((_x_ & 0xff) << 0)

#define	MX31_PIN_D10(_x_)			((_x_ & 0xff) << 24)
#define	MX31_PIN_D9(_x_)			((_x_ & 0xff) << 16)
#define	MX31_PIN_D8(_x_)			((_x_ & 0xff) << 8)
#define	MX31_PIN_D7(_x_)			((_x_ & 0xff) << 0)

#define	MX31_PIN_D14(_x_)			((_x_ & 0xff) << 24)
#define	MX31_PIN_D13(_x_)			((_x_ & 0xff) << 16)			
#define	MX31_PIN_D12(_x_)			((_x_ & 0xff) << 8)
#define	MX31_PIN_D11(_x_)			((_x_ & 0xff) << 0)

#define	MX31_PIN_NFWP_B(_x_)		((_x_ & 0xff) << 24)
#define	MX31_PIN_NFCE_B(_x_)		((_x_ & 0xff) << 16)
#define	MX31_PIN_NFRB(_x_)			((_x_ & 0xff) << 8)
#define	MX31_PIN_D15(_x_)			((_x_ & 0xff) << 0)

#define	MX31_PIN_NFWE_B(_x_)		((_x_ & 0xff) << 24)
#define	MX31_PIN_NFRE_B(_x_)		((_x_ & 0xff) << 16)
#define	MX31_PIN_NFALE(_x_)			((_x_ & 0xff) << 8)
#define	MX31_PIN_NFCLE(_x_)			((_x_ & 0xff) << 0)

#define	MX31_PIN_SDQS0(_x_)			((_x_ & 0xff) << 24)
#define	MX31_PIN_SDQS1(_x_)			((_x_ & 0xff) << 16)
#define	MX31_PIN_SDQS2(_x_)			((_x_ & 0xff) << 8)
#define	MX31_PIN_SDQS3(_x_)			((_x_ & 0xff) << 0)

#define	MX31_PIN_SDCKE0(_x_)		((_x_ & 0xff) << 24)
#define	MX31_PIN_SDCKE1(_x_)		((_x_ & 0xff) << 16)
#define	MX31_PIN_SDCLK(_x_)			((_x_ & 0xff) << 8)
#define	MX31_PIN_SDCLK_B(_x_)		((_x_ & 0xff) << 0)

#define	MX31_PIN_RW(_x_)			((_x_ & 0xff) << 24)
#define	MX31_PIN_RAS(_x_)			((_x_ & 0xff) << 16)
#define	MX31_PIN_CAS(_x_)			((_x_ & 0xff) << 8)
#define	MX31_PIN_SDWE(_x_)			((_x_ & 0xff) << 0)

#define	MX31_PIN_CS5(_x_)			((_x_ & 0xff) << 24)
#define	MX31_PIN_ECB(_x_)			((_x_ & 0xff) << 16)
#define	MX31_PIN_LBA(_x_)			((_x_ & 0xff) << 8)
#define	MX31_PIN_BCLK(_x_)			((_x_ & 0xff) << 0)

#define	MX31_PIN_CS1(_x_)			((_x_ & 0xff) << 24)
#define	MX31_PIN_CS2(_x_)			((_x_ & 0xff) << 16)
#define	MX31_PIN_CS3(_x_)			((_x_ & 0xff) << 8)
#define	MX31_PIN_CS4(_x_)			((_x_ & 0xff) << 0)

#define	MX31_PIN_EB0(_x_)			((_x_ & 0xff) << 24)
#define	MX31_PIN_EB1(_x_)			((_x_ & 0xff) << 16)
#define	MX31_PIN_OE(_x_)			((_x_ & 0xff) << 8)
#define	MX31_PIN_CS0(_x_)			((_x_ & 0xff) << 0)

#define	MX31_PIN_DQM0(_x_)			((_x_ & 0xff) << 24)
#define	MX31_PIN_DQM1(_x_)			((_x_ & 0xff) << 16)
#define	MX31_PIN_DQM2(_x_)			((_x_ & 0xff) << 8)
#define	MX31_PIN_DQM3(_x_)			((_x_ & 0xff) << 0)

#define	MX31_PIN_SD28(_x_)			((_x_ & 0xff) << 24)
#define	MX31_PIN_SD29(_x_)			((_x_ & 0xff) << 16)
#define	MX31_PIN_SD30(_x_)			((_x_ & 0xff) << 8)
#define	MX31_PIN_SD31(_x_)			((_x_ & 0xff) << 0)

#define	MX31_PIN_SD24(_x_)			((_x_ & 0xff) << 24)
#define	MX31_PIN_SD25(_x_)			((_x_ & 0xff) << 16)
#define	MX31_PIN_SD26(_x_)			((_x_ & 0xff) << 8)
#define	MX31_PIN_SD27(_x_)			((_x_ & 0xff) << 0)

#define	MX31_PIN_SD20(_x_)			((_x_ & 0xff) << 24)
#define	MX31_PIN_SD21(_x_)			((_x_ & 0xff) << 16)
#define	MX31_PIN_SD22(_x_)			((_x_ & 0xff) << 8)
#define	MX31_PIN_SD23(_x_)			((_x_ & 0xff) << 0)

#define	MX31_PIN_SD16(_x_)			((_x_ & 0xff) << 24)
#define	MX31_PIN_SD17(_x_)			((_x_ & 0xff) << 16)
#define	MX31_PIN_SD18(_x_)			((_x_ & 0xff) << 8)
#define	MX31_PIN_SD19(_x_)			((_x_ & 0xff) << 0)

#define	MX31_PIN_SD12(_x_)			((_x_ & 0xff) << 24)
#define	MX31_PIN_SD13(_x_)			((_x_ & 0xff) << 16)
#define	MX31_PIN_SD14(_x_)			((_x_ & 0xff) << 8)
#define	MX31_PIN_SD15(_x_)			((_x_ & 0xff) << 0)

#define	MX31_PIN_SD8(_x_)			((_x_ & 0xff) << 24)
#define	MX31_PIN_SD9(_x_)			((_x_ & 0xff) << 16)
#define	MX31_PIN_SD10(_x_)			((_x_ & 0xff) << 8)
#define	MX31_PIN_SD11(_x_)			((_x_ & 0xff) << 0)

#define	MX31_PIN_SD4(_x_)			((_x_ & 0xff) << 24)
#define	MX31_PIN_SD5(_x_)			((_x_ & 0xff) << 16)
#define	MX31_PIN_SD6(_x_)			((_x_ & 0xff) << 8)
#define	MX31_PIN_SD7(_x_)			((_x_ & 0xff) << 0)

#define	MX31_PIN_SD0(_x_)			((_x_ & 0xff) << 24)
#define	MX31_PIN_SD1(_x_)			((_x_ & 0xff) << 16)
#define	MX31_PIN_SD2(_x_)			((_x_ & 0xff) << 8)
#define	MX31_PIN_SD3(_x_)			((_x_ & 0xff) << 0)

#define	MX31_PIN_A24(_x_)			((_x_ & 0xff) << 24)
#define	MX31_PIN_A25(_x_)			((_x_ & 0xff) << 16)
#define	MX31_PIN_SDBA1(_x_)			((_x_ & 0xff) << 8)
#define	MX31_PIN_SDBA0(_x_)			((_x_ & 0xff) << 0)

#define	MX31_PIN_A20(_x_)			((_x_ & 0xff) << 24)
#define	MX31_PIN_A21(_x_)			((_x_ & 0xff) << 16)
#define	MX31_PIN_A22(_x_)			((_x_ & 0xff) << 8)
#define	MX31_PIN_A23(_x_)			((_x_ & 0xff) << 0)

#define	MX31_PIN_A16(_x_)			((_x_ & 0xff) << 24)
#define	MX31_PIN_A17(_x_)			((_x_ & 0xff) << 16)
#define	MX31_PIN_A18(_x_)			((_x_ & 0xff) << 8)
#define	MX31_PIN_A19(_x_)			((_x_ & 0xff) << 0)

#define	MX31_PIN_A12(_x_)			((_x_ & 0xff) << 24)
#define	MX31_PIN_A13(_x_)			((_x_ & 0xff) << 16)
#define	MX31_PIN_A14(_x_)			((_x_ & 0xff) << 8)
#define	MX31_PIN_A15(_x_)			((_x_ & 0xff) << 0)

#define	MX31_PIN_A9(_x_)			((_x_ & 0xff) << 24)
#define	MX31_PIN_A10(_x_)			((_x_ & 0xff) << 16)
#define	MX31_PIN_MA10(_x_)			((_x_ & 0xff) << 8)
#define	MX31_PIN_A11(_x_)			((_x_ & 0xff) << 0)

#define	MX31_PIN_A5(_x_)			((_x_ & 0xff) << 24)
#define	MX31_PIN_A6(_x_)			((_x_ & 0xff) << 16)
#define	MX31_PIN_A7(_x_)			((_x_ & 0xff) << 8)
#define	MX31_PIN_A8(_x_)			((_x_ & 0xff) << 0)

#define	MX31_PIN_A1(_x_)			((_x_ & 0xff) << 24)
#define	MX31_PIN_A2(_x_)			((_x_ & 0xff) << 16)
#define	MX31_PIN_A3(_x_)			((_x_ & 0xff) << 8)
#define	MX31_PIN_A4(_x_)			((_x_ & 0xff) << 0)

#define	MX31_PIN_DVFS1(_x_)			((_x_ & 0xff) << 24)
#define	MX31_PIN_VPG0(_x_)			((_x_ & 0xff) << 16)
#define	MX31_PIN_VPG1(_x_)			((_x_ & 0xff) << 8)
#define	MX31_PIN_A0(_x_)			((_x_ & 0xff) << 0)

#define	MX31_PIN_CKIL(_x_)			((_x_ & 0xff) << 24)
#define MX31_PIN_POWER_FAIL(_x_)	((_x_ & 0xff) << 16)
#define	MX31_PIN_VSTBY(_x_)			((_x_ & 0xff) << 8)
#define	MX31_PIN_DVFS0(_x_)			((_x_ & 0xff) << 0)

#define	MX31_PIN_BOOT_MODE1(_x_)	((_x_ & 0xff) << 24)
#define	MX31_PIN_BOOT_MODE2(_x_)	((_x_ & 0xff) << 16)
#define	MX31_PIN_BOOT_MODE3(_x_)	((_x_ & 0xff) << 8)
#define	MX31_PIN_BOOT_MODE4(_x_)	((_x_ & 0xff) << 0)

#define	MX31_PIN_RESET_IN_B(_x_)	((_x_ & 0xff) << 24)
#define	MX31_PIN_POR_B(_x_)			((_x_ & 0xff) << 16)
#define	MX31_PIN_CLKO(_x_)			((_x_ & 0xff) << 8)
#define	MX31_PIN_BOOT_MODE0(_x_)	((_x_ & 0xff) << 0)

#define	MX31_PIN_STX0(_x_)			((_x_ & 0xff) << 24)
#define	MX31_PIN_SRX0(_x_)			((_x_ & 0xff) << 16)
#define	MX31_PIN_SIMPD0(_x_)		((_x_ & 0xff) << 8)
#define	MX31_PIN_CKIH(_x_)			((_x_ & 0xff) << 0)

#define	MX31_PIN_GPIO3_1(_x_)		((_x_ & 0xff) << 24)
#define	MX31_PIN_SCLK0(_x_)			((_x_ & 0xff) << 16)
#define	MX31_PIN_SRST0(_x_)			((_x_ & 0xff) << 8)
#define	MX31_PIN_SVEN0(_x_)			((_x_ & 0xff) << 0)

#define	MX31_PIN_GPIO1_4(_x_)		((_x_ & 0xff) << 24)
#define	MX31_PIN_GPIO1_5(_x_)		((_x_ & 0xff) << 16)
#define	MX31_PIN_GPIO1_6(_x_)		((_x_ & 0xff) << 8)
#define	MX31_PIN_GPIO3_0(_x_)		((_x_ & 0xff) << 0)

#define	MX31_PIN_GPIO1_0(_x_)		((_x_ & 0xff) << 24)
#define	MX31_PIN_GPIO1_1(_x_)		((_x_ & 0xff) << 16)
#define	MX31_PIN_GPIO1_2(_x_)		((_x_ & 0xff) << 8)
#define	MX31_PIN_GPIO1_3(_x_)		((_x_ & 0xff) << 0)

#define	MX31_PIN_CAPTURE(_x_)		 ((_x_ & 0xff) << 24)
#define	MX31_PIN_COMPARE(_x_)		 ((_x_ & 0xff) << 16)
#define	MX31_PIN_WATCHDOG_RST(_x_)	 ((_x_ & 0xff) << 8)
#define	MX31_PIN_PWMO(_x_)			 ((_x_ & 0xff) << 0)

//// MX31_PIN_TTM_PAD = can not be written to.
//// cspi_miso = U2_RXD, cspi3_sclk = U2_RTS, cspi3_spi_rdy = U2_CTS, ttm_pad = default
//IOMUX_CTL_REG(SW_MUX_CTL_REGISTER1) = MX31_PIN_CSPI3_MISO(OUTPUTCONFIG_ALT1 | INPUTCONFIG_NONE)
//									| MX31_PIN_CSPI3_SCLK(OUTPUTCONFIG_ALT1 | INPUTCONFIG_NONE)
//									| MX31_PIN_CSPI3_SPI_RDY(OUTPUTCONFIG_ALT1 | INPUTCONFIG_NONE)
//									| MX31_PIN_TTM_PAD();
//
//// MX31_PIN_CLKSS and MX31_PIN_CE_CONTROL = can not be written to.
//// reset_b = NC, ce_control = default, ctl_clkss = default, cspi3_mosi = U2_RXD
//IOMUX_CTL_REG(SW_MUX_CTL_REGISTER2) = MX31_PIN_ATA_RESET_B(OUTPUTCONFIG_GPIO | INPUTCONFIG_GPIO)
//									| MX31_PIN_CE_CONTROL(OUTPUTCONFIG_FUNC | INPUTCONFIG_FUNC)
//									| MX31_PIN_CLKSS()
//									| MX31_PIN_CSPI3_MOSI(OUTPUTCONFIG_GPIO | INPUTCONFIG_ALT2);
//
//// ata_cs1 = NC, ata_dior = D_TXD, ata_diow = NC, ata, dmack = NC
//IOMUX_CTL_REG(SW_MUX_CTL_REGISTER3) = MX31_PIN_ATA_CS1(OUTPUTCONFIG_GPIO | INPUTCONFIG_GPIO)
//									| MX31_PIN_ATA_DIOR(OUTPUTCONFIG_ALT1 | INPUTCONFIG_NONE)
//									| MX31_PIN_ATA_DIOW(OUTPUTCONFIG_GPIO | INPUTCONFIG_GPIO)
//									| MX31_PIN_ATA_DMACK(OUTPUTCONFIG_GPIO | INPUTCONFIG_GPIO);
//
//// sd1_data1 = SD_D1, sd1_data2 = SD_D2, sd1_data3 = SD_D3, ata_cs0 = D_RXD
//IOMUX_CTL_REG(SW_MUX_CTL_REGISTER4) = MX31_PIN_SD1_DATA1(OUTPUTCONFIG_FUNC | INPUTCONFIG_FUNC)
//									| MX31_PIN_SD1_DATA2(OUTPUTCONFIG_FUNC | INPUTCONFIG_FUNC)
//									| MX31_PIN_SD1_DATA3(OUTPUTCONFIG_FUNC | INPUTCONFIG_FUNC)
//									| MX31_PIN_ATA_CS0(OUTPUTCONFIG_GPIO | INPUTCONFIG_ALT1);
//
//// d3_spl = NC, sd1_cmd = SD_CMD, sd1_clk - SD_CLK, sd1_data0 = SD_D0
//IOMUX_CTL_REG(SW_MUX_CTL_REGISTER5) = MX31_PIN_D3_SPL(OUTPUTCONFIG_GPIO | INPUTCONFIG_GPIO)
//									| MX31_PIN_SD1_CMD(OUTPUTCONFIG_FUNC | INPUTCONFIG_FUNC)
//									| MX31_PIN_SD1_CLK(OUTPUTCONFIG_FUNC | INPUTCONFIG_NONE)
//									| MX31_PIN_SD1_DATA0(OUTPUTCONFIG_FUNC | INPUTCONFIG_FUNC);
//
//// vsync3 = LCD_VSYNC, contrast = NC, d3_rev = NC, d3_cls = NC
//IOMUX_CTL_REG(SW_MUX_CTL_REGISTER6) = MX31_PIN_VSYNC3(OUTPUTCONFIG_FUNC | INPUTCONFIG_NONE)
//									| MX31_PIN_CONTRAST(OUTPUTCONFIG_GPIO | INPUTCONFIG_NONE)
//									| MX31_PIN_D3_REV(OUTPUTCONFIG_GPIO | INPUTCONFIG_NONE)
//									| MX31_PIN_D3_CLS(OUTPUTCONFIG_GPIO | INPUTCONFIG_NONE);
//
//// ser_rs = NC, par_rs = NC, write = NC, read = NC
//IOMUX_CTL_REG(SW_MUX_CTL_REGISTER7) = MX31_PIN_SER_RS(OUTPUTCONFIG_GPIO | INPUTCONFIG_GPIO)
//									| MX31_PIN_PAR_RS(OUTPUTCONFIG_GPIO | INPUTCONFIG_GPIO)
//									| MX31_PIN_WRITE(OUTPUTCONFIG_GPIO | INPUTCONFIG_GPIO)
//									| MX31_PIN_READ(OUTPUTCONFIG_GPIO | INPUTCONFIG_GPIO);
//
//// sd_d_io = NC, sd_d_clk = NC, lcs0 = NC, lcs1 = NC
//IOMUX_CTL_REG(SW_MUX_CTL_REGISTER8) = MX31_PIN_SD_D_IO(OUTPUTCONFIG_GPIO | INPUTCONFIG_GPIO)
//									| MX31_PIN_SD_D_CLK(OUTPUTCONFIG_GPIO | INPUTCONFIG_GPIO)
//									| MX31_PIN_LCS0(OUTPUTCONFIG_GPIO | INPUTCONFIG_GPIO)
//									| MX31_PIN_LCS1(OUTPUTCONFIG_GPIO | INPUTCONFIG_GPIO);
//
//// hsync = LCD_HSYNC, fpshift = LCD_PCLK, drdy0 = LCD_OE, sd_d_i = NC
//IOMUX_CTL_REG(SW_MUX_CTL_REGISTER9) = MX31_PIN_HSYNC(OUTPUTCONFIG_FUNC | INPUTCONFIG_NONE)
//									| MX31_PIN_FPSHIFT(OUTPUTCONFIG_FUNC | INPUTCONFIG_NONE)
//									| MX31_PIN_DRDY0(OUTPUTCONFIG_FUNC | INPUTCONFIG_NONE)
//									| MX31_PIN_SD_D_I(OUTPUTCONFIG_GPIO | INPUTCONFIG_GPIO);
//
//// ld15 = LCD_R3, ld16 = LCD_R4, ld17 = LCD_R5, sd_d_i = NC
//IOMUX_CTL_REG(SW_MUX_CTL_REGISTER10) = MX31_PIN_LD15(OUTPUTCONFIG_FUNC | INPUTCONFIG_FUNC)
//									| MX31_PIN_LD16(OUTPUTCONFIG_FUNC | INPUTCONFIG_FUNC)
//									| MX31_PIN_LD17(OUTPUTCONFIG_FUNC | INPUTCONFIG_FUNC)
//									| MX31_PIN_VSYNC0(OUTPUTCONFIG_GPIO | INPUTCONFIG_GPIO);
//
//// ld11 = LCD_G5, ld12 = LCD_R0, ld13 = LCD_R1, ld14 = LCD_R2
//IOMUX_CTL_REG(SW_MUX_CTL_REGISTER11) = MX31_PIN_LD11(OUTPUTCONFIG_FUNC | INPUTCONFIG_FUNC)
//									| MX31_PIN_LD12(OUTPUTCONFIG_FUNC | INPUTCONFIG_FUNC)
//									| MX31_PIN_LD13(OUTPUTCONFIG_FUNC | INPUTCONFIG_FUNC)
//									| MX31_PIN_LD14(OUTPUTCONFIG_FUNC | INPUTCONFIG_FUNC);
//
//// ld7 = LCD_G1, ld8 = LCD_G2, ld9 = LCD_G3, ld10 = LCD_G4
//IOMUX_CTL_REG(SW_MUX_CTL_REGISTER12) = MX31_PIN_LD7(OUTPUTCONFIG_FUNC | INPUTCONFIG_FUNC)
//									| MX31_PIN_LD8(OUTPUTCONFIG_FUNC | INPUTCONFIG_FUNC)
//									| MX31_PIN_LD9(OUTPUTCONFIG_FUNC | INPUTCONFIG_FUNC)
//									| MX31_PIN_LD10(OUTPUTCONFIG_FUNC | INPUTCONFIG_FUNC);
//
//// ld3 = LCD_B3, ld4 = LCD_B4, ld5 = LCD_B5, ld6 = LCD_G0
//IOMUX_CTL_REG(SW_MUX_CTL_REGISTER13) = MX31_PIN_LD3(OUTPUTCONFIG_FUNC | INPUTCONFIG_FUNC)
//									| MX31_PIN_LD4(OUTPUTCONFIG_FUNC | INPUTCONFIG_FUNC)
//									| MX31_PIN_LD5(OUTPUTCONFIG_FUNC | INPUTCONFIG_FUNC)
//									| MX31_PIN_LD6(OUTPUTCONFIG_FUNC | INPUTCONFIG_FUNC);
//
//// usbh2_data1 = UH2_D1, ld0 = LCD_B0, ld1 = LCD_B1, ld2 = LCD_B0
//IOMUX_CTL_REG(SW_MUX_CTL_REGISTER14) = MX31_PIN_USBH2_DATA1(OUTPUTCONFIG_FUNC | INPUTCONFIG_FUNC)
//									| MX31_PIN_LD0(OUTPUTCONFIG_FUNC | INPUTCONFIG_FUNC)
//									| MX31_PIN_LD1(OUTPUTCONFIG_FUNC | INPUTCONFIG_FUNC)
//									| MX31_PIN_LD2(OUTPUTCONFIG_FUNC | INPUTCONFIG_FUNC);
//
//// usbh2_dir = UH2_DIR, usbh2_stp = UH2_STP, usbh2_nxt = UH2_NXT, usbh2_data0 = UH2_D0
//IOMUX_CTL_REG(SW_MUX_CTL_REGISTER15) = MX31_PIN_USBH2_DIR(OUTPUTCONFIG_FUNC | INPUTCONFIG_NONE)
//									| MX31_PIN_USBH2_STP(OUTPUTCONFIG_FUNC | INPUTCONFIG_FUNC)
//									| MX31_PIN_USBH2_NXT(OUTPUTCONFIG_FUNC | INPUTCONFIG_FUNC)
//									| MX31_PIN_USBH2_DATA0(OUTPUTCONFIG_FUNC | INPUTCONFIG_FUNC);
//
//// usbotg_data5 = UD_D5, usbotg_data6 = UD_D6, usbotg_data7 = UD_D7, usbh2_clk = UH2_CLK
//IOMUX_CTL_REG(SW_MUX_CTL_REGISTER16) = MX31_PIN_USBOTG_DATA5(OUTPUTCONFIG_FUNC | INPUTCONFIG_FUNC)
//									| MX31_PIN_USBOTG_DATA6(OUTPUTCONFIG_FUNC | INPUTCONFIG_FUNC)
//									| MX31_PIN_USBOTG_DATA7(OUTPUTCONFIG_FUNC | INPUTCONFIG_FUNC)
//									| MX31_PIN_USBH2_CLK(OUTPUTCONFIG_FUNC | INPUTCONFIG_FUNC);
//
//// usbotg_data1 = UD_D1, usbotg_data2 = UD_D2, usbotg_data3 = UD_D3, usbotg_data4 = UD_D4
//IOMUX_CTL_REG(SW_MUX_CTL_REGISTER17) = MX31_PIN_USBOTG_DATA1(OUTPUTCONFIG_FUNC | INPUTCONFIG_FUNC)
//									| MX31_PIN_USBOTG_DATA2(OUTPUTCONFIG_FUNC | INPUTCONFIG_FUNC)
//									| MX31_PIN_USBOTG_DATA3(OUTPUTCONFIG_FUNC | INPUTCONFIG_FUNC)
//									| MX31_PIN_USBOTG_DATA4(OUTPUTCONFIG_FUNC | INPUTCONFIG_FUNC);
//
//// usbotg_dir = UD_DIR, usbotg_stp = UD_STP, usbotg_nxt = UD_NXT, usbotg_data0 = UD_D0
//IOMUX_CTL_REG(SW_MUX_CTL_REGISTER18) = MX31_PIN_USBOTG_DIR(OUTPUTCONFIG_FUNC | INPUTCONFIG_FUNC)
//									| MX31_PIN_USBOTG_STP(OUTPUTCONFIG_FUNC | INPUTCONFIG_NONE)
//									| MX31_PIN_USBOTG_NXT(OUTPUTCONFIG_FUNC | INPUTCONFIG_FUNC)
//									| MX31_PIN_USBOTG_DATA0(OUTPUTCONFIG_FUNC | INPUTCONFIG_FUNC);
//
//// usb_pwr = NC, usb_oc = CF_RST, usb_byp = NC, usbotg_clk = UD_CLK
//IOMUX_CTL_REG(SW_MUX_CTL_REGISTER19) = MX31_PIN_USB_PWR(OUTPUTCONFIG_GPIO | INPUTCONFIG_GPIO)
//									| MX31_PIN_USB_OC(OUTPUTCONFIG_GPIO | INPUTCONFIG_NONE)
//									| MX31_PIN_USB_BYP(OUTPUTCONFIG_GPIO | INPUTCONFIG_GPIO)
//									| MX31_PIN_USBOTG_CLK(OUTPUTCONFIG_FUNC | INPUTCONFIG_FUNC);
//
//// MX31_PIN_TDO and MX31_PIN_SJC_MOD = can not be written to
//// tdo = TDO_C, trstb = TRST_C, de_b = TP1, sjc_mod = GND
//IOMUX_CTL_REG(SW_MUX_CTL_REGISTER20) = MX31_PIN_TDO()
//									| MX31_PIN_TRSTB(OUTPUTCONFIG_FUNC | INPUTCONFIG_FUNC)
//									| MX31_PIN_DE_B(OUTPUTCONFIG_FUNC | INPUTCONFIG_FUNC)
//									| MX31_PIN_SJC_MOD();
//
//// MX31_PIN_RTCK = can not be written to.
//// rtck = NC, tck = TCK_C, tms = TMS_C, tdi = TDI_C
//IOMUX_CTL_REG(SW_MUX_CTL_REGISTER21) = MX31_PIN_RTCK()
//									| MX31_PIN_TCK(OUTPUTCONFIG_FUNC | INPUTCONFIG_FUNC)
//									| MX31_PIN_TMS(OUTPUTCONFIG_FUNC | INPUTCONFIG_FUNC)
//									| MX31_PIN_TDI(OUTPUTCONFIG_FUNC | INPUTCONFIG_FUNC);
//
//// key_col4 = NC, key_col5 = NC, key_col6 = NC, key_col7 = NC
//IOMUX_CTL_REG(SW_MUX_CTL_REGISTER22) = MX31_PIN_KEY_COL4(OUTPUTCONFIG_GPIO | INPUTCONFIG_GPIO)
//									| MX31_PIN_KEY_COL5(OUTPUTCONFIG_GPIO | INPUTCONFIG_GPIO)
//									| MX31_PIN_KEY_COL6(OUTPUTCONFIG_GPIO | INPUTCONFIG_GPIO)
//									| MX31_PIN_KEY_COL7(OUTPUTCONFIG_GPIO | INPUTCONFIG_GPIO);
//
//// key_col0 = NC, key_col1 = NC, key_col2 = NC, key_col3 = NC
//IOMUX_CTL_REG(SW_MUX_CTL_REGISTER23) = MX31_PIN_KEY_COL0(OUTPUTCONFIG_GPIO | INPUTCONFIG_NONE)
//									| MX31_PIN_KEY_COL1(OUTPUTCONFIG_GPIO | INPUTCONFIG_NONE)
//									| MX31_PIN_KEY_COL2(OUTPUTCONFIG_GPIO | INPUTCONFIG_NONE)
//									| MX31_PIN_KEY_COL3(OUTPUTCONFIG_GPIO | INPUTCONFIG_NONE);
//
//// key_row4 = NC, key_row5 = NC, key_row6 = NC, key_row7 = NC
//IOMUX_CTL_REG(SW_MUX_CTL_REGISTER24) = MX31_PIN_KEY_ROW4(OUTPUTCONFIG_GPIO | INPUTCONFIG_GPIO)
//									| MX31_PIN_KEY_ROW5(OUTPUTCONFIG_GPIO | INPUTCONFIG_GPIO)
//									| MX31_PIN_KEY_ROW6(OUTPUTCONFIG_GPIO | INPUTCONFIG_GPIO)
//									| MX31_PIN_KEY_ROW7(OUTPUTCONFIG_GPIO | INPUTCONFIG_GPIO);
//
//// key_row0 = NC, key_row1 = NC, key_row2 = NC, key_row3 = NC
//IOMUX_CTL_REG(SW_MUX_CTL_REGISTER25) = MX31_PIN_KEY_ROW0(OUTPUTCONFIG_GPIO | INPUTCONFIG_NONE)
//									| MX31_PIN_KEY_ROW1(OUTPUTCONFIG_GPIO | INPUTCONFIG_NONE)
//									| MX31_PIN_KEY_ROW2(OUTPUTCONFIG_GPIO | INPUTCONFIG_NONE)
//									| MX31_PIN_KEY_ROW3(OUTPUTCONFIG_GPIO | INPUTCONFIG_NONE);
//
//// txd2 = U1_TXD, rts2 = U1_RTS, cts2 = U1_CTS, batt_line = NC
//IOMUX_CTL_REG(SW_MUX_CTL_REGISTER26) = MX31_PIN_TXD2(OUTPUTCONFIG_FUNC | INPUTCONFIG_NONE)
//									| MX31_PIN_RTS2(OUTPUTCONFIG_FUNC | INPUTCONFIG_FUNC)
//									| MX31_PIN_CTS2(OUTPUTCONFIG_FUNC | INPUTCONFIG_NONE)
//									| MX31_PIN_BATT_LINE(OUTPUTCONFIG_GPIO | INPUTCONFIG_GPIO);
//
//// ri_dte1, dcd_dte1, and dtr_dce2 are set to CSPI1 signals by GPR(2)
//// rxd2 = U1_RXD
//IOMUX_CTL_REG(SW_MUX_CTL_REGISTER27) = MX31_PIN_RI_DTE1()
//									| MX31_PIN_DCD_DTE1()
//									| MX31_PIN_DTR_DCE2()
//									| MX31_PIN_RXD2(OUTPUTCONFIG_GPIO | INPUTCONFIG_FUNC);
//
//// dtr_dte1 and dsr_dte1 are set to CSPI1 signals by GPR(2)
//// ri_dce1 = SPI0_RDY, dcd_dce1 = NC
//IOMUX_CTL_REG(SW_MUX_CTL_REGISTER28) = MX31_PIN_RI_DCE1(OUTPUTCONFIG_ALT1 | INPUTCONFIG_ALT1)
//									| MX31_PIN_DCD_DCE1(OUTPUTCONFIG_GPIO | INPUTCONFIG_GPIO)
//									| MX31_PIN_DTR_DTE1()
//									| MX31_PIN_DSR_DTE1();
//
//// rts1 = U0_RTS, cts1 = U0_CTS, dtr_dce1 = NC, dsr_dce1 = SPI0_CLK
//IOMUX_CTL_REG(SW_MUX_CTL_REGISTER29) = MX31_PIN_RTS1(OUTPUTCONFIG_FUNC | INPUTCONFIG_FUNC)
//									| MX31_PIN_CTS1(OUTPUTCONFIG_FUNC | INPUTCONFIG_NONE)
//									| MX31_PIN_DTR_DCE1(OUTPUTCONFIG_GPIO | INPUTCONFIG_GPIO)
//									| MX31_PIN_DSR_DCE1(OUTPUTCONFIG_ALT1 | INPUTCONFIG_ALT1);
//
//// cspi2_sclk = SPI1_CLK, cspi2_spi_rdy = SPI1_RDY, rxd1 = U0_RXD, txd1 = U0_TXD
//IOMUX_CTL_REG(SW_MUX_CTL_REGISTER30) = MX31_PIN_CSPI2_SCLK(OUTPUTCONFIG_FUNC | INPUTCONFIG_FUNC)
//									| MX31_PIN_CSPI2_SPI_RDY(OUTPUTCONFIG_FUNC | INPUTCONFIG_FUNC)
//									| MX31_PIN_RXD1(OUTPUTCONFIG_GPIO | INPUTCONFIG_FUNC)
//									| MX31_PIN_TXD1(OUTPUTCONFIG_FUNC | INPUTCONFIG_NONE);
//
//// cspi2_miso = SPI1_MISO, cspi2_ss0 = SPI1_CS0, cspi2_ss1 = SPI1_CS1, cspi2_ss2 = NC
//IOMUX_CTL_REG(SW_MUX_CTL_REGISTER31) = MX31_PIN_CSPI2_MISO(OUTPUTCONFIG_FUNC | INPUTCONFIG_FUNC)
//									| MX31_PIN_CSPI2_SS0(OUTPUTCONFIG_FUNC | INPUTCONFIG_FUNC)
//									| MX31_PIN_CSPI2_SS1(OUTPUTCONFIG_FUNC | INPUTCONFIG_FUNC)
//									| MX31_PIN_CSPI2_SS2(OUTPUTCONFIG_GPIO | INPUTCONFIG_NONE);
//
//// cspi1_ss2 = UH1_RCV, cspi1_sclk = UH1_OE, cspi1_spi_rdy = UH1_FS, cspi2_mosi = SPI1_MOSI
//IOMUX_CTL_REG(SW_MUX_CTL_REGISTER32) = MX31_PIN_CSPI1_SS2(OUTPUTCONFIG_ALT1 | INPUTCONFIG_ALT1)
//									| MX31_PIN_CSPI1_SCLK(OUTPUTCONFIG_ALT1 | INPUTCONFIG_ALT1)
//									| MX31_PIN_CSPI1_SPI_RDY(OUTPUTCONFIG_ALT1 | INPUTCONFIG_ALT1)
//									| MX31_PIN_CSPI2_MOSI(OUTPUTCONFIG_FUNC | INPUTCONFIG_FUNC);
//
//// cspi1_mosi = UH1_RXDM, cspi1_miso = UH1_RXDP, cspi1_ss0 = UH1_TXDM, cspi1_ss1 = UH1_TXDP
//IOMUX_CTL_REG(SW_MUX_CTL_REGISTER33) = MX31_PIN_CSPI1_MOSI(OUTPUTCONFIG_GPIO | INPUTCONFIG_ALT1)
//									| MX31_PIN_CSPI1_MISO(OUTPUTCONFIG_GPIO | INPUTCONFIG_ALT1)
//									| MX31_PIN_CSPI1_SS0(OUTPUTCONFIG_ALT1 | INPUTCONFIG_NONE)
//									| MX31_PIN_CSPI1_SS1(OUTPUTCONFIG_ALT1 | INPUTCONFIG_NONE);
//
//// stxd6 = AC_SDOUT, srxd6 = AC_SDIN, sck6 = AC_BCLK, sfs6 = AC_SYNC
//IOMUX_CTL_REG(SW_MUX_CTL_REGISTER34) = MX31_PIN_STXD6(OUTPUTCONFIG_FUNC | INPUTCONFIG_FUNC)
//									| MX31_PIN_SRXD6(OUTPUTCONFIG_FUNC | INPUTCONFIG_FUNC)
//									| MX31_PIN_SCK6(OUTPUTCONFIG_FUNC | INPUTCONFIG_FUNC)
//									| MX31_PIN_SFS6(OUTPUTCONFIG_FUNC | INPUTCONFIG_FUNC);
//
//// stxd5 = SSI_TXD, srxd5 = SSI_RXD, sck5 = SSI_CLK, sfs5 = SSI_FRM
//IOMUX_CTL_REG(SW_MUX_CTL_REGISTER35) = MX31_PIN_STXD5(OUTPUTCONFIG_FUNC | INPUTCONFIG_FUNC)
//									| MX31_PIN_SRXD5(OUTPUTCONFIG_FUNC | INPUTCONFIG_FUNC)
//									| MX31_PIN_SCK5(OUTPUTCONFIG_FUNC | INPUTCONFIG_FUNC)
//									| MX31_PIN_SFS5(OUTPUTCONFIG_FUNC | INPUTCONFIG_FUNC);
//
//// stxd4 = NC, srxd4 = NC, sck4 = SSI_MCLK, sfs4 = NC
//IOMUX_CTL_REG(SW_MUX_CTL_REGISTER36) = MX31_PIN_STXD4(OUTPUTCONFIG_GPIO | INPUTCONFIG_GPIO)
//									| MX31_PIN_SRXD4(OUTPUTCONFIG_GPIO | INPUTCONFIG_GPIO)
//									| MX31_PIN_SCK4(OUTPUTCONFIG_FUNC | INPUTCONFIG_FUNC)
//									| MX31_PIN_SFS4(OUTPUTCONFIG_GPIO | INPUTCONFIG_GPIO);
//
//// stxd3 = AC_RST, srxd3 = NC, sck3 = NC, sfs3 = NC
//IOMUX_CTL_REG(SW_MUX_CTL_REGISTER37) = MX31_PIN_STXD3(OUTPUTCONFIG_GPIO | INPUTCONFIG_NONE)
//									| MX31_PIN_SRXD3(OUTPUTCONFIG_GPIO | INPUTCONFIG_GPIO)
//									| MX31_PIN_SCK3(OUTPUTCONFIG_GPIO | INPUTCONFIG_NONE)
//									| MX31_PIN_SFS3(OUTPUTCONFIG_GPIO | INPUTCONFIG_NONE);
//
//// csi_hsync = VIP_HSYNC, csi_pixclk = VIP_PCLK, i2c_clk = I2C_SCL, i2c_dat = I2C_SDA
//IOMUX_CTL_REG(SW_MUX_CTL_REGISTER38) = MX31_PIN_CSI_HSYNC(OUTPUTCONFIG_FUNC | INPUTCONFIG_FUNC)
//									| MX31_PIN_CSI_PIXCLK(OUTPUTCONFIG_FUNC | INPUTCONFIG_FUNC)
//									| MX31_PIN_I2C_CLK(OUTPUTCONFIG_FUNC | INPUTCONFIG_FUNC)
//									| MX31_PIN_I2C_DAT(OUTPUTCONFIG_FUNC | INPUTCONFIG_FUNC);
//
//// csi_d14 = VIP_D8, csi_d15 = VIP_D9, csi_mclk = VIP_MCLK, csi_vsync = VIP_VSYNC
//IOMUX_CTL_REG(SW_MUX_CTL_REGISTER39) = MX31_PIN_CSI_D14(OUTPUTCONFIG_FUNC | INPUTCONFIG_FUNC)
//									| MX31_PIN_CSI_D15(OUTPUTCONFIG_FUNC | INPUTCONFIG_FUNC)
//									| MX31_PIN_CSI_MCLK(OUTPUTCONFIG_FUNC | INPUTCONFIG_NONE)
//									| MX31_PIN_CSI_VSYNC(OUTPUTCONFIG_FUNC | INPUTCONFIG_FUNC);
//
//// csi_d10 = VIP_D4, csi_d11 = VIP_D5, csi_D12 = VIP_D6, csi_D13 = VIP_D7
//IOMUX_CTL_REG(SW_MUX_CTL_REGISTER40) = MX31_PIN_CSI_D10(OUTPUTCONFIG_FUNC | INPUTCONFIG_FUNC)
//									| MX31_PIN_CSI_D11(OUTPUTCONFIG_FUNC | INPUTCONFIG_FUNC)
//									| MX31_PIN_CSI_D12(OUTPUTCONFIG_FUNC | INPUTCONFIG_FUNC)
//									| MX31_PIN_CSI_D13(OUTPUTCONFIG_FUNC | INPUTCONFIG_FUNC);
//
//// csi_d6 = VIP_D0, csi_d7 = VIP_D1, csi_D8 = VIP_D2, csi_D9 = VIP_D3
//IOMUX_CTL_REG(SW_MUX_CTL_REGISTER41) = MX31_PIN_CSI_D6(OUTPUTCONFIG_FUNC | INPUTCONFIG_FUNC)
//									| MX31_PIN_CSI_D7(OUTPUTCONFIG_FUNC | INPUTCONFIG_FUNC)
//									| MX31_PIN_CSI_D8(OUTPUTCONFIG_FUNC | INPUTCONFIG_FUNC)
//									| MX31_PIN_CSI_D9(OUTPUTCONFIG_FUNC | INPUTCONFIG_FUNC);
//
//// m_request = NC, m_grant = NC, csi_d4 = NC, csi_d5 = NC
//IOMUX_CTL_REG(SW_MUX_CTL_REGISTER42) = MX31_PIN_M_REQUEST(OUTPUTCONFIG_FUNC | INPUTCONFIG_FUNC)
//									| MX31_PIN_M_GRANT(OUTPUTCONFIG_FUNC | INPUTCONFIG_FUNC)
//									| MX31_PIN_CSI_D4(OUTPUTCONFIG_GPIO | INPUTCONFIG_GPIO)
//									| MX31_PIN_CSI_D5(OUTPUTCONFIG_GPIO | INPUTCONFIG_GPIO);
//
//// pc_rst = UH2_D5, isis16 = UH2_D6, pc_rw_b = UH2_D7, pc_poe = NC
//IOMUX_CTL_REG(SW_MUX_CTL_REGISTER43) = MX31_PIN_PC_RST(OUTPUTCONFIG_ALT1 | INPUTCONFIG_ALT1)
//									| MX31_PIN_IOIS16(OUTPUTCONFIG_ALT1 | INPUTCONFIG_ALT1)
//									| MX31_PIN_PC_RW_B(OUTPUTCONFIG_ALT1 | INPUTCONFIG_ALT1)
//									| MX31_PIN_PC_POE(OUTPUTCONFIG_GPIO | INPUTCONFIG_GPIO);
//
//// pc_vs1 = NC, pc_vs2 = UH2_D2, pc_bvd1 = UH2_D3, pc_bvd2 = UH2_D4
//IOMUX_CTL_REG(SW_MUX_CTL_REGISTER44) = MX31_PIN_PC_VS1(OUTPUTCONFIG_GPIO | INPUTCONFIG_GPIO)
//									| MX31_PIN_PC_VS2(OUTPUTCONFIG_ALT1 | INPUTCONFIG_ALT1)
//									| MX31_PIN_PC_BVD1(OUTPUTCONFIG_ALT1 | INPUTCONFIG_ALT1)
//									| MX31_PIN_PC_BVD2(OUTPUTCONFIG_ALT1 | INPUTCONFIG_ALT1);
//
//// pc_cd2_b = CF_CD, pc_wait_b = CF_WAIT, pc_ready = CF_RDY, pc_pwron = NC
//IOMUX_CTL_REG(SW_MUX_CTL_REGISTER45) = MX31_PIN_PC_CD2_B(OUTPUTCONFIG_FUNC | INPUTCONFIG_FUNC)
//									| MX31_PIN_PC_WAIT_B(OUTPUTCONFIG_FUNC | INPUTCONFIG_FUNC)
//									| MX31_PIN_PC_READY(OUTPUTCONFIG_FUNC | INPUTCONFIG_FUNC)
//									| MX31_PIN_PC_PWRON(OUTPUTCONFIG_GPIO | INPUTCONFIG_GPIO);
//
//// d2, d1, and d0 are not writable, reset values are correct.
//// d2 = LD2, d1 = LD1, d0 = LD0, pc_cd1_b = CF_CD
//IOMUX_CTL_REG(SW_MUX_CTL_REGISTER46) = MX31_PIN_D2()
//									| MX31_PIN_D1()
//									| MX31_PIN_D0()
//									| MX31_PIN_PC_CD1_B(OUTPUTCONFIG_FUNC | INPUTCONFIG_FUNC);
//
//// d6, d5, d4 and d3 are not writable, reset values are correct.
//// d6 = LD6, d5 = LD5, d4 = LD4, d3 = LD3
//IOMUX_CTL_REG(SW_MUX_CTL_REGISTER47) = MX31_PIN_D6()
//									| MX31_PIN_D5()
//									| MX31_PIN_D4()
//									| MX31_PIN_D3();
//
//// d10, d9, d8 and d7 are not writable, reset values are correct.
//// d10 = LD10, d9 = LD9, d8 = LD8, d7 = LD7
//IOMUX_CTL_REG(SW_MUX_CTL_REGISTER48) = MX31_PIN_D10()
//									| MX31_PIN_D9()
//									| MX31_PIN_D8()
//									| MX31_PIN_D7();
//
//// d14, d13, d12 and d11 are not writable, reset values are correct.
//// d14 = LD14, d13 = LD13, d12 = LD12, d11 = L11
//IOMUX_CTL_REG(SW_MUX_CTL_REGISTER49) = MX31_PIN_D14()
//									| MX31_PIN_D13()
//									| MX31_PIN_D12()
//									| MX31_PIN_D11();
//
//// d15 is not writable, reset value is correct.
//// nfwp_b = *N_WP, nfce_b = *N_CE, nfrb = N_RDY, d15 = LD15
//IOMUX_CTL_REG(SW_MUX_CTL_REGISTER50) = MX31_PIN_NFWP_B(OUTPUTCONFIG_FUNC | INPUTCONFIG_FUNC)
//									| MX31_PIN_NFCE_B(OUTPUTCONFIG_FUNC | INPUTCONFIG_FUNC)
//									| MX31_PIN_NFRB(OUTPUTCONFIG_FUNC | INPUTCONFIG_FUNC)
//									| MX31_PIN_D15();
//
//// nfwe_b = *N_WE, nfre_b = *N_RE, nfale = N_ALE, nfcle = N_CLE
//IOMUX_CTL_REG(SW_MUX_CTL_REGISTER51) = MX31_PIN_NFWE_B(OUTPUTCONFIG_FUNC | INPUTCONFIG_FUNC)
//									| MX31_PIN_NFRE_B(OUTPUTCONFIG_FUNC | INPUTCONFIG_FUNC)
//									| MX31_PIN_NFALE(OUTPUTCONFIG_FUNC | INPUTCONFIG_FUNC)
//									| MX31_PIN_NFCLE(OUTPUTCONFIG_FUNC | INPUTCONFIG_FUNC);
//
//// sdqs0, sdqs1, sdqs2, and sdqs3 are not writable, reset values are correct.
//IOMUX_CTL_REG(SW_MUX_CTL_REGISTER52) = MX31_PIN_SDQS0()
//									| MX31_PIN_SDQS1()
//									| MX31_PIN_SDQS2()
//									| MX31_PIN_SDQS3();
//
//// sdclk_b is not writable, reset value is correct.
//// sdcke0 = SDCKE, sdcke1 = NC, sdclk = SDCLK, sdclk_b = *SDCLK
//IOMUX_CTL_REG(SW_MUX_CTL_REGISTER53) = MX31_PIN_SDCKE0(OUTPUTCONFIG_FUNC | INPUTCONFIG_FUNC)
//									| MX31_PIN_SDCKE1(OUTPUTCONFIG_GPIO | INPUTCONFIG_FUNC)
//									| MX31_PIN_SDCLK(OUTPUTCONFIG_FUNC | INPUTCONFIG_FUNC)
//									| MX31_PIN_SDCLK_B();
//
//// rw = *WE, ras = *SDRAS, cas = *SDCAS, sdwe = *SDWE
//IOMUX_CTL_REG(SW_MUX_CTL_REGISTER54) = MX31_PIN_RW(OUTPUTCONFIG_FUNC | INPUTCONFIG_FUNC)
//									| MX31_PIN_RAS(OUTPUTCONFIG_FUNC | INPUTCONFIG_FUNC)
//									| MX31_PIN_CAS(OUTPUTCONFIG_FUNC | INPUTCONFIG_FUNC)
//									| MX31_PIN_SDWE(OUTPUTCONFIG_FUNC | INPUTCONFIG_FUNC);
//
//// ecb is not writable, reset value is correct.
//// cs5 = *CS5, ecb = ECB, lba = LBA, bclk = NC
//IOMUX_CTL_REG(SW_MUX_CTL_REGISTER55) = MX31_PIN_CS5(OUTPUTCONFIG_FUNC | INPUTCONFIG_FUNC)
//									| MX31_PIN_ECB()
//									| MX31_PIN_LBA(OUTPUTCONFIG_FUNC | INPUTCONFIG_FUNC)
//									| MX31_PIN_BCLK(OUTPUTCONFIG_GPIO | INPUTCONFIG_GPIO);
//
//// cs1 = *CS1, cs2 = *SDCS, cs3 = NC, cs4 = *DTACK
//IOMUX_CTL_REG(SW_MUX_CTL_REGISTER56) = MX31_PIN_CS1(OUTPUTCONFIG_FUNC | INPUTCONFIG_FUNC)
//									| MX31_PIN_CS2(OUTPUTCONFIG_FUNC | INPUTCONFIG_FUNC)
//									| MX31_PIN_CS3(OUTPUTCONFIG_GPIO | INPUTCONFIG_GPIO)
//									| MX31_PIN_CS4(OUTPUTCONFIG_ALT1 | INPUTCONFIG_ALT1);
//
//// eb0 = *BE0, eb1 = *BE1, oe = *OE, cs0 = *CS0
//IOMUX_CTL_REG(SW_MUX_CTL_REGISTER57) = MX31_PIN_EB0(OUTPUTCONFIG_FUNC | INPUTCONFIG_FUNC)
//									| MX31_PIN_EB1(OUTPUTCONFIG_FUNC | INPUTCONFIG_FUNC)
//									| MX31_PIN_OE(OUTPUTCONFIG_FUNC | INPUTCONFIG_FUNC)
//									| MX31_PIN_CS0(OUTPUTCONFIG_FUNC | INPUTCONFIG_FUNC);
//
//// dqm0 = DQM0, dqm1 = DQM1, dqm2 = DQM2, dqm3 = DQM3
//IOMUX_CTL_REG(SW_MUX_CTL_REGISTER58) = MX31_PIN_DQM0(OUTPUTCONFIG_FUNC | INPUTCONFIG_FUNC)
//									| MX31_PIN_DQM1(OUTPUTCONFIG_FUNC | INPUTCONFIG_FUNC)
//									| MX31_PIN_DQM2(OUTPUTCONFIG_FUNC | INPUTCONFIG_FUNC)
//									| MX31_PIN_DQM3(OUTPUTCONFIG_FUNC | INPUTCONFIG_FUNC);
//
//// sd28, sd29, sd30 and sd31 are not writable, reset values are correct.
//IOMUX_CTL_REG(SW_MUX_CTL_REGISTER59) = MX31_PIN_SD28()
//									| MX31_PIN_SD29()
//									| MX31_PIN_SD30()
//									| MX31_PIN_SD31();
//
//// sd24, sd25, sd26 and sd27 are not writable, reset values are correct.
//IOMUX_CTL_REG(SW_MUX_CTL_REGISTER60) = MX31_PIN_SD24()
//									| MX31_PIN_SD25()
//									| MX31_PIN_SD26()
//									| MX31_PIN_SD27();
//
//// sd20, sd21, sd22 and sd23 are not writable, reset values are correct.
//IOMUX_CTL_REG(SW_MUX_CTL_REGISTER61) = MX31_PIN_SD20()
//									| MX31_PIN_SD21()
//									| MX31_PIN_SD22()
//									| MX31_PIN_SD23();
//
//// sd16, sd17, sd18 and sd19 are not writable, reset values are correct.
//IOMUX_CTL_REG(SW_MUX_CTL_REGISTER62) = MX31_PIN_SD16()
//									| MX31_PIN_SD17()
//									| MX31_PIN_SD18()
//									| MX31_PIN_SD19();
//
//// sd12, sd13, sd14 and sd15 are not writable, reset values are correct.
//IOMUX_CTL_REG(SW_MUX_CTL_REGISTER63) = MX31_PIN_SD12()
//									| MX31_PIN_SD13()
//									| MX31_PIN_SD14()
//									| MX31_PIN_SD15();
//
//// sd8, sd9, sd10 and sd11 are not writable, reset values are correct.
//IOMUX_CTL_REG(SW_MUX_CTL_REGISTER64) = MX31_PIN_SD8()
//									| MX31_PIN_SD9()
//									| MX31_PIN_SD10()
//									| MX31_PIN_SD11();
//
//// sd4, sd5, sd6 and sd7 are not writable, reset values are correct.
//IOMUX_CTL_REG(SW_MUX_CTL_REGISTER65) = MX31_PIN_SD4()
//									| MX31_PIN_SD5()
//									| MX31_PIN_SD6()
//									| MX31_PIN_SD7();
//
//// sd0, sd1, sd2 and sd3 are not writable, reset values are correct.
//IOMUX_CTL_REG(SW_MUX_CTL_REGISTER66) = MX31_PIN_SD0()
//									| MX31_PIN_SD1()
//									| MX31_PIN_SD2()
//									| MX31_PIN_SD3();
//
//// a24 = A24, a25 = A25, sdba1 = SDBA1, sdba0 = SDBA0
//IOMUX_CTL_REG(SW_MUX_CTL_REGISTER67) = MX31_PIN_A24(OUTPUTCONFIG_FUNC | INPUTCONFIG_FUNC)
//									| MX31_PIN_A25(OUTPUTCONFIG_FUNC | INPUTCONFIG_FUNC)
//									| MX31_PIN_SDBA1(OUTPUTCONFIG_FUNC | INPUTCONFIG_FUNC)
//									| MX31_PIN_SDBA0(OUTPUTCONFIG_FUNC | INPUTCONFIG_FUNC);
//
//// address lines are one to one.
//IOMUX_CTL_REG(SW_MUX_CTL_REGISTER68) = MX31_PIN_A20(OUTPUTCONFIG_FUNC | INPUTCONFIG_FUNC)
//									| MX31_PIN_A21(OUTPUTCONFIG_FUNC | INPUTCONFIG_FUNC)
//									| MX31_PIN_A22(OUTPUTCONFIG_FUNC | INPUTCONFIG_FUNC)
//									| MX31_PIN_A23(OUTPUTCONFIG_FUNC | INPUTCONFIG_FUNC);
//
//// address lines are one to one.
//IOMUX_CTL_REG(SW_MUX_CTL_REGISTER69) = MX31_PIN_A16(OUTPUTCONFIG_FUNC | INPUTCONFIG_FUNC)
//									| MX31_PIN_A17(OUTPUTCONFIG_FUNC | INPUTCONFIG_FUNC)
//									| MX31_PIN_A18(OUTPUTCONFIG_FUNC | INPUTCONFIG_FUNC)
//									| MX31_PIN_A19(OUTPUTCONFIG_FUNC | INPUTCONFIG_FUNC);
//
//// address lines are one to one.
//IOMUX_CTL_REG(SW_MUX_CTL_REGISTER70) = MX31_PIN_A12(OUTPUTCONFIG_FUNC | INPUTCONFIG_FUNC)
//									| MX31_PIN_A13(OUTPUTCONFIG_FUNC | INPUTCONFIG_FUNC)
//									| MX31_PIN_A14(OUTPUTCONFIG_FUNC | INPUTCONFIG_FUNC)
//									| MX31_PIN_A15(OUTPUTCONFIG_FUNC | INPUTCONFIG_FUNC);
//
//// address lines are one to one.
//IOMUX_CTL_REG(SW_MUX_CTL_REGISTER71) = MX31_PIN_A9(OUTPUTCONFIG_FUNC | INPUTCONFIG_FUNC)
//									| MX31_PIN_A10(OUTPUTCONFIG_FUNC | INPUTCONFIG_FUNC)
//									| MX31_PIN_MA10(OUTPUTCONFIG_FUNC | INPUTCONFIG_FUNC)
//									| MX31_PIN_A11(OUTPUTCONFIG_FUNC | INPUTCONFIG_FUNC);
//
//// address lines are one to one.
//IOMUX_CTL_REG(SW_MUX_CTL_REGISTER72) = MX31_PIN_A5(OUTPUTCONFIG_FUNC | INPUTCONFIG_FUNC)
//									| MX31_PIN_A6(OUTPUTCONFIG_FUNC | INPUTCONFIG_FUNC)
//									| MX31_PIN_A7(OUTPUTCONFIG_FUNC | INPUTCONFIG_FUNC)
//									| MX31_PIN_A8(OUTPUTCONFIG_FUNC | INPUTCONFIG_FUNC);
//
//// address lines are one to one.
//IOMUX_CTL_REG(SW_MUX_CTL_REGISTER73) = MX31_PIN_A1(OUTPUTCONFIG_FUNC | INPUTCONFIG_FUNC)
//									| MX31_PIN_A2(OUTPUTCONFIG_FUNC | INPUTCONFIG_FUNC)
//									| MX31_PIN_A3(OUTPUTCONFIG_FUNC | INPUTCONFIG_FUNC)
//									| MX31_PIN_A4(OUTPUTCONFIG_FUNC | INPUTCONFIG_FUNC);
//
//// dvfs1 = NC, vpg0 = NC, vpg1 = NC, a0 = A0
//IOMUX_CTL_REG(SW_MUX_CTL_REGISTER74) = MX31_PIN_DVFS1(OUTPUTCONFIG_GPIO | INPUTCONFIG_GPIO)
//									| MX31_PIN_VPG0(OUTPUTCONFIG_GPIO | INPUTCONFIG_GPIO)
//									| MX31_PIN_VPG1(OUTPUTCONFIG_GPIO | INPUTCONFIG_GPIO)
//									| MX31_PIN_A0(OUTPUTCONFIG_FUNC | INPUTCONFIG_FUNC);
//
//// ckil and power_fail are not writable, reset values are correct.
////ckil = 32K, power_fail = PWR_FAIL, vstby = VSTBY, dvfs0 = NC
//IOMUX_CTL_REG(SW_MUX_CTL_REGISTER75) = MX31_PIN_CKIL()
//									| MX31_PIN_POWER_FAIL()
//									| MX31_PIN_VSTBY(OUTPUTCONFIG_FUNC | INPUTCONFIG_FUNC)
//									| MX31_PIN_DVFS0(OUTPUTCONFIG_GPIO | INPUTCONFIG_GPIO);
//
//// boot_mode1, boot_mode2, boot_mode3, and boot_mode4 are not writable, reset values are correct.
//IOMUX_CTL_REG(SW_MUX_CTL_REGISTER76) = MX31_PIN_BOOT_MODE1()
//									| MX31_PIN_BOOT_MODE2()
//									| MX31_PIN_BOOT_MODE3()
//									| MX31_PIN_BOOT_MODE4();
//
//// por_b, clko, and boot_mode0 are not writable, reset values are correct.
//// reset_in_b = *RST_IN (this is set in the GPR)
//IOMUX_CTL_REG(SW_MUX_CTL_REGISTER77) = MX31_PIN_RESET_IN_B(OUTPUTCONFIG_FUNC | INPUTCONFIG_FUNC)
//									| MX31_PIN_POR_B()
//									| MX31_PIN_CLKO()
//									| MX31_PIN_BOOT_MODE0();
//
//// ckih is not writable, reset value is correct.
//// stx0 = GPIO1, srx0 = GPIO4, simpd0 = GPIO5
//IOMUX_CTL_REG(SW_MUX_CTL_REGISTER78) = MX31_PIN_STX0(OUTPUTCONFIG_GPIO | INPUTCONFIG_GPIO)
//									| MX31_PIN_SRX0(OUTPUTCONFIG_GPIO | INPUTCONFIG_GPIO)
//									| MX31_PIN_SIMPD0(OUTPUTCONFIG_GPIO | INPUTCONFIG_GPIO)
//									| MX31_PIN_CKIH();
//
//// gpio3_1 = VF_EN, sclk0 = GPIO8, srst0 = GPIO9, sven0 = GPIO0
//IOMUX_CTL_REG(SW_MUX_CTL_REGISTER79) = MX31_PIN_GPIO3_1(OUTPUTCONFIG_GPIO | INPUTCONFIG_NONE)
//									| MX31_PIN_SCLK0(OUTPUTCONFIG_GPIO | INPUTCONFIG_GPIO)
//									| MX31_PIN_SRST0(OUTPUTCONFIG_GPIO | INPUTCONFIG_GPIO)
//									| MX31_PIN_SVEN0(OUTPUTCONFIG_GPIO | INPUTCONFIG_GPIO);
//
//// gpio1_4 = UH1_SUSP, gpio1_5 = PWR_RDY, gpio1_6 = UH1_MODE, gpio3_0 = NC
//IOMUX_CTL_REG(SW_MUX_CTL_REGISTER80) = MX31_PIN_GPIO1_4(OUTPUTCONFIG_ALT1 | INPUTCONFIG_NONE)
//									| MX31_PIN_GPIO1_5(OUTPUTCONFIG_GPIO | INPUTCONFIG_FUNC)
//									| MX31_PIN_GPIO1_6(OUTPUTCONFIG_GPIO | INPUTCONFIG_GPIO)
//									| MX31_PIN_GPIO3_0(OUTPUTCONFIG_GPIO | INPUTCONFIG_GPIO);
//
//// gpio1_0 = *PIRQ, gpio1_1 = *E_INT, gpio1_2 = *EXP_INT, gpio1_3 = *I2C_INT
//IOMUX_CTL_REG(SW_MUX_CTL_REGISTER81) = MX31_PIN_GPIO1_0(OUTPUTCONFIG_GPIO | INPUTCONFIG_GPIO)
//									| MX31_PIN_GPIO1_1(OUTPUTCONFIG_FUNC | INPUTCONFIG_GPIO)
//									| MX31_PIN_GPIO1_2(OUTPUTCONFIG_FUNC | INPUTCONFIG_GPIO)
//									| MX31_PIN_GPIO1_3(OUTPUTCONFIG_GPIO | INPUTCONFIG_GPIO);
//
//// capture = GPIO2, compare = GPIO3, watchdog_rst = NC, pwm0 = LCD_BKL
//IOMUX_CTL_REG(SW_MUX_CTL_REGISTER82) = MX31_PIN_CAPTURE(OUTPUTCONFIG_GPIO | INPUTCONFIG_GPIO)
//									| MX31_PIN_COMPARE(OUTPUTCONFIG_GPIO | INPUTCONFIG_GPIO)
//									| MX31_PIN_WATCHDOG_RST(OUTPUTCONFIG_GPIO | INPUTCONFIG_GPIO)
//									| MX31_PIN_PWMO(OUTPUTCONFIG_GPIO | INPUTCONFIG_NONE);
