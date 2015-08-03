/**
 * @file
 *
 * @brief AM335x Register Base Addresses and Offsets
 *
 * @ingroup am335x
 */

/*
 * Copyright (C) 2015 Jarielle Catbagan <jcatbagan93@gmail.com>
 *
 * The license and distribution terms for this file may be
 * found in the file LICENSE in this distribution or at
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 *
 * Description: This file contains register base addresses and offsets as
 * well as access macros for the AM335x on-chip peripherals.  Peripherals
 * not used by Umon have not been test (and may not be defined). Use
 * these defines with caution!
 */

#ifndef AM335X_H
#define AM335X_H

#include "bits.h"


/*===========================================================================*/
/* AM335x device identification and feature enumeration */
/*===========================================================================*/
#define AM335X_DEVICE_IDENTIFICATION	0x44E10600
#define AM335X_DEVICE_FEATURE		0x44E10604
/*===========================================================================*/


/*===========================================================================*/
/* L3 and L4 Interconnects */
/*===========================================================================*/
#define L3F_CFG_REGS_BASE		0x44000000
#define L3S_CFG_REGS_BASE		0x44800000
#define L4_WKUP_BASE			0x44C00000
#define L4_PER_BASE			0x48000000
#define L4_FAST_BASE			0x4A000000
/*===========================================================================*/


/*===========================================================================*/
/* EMIF0 Configuration Registers */
/*===========================================================================*/
#define EMIF0_BASE			0x4C000000
#define EMIF0_REG(_x_)			*(vulong *)(EMIF0_BASE + _x_)
/*---------------------------------------------------------------------------*/
/* EMIFO Register offsets */
#define EMIF_MOD_ID_REV			0x0000
#define STATUS				0x0004
#define SDRAM_CONFIG			0x0008
#define SDRAM_CONFIG_2			0x000C
#define SDRAM_REF_CTRL			0x0010
#define SDRAM_REF_CTRL_SHDW		0x0014
#define SDRAM_TIM_1			0x0018
#define SDRAM_TIM_1_SHDW		0x001C
#define SDRAM_TIM_2			0x0020
#define SDRAM_TIM_2_SHDW		0x0024
#define SDRAM_TIM_3			0x0028
#define SDRAM_TIM_3_SHDW		0x002C
#define PWR_MGMT_CTRL			0x0038
#define PWR_MGMT_CTRL_SHDW		0x003C
#define INTERFACE_CONFIG		0x0054
#define INTERFACE_CONFIG_VAL_1		0x0058
#define INTERFACE_CONFIG_VAL_2		0x005C
#define PERF_CNT_1			0x0080
#define PERF_CNT_2			0x0084
#define PERF_CNT_CFG			0x0088
#define PERF_CNT_SEL			0x008C
#define PERF_CNT_TIM			0x0090
#define READ_IDLE_CTRL			0x0098
#define READ_IDLE_CTRL_SHDW		0x009C
#define IRQSTATUS_RAW_SYS		0x00A4
#define IRQSTATUS_SYS			0x00AC
#define IRQENABLE_SET_SYS		0x00B4
#define IRQENABLE_CLR_SYS		0x00BC
#define ZQ_CONFIG			0x00C8
#define RW_LVL_RAMP_WNDW		0x00D4
#define RW_LVL_RAMP_CTRL		0x00D8
#define RW_LVL_CTRL			0x00DC
#define DDR_PHY_CTRL_1			0x00E4
#define DDR_PHY_CTRL_1_SHDW		0x00E8
#define PRIORITY_TO_CLASS_SRVC_MAP	0x0100
#define CONN_ID_TO_CLASS_SRVC_1_MAP	0x0104
#define CONN_ID_TO_CLASS_SRVC_2_MAP	0x0108
#define RW_EXEC_THRESHOLD		0x0120
/* Register fields and values */
#define SDRAM_CONFIG_REG_SDRAM_TYPE		0xE0000000
#define SDRAM_CONFIG_REG_SDRAM_TYPE_DDR1	0x00000000
#define SDRAM_CONFIG_REG_SDRAM_TYPE_LPDDR1	0x20000000
#define SDRAM_CONFIG_REG_SDRAM_TYPE_DDR2	0x40000000
#define SDRAM_CONFIG_REG_SDRAM_TYPE_DDR3	0x60000000
#define SDRAM_CONFIG_REG_IBANK_POS		0x18000000
#define SDRAM_CONFIG_REG_IBANK_POS_0		0x00000000
#define SDRAM_CONFIG_REG_IBANK_POS_1		0x08000000
#define SDRAM_CONFIG_REG_IBANK_POS_2		0x10000000
#define SDRAM_CONFIG_REG_IBANK_POS_3		0x18000000
#define SDRAM_CONFIG_REG_DDR_TERM		0x07000000
#define SDRAM_CONFIG_REG_DDR_TERM_DISABLE	0x00000000
#define SDRAM_CONFIG_REG_DDR_TERM_DDR2_75OHM	0x01000000
#define SDRAM_CONFIG_REG_DDR_TERM_DDR2_150OHM	0x02000000
#define SDRAM_CONFIG_REG_DDR_TERM_DDR2_50OHM	0x03000000
#define SDRAM_CONFIG_REG_DDR_TERM_DDR3_RZQ_4	0x01000000
#define SDRAM_CONFIG_REG_DDR_TERM_DDR3_RZQ_2	0x02000000
#define SDRAM_CONFIG_REG_DDR_TERM_DDR3_RZQ_6	0x03000000
#define SDRAM_CONFIG_REG_DDR_TERM_DDR3_RZQ_12	0x04000000
#define SDRAM_CONFIG_REG_DDR_TERM_DDR3_RZQ_8	0x05000000
#define SDRAM_CONFIG_REG_DDR2_DDQS		0x00800000
#define SDRAM_CONFIG_REG_DDR2_DDQS_SINGLE_DQS	0x00000000
#define SDRAM_CONFIG_REG_DDR2_DDQS_DIFF_DQS	0x00800000
#define SDRAM_CONFIG_REG_DYN_ODT		0x00600000
#define SDRAM_CONFIG_REG_DYN_ODT_DISABLE	0x00000000
#define SDRAM_CONFIG_REG_DYN_ODT_RZQ_4		0x00200000
#define SDRAM_CONFIG_REG_DYN_ODT_RZQ_2		0x00400000
#define SDRAM_CONFIG_REG_DDR_DISABLE_DLL	0x00100000
#define SDRAM_CONFIG_REG_DDR_DISABLE_DLL_ENABLE		0x00000000
#define SDRAM_CONFIG_REG_DDR_DISABLE_DLL_DISABLE	0x00100000
#define SDRAM_CONFIG_REG_SDRAM_DRIVE		0x000C0000
#define SDRAM_CONFIG_REG_SDRAM_DRIVE_RZQ_6	0x00000000
#define SDRAM_CONFIG_REG_SDRAM_DRIVE_RZQ_7	0x00040000
#define SDRAM_CONFIG_REG_CAS_WR_LATENCY		0x00030000
#define SDRAM_CONFIG_REG_CAS_WR_LATENCY_5	0x00000000
#define SDRAM_CONFIG_REG_CAS_WR_LATENCY_6	0x00010000
#define SDRAM_CONFIG_REG_CAS_WR_LATENCY_7	0x00020000
#define SDRAM_CONFIG_REG_CAS_WR_LATENCY_8	0x00030000
#define SDRAM_CONFIG_REG_NARROW_MODE		0x0000C000
#define SDRAM_CONFIG_REG_NARROW_MODE_32BIT	0x00000000
#define SDRAM_CONFIG_REG_NARROW_MODE_16BIT	0x00004000
#define SDRAM_CONFIG_REG_CAS_LATENCY		0x00003C00
#define SDRAM_CONFIG_REG_CAS_LATENCY_5		0x00000800
#define SDRAM_CONFIG_REG_CAS_LATENCY_6		0x00001000
#define SDRAM_CONFIG_REG_CAS_LATENCY_7		0x00001800
#define SDRAM_CONFIG_REG_CAS_LATENCY_8		0x00002000
#define SDRAM_CONFIG_REG_CAS_LATENCY_9		0x00002800
#define SDRAM_CONFIG_REG_CAS_LATENCY_10		0x00003000
#define SDRAM_CONFIG_REG_CAS_LATENCY_11		0x00003800
#define SDRAM_CONFIG_REG_ROWSIZE		0x00000380
#define SDRAM_CONFIG_REG_ROWSIZE_9BIT		0x00000000
#define SDRAM_CONFIG_REG_ROWSIZE_10BIT		0x00000080
#define SDRAM_CONFIG_REG_ROWSIZE_11BIT		0x00000100
#define SDRAM_CONFIG_REG_ROWSIZE_12BIT		0x00000180
#define SDRAM_CONFIG_REG_ROWSIZE_13BIT		0x00000200
#define SDRAM_CONFIG_REG_ROWSIZE_14BIT		0x00000280
#define SDRAM_CONFIG_REG_ROWSIZE_15BIT		0x00000300
#define SDRAM_CONFIG_REG_ROWSIZE_16BIT		0x00000380
#define SDRAM_CONFIG_REG_IBANK			0x00000070
#define SDRAM_CONFIG_REG_IBANK_1		0x00000000
#define SDRAM_CONFIG_REG_IBANK_2		0x00000010
#define SDRAM_CONFIG_REG_IBANK_4		0x00000020
#define SDRAM_CONFIG_REG_IBANK_8		0x00000030
#define SDRAM_CONFIG_REG_EBANK			0x00000008
#define SDRAM_CONFIG_REG_EBANK_1		0x00000000
#define SDRAM_CONFIG_REG_PAGESIZE		0x00000007
#define SDRAM_CONFIG_REG_PAGESIZE_256_WORD	0x00000000
#define SDRAM_CONFIG_REG_PAGESIZE_512_WORD	0x00000001
#define SDRAM_CONFIG_REG_PAGESIZE_1024_WORD	0x00000002
#define SDRAM_CONFIG_REG_PAGESIZE_2048_WORD	0x00000003
#define SDRAM_CONFIG_2_REG_EBANK_POS		0x08000000
#define SDRAM_CONFIG_2_REG_EBANK_POS_0		0x00000000
#define SDRAM_CONFIG_2_REG_EBANK_POS_1		0x08000000
/*===========================================================================*/


/*===========================================================================*/
/* McASPx Data Registers */
/*===========================================================================*/
#define MCASP0_DATA_REGS_BASE		0x46000000
#define MCASP1_DATE_REGS_BASE		0x46400000
/*===========================================================================*/


/*===========================================================================*/
/* L4_WKUP Peripherals */
/*===========================================================================*/
/* Clock Module Peripheral Registers */
#define CM_PER_BASE			(L4_WKUP_BASE + 0x00200000)
#define CM_PER_REG(_x_)			*(vulong *)(CM_PER_BASE + _x_)
/* Clock Module Wakeup Registers */
#define CM_WKUP_BASE			(L4_WKUP_BASE + 0x00200400)
#define CM_WKUP_REG(_x_)		*(vulong *)(CM_WKUP_BASE + _x_)
/* Clock Module PLL Registers */
#define CM_DPLL_BASE			(L4_WKUP_BASE + 0x00200500)
#define CM_DPLL_REG(_x_)		*(vulong *)(CM_DPLL_BASE + _x_)
/* Clock Module MPU Registers */
#define CM_MPU_BASE			(L4_WKUP_BASE + 0x00200600)
#define CM_MPU_REG(_x_)			*(vulong *)(CM_MPU_BASE + _x_)
/* Clock Module Device Registers */
#define CM_DEVICE_BASE			(L4_WKUP_BASE + 0x00200700)
#define CM_DEVICE_REG(_x_)		*(vulong *)(CM_DEVICE_BASE + _x_)
/* Clock Module RTC Registers */
#define CM_RTC_BASE			(L4_WKUP_BASE + 0x00200800)
#define CM_RTC_REG(_x_)			*(vulong *)(CM_RTC_BASE + _x_)
/* Clock Module Graphics Controller Registers */
#define CM_GFX_BASE			(L4_WKUP_BASE + 0x00200900)
#define CM_GFX_REG(_x_)			*(vulong *)(CM_GFX_BASE + _x_)
/* Clock Module Efuse Registers */
#define CM_CEFUSE_BASE			(L4_WKUP_BASE + 0x00200A00)
#define CM_CEFUSE_REG(_x_)		*(vulong *)(CM_CEFUSE_BASE + _x_)
/* Power Reset Module Interrupt Registers */
#define PRM_IRQ_BASE			(L4_WKUP_BASE + 0x00200B00)
#define PRM_IRQ_REG(_x_)		*(vulong *)(PRM_IRQ_BASE + _x_)
/* Power Reset Module Peripheral Registers */
#define PRM_PER_BASE			(L4_WKUP_BASE + 0x00200C00)
#define PRM_PER_REG(_x_)		*(vulong *)(PRM_PER_BASE + _x_)
/* Power Reset Module Wakeup Registers */
#define PRM_WKUP_BASE			(L4_WKUP_BASE + 0x00200D00)
#define PRM_WKUP_REG(_x_)		*(vulong *)(PRM_WKUP_BASE + _x_)
/* Power Reset Module MPU Registers */
#define PRM_MPU_BASE			(L4_WKUP_BASE + 0x00200E00)
#define PRM_MPU_REG(_x_)		*(vulong *)(PRM_MPU_BASE + _x_)
/* Power Reset Module Device Registers */
#define PRM_DEV_BASE			(L4_WKUP_BASE + 0x00200F00)
#define PRM_DEV_REG(_x_)		*(vulong *)(PRM_DEV_BASE + _x_)
/* Power Reset Module RTC Registers */
#define PRM_RTC_BASE			(L4_WKUP_BASE + 0x00201000)
#define PRM_RTC_REG(_x_)		*(vulong *)(PRM_RTC_BASE + _x_)
/* Power Reset Module Graphics Controller Registers */
#define PRM_GFX_BASE			(L4_WKUP_BASE + 0x00201100)
#define PRM_GFX_REG(_x_)		*(vulong *)(PRM_GFX_BASE + _x_)
/* Power Reset Module Efuse Registers */
#define PRM_CEFUSE_BASE			(L4_WKUP_BASE + 0x00201200)
#define PRM_CEFUSE_REG(_x_)		*(vulong *)(PRM_CEFUSE_BASE + _x_)
/* DMTimer0 Registers */
#define DMTIMER0_BASE			(L4_WKUP_BASE + 0x00205000)
#define DMTIMER0_REG(_x_)		*(vulong *)(DMTIMER0_BASE + _x_)
/* UART0 Registers */
#define UART0_BASE			(L4_WKUP_BASE + 0x00209000)
#define UART0_REG(_x_)			*(vulong *)(UART0_BASE + _x_)
/* I2C0 Registers */
#define I2C0_BASE			(L4_WKUP_BASE + 0x0020B000)
#define I2C0_REG(_x_)			*(vulong *)(I2C0_BASE + _x_)
/* ADC_TSC Registers */
#define ADC_TSC_BASE			(L4_WKUP_BASE + 0x0020D000)
#define ADC_TSC_REG(_x_)		*(vulong *)(ADC_TSC_BASE + _x_)
/* Control Module */
#define CNTL_MODULE_BASE		(L4_WKUP_BASE + 0x00210000)
#define CNTL_MODULE_REG(_x_)		*(vulong *)(CNTL_MODULE_BASE + _x_)
/* DDR2/3/mDDR PHY Registers */
#define DDR_PHY_BASE			(L4_WKUP_BASE + 0x00212000)
#define DDR_PHY_REG(_x_)		*(vulong *)(DDR_PHY_BASE + _x_)
/* DMTimer1 1ms Registers */
#define DMTIMER1_1MS_BASE		(L4_WKUP_BASE + 0x00231000)
#define DMTIMER1_1MS_REG(_x_)		*(vulong *)(DMTIMER1_1MS_BASE + _x_)
/* L3 Registers */
#define SMARTREFLEX0_BASE		(L4_WKUP_BASE + 0x00237000)
#define SMARTREFLEX0_REG(_x_)		*(vulong *)(SMARTREFLEX0_BASE + _x_)
#define SMARTREFLEX1_BASE		(L4_WKUP_BASE + 0x00239000)
#define SMARTREFLEX1_REG(_x_)		*(vulong *)(SMARTREFLEX1_BASE + _x_)
/* RTC Registers */
#define RTCSS_BASE			(L4_WKUP_BASE + 0x0023E000)
#define RTCSS_REG(_x_)			*(vulong *)(RTCSS_BASE + _x_)
/* Debug Registers */
#define DEBUGSS_HWMSTR1_BASE		(L4_WKUP_BASE + 0x00240000)
#define DEBUGSS_HWMSTR1_REG(_x_)	*(vulong *)(DEBUGSS_HWMSTR1_BASE + _x_)
/*===========================================================================*/


/*===========================================================================*/
/* CM_PER Registers */
/*===========================================================================*/
#define CM_PER_L3_CLKSTCTRL		0x0C
#define CM_PER_EMIF_CLKCTRL		0x28
#define CM_PER_MMC0_CLKCTRL		0x3C
#define CM_PER_MMC0_CLKCTRL_MODULEMODE_ENABLE	0x02
#define CM_PER_MMC0_CLKCTRL_IDLEST	0x00030000
#define CM_PER_GPIO1_CLKCTRL		0xAC
/*===========================================================================*/


/*===========================================================================*/
/* CM_WKUP Registers */
/*===========================================================================*/
#define CM_WKUP_CLKSTCTRL               0x00
#define CM_WKUP_CONTROL_CLKCTRL         0x04
#define CM_WKUP_GPIO0_CLKCTRL           0x08
#define CM_WKUP_L4WKUP_CLKCTRL          0x0C
#define CM_WKUP_TIMER0_CLKCTRL          0x10
#define CM_WKUP_DEBUGSS_CLKCTRL         0x14
#define CM_L3_AON_CLKSTCTRL             0x18
#define CM_AUTOIDLE_DPLL_MPU            0x1C
#define CM_IDLEST_DPLL_MPU              0x20
#define CM_SSC_DELTAMSTEP_DPLL_MPU      0x24
#define CM_SSC_MODFREQDIV_DPLL_MPU      0x28
#define CM_CLKSEL_DPLL_MPU              0x2C
#define CM_AUTOIDLE_DPLL_DDR            0x30
#define CM_IDLEST_DPLL_DDR              0x34
#define CM_SSC_DELTAMSTEP_DPLL_DDR      0x38
#define CM_SSC_MODFREQDIV_DPLL_DDR      0x3C
#define CM_CLKSEL_DPLL_DDR              0x40
#define CM_AUTOIDLE_DPLL_DISP           0x44
#define CM_IDLEST_DPLL_DISP             0x48
#define CM_SSC_DELTAMSTEP_DPLL_DISP     0x4C
#define CM_SSC_MODFREQDIV_DPLL_DISP     0x50
#define CM_CLKSEL_DPLL_DISP             0x54
#define CM_AUTOIDLE_DPLL_CORE           0x58
#define CM_IDLEST_DPLL_CORE             0x5C
#define CM_SSC_DELTAMSTEP_DPLL_CORE     0x60
#define CM_SSC_MODFREQDIV_DPLL_CORE     0x64
#define CM_CLKSEL_DPLL_CORE             0x68
#define CM_AUTOIDLE_DPLL_PER            0x6C
#define CM_IDLEST_DPLL_PER              0x70
#define CM_SSC_DELTAMSTEP_DPLL_PER      0x74
#define CM_SSC_MODFREQDIV_DPLL_PER      0x78
#define CM_CLKDCOLDO_DPLL_PER           0x7C
#define CM_DIV_M4_DPLL_CORE             0x80
#define CM_DIV_M5_DPLL_CORE             0x84
#define CM_CLKMODE_DPLL_MPU             0x88
#define CM_CLKMODE_DPLL_PER             0x8C
#define CM_CLKMODE_DPLL_CORE            0x90
#define CM_CLKMODE_DPLL_DDR             0x94
#define CM_CLKMODE_DPLL_DISP            0x98
#define CM_CLKSEL_DPLL_PER           	0x9C
#define CM_DIV_M2_DPLL_DDR              0xA0
#define CM_DIV_M2_DPLL_DISP             0xA4
#define CM_DIV_M2_DPLL_MPU              0xA8
#define CM_DIV_M2_DPLL_PER              0xAC
#define CM_WKUP_WKUP_M3_CLKCTRL         0xB0
#define CM_WKUP_UART0_CLKCTRL           0xB4
#define CM_WKUP_I2C0_CLKCTRL            0xB8
#define CM_WKUP_ADC_TSC_CLKCTRL         0xBC
#define CM_WKUP_SMARTREFLEX0_CLKCT      0xC0
#define CM_WKUP_TIMER1_CLKCTRL          0xC4
#define CM_WKUP_SMARTREFLEX1_CLKCT      0xC8
#define CM_L4_WKUP_AON_CLKSTCTRL        0xCC
#define CM_WKUP_WDT1_CLKCTRL            0xD4
#define CM_DIV_M6_DPLL_CORE             0xD8
/*===========================================================================*/


#define CM_CLKSEL_DPLL_PER_DPLL_MULT	(960 << 8)
#define CM_CLKSEL_DPLL_PER_DPLL_DIV	(23)
#define CM_CLKSEL_DPLL_PER_DPLL_SD_DIV	(4 << 24)


/*===========================================================================*/
/* Control Module Registers */
/*===========================================================================*/
#define CONTROL_STATUS			0x0040
#define CONTROL_EMIF_SDRAM_CONFIG	0x0110
#define CONF_GPMC_A5                    0x0854
#define CONF_GPMC_A6                    0x0858
#define CONF_GPMC_A7                    0x085c
#define CONF_GPMC_A8                    0x0860
#define CONF_MMC0_DAT3			0x08F0
#define CONF_MMC0_DAT2			0x08F4
#define CONF_MMC0_DAT1			0x08F8
#define CONF_MMC0_DAT0			0x08FC
#define CONF_MMC0_CLK			0x0900
#define CONF_MMC0_CMD			0x0904
#define CONF_SPI0_CS1			0x0960
#define CONF_UART0_RXD                  0x0970
#define CONF_UART0_TXD                  0x0974
#define DDR_IO_CTRL			0x0E04
#define VTP_CTRL			0x0E0C
#define VREF_CTRL			0x0E14
#define DDR_CKE_CTRL			0x131C
#define DDR_CMD0_IOCTRL			0x1404
#define DDR_CMD1_IOCTRL			0x1408
#define DDR_CMD2_IOCTRL			0x140C
#define DDR_DATA0_IOCTRL		0x1440
#define DDR_DATA1_IOCTRL		0x1444
/*===========================================================================*/


/*===========================================================================*/
/* DDR2/3/mDDR PHY Registers */
/*===========================================================================*/
#define CMD0_REG_PHY_CTRL_SLAVE_RATIO_0		0x001C
#define CMD0_REG_PHY_DLL_LOCK_DIFF_0		0x0028
#define CMD0_REG_PHY_INVERT_CLKOUT_0		0x002C
#define CMD1_REG_PHY_CTRL_SLAVE_RATIO_0		0x0050
#define CMD1_REG_PHY_DLL_LOCK_DIFF_0		0x005C
#define CMD1_REG_PHY_INVERT_CLKOUT_0		0x0060
#define CMD2_REG_PHY_CTRL_SLAVE_RATIO_0		0x0084
#define CMD2_REG_PHY_DLL_LOCK_DIFF_0		0x0090
#define CMD2_REG_PHY_INVERT_CLKOUT_0		0x0094
#define DATA0_REG_PHY_RD_DQS_SLAVE_RATIO_0	0x00C8
#define DATA0_REG_PHY_WR_DQS_SLAVE_RATIO_0	0x00DC
#define DATA0_REG_PHY_WRLVL_INIT_RATIO_0	0x00F0
#define DATA0_REG_PHY_WRLVL_INIT_MODE_0		0x00F8
#define DATA0_REG_PHY_GATELVL_INIT_RATIO_0	0x00FC
#define DATA0_REG_PHY_GATELVL_INIT_MODE_0	0x0104
#define DATA0_REG_PHY_FIFO_WE_SLAVE_RATIO_0	0x0108
#define DATA0_REG_PHY_DQ_OFFSET_0		0x011C
#define DATA0_REG_PHY_WR_DATA_SLAVE_RATIO_0	0x0120
#define DATA0_REG_PHY_USE_RANK0_DELAYS		0x0134
#define DATA0_REG_PHY_LOCK_DIFF_0		0x0138
#define DATA1_REG_PHY_RD_DQS_SLAVE_RATIO_0	0x016C
#define DATA1_REG_PHY_WR_DQS_SLAVE_RATIO_0	0x0180
#define DATA1_REG_PHY_WRLVL_INIT_RATIO_0	0x0194
#define DATA1_REG_PHY_WRLVL_INIT_MODE_0		0x019C
#define DATA1_REG_PHY_GATELVL_INIT_RATIO_0	0x01A0
#define DATA1_REG_PHY_GATELVL_INIT_MODE_0	0x01A8
#define DATA1_REG_PHY_FIFO_WE_SLAVE_RATIO_0	0x01AC
#define DATA1_REG_PHY_DQ_OFFSET_0		0x01C0
#define DATA1_REG_PHY_WR_DATA_SLAVE_RATIO_0	0x01C4
#define DATA1_REG_PHY_USE_RANK0_DELAYS		0x01D8
#define DATA1_REG_PHY_LOCK_DIFF_0		0x01DC
/*===========================================================================*/


/*===========================================================================*/
/* Pad control bits: */
/*===========================================================================*/
#define SLEWSLOW                        (1 << 6)
#define SLEWFAST                        0
#define RX_ON                           (1 << 5)
#define RX_OFF                          0
#define PULLUP                          (1 << 4)
#define PULLDOWN                        0
#define PULL_OFF                        (1 << 3)
#define PULL_ON                         0
#define MUXMODE_0                       (0 & 7)
#define MUXMODE_1                       (1 & 7)
#define MUXMODE_2                       (2 & 7)
#define MUXMODE_3                       (3 & 7)
#define MUXMODE_4                       (4 & 7)
#define MUXMODE_5                       (5 & 7)
#define MUXMODE_6                       (6 & 7)
#define MUXMODE_7                       (7 & 7)
/*===========================================================================*/


/*===========================================================================*/
/* L4_PER Peripherals */
/*===========================================================================*/
/* UART1 Registers */
#define UART1_BASE			(L4_PER_BASE + 0x00022000)
#define UART1_REG(_x_)			*(vulong *)(UART1_BASE + _x_)
/* UART2 Registers */
#define UART2_BASE			(L4_PER_BASE + 0x00024000)
#define UART2_REG(_x_)			*(vulong *)(UART2_BASE + _x_)
/* I2C1 Registers */
#define I2C1_BASE			(L4_PER_BASE + 0x0002A000)
#define I2C1_REG(_x_)			*(vulong *)(I2C1_BASE + _x_)
/* McSPI0 Registers */
#define MCSPI0_BASE			(L4_PER_BASE + 0x00030000)
#define MCSPI0_REG(_x_)			*(vulong *)(MCSPI0_BASE + _x_)
/* McASP0 CFG Registers */
#define MCASP0_CFG_BASE			(L4_PER_BASE + 0x00038000)
#define MCASP0_CFG_REG(_x_)		*(vulong *)(MCASP0_CFG_BASE + _x_)
/* McASP1 CFG Registers */
#define MCASP1_CFG_BASE			(L4_PER_BASE + 0x0003C000)
#define MCASP1_CFG_REG(_x_)		*(vulong *)(MCASP1_CFG_BASE + _x_)
/* DMTimer2 Registers */
#define DMTIMER2_BASE			(L4_PER_BASE + 0x00040000)
#define DMTIMER2_REG(_x_)		*(vulong *)(DMTIMER2_BASE + _x_)
/* DMTimer3 Registers */
#define DMTIMER3_BASE			(L4_PER_BASE + 0x00042000)
#define DMTIMER3_REG(_x_)		*(vulong *)(DMTIMER3_BASE + _x_)
/* DMTimer4 Registers */
#define DMTIMER4_BASE			(L4_PER_BASE + 0x00044000)
#define DMTIMER4_REG(_x_)		*(vulong *)(DMTIMER4_BASE + _x_)
/* DMTimer5 Registers */
#define DMTIMER5_BASE			(L4_PER_BASE + 0x00046000)
#define DMTIMER5_REG(_x_)		*(vulong *)(DMTIMER5_BASE + _x_)
/* DMTimer6 Registers */
#define DMTIMER6_BASE			(L4_PER_BASE + 0x00048000)
#define DMTIMER6_REG(_x_)		*(vulong *)(DMTIMER6_BASE + _x_)
/* DMTimer7 Registers */
#define DMTIMER7_BASE			(L4_PER_BASE + 0x0004A000)
#define DMTIMER7_REG(_x_)		*(vulong *)(DMTIMER7_BASE + _x_)
/* ELM Registers */
#define ELM_BASE			(L4_PER_BASE + 0x00080000)
#define ELM_REG(_x_)			*(vulong *)(ELM_BASE + _x_)
/* Mailbox 0 Registers */
#define MAILBOX0_BASE			(L4_PER_BASE + 0x000C8000)
#define MAILBOX0_REG(_x_)		*(vulong *)(MAILBOX0_BASE + _x_)
/* Spinlock Registers */
#define SPINLOCK_BASE			(L4_PER_BASE + 0x000CA000)
#define SPINLOCK_REG(_x_)		*(vulong *)(SPINLOCK_BASE + _x_)
/* OCP Watchpoint Registers */
#define OCP_WATCHPOINT_BASE		(L4_PER_BASE + 0x0018C000)
#define OCP_WATCHPOINT_REG(_x_)		*(vulong *)(OCP_WATCHPOINT_BASE + _x_)
/* I2C2 Registers */
#define I2C2_BASE			(L4_PER_BASE + 0x0019C000)
#define I2C2_REG(_x_)			*(vulong *)(I2C2_BASE + _x_)
/* McSPI1 Registers */
#define MCSPI1_BASE			(L4_PER_BASE + 0x001A0000)
#define MCSPI1_REG(_x_)			*(vulong *)(MSCPI1_BASE + _x_)
/* UART3 Registers */
#define UART3_BASE			(L4_PER_BASE + 0x001A6000)
#define UART3_REG(_x_)			*(vulong *)(UART3_BASE + _x_)
/* UART4 Registers */
#define UART4_BASE			(L4_PER_BASE + 0x001A8000)
#define UART4_REG(_x_)			*(vulong *)(UART4_BASE + _x_)
/* UART5 Registers */
#define UART5_BASE			(L4_PER_BASE + 0x001AA000)
#define UART5_REG(_x_)			*(vulong *)(UART5_BASE + _x_)
/* DCAN0 Registers */
#define DCAN0_BASE			(L4_PER_BASE + 0x001CC000)
#define DCAN0_REG(_x_)			*(vulong *)(DCAN0_BASE + _x_)
/* DCAN1 Registers */
#define DCAN1_BASE			(L4_PER_BASE + 0x001D0000)
#define DCAN1_REG(_x_)			*(vulong *)(DCAN1_BASE + _x_)
/* Interrupt Controller Registers */
#define INTCPS_BASE			(L4_PER_BASE + 0x00200000)
#define INTCPS_REG(_x_)			*(vulong *)(INTCPS_BASE + _x_)
/* MPUSS Config Register */
#define MPUSS_BASE			(L4_PER_BASE + 0x00240000)
#define MPUSS_REG(_x_)			*(vulong *)(MPUSS_BASE + _x_)
/* PWMSS0 Configuration Registers */
#define PWMSS0_BASE			(L4_PER_BASE + 0x00300000)
#define PWMSS0_REG(_x_)			*(vulong *)(PWMSS0_BASE + _x_)
/* PWMSS eCAP0 Registers */
#define ECAP0_BASE			(L4_PER_BASE + 0x00300100)
#define ECAP0_REG(_x_)			*(vulong *)(ECAP0_BASE + _x_)
/* PWMSS eQEP0 Registers */
#define EQEP0_BASE			(L4_PER_BASE + 0x00300180)
#define EQEP0_REG(_x_)			*(vulong *)(EQEP0_BASE + _x_)
/* PWMSS ePWM0 Registers */
#define EPWM0_BASE			(L4_PER_BASE + 0x00300200)
#define EPWM0_REG(_x_)			*(vulong *)(EPWM0_BASE + _x_)
/* PWMSS1 Configuration Registers */
#define PWMSS1_BASE			(L4_PER_BASE + 0x00302000)
#define PWMSS1_REG(_x_)			*(vulong *)(PWMSS1_BASE + _x_)
/* PWMSS eCAP1 Registers */
#define ECAP1_BASE			(L4_PER_BASE + 0x00302100)
#define ECAP1_REG(_x_)			*(vulong *)(ECAP1_BASE + _x_)
/* PWMSS eQEP1 Registers */
#define EQEP1_BASE			(L4_PER_BASE + 0x00302180)
#define EQEP1_REG(_x_)			*(vulong *)(EQEP1_BASE + _x_)
/* PWMSS ePWM1 Registers */
#define EPWM1_BASE			(L4_PER_BASE + 0x00302200)
#define EPWM1_REG(_x_)			*(vulong *)(EPwM_BASE + _x_)
/* PWMSS2 Configuration Registers */
#define PWMSS2_BASE			(L4_PER_BASE + 0x00304000)
#define PWMSS2_REG(_x_)			*(vulong *)(PWMSS2_BASE + _x_)
/* PWMSS eCAP2 Registers */
#define ECAP2_BASE			(L4_PER_BASE + 0x00304100)
#define ECAP2_REG(_x_)			*(vulong *)(ECAP2_BASE + _x_)
/* PWMSS eQEP2 Registers */
#define EQEP2_BASE			(L4_PER_BASE + 0x00304180)
#define EQEP2_REG(_x_)			*(vulong *)(EQEP2_BASE + _x_)
/* PWMSS ePWM2 Registers */
#define EPWM2_BASE			(L4_PER_BASE + 0x00304200)
#define EPWM2_REG(_x_)			*(vulong *)(EPWM2_BASE + _x_)
/* LCD Contoller Registers */
#define LCD_CNTLR_BASE			(L4_PER_BASE + 0x0030E000)
#define LCD_CNTLR_REG(_x_)		*(vulong *)(LCD_CNTLR_BASE + _x_)
/*===========================================================================*/


/*===========================================================================*/
/* L4_FAST Peripherals */
/*===========================================================================*/
#define L4_FAST_BASE			0x4A000000
/* Ethernet Switch Subsystem Registers */
#define CPSW_SS_BASE			(LF_FAST_BASE + 0x00100000)
#define CPSW_SS_REG(_x_)		*(vulong *)(CPSW_SS_BASE + _x_)
/* Ethernet Switch Port Control Registers */
#define CPSW_PORT_BASE			(LF_FAST_BASE + 0x00100100)
#define CPSW_PORT_REG(_x_)		*(vulong *)(CPSW_PORT_BASE + _x_)
/* CPPI DMA Controller Module Registers */
#define CPSW_CPDMA_BASE			(LF_FAST_BASE + 0x00100800)
#define CPSW_CPDMA_REG(_x_)		*(vulong *)(CPSW_CPDMA_BASE + _x_)
/* Ethernet Statistics Registers */
#define CPSW_STATS_BASE			(LF_FAST_BASE + 0x00100900)
#define CPSW_STATS_REG(_x_)		*(vulong *)(CPSW_STATS_BASE + _x_)
/* CPPI DMA State RAM Registers */
#define CPSW_STATERAM_BASE		(LF_FAST_BASE + 0x00100A00)
#define CPSW_STATERAM_REG(_x_)		*(vulong *)(CPSW_STATERAM_BASE + _x_)
/* Ethenet Time Sync Module Registers */
#define CPSW_CPTS_BASE			(LF_FAST_BASE + 0x00100C00)
#define CPSW_CPTS_REG(_x_)		*(vulong *)(CPSW_CPTS_BASE + _x_)
/* Ethernet Address Lookup Engine Registers */
#define CPSW_ALE_BASE			(LF_FAST_BASE + 0x00100D00)
#define CPSW_ALE_REG(_x_)		*(vulong *)(CPSW_ALE_BASE + _x_)
/* Ethernet Silver for Port 1 Registers */
#define CPSW_SL1_BASE			(LF_FAST_BASE + 0x00100D80)
#define CPSW_SL1_REG(_x_)		*(vulong *)(CPSW_SL1_BASE + _x_)
/* Ethernet Silver for Port 2 Registers */
#define CPSW_SL2_BASE			(LF_FAST_BASE + 0x00100DC0)
#define CPSW_SL2_REG(_x_)		*(vulong *)(CPSW_SL2_BASE + _x_)
/* Ethernet MDIO Controller Registers */
#define MDIO_BASE			(LF_FAST_BASE + 0x00101000)
#define MDIO_REG(_x_)			*(vulong *)(MDIO_BASE + _x_)
/* Ethernet Subsystem Wrapper for RMII/RGMII Registers */
#define CPSW_WR_BASE			(LF_FAST_BASE + 0x00101200)
#define CPSW_WR_REG(_x_)		*(vulong *)(CPSW_WR_BASE + _x_)
/*===========================================================================*/


/*===========================================================================*/
/* UART Register offsets */
/*===========================================================================*/
	/* Transmit Holding Register (write only) */
#define UART_THR		0x00
	/* Receive Holding Register (read only) */
#define UART_RHR		0x00
	/* Baud divisor lower byte (read/write) */
#define UART_DLL      		0x00
	/* Interrupt Enable Register (read/write) */
#define UART_IER		0x04
	/* Baud divisor higher byte (read/write) */
#define UART_DLH      		0x04
	/* Enhanced Feature Register */
#define UART_EFR		0x08
	/* Interrupt Identification Register (read only) */
#define UART_IIR		0x08
	/* FIFO Control Register (write only) */
#define UART_FCR		0x08
	/* Line Control Register (read/write) */
#define UART_LCR		0x0C
	/* Modem Control Register (read/write) */
#define UART_MCR		0x10
	/* XON1/ADDR1 Register */
#define UART_XON1_ADDR1		0x10
	/* XON2/ADDR2 Register */
#define UART_XON2_ADDR2		0x14
	/* Line Status Register (read only) */
#define UART_LSR		0x14
	/* Transmission Control Register */
#define UART_TCR		0x18
	/* Modem Status Register (read only) */
#define UART_MSR		0x18
	/* XOFF1 Register */
#define UART_XOFF1		0x18
	/* Scratch Pad Register (read/write) */
#define UART_SPR		0x1C
	/* Trigger Level Register */
#define UART_TLR		0x1C
	/* XOFF2 Register */
#define UART_XOFF2		0x1C
	/* Mode Definition Register 1 */
#define UART_MDR1		0x20
	/* Mode Definition Register 2 */
#define UART_MDR2		0x24
	/* Transmit Frame Length Register Low (IrDA modes only) */
#define UART_TXFLL		0x28
	/* Status FIFO Line Status Register (IrDA modes only) */
#define UART_SFLSR		0x28
	/* Resume Register (IR-IrDA and IR-CIR modes only) */
#define UART_RESUME		0x2C
	/* Transmit Frame Length Register High (IrDA modes only) */
#define UART_TXFLH		0x2C
	/* Receive Frame Length Register Low (IrDA modes only) */
#define UART_RXFLL		0x30
	/* Status FIFO Register Low (IrDA modes only) */
#define UART_SFREGL		0x30
	/* Status FIFO Register High (IrDA modes only) */
#define UART_SFREGH		0x34
	/* Receive Frame Length Register High (IrDA modes only) */
#define UART_RXFLH		0x34
	/* BOF Control Register (IrDA modes only_ */
#define UART_BLR		0x38
	/* UART Autobauding Status Register (UART autobauding
	   mode only */
#define UART_UASR		0x38
	/* Auxiliary Control Register (IrDA-CIR modes only) */
#define UART_ACREG		0x3C
	/* Supplementary Control Register */
#define UART_SCR		0x40
	/* Supplementary Status Register */
#define UART_SSR		0x44
	/* BOF Length Register (IR-IrDA and IR-CIR modes only) */
#define UART_EBLR		0x48
	/* Module Version Register */
#define UART_MVR		0x50
	/* System Configuration Register */
#define UART_SYSC		0x54
	/* System Status Register */
#define UART_SYSS		0x58
	/* Wake-up Enable Register */
#define UART_WER		0x5C
	/* Carrier Frequency Prescalar Register */
#define UART_CFPS		0x60
	/* Received FIFO Level Register */
#define UART_RXFIFO_LVL		0x64
	/* Transmit FIFO Level Register */
#define UART_TXFIFO_LVL		0x68
	/* IER2 Register */
#define UART_IER2		0x6C
	/* ISR2 Register */
#define UART_ISR2		0x70
	/* FREQ_SEL Register */
#define UART_FREQ_SEL		0x74
	/* Mode Definition Register 3 */
#define UART_MDR3		0x80
	/* TX DMA Threshold Register */
#define UART_TX_DMA_THRESHOLD	0x84
/*===========================================================================*/


/*===========================================================================*/
/* USB */
/*===========================================================================*/
#define USB_SUBSYS_BASE_ADDR		0x47400000
/*---------------------------------------------------------------------------*/
/* USB Subsystem Registers offset */
#define USBSS_BASE_REG_OFFSET		0x0000
#define USBSS_REVREG			0x0000
#define USBSS_SYSCONFIG			0x0010
#define USBSS_IRQSTATRAW		0x0024
#define USBSS_IRQSTAT			0x0028
#define USBSS_IRQENABLER		0x002C
#define USBSS_IRQCLEARR			0x0030
#define USBSS_IRQDMATHOLDTX00		0x0100
#define USBSS_IRQDMATHOLDTX01		0x0104
#define USBSS_IRQDMATHOLDTX02		0x0108
#define USBSS_IRQDMATHOLDTX03		0x010C
#define USBSS_IRQDMATHOLDRX00		0x0110
#define USBSS_IRQDMATHOLDRX01		0x0114
#define USBSS_IRQDMATHOLDRX02		0x0118
#define USBSS_IRQDMATHOLDRX03		0x011C
#define USBSS_IRQDMATHOLDTX10		0x0120
#define USBSS_IRQDMATHOLDTX11		0x0124
#define USBSS_IRQDMATHOLDTX12		0x0128
#define USBSS_IRQDMATHOLDTX13		0x012C
#define USBSS_IRQDMATHOLDRX10		0x0130
#define USBSS_IRQDMATHOLDRX11		0x0134
#define USBSS_IRQDMATHOLDRX12		0x0138
#define USBSS_IRQDMATHOLDRX13		0x013C
#define USBSS_IRQDMAENABLE0		0x0140
#define USBSS_IRQDMAENABLE1		0x0144
#define USBSS_IRQFRAMETHOLDTX00		0x0200
#define USBSS_IRQFRAMETHOLDTX01		0x0204
#define USBSS_IRQFRAMETHOLDTX02		0x0208
#define USBSS_IRQFRAMETHOLDTX03		0x020C
#define USBSS_IRQFRAMETHOLDRX00		0x0210
#define USBSS_IRQFRAMETHOLDRX01		0x0214
#define USBSS_IRQFRAMETHOLDRX02		0x0218
#define USBSS_IRQFRAMETHOLDRX03		0x021C
#define USBSS_IRQFRAMETHOLDTX10		0x0220
#define USBSS_IRQFRAMETHOLDTX11		0x0224
#define USBSS_IRQFRAMETHOLDTX12		0x0228
#define USBSS_IRQFRAMETHOLDTX13		0x022C
#define USBSS_IRQFRAMETHOLDRX10		0x0230
#define USBSS_IRQFRAMETHOLDRX11		0x0234
#define USBSS_IRQFRAMETHOLDRX12		0x0238
#define USBSS_IRQFRAMETHOLDRX13		0x023C
#define USBSS_IRQFRAMEENABLE0		0x0240
#define USBSS_IRQFRAMEENABLE1		0x0244
/*---------------------------------------------------------------------------*/
/* USB Controller Registers offset  */
#define USBCNTLR_USB0_BASE_REG_OFFSET	0x1000 /* USB0 */
#define USBCNTLR_USB1_BASE_REG_OFFSET	0x1800 /* USB1 */
#define USBCNTLR_REV			0x0000
#define USBCNTLR_CTRL			0x0014
#define USBCNTLR_STAT			0x0018
#define USBCNTLR_IRQMSTAT		0x0020
#define USBCNTLR_IRQSTATRAW0		0x0028
#define USBCNTLR_IRQSTATRAW1		0x002C
#define USBCNTLR_IRQSTAT0		0x0030
#define USBCNTLR_IRQSTAT1		0x0034
#define USBCNTLR_IRQENABLESET0		0x0038
#define USBCNTLR_IRQENABLESET1		0x003C
#define USBCNTLR_IRQENABLECLR0		0x0040
#define USBCNTLR_IRQENABLECLR1		0x0044
#define USBCNTLR_TXMODE			0x0070
#define USBCNTLR_RXMODE			0x0074
#define USBCNTLR_GENRNDISEP1		0x0080
#define USBCNTLR_GENRNDISEP2		0x0084
#define USBCNTLR_GENRNDISEP3		0x0088
#define USBCNTLR_GENRNDISEP4		0x008C
#define USBCNTLR_GENRNDISEP5		0x0090
#define USBCNTLR_GENRNDISEP6		0x0094
#define USBCNTLR_GENRNDISEP7		0x0098
#define USBCNTLR_GENRNDISEP8		0x009C
#define USBCNTLR_GENRNDISEP9		0x00A0
#define USBCNTLR_GENRNDISEP10		0x00A4
#define USBCNTLR_GENRNDISEP11		0x00A8
#define USBCNTLR_GENRNDISEP12		0x00AC
#define USBCNTLR_GENRNDISEP13		0x00B0
#define USBCNTLR_GENRNDISEP14		0x00B4
#define USBCNTLR_GENRNDISEP15		0x00B8
#define USBCNTLR_AUTOREQ		0x00D0
#define USBCNTLR_SRPFIXTIME		0x00D4
#define USBCNTLR_TDOWN			0x00D8
#define USBCNTLR_UTMI			0x00E0
#define USBCNTLR_MGCUTMILB		0x00E4
#define USBCNTLR_MODE			0x00E8
/*---------------------------------------------------------------------------*/
/* USB PHY Registers offset */
#define USBPHY_USB0_BASE_REG_OFFSET	0x1300 /* USB0 */
#define USBPHY_USB1_BASE_REG_OFFSET	0x1B00 /* USB1 */
#define USBPHY_TERMINATION_CNTL		0x0000
#define USBPHY_RX_CALIB			0x0004
#define USBPHY_DLLHS_2			0x0008
#define USBPHY_RX_TEST_2		0x000C
#define USBPHY_CHRG_DET			0x0014
#define USBPHY_PWR_CNTL			0x0018
#define USBPHY_UTMI_INTERFACE_CNTL_1	0x001C
#define USBPHY_UTMI_INTERFACE_CNTL_2	0x0020
#define USBPHY_BIST			0x0024
#define USBPHY_BIST_CRC			0x0028
#define USBPHY_CDR_BIST2		0x002C
#define USBPHY_GPIO			0x0030
#define USBPHY_DLLHS			0x0034
#define USBPHY_USB2PHYCM_CONFIG		0x003C
#define USBPHY_AD_INTERFACE_REG1	0x0044
#define USBPHY_AD_INTERFACE_REG2	0x0048
#define USBPHY_AD_INTERFACE_REG3	0x004C
#define USBPHY_ANA_CONFIG2		0x0054
/*---------------------------------------------------------------------------*/
/* USB Core Registers offset */
#define USBCORE_USB0_BASE_REG_OFFSET	0x1400
#define USBCORE_USB1_BASE_REG_OFFSET	0x1C00
/*---------------------------------------------------------------------------*/
/* USB CPPI DMA Controller Registers offset */
#define USBCPPIDMACNTLR_BASE_REG_OFFSET 0x2000
#define USBCPPIDMACNTLR_DMAREVID	0x0000
#define USBCPPIDMACNTLR_TDFDQ		0x0004
#define USBCPPIDMACNTLR_DMAEMU		0x0008
#define USBCPPIDMACNTLR_TXGCR0		0x0800
#define USBCPPIDMACNTLR_RXGCR0		0x0808
#define USBCPPIDMACNTLR_RXHPCRA0	0x080C
#define USBCPPIDMACNTLR_RXHPCRB0	0x0810
#define USBCPPIDMACNTLR_TXGCR1		0x0820
#define USBCPPIDMACNTLR_RXGCR1		0x0828
#define USBCPPIDMACNTLR_RXHPCRA1	0x082C
#define USBCPPIDMACNTLR_RXHPCRB1	0x0830
#define USBCPPIDMACNTLR_TXGCR2		0x0840
#define USBCPPIDMACNTLR_RXGCR2		0x0848
#define USBCPPIDMACNTLR_RXHPCRA2	0x084C
#define USBCPPIDMACNTLR_RXHPCRB2	0x0850
#define USBCPPIDMACNTLR_TXGCR3		0x0860
#define USBCPPIDMACNTLR_RXGCR3		0x0868
#define USBCPPIDMACNTLR_RXHPCRA3	0x086C
#define USBCPPIDMACNTLR_RXHPCRB3	0x0870
#define USBCPPIDMACNTLR_TXGCR4		0x0880
#define USBCPPIDMACNTLR_RXGCR4		0x0888
#define USBCPPIDMACNTLR_RXHPCRA4	0x088C
#define USBCPPIDMACNTLR_RXHPCRB4	0x0890
#define USBCPPIDMACNTLR_TXGCR5		0x08A0
#define USBCPPIDMACNTLR_RXGCR5		0x08A8
#define USBCPPIDMACNTLR_RXHPCRA5	0x08AC
#define USBCPPIDMACNTLR_RXHPCRB5	0x08B0
#define USBCPPIDMACNTLR_TXGCR6		0x08C0
#define USBCPPIDMACNTLR_RXGCR6		0x08C8
#define USBCPPIDMACNTLR_RXHPCRA6	0x08CC
#define USBCPPIDMACNTLR_RXHPCRB6	0x08D0
#define USBCPPIDMACNTLR_TXGCR7		0x08E0
#define USBCPPIDMACNTLR_RXGCR7		0x08E8
#define USBCPPIDMACNTLR_RXHPCRA7	0x08EC
#define USBCPPIDMACNTLR_RXHPCRB7	0x08F0
#define USBCPPIDMACNTLR_TXGCR8		0x0900
#define USBCPPIDMACNTLR_RXGCR8		0x0908
#define USBCPPIDMACNTLR_RXHPCRA8	0x090C
#define USBCPPIDMACNTLR_RXHPCRB8	0x0910
#define USBCPPIDMACNTLR_TXGCR9		0x0920
#define USBCPPIDMACNTLR_RXGCR9		0x0928
#define USBCPPIDMACNTLR_RXHPCRA9	0x092C
#define USBCPPIDMACNTLR_RXHPCRB9	0x0930
#define USBCPPIDMACNTLR_TXGCR10		0x0940
#define USBCPPIDMACNTLR_RXGCR10		0x0948
#define USBCPPIDMACNTLR_RXHPCRA10	0x094C
#define USBCPPIDMACNTLR_RXHPCRB10	0x0950
#define USBCPPIDMACNTLR_TXGCR11		0x0960
#define USBCPPIDMACNTLR_RXGCR11		0x0968
#define USBCPPIDMACNTLR_RXHPCRA11	0x096C
#define USBCPPIDMACNTLR_RXHPCRB11	0x0970
#define USBCPPIDMACNTLR_TXGCR12		0x0980
#define USBCPPIDMACNTLR_RXGCR12		0x0988
#define USBCPPIDMACNTLR_RXHPCRA12	0x098C
#define USBCPPIDMACNTLR_RXHPCRB12	0x0990
#define USBCPPIDMACNTLR_TXGCR13		0x09A0
#define USBCPPIDMACNTLR_RXGCR13		0x09A8
#define USBCPPIDMACNTLR_RXHPCRA13	0x09AC
#define USBCPPIDMACNTLR_RXHPCRB13	0x09B0
#define USBCPPIDMACNTLR_TXGCR14		0x09C0
#define USBCPPIDMACNTLR_RXGCR14		0x09C8
#define USBCPPIDMACNTLR_RXHPCRA14	0x09CC
#define USBCPPIDMACNTLR_RXHPCRB14	0x09D0
#define USBCPPIDMACNTLR_TXGCR15		0x09E0
#define USBCPPIDMACNTLR_RXGCR15		0x09E8
#define USBCPPIDMACNTLR_RXHPCRA15	0x09EC
#define USBCPPIDMACNTLR_RXHPCRB15	0x09F0
#define USBCPPIDMACNTLR_TXGCR16		0x0A00
#define USBCPPIDMACNTLR_RXGCR16		0x0A08
#define USBCPPIDMACNTLR_RXHPCRA16	0x0A0C
#define USBCPPIDMACNTLR_RXHPCRB16	0x0A10
#define USBCPPIDMACNTLR_TXGCR17		0x0A20
#define USBCPPIDMACNTLR_RXGCR17		0x0A28
#define USBCPPIDMACNTLR_RXHPCRA17	0x0A2C
#define USBCPPIDMACNTLR_RXHPCRB17	0x0A30
#define USBCPPIDMACNTLR_TXGCR18		0x0A40
#define USBCPPIDMACNTLR_RXGCR18		0x0A48
#define USBCPPIDMACNTLR_RXHPCRA18	0x0A4C
#define USBCPPIDMACNTLR_RXHPCRB18	0x0A50
#define USBCPPIDMACNTLR_TXGCR19		0x0A60
#define USBCPPIDMACNTLR_RXGCR19		0x0A68
#define USBCPPIDMACNTLR_RXHPCRA19	0x0A6C
#define USBCPPIDMACNTLR_RXHPCRB19	0x0A70
#define USBCPPIDMACNTLR_TXGCR20		0x0A80
#define USBCPPIDMACNTLR_RXGCR20		0x0A88
#define USBCPPIDMACNTLR_RXHPCRA20	0x0A8C
#define USBCPPIDMACNTLR_RXHPCRB20	0x0A90
#define USBCPPIDMACNTLR_TXGCR21		0x0AA0
#define USBCPPIDMACNTLR_RXGCR21		0x0AA8
#define USBCPPIDMACNTLR_RXHPCRA21	0x0AAC
#define USBCPPIDMACNTLR_RXHPCRB21	0x0AB0
#define USBCPPIDMACNTLR_TXGCR22		0x0AC0
#define USBCPPIDMACNTLR_RXGCR22		0x0AC8
#define USBCPPIDMACNTLR_RXHPCRA22	0x0ACC
#define USBCPPIDMACNTLR_RXHPCRB22	0x0AD0
#define USBCPPIDMACNTLR_TXGCR23		0x0AE0
#define USBCPPIDMACNTLR_RXGCR23		0x0AE8
#define USBCPPIDMACNTLR_RXHPCRA23	0x0AEC
#define USBCPPIDMACNTLR_RXHPCRB23	0x0AF0
#define USBCPPIDMACNTLR_TXGCR24		0x0B00
#define USBCPPIDMACNTLR_RXGCR24		0x0B08
#define USBCPPIDMACNTLR_RXHPCRA24	0x0B0C
#define USBCPPIDMACNTLR_RXHPCRB24	0x0B10
#define USBCPPIDMACNTLR_TXGCR25		0x0B20
#define USBCPPIDMACNTLR_RXGCR25		0x0B28
#define USBCPPIDMACNTLR_RXHPCRA25	0x0B2C
#define USBCPPIDMACNTLR_RXHPCRB25	0x0B30
#define USBCPPIDMACNTLR_TXGCR26		0x0B40
#define USBCPPIDMACNTLR_RXGCR26		0x0B48
#define USBCPPIDMACNTLR_RXHPCRA26	0x0B4C
#define USBCPPIDMACNTLR_RXHPCRB26	0x0B50
#define USBCPPIDMACNTLR_TXGCR27		0x0B60
#define USBCPPIDMACNTLR_RXGCR27		0x0B68
#define USBCPPIDMACNTLR_RXHPCRA27	0x0B6C
#define USBCPPIDMACNTLR_RXHPCRB27	0x0B70
#define USBCPPIDMACNTLR_TXGCR28		0x0B80
#define USBCPPIDMACNTLR_RXGCR28		0x0B88
#define USBCPPIDMACNTLR_RXHPCRA28	0x0B8C
#define USBCPPIDMACNTLR_RXHPCRB28	0x0B90
#define USBCPPIDMACNTLR_TXGCR29		0x0BA0
#define USBCPPIDMACNTLR_RXGCR29		0x0BA8
#define USBCPPIDMACNTLR_RXHPCRA29	0x0BAC
#define USBCPPIDMACNTLR_RXHPCRB29	0x0BB0
/*---------------------------------------------------------------------------*/
/* USB CPPI DMA Scheduler Registers offset */
#define USBCPPIDMASCHED_BASE_REG_OFFSET	0x3000
#define USBCPPIDMASCHED_CNTL		0x0000
/*---------------------------------------------------------------------------*/
/* USB Queue Manager Registers offset */
#define USBQUEUEMNGER_BASEREG_OFFSET	0x4000
#define USBQUEUEMNGER_QMGRREVID		0x0000
#define USBQUEUEMNGER_QMGRRST		0x0008
#define USBQUEUEMNGER_FDNSCO		0x0020
#define USBQUEUEMNGER_FDNSC1		0x0024
#define USBQUEUEMNGER_FDNSC2		0x0028
#define USBQUEUEMNGER_FDNSC3		0x002C
#define USBQUEUEMNGER_FDNSC4		0x0030
#define USBQUEUEMNGER_FDNSC5		0x0034
#define USBQUEUEMNGER_FDNSC6		0x0038
#define USBQUEUEMNGER_FDNSC7		0x003C
#define USBQUEUEMNGER_LRAM0BASE		0x0080
#define USBQUEUEMNGER_LRAM0SIZE		0x0084
#define USBQUEUEMNGER_LRAM1BASE		0x0088
#define USBQUEUEMNGER_PEND0		0x0090
#define USBQUEUEMNGER_PEND1		0x0094
#define USBQUEUEMNGER_PEND2		0x0098
#define USBQUEUEMNGER_PEND3		0x009C
#define USBQUEUEMNGER_PEND4		0x00A0
#define USBQUEUEMNGER_QMEMRBASE0	0x1000
#define USBQUEUEMNGER_QMEMCNTL0		0x1004
#define USBQUEUEMNGER_QMEMRBASE1	0x1010
#define USBQUEUEMNGER_QMEMCNTL1		0x1014
#define USBQUEUEMNGER_QMEMRBASE2	0x1020
#define USBQUEUEMNGER_QMEMCNTL2		0x1024
#define USBQUEUEMNGER_QMEMRBASE3	0x1030
#define USBQUEUEMNGER_QMEMCNTL3		0x1034
#define USBQUEUEMNGER_QMEMRBASE4	0x1040
#define USBQUEUEMNGER_QMEMCNTL4		0x1044
#define USBQUEUEMNGER_QMEMRBASE5	0x1050
#define USBQUEUEMNGER_QMEMCNTL5		0x1054
#define USBQUEUEMNGER_QMEMRBASE6	0x1060
#define USBQUEUEMNGER_QMEMCNTL6		0x1064
#define USBQUEUEMNGER_QMEMRBASE7	0x1070
#define USBQUEUEMNGER_QMEMCNTL7		0x1074
/*===========================================================================*/


/*===========================================================================*/
/* SPI */
/*===========================================================================*/
	/* Revision Register */
#define MCSPI_REVISION			0x0000
	/* System Configuration Register */
#define MCSPI_SYSCONFIG			0x0110
	/* System Status Register */
#define MCSPI_SYSSTATUS			0x0114
	/* Interrupt Status Register offset */
#define MCSPI_IRQSTATUS			0x0118
/*---------------------------------------------------------------------------*/
/* Interrupt Status Register bit defines */
#define MCSPI_IRQSTATUS_RX3_FULL	BIT14
#define MCSPI_IRQSTATUS_TX3_UNDERFLOW	BIT13
#define MCSPI_IRQSTATUS_TX3_EMPTY	BIT12
#define MCSPI_IRQSTATUS_RX2_FULL	BIT10
#define MCSPI_IRQSTATUS_TX2_UNDERFLOW	BIT9
#define MCSPI_IRQSTATUS_TX2_EMPTY	BIT8
#define MCSPI_IRQSTATUS_RX1_FULL	BIT6
#define MCSPI_IRQSTATUS_TX1_UNDERFLOW	BIT5
#define MCSPI_IRQSTATUS_TX1_EMPTY	BIT4
#define MCSPI_IRQSTATUS_RX0_OVERFLOW	BIT3
#define MCSPI_IRQSTATUS_RX0_FULL	BIT2
#define MCSPI_IRQSTATUS_TX0_UNDERFLOW	BIT1
#define MCSPI_IRQSTATUS_TX0_EMPTY	BIT0
	/* Interrupt Enable Register offset */
#define MCSPI_IRQENABLE			0x011C
	/* System Register offset */
#define MCSPI_SYST			0x0124
	/* Module Control Register offset */
#define MCSPI_MODULCTRL			0x0128
/*---------------------------------------------------------------------------*/
/* Configuration Registers offset */

	/* Channel 0 Configuration Register offset */
#define MCSPI_CH0CONF			0x012C
	/* Channel 1 Configuration Register offset */
#define MCSPI_CH1CONF			0x0140
	/* Channel 2 Configuration Register offset */
#define MCSPI_CH2CONF			0x0154
	/* Channel 3 Configuration Register offset */
#define MCSPI_CH3CONF			0x0168


/* Configuration Register bit defines */

	/* 1 = One clock cycle granularity */
#define MCSPI_CHXCONF_CLKG		BIT29
	/* 1 = FIFO buffer is used to Receive data */
#define MCSPI_CHXCONF_FFER		BIT28
	/* 1 = FIFO buffer is used to Transmit data */
#define MCSPI_CHXCONF_FFEW		BIT27
	/* 0.5 clock cycles between CS toggling and first (or last) edge
	   of SPI clock */
#define MCSPI_CHXCONF_TCS_0_5		(0x00 << 25)
	/* 1.5 clock cycles between CS toggling and first (or last) edge
	   of SPI clock */
#define MCSPI_CHXCONF_TCS_1_5		(0x01 << 25)
	/* 2.5 clock cycles between CS toggling and first (or last) edge
	   of SPI clock */
#define MCSPI_CHXCONF_TCS_2_5		(0x02 << 25)
	/* 3.5 clock cycles between CS toggling and first (or last) edge
	   of SPI clock */
#define MCSPI_CHXCONF_TCS_3_5		(0x03 << 25)
	/* 1 = Start bit polarity is held to 1 during SPI transfer */
#define MCSPI_CHXCONF_SBPOL		BIT24
	/* 1 = Start bit added before SPI transfer
	   0 = default length specified by WL */
#define MCSPI_CHXCONF_SBE		BIT23
	/* Slave select detection enabled on CS0 */
#define MCSPI_CHXCONF_SPIENSLV_0	(0x00 << 21)
	/* Slave select detection enabled on CS1 */
#define MCSPI_CHXCONF_SPIENSLV_1	(0x01 << 21)
	/* Slave select detection enabled on CS2 */
#define MCSPI_CHXCONF_SPIENSLV_2	(0x02 << 21)
	/* Slave select detection enabled on CS3 */
#define MCSPI_CHXCONF_SPIENSLV_3	(0x03 << 21)
	/* 1 = CSx high when EPOL is 0 and low whel EPOL is 1 */
#define MCSPI_CHXCONF_FORCE		BIT20
	/* Turbo is activated */
#define MCSPI_CHXCONF_TURBO		BIT19
	/* 1 = spim_simo selected for reception
	   0 = spim_somi selected for reception */
#define MCSPI_CHXCONF_IS		BIT18
	/* 1 = no transmission on spim_simo
	   0 = spim_simo selected for transmission */
#define MCSPI_CHXCONF_DPE1		BIT17
	/* 1 = no transmission on spim_somi
	   0 = spim_somi selected for transmission */
#define MCSPI_CHXCONF_DPE0		BIT16
	/* 1 = DMA read request enabled */
#define MCSPI_CHXCONF_DMAR		BIT15
	/* 1 = DMA write request enabled */
#define MCSPI_CHXCONF_DMAW		BIT14
	/* Transmit and receive mode */
#define MCSPI_CHXCONF_TRM_TR		(0x00 << 12)
	/* Receive-only mode */
#define MCSPI_CHXCONF_TRM_RO		(0x01 << 12)
	/* Transmit-only mode */
#define MCSPI_CHXCONF_TRM_TO		(0x02 << 12)
	/* SPI word length, 0x7 = 8-bit */
#define MCSPI_CHXCONF_WL(_x_)		((_x_ & 0x1f) << 7)
	/* 1 = SPIM_CSx is low during active state,
	   0 = high during active state */
#define MCSPI_CHxCONF_EPOL		BIT6
	/* Frequency divider for spim_clk */
#define MCSPI_CHXCONF_CLKD(_x_)		((_x_ & 0xf) << 2)
	/* 1 = spim_clk is low during active state
	   0 = high during active state */
#define MCSPI_CHXCONF_POL		BIT1
	/* 1 = data latched on even-numbered edges
	   0 = data latched on odd-numbered edges */
#define MCSPI_CHXCONF_PHA		BIT0
/*---------------------------------------------------------------------------*/
/* Status Registers offset */
#define MCSPI_CH0STAT			0x0130	// Channel 0 Status Register
#define MCSPI_CH1STAT			0x0144	// Channel 1 Status Register
#define MCSPI_CH2STAT			0x0158	// Channel 2 Status Register
#define MCSPI_CH3STAT			0x016C	// Channel 3 Status Register
/* Status Register bit defines */
#define MCSPI_CHXSAT_RXF_FULL		BIT6
#define MCSPI_CHXSAT_RXF_EMPTY		BIT5
#define MCSPI_CHXSAT_TXF_FULL		BIT4
#define MCSPI_CHXSAT_TXF_EMPTY		BIT3
#define MCSPI_CHXSAT_EOT		BIT2
#define MCSPI_CHXSAT_TX0_EMPTY		BIT1
#define MCSPI_CHXSAT_RX0_FULL		BIT0
/*---------------------------------------------------------------------------*/
/* Control Registers offset */
#define MCSPI_CH0CTRL		0x0134	// Channel 0 Control Register offset
#define MCSPI_CH1CTRL		0x0148	// Channel 1 Control Register offset
#define MCSPI_CH2CTRL		0x015C	// Channel 2 Control Register offset
#define MCSPI_CH3CTRL		0x0170	// Channel 3 Control Register offset
/*---------------------------------------------------------------------------*/
/* FIFO Buffer Registers offset */

	/* Channel 0 FIFO Transmit Buffer Register offset */
#define MCSPI_TX0		0x0138
	/* Channel 0 FIFO Receive Buffer Register offset */
#define MCSPI_RX0		0x013C
	/* Channel 1 FIFO Transmit Buffer Register offset */
#define MCSPI_TX1		0x014C
	/* Channel 1 FIFO Receive Buffer Register offset */
#define MCSPI_RX1		0x0150
	/* Channel 2 FIFO Transmit Buffer Register offset */
#define MCSPI_TX2		0x0160
	/* Channel 2 FIFO Receive Buffer Register offset */
#define MCSPI_RX2		0x0164
	/* Channel 3 FIFO Transmit Buffer Register offset */
#define MCSPI_TX3		0x0174
	/* Channel 3 FIFO Receive Buffer Register offset */
#define MCSPI_RX3		0x0178
/*---------------------------------------------------------------------------*/
	/* Transfer Levels Register */
#define MCSPI_XFERLEVEL		0x017C
	/* DMA Address Aligned FIFO Transmitter Register */
#define MCSPI_DAFTX		0x0180
	/* DMA Address Aligned FIFO Receiver Register */
#define MCSPI_DAFRX		0x01A0
/*===========================================================================*/


/*===========================================================================*/
/* General Purpose I/O */
/*===========================================================================*/
/* GPIO0 Registers */
#define GPIO0_BASE			(L4_WKUP_BASE + 0x00207000)
#define GPIO0_REG(_x_)			*(vulong *)(GPIO0_BASE + _x_)
/* GPIO1 Registers */
#define GPIO1_BASE			(L4_PER_BASE + 0x0004C000)
#define GPIO1_REG(_x_)			*(vulong *)(GPIO1_BASE + _x_)
/* GPIO2 Registers */
#define GPIO2_BASE			(L4_PER_BASE + 0x001AC000)
#define GPIO2_REG(_x_)			*(vulong *)(GPIO2_BASE + _x_)
/* GPIO3 Registers */
#define GPIO3_BASE			(L4_PER_BASE + 0x001AE000)
#define GPIO3_REG(_x_)			*(vulong *)(GPIO3_BASE + _x_)
/*---------------------------------------------------------------------------*/
/* GPIOx Register offsets */
#define GPIOX_REVISION			0x0000
#define GPIOX_SYSCONFIG			0x0010
#define GPIOX_EOI			0x0020
#define GPIOX_IRQSTATUS_RAW_0		0x0024
#define GPIOX_IRQSTATUS_RAW_1		0x0028
#define GPIOX_IRQSTATUS_0		0x002C
#define GPIOX_IRQSTATUS_1		0x0030
#define GPIOX_IRQSTATUS_SET_0		0x0034
#define GPIOX_IRQSTATUS_SET_1		0x0038
#define GPIOX_IRQSTATUS_CLR_0		0x003C
#define GPIOX_IRQSTATUS_CLR_1		0x0040
#define GPIOX_IRQWAKEN_0		0x0044
#define GPIOX_IRQWAKEN_1		0x0048
#define GPIOX_SYSSTATUS			0x0114
#define GPIOX_CTRL			0x0130
#define GPIOX_OE		 	0x0134
#define GPIOX_DATAIN	 		0x0138
#define GPIOX_DATAOUT 			0x013C
#define GPIOX_LEVELDETECT0		0x0140
#define GPIOX_LEVELDETECT1		0x0144
#define GPIOX_RISINGDETECT		0x0148
#define GPIOX_FALLINGDETECT		0x014C
#define GPIOX_DEBOUNCEENABLE		0x0150
#define GPIOX_DEBOUNCINGTIME		0x0154
#define GPIOX_CLEARDATAOUT		0x0190
#define GPIOX_SETDATAOUT		0x0194
/*===========================================================================*/


/*===========================================================================*/
/* Watchdog Timer */
/*===========================================================================*/
/* Watchdog Timer Registers offset*/
#define WDT1_BASE			(L4_WKUP_BASE + 0x00235000)
#define WDT1_REG(_x_)			*(vulong *)(WDT1_BASE + _x_)
/*---------------------------------------------------------------------------*/
/* WatchDog Timer Register offsets */

	/* Watchdow Identification Register */
#define WDT1_WIDR			0x00
	/* Watchdog System Control Register */
#define WDT1_WDSC			0x10
	/* Watchdog Status Register */
#define WDT1_WDST			0x14
	/* Watchdog Interrupt Status Register */
#define WDT1_WISR			0x18
	/* Watchdog Interrupt Enable Register */
#define WDT1_WIER			0x1C
	/* Watchdog Control Register */
#define WDT1_WCLR			0x24
	/* Watchdog Counter Register */
#define WDT1_WCRR			0x28
	/* Watchdog Load Register */
#define WDT1_WLDR			0x2C
	/* Watchdog Trigger Register */
#define WDT1_WTGR			0x30
	/* Watchdog Write Posting Bits Register */
#define WDT1_WWPS			0x34
	/* Watchdog Delay Configuration Register */
#define WDT1_WDLY			0x44
	/* Watchdog Start/Stop Register */
#define WDT1_WSPR			0x48
	/* Watchdog Raw Interrupt Status Register */
#define WDT1_WIRQSTATRAW		0x54
	/* Watchdog Interrupt Status Register */
#define WDT1_WIRQSTAT			0x58
	/* Watchdog Interrupt Enable Set Register */
#define WDT1_WIRQENSET			0x5C
	/* Watchdog Interrupt Enable Clear Register */
#define WDT1_WIRQENCLR			0x60
/*===========================================================================*/


/*===========================================================================*/
/* MMC */
/*===========================================================================*/
/* MMCHS0 Registers base */
#define MMCHS0_BASE			(L4_PER_BASE + 0x00060000)
#define MMCHS0_REG(_x_)			*(vulong *)(MMCHS0_BASE + _x_)
/* MMC Registers base */
#define MMC1_BASE			(L4_PER_BASE + 0x001D8000)
#define MMC1_REG(_x_)			*(vulong *)(MMC1_BASE + _x_)
/*---------------------------------------------------------------------------*/
/* MMC Register offsets */

	/* System Configuration */
#define SD_SYSCONFIG			0x0110
#define SD_SYSCONFIG_CLOCKACTIVITY	0x00000300
#define SD_SYSCONFIG_SIDLEMODE		0x00000018
#define SD_SYSCONFIG_SIDLEMODE_IDLE	0x00000000
#define SD_SYSCONFIG_SIDLEMODE_IGNORE	0x00000008
#define SD_SYSCONFIG_SIDLEMODE_WKUP	0x00000010
#define SD_SYSCONFIG_ENAWAKEUP		0x00000004
#define SD_SYSCONFIG_ENAWAKEUP_DISABLE	0x00000000
#define SD_SYSCONFIG_ENAWAKEUP_ENABLE	0x00000004
#define SD_SYSCONFIG_SOFTRESET		0x00000002
#define SD_SYSCONFIG_AUTOIDLE		0x00000001
#define SD_SYSCONFIG_AUTOIDLE_AUTOGATE	0x00000001
	/* System Status */
#define SD_SYSSTATUS			0x0114
#define SD_SYSSTATUS_RESETDONE		0x00000001
	/* Card Status Response Error */
#define SD_CSRE				0x0124
	/* System Test */
#define SD_SYSTEST			0x0128
	/* Configuration */
#define SD_CON				0x012C
#define SD_CON_SDMA_LNE			0x00200000
#define SD_CON_DMA_MNS			0x00100000
#define SD_CON_DDR			0x00080000
#define SD_CON_BOOT_CF0			0x00040000
#define SD_CON_BOOT_ACK			0x00020000
#define SD_CON_CLKEXTFREE		0x00010000
#define SD_CON_PADEN			0x00008000
#define SD_CON_CEATA			0x00001000
#define SD_CON_CTPL			0x00000800
#define SD_CON_DVAL			0x00000600
#define SD_CON_DVAL_33US		0x00000000
#define SD_CON_DVAL_231US		0x00000200
#define SD_CON_DVAL_1MS			0x00000400
#define SD_CON_DVAL_840US		0x00000600
#define SD_CON_WPP			0x00000100
#define SD_CON_CDP			0x00000080
#define SD_CON_CDP_ACTIVE_LOW		0x00000080
#define SD_CON_CDP_ACTIVE_HIGH		0x00000000
#define SD_CON_MIT			0x00000040
#define SD_CON_DW8			0x00000020
#define SD_CON_DW8_1BIT_OR_4BIT		0x00000000
#define SD_CON_DW8_8BIT			0x00000020
#define SD_CON_MODE			0x00000010
#define SD_CON_MODE_SYSTEST		0x00000010
#define SD_CON_MODE_NORMAL		0x00000000
#define SD_CON_STR			0x00000008
#define SD_CON_HR			0x00000004
#define SD_CON_INIT			0x00000002
#define SD_CON_INIT_START		0x00000002
#define SD_CON_INIT_END			0x00000000
#define SD_CON_OD			0x00000001
#define SD_CON_OD_ENABLE		0x00000001
#define SD_CON_OD_DISABLE		0x00000000
	/* Power Counter */
#define SD_PWCNT			0x0130
	/* SDMA System Address */
#define SD_SDMASA			0x0200
	/* Transfer Length Configuration */
#define SD_BLK				0x0204
#define SD_BLK_512_BYTES		0x00000200
	/* Command Argument */
#define SD_ARG				0x0208
	/* Command and Transfer Mode */
#define SD_CMD				0x020C
#define SD_CMD_CMD0_GO_IDLE_STATE		0x00000000
#define SD_CMD_CMD0_GO_PRE_IDLE_STATE		0x00000000
#define SD_CMD_CMD0_BOOT_INITIATION		0x00000000
#define SD_CMD_CMD1_SEND_OP_COND		0x01000000
#define SD_CMD_CMD2_ALL_SEND_CID		0x02000000
#define SD_CMD_CMD3_SEND_RELATIVE_ADDR		0x03000000
#define SD_CMD_CMD3_SET_RELATIVE_ADDR		0x03000000
#define SD_CMD_CMD4_SET_DSR			0x04000000
#define SD_CMD_CMD5_SLEEP_AWAKE			0x05000000
#define SD_CMD_CMD6_SWITCH_FUNC			0x06000000
#define SD_CMD_CMD6_SWITCH			0x06000000
#define SD_CMD_CMD7_SELECT_DESELECT_CARD	0x07000000
#define SD_CMD_CMD8_SEND_IF_COND		0x08000000
#define SD_CMD_CMD8_SEND_EXT_CSD		0x08000000
#define SD_CMD_CMD9_SEND_CSD			0x09000000
#define SD_CMD_CMD10_SEND_CID			0x0A000000
#define SD_CMD_CMD11_READ_DAT			0x0B000000
#define SD_CMD_CMD12_STOP_TRANSMISSION		0x0C000000
#define SD_CMD_CMD13_SEND_STATUS		0x0D000000
#define SD_CMD_CMD14_BUSTEST_R			0x0E000000
#define SD_CMD_CMD15_GO_INACTIVE_STATE		0x0F000000
#define SD_CMD_CMD16_SET_BLOCKLEN		0x10000000
#define SD_CMD_CMD17_READ_SINGLE_BLOCK		0x11000000
#define SD_CMD_CMD18_READ_MULTIPLE_BLOCK	0x12000000
#define SD_CMD_CMD19_BUSTEST_W			0x13000000
#define SD_CMD_CMD20_WRITE_DAT_UNTIL_STOP	0x14000000
#define SD_CMD_CMD24_WRITE_BLOCK		0x18000000
#define SD_CMD_CMD25_WRITE_MULTIPLE_BLOCK	0x19000000
#define SD_CMD_CMD26_PROGRAM_CID		0x1A000000
#define SD_CMD_CMD27_PROGRAM_CSD		0x1B000000
#define SD_CMD_CMD28_SET_WRITE_PROT		0x1C000000
#define SD_CMD_CMD29_CLR_WRITE_PROT		0x1D000000
#define SD_CMD_CMD30_SEND_WRITE_PROT		0x1E000000
#define SD_CMD_CMD31_SEND_WRITE_PROT_TYPE	0x1F000000
#define SD_CMD_CMD32_ERASE_WR_BLK_START		0x20000000
#define SD_CMD_CMD33_ERASE_WR_BLK_END		0x21000000
#define SD_CMD_CMD35_ERASE_GROUP_START		0x23000000
#define SD_CMD_CMD36_ERASE_GROUP_END		0x24000000
#define SD_CMD_CMD38_ERASE			0x26000000
#define SD_CMD_CMD39_FAST_IO			0x27000000
#define SD_CMD_CMD40_GO_IRQ_STATE		0x28000000
#define SD_CMD_CMD42_LOCK_UNLOCK		0x2A000000
#define SD_CMD_CMD55_APP_CMD			0x37000000
#define SD_CMD_CMD56_GEN_CMD			0x38000000

#define SD_CMD_ACMD6_SET_BUS_WIDTH		0x06000000
#define SD_CMD_ACMD13_SD_STATUS			0x0D000000
#define SD_CMD_ACMD22_SEND_NUM_WR_BLOCKS	0x16000000
#define SD_CMD_ACMD23_SET_WR_BLK_ERASE_COUNT	0x17000000
#define SD_CMD_ACMD41_SD_SEND_OP_COND		0x29000000
#define SD_CMD_ACMD42_SET_CLR_CARD_DETECT	0x2A000000
#define SD_CMD_ACMD51_SEND_SCR			0x33000000

#define SD_CMD_CMD_TYPE_NORMAL			0x00000000
#define SD_CMD_CMD_TYPE_SUSPEND			0x00400000
#define SD_CMD_CMD_TYPE_RESUME			0x00800000
#define SD_CMD_CMD_TYPE_ABORT			0x00C00000

#define SD_CMD_DP_NO_DATA_PRESENT		0x00000000
#define SD_CMD_DP_DATA_PRESENT			0x00200000

#define SD_CMD_CICE_DISABLE			0x00000000
#define SD_CMD_CICE_ENABLE			0x00100000

#define SD_CMD_CCCE_DISABLE			0x00000000
#define SD_CMD_CCCE_ENABLE			0x00080000

#define RSP_TYPE_NO_RESPONSE		0x00000000
#define RSP_TYPE_136			0x00010000
#define RSP_TYPE_48			0x00020000
#define RSP_TYPE_48_WITH_BUSY		0x00030000
#define SD_CMD_RSP_TYPE_NO_RESPONSE	RSP_TYPE_NO_RESPONSE
#define SD_CMD_RSP_TYPE_R1		RSP_TYPE_48
#define SD_CMD_RSP_TYPE_R1B		RSP_TYPE_48_WITH_BUSY
#define SD_CMD_RSP_TYPE_R2		RSP_TYPE_136
#define SD_CMD_RSP_TYPE_R3		RSP_TYPE_48
#define SD_CMD_RSP_TYPE_R4		RSP_TYPE_48
#define SD_CMD_RSP_TYPE_R5		RSP_TYPE_48
#define SD_CMD_RSP_TYPE_R5B		RSP_TYPE_48_WITH_BUSY
#define SD_CMD_RSP_TYPE_R6		RSP_TYPE_48
#define SD_CMD_RSP_TYPE_R7		RSP_TYPE_48

#define SD_CMD_MSBS_SINGLE		0x00000000
#define SD_CMD_MSBS_MULTIPLE		0x00000020

#define SD_CMD_DDIR_WRITE		0x00000000
#define SD_CMD_DDIR_READ		0x00000010

#define SD_CMD_ACEN_DISABLE		0x00000000
#define SD_CMD_ACEN_CMD12_ENABLE	0x00000004

#define SD_CMD_BCE_DISABLE		0x00000000
#define SD_CMD_BCE_ENABLE		0x00000002

#define SD_CMD_DE_DISABLE		0x00000000
#define SD_CMD_DE_ENABLE		0x00000001

	/* Command Response 0 and 1 */
#define SD_RSP10			0x0210
#define SD_RSP10_R1_CURRENT_STATE			0x00001E00
#define SD_RSP10_R1_CURRENT_STATE_IDLE			0x00000000
#define SD_RSP10_R1_CURRENT_STATE_READY			0x00000200
#define SD_RSP10_R1_CURRENT_STATE_IDENTIFICATION	0x00000400
#define SD_RSP10_R1_CURRENT_STATE_STANDBY		0x00000600
#define SD_RSP10_R1_CURRENT_STATE_TRANSFER		0x00000800
#define SD_RSP10_R1_CURRENT_STATE_SENDING_DATA		0x00000A00
#define SD_RSP10_R1_CURRENT_STATE_RECEIVE_DATA		0x00000C00
#define SD_RSP10_R1_CURRENT_STATE_PROGRAMMING		0x00000E00
#define SD_RSP10_R1_CURRENT_STATE_DISCONNECT		0x00001000
#define SD_RSP10_R3_CARD_CAPACITY_STATUS		0x40000000
#define SD_RSP10_R3_CARD_POWER_UP_STATUS		0x80000000
	/* Command Response 2 and 3 */
#define SD_RSP32			0x0214
	/* Command Response 4 and 5 */
#define SD_RSP54			0x0218
	/* Command Response 6 and 7 */
#define SD_RSP76			0x021C
	/* Data */
#define SD_DATA				0x0220
	/* Present State */
#define SD_PSTATE			0x0224
#define SD_PSTATE_CLEV			0x01000000
#define SD_PSTATE_DLEV			0x00F00000
#define SD_PSTATE_WP			0x00080000
#define SD_PSTATE_CDPL			0x00040000
#define SD_PSTATE_CSS			0x00020000
#define SD_PSTATE_CINS			0x00010000
#define SD_PSTATE_BRE			0x00000800
#define SD_PSTATE_BWE			0x00000400
#define SD_PSTATE_RTA			0x00000200
#define SD_PSTATE_WTA			0x00000100
#define SD_PSTATE_DLA			0x00000004
#define SD_PSTATE_DATI			0x00000002
#define SD_PSTATE_CMDI			0x00000001
	/* Host Control */
#define SD_HCTL				0x0228
#define SD_HCTL_SDVS			0x00000E00
#define SD_HCTL_SDVS_VS18		0x00000A00
#define SD_HCTL_SDVS_VS30		0x00000C00
#define SD_HCTL_SDVS_VS33		0x00000E00
#define SD_HCTL_SDBP			0x00000100
#define SD_HCTL_DMAS			0x00000180
#define SD_HCTL_DMAS_32BIT_ADDR_ADMA2	0x00000010
#define SD_HCTL_DTW			0x00000002
#define SD_HCTL_DTW_1BIT		0x00000000
#define SD_HCTL_DTW_4BIT		0x00000002
	/* SD System Control */
#define SD_SYSCTL			0x022C
#define SD_SYSCTL_SRC			0x02000000
#define SD_SYSCTL_SRA			0x01000000
#define SD_SYSCTL_DTO			0x000F0000
#define SD_SYSCTL_DTO_TCF_2_13		0x00000000
#define SD_SYSCTL_DTO_TCF_2_14		0x00010000
#define SD_SYSCTL_DTO_TCF_2_27		0x000E0000
#define SD_SYSCTL_CLKD			0x0000FFC0
#define SD_SYSCTL_CEN			0x00000004
#define SD_SYSCTL_CEN_ENABLE		0x00000004
#define SD_SYSCTL_ICS			0x00000002
#define SD_SYSCTL_ICE			0x00000001
#define SD_SYSCTL_ICE_ENABLE		0x00000001
	/* SD Interrupt Status */
#define SD_STAT				0x0230
#define SD_STAT_BADA			0x20000000
#define SD_STAT_CERR			0x10000000
#define SD_STAT_ADMAE			0x02000000
#define SD_STAT_ACE			0x01000000
#define SD_STAT_DEB			0x00400000
#define SD_STAT_DCRC			0x00200000
#define SD_STAT_DTO			0x00100000
#define SD_STAT_CIE			0x00080000
#define SD_STAT_CEB			0x00040000
#define SD_STAT_CCRC			0x00020000
#define SD_STAT_CTO			0x00010000
#define SD_STAT_ERRI			0x00008000
#define SD_STAT_BSR			0x00000400
#define SD_STAT_OBI			0x00000200
#define SD_STAT_CIRQ			0x00000100
#define SD_STAT_CREM			0x00000080
#define SD_STAT_CINS			0x00000040
#define SD_STAT_BRR			0x00000020
#define SD_STAT_BWR			0x00000010
#define SD_STAT_DMA			0x00000008
#define SD_STAT_BGE			0x00000004
#define SD_STAT_TC			0x00000002
#define SD_STAT_CC			0x00000001
	/* SD Interrupt Enable */
#define SD_IE				0x0234
#define SD_IE_BADA_ENABLE		0x20000000
#define SD_IE_CERR_ENABLE		0x10000000
#define SD_IE_ADMA_ENABLE		0x02000000
#define SD_IE_ACE_ENABLE		0x01000000
#define SD_IE_DEB_ENABLE		0x00400000
#define SD_IE_DCRC_ENABLE		0x00200000
#define SD_IE_DTO_ENABLE		0x00100000
#define SD_IE_CIE_ENABLE		0x00080000
#define SD_IE_CEB_ENABLE		0x00040000
#define SD_IE_CCRC_ENABLE		0x00020000
#define SD_IE_CTO_ENABLE		0x00010000
#define SD_IE_BSR_ENABLE		0x00000400
#define SD_IE_OBI_ENABLE		0x00000200
#define SD_IE_CIRQ_ENABLE		0x00000100
#define SD_IE_CREM_ENABLE		0x00000080
#define SD_IE_CINS_ENABLE		0x00000040
#define SD_IE_BRR_ENABLE		0x00000020
#define SD_IE_BWR_ENABLE		0x00000010
#define SD_IE_DMA_ENABLE		0x00000008
#define SD_IE_BGE_ENABLE		0x00000004
#define SD_IE_TC_ENABLE			0x00000002
#define SD_IE_CC_ENABLE			0x00000001
	/* SD Interrupt Enable Set */
#define SD_ISE				0x0238
	/* Auto CMD12 Error Status */
#define SD_AC12				0x023C
	/* Capabilities */
#define SD_CAPA				0x0240
#define SD_CAPA_VS18			0x04000000
#define SD_CAPA_VS30			0x02000000
#define SD_CAPA_VS33			0x01000000
	/* Maximum Current Capabilities */
#define SD_CUR_CAPA			0x0148
	/* Force Event */
#define SD_FE				0x0250
	/* ADMA Error Status */
#define SD_ADMAES			0x0254
	/* ADMA System Address Low bits */
#define SD_ADMASAL			0x0258
	/* ADMA System Address High bits */
#define SD_ADMASAH			0x025C
	/* Versions */
#define SD_REV				0x02FC
/*===========================================================================*/

#endif /* AM335X_H */
