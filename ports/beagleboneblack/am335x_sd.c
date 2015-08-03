/*
 * Copyright (c) 2015 Jarielle Catbagan <jcatbagan93@gmail.com>
 *
 * The license and distribution terms for this file may be
 * found in the file LICENSE in this distribution or at
 * http://www.apache.org/licenses/LICENSE-2.0
 */

#include "stddefs.h"
#include "am335x.h"
#include "sd.h"

uint16_t sdrca;

int
sdcmd(uint32_t cmd, uint32_t arg, uint32_t resp[4])
{
	/* Clear the SD_STAT register for proper update of status bits after CMD invocation */
	MMCHS0_REG(SD_STAT) = 0xFFFFFFFF;

	MMCHS0_REG(SD_ARG) = arg;
	MMCHS0_REG(SD_CMD) = cmd;

	/* CMDx complete? */
	while (!(MMCHS0_REG(SD_STAT) & (SD_STAT_CC | SD_STAT_ERRI)));

	resp[0] = MMCHS0_REG(SD_RSP10);
	resp[1] = MMCHS0_REG(SD_RSP32);
	resp[2] = MMCHS0_REG(SD_RSP54);
	resp[3] = MMCHS0_REG(SD_RSP76);

	/* CMDx error? */
	if (MMCHS0_REG(SD_STAT) & SD_STAT_ERRI)
		return(-1);
	else
		return(0);
}

int
sdCardCmd(int interface, int cmd, unsigned long arg, unsigned char *resp)
{
	return(-1);
}

int
sdInit(int interface, int verbosity)
{
	uint32_t cmd, arg, resp[4];

	/* Enable MMC0 clocks */
	CM_PER_REG(CM_PER_MMC0_CLKCTRL) |= CM_PER_MMC0_CLKCTRL_MODULEMODE_ENABLE;
	while (CM_PER_REG(CM_PER_MMC0_CLKCTRL) & CM_PER_MMC0_CLKCTRL_IDLEST);

	/* Reset the MMC/SD controller */
	MMCHS0_REG(SD_SYSCONFIG) = SD_SYSCONFIG_SOFTRESET;
	while (!(MMCHS0_REG(SD_SYSSTATUS) & SD_SYSSTATUS_RESETDONE));

	/* Reset the command and data lines */
	MMCHS0_REG(SD_SYSCTL) |= SD_SYSCTL_SRA;
	while (MMCHS0_REG(SD_SYSCTL) & SD_SYSCTL_SRA);

	/* Configure the MMC/SD controller capabilities to enable 3.0 V operating voltage */
	MMCHS0_REG(SD_CAPA) = SD_CAPA_VS30;

	/* Configure SD_IE register to update certain status bits in SD_STAT */
	MMCHS0_REG(SD_IE) = SD_IE_BADA_ENABLE | SD_IE_CERR_ENABLE | SD_IE_ACE_ENABLE |
		SD_IE_DEB_ENABLE | SD_IE_DCRC_ENABLE | SD_IE_DTO_ENABLE | SD_IE_CIE_ENABLE |
		SD_IE_CEB_ENABLE | SD_IE_CCRC_ENABLE | SD_IE_CIRQ_ENABLE | SD_IE_CREM_ENABLE |
		SD_IE_CINS_ENABLE | SD_IE_BRR_ENABLE | SD_IE_BWR_ENABLE |
		SD_IE_TC_ENABLE | SD_IE_CC_ENABLE;

	/* Disable open-drain mode (only used for MMC cards) and 8-bit data width  */
	MMCHS0_REG(SD_CON) &= ~(SD_CON_OD | SD_CON_DW8);

	/* Configure the operating voltage to 3.0 V */
	MMCHS0_REG(SD_HCTL) &= ~(SD_HCTL_SDVS);
	MMCHS0_REG(SD_HCTL) |= SD_HCTL_SDVS_VS30;

	/* Set the data width to 4-bits */
	MMCHS0_REG(SD_HCTL) |= SD_HCTL_DTW;

	/* Turn on the bus */
	MMCHS0_REG(SD_HCTL) |= SD_HCTL_SDBP;
	while (!(MMCHS0_REG(SD_HCTL) & SD_HCTL_SDBP));

	/* Enable the internal clock */
	MMCHS0_REG(SD_SYSCTL) |= SD_SYSCTL_ICE;

	/* Configure Clock Frequency Select to 100 KHz */
	MMCHS0_REG(SD_SYSCTL) = (MMCHS0_REG(SD_SYSCTL) & ~(SD_SYSCTL_CLKD)) | (960 << 6);

	/* Wait for clock to stabilize */
	while (!(MMCHS0_REG(SD_SYSCTL) & SD_SYSCTL_ICS));

	/* Configure SD_SYSCONFIG */
	MMCHS0_REG(SD_SYSCONFIG) &= ~(SD_SYSCONFIG_CLOCKACTIVITY | SD_SYSCONFIG_SIDLEMODE);
	MMCHS0_REG(SD_SYSCONFIG) |= SD_SYSCONFIG_SIDLEMODE_WKUP | SD_SYSCONFIG_ENAWAKEUP_ENABLE |
		SD_SYSCONFIG_AUTOIDLE_AUTOGATE;

	/* Enable the clock to the SD card */
	MMCHS0_REG(SD_SYSCTL) |= SD_SYSCTL_CEN_ENABLE;

	/* Perform the Initialization Stream as specified in the AM335x TRM, Section 18.3.3.2
	   "Card Detection, Identicication, and Selection" */
	MMCHS0_REG(SD_CON) |= SD_CON_INIT;
	/* Clear the SD_STAT register */
	MMCHS0_REG(SD_STAT) = 0xFFFFFFFF;
	MMCHS0_REG(SD_ARG) = 0x00000000;
	MMCHS0_REG(SD_CMD) = 0x00000000;
	while (!(MMCHS0_REG(SD_STAT) & SD_STAT_CC));
	/* Clear CC flag in SD_STAT */
	MMCHS0_REG(SD_STAT) |= SD_STAT_CC;
	MMCHS0_REG(SD_CON) &= ~SD_CON_INIT;

	/* Change the clock frequency to 6 MHz and set the DTO to the maximum value setting */
	MMCHS0_REG(SD_SYSCTL) &= ~SD_SYSCTL_DTO;
	MMCHS0_REG(SD_SYSCTL) |= SD_SYSCTL_DTO_TCF_2_27;
	MMCHS0_REG(SD_SYSCTL) = (MMCHS0_REG(SD_SYSCTL) & ~SD_SYSCTL_CLKD) | (16 << 6);

	/* Wait for clock to stabilize */
	while ((MMCHS0_REG(SD_SYSCTL) & SD_SYSCTL_ICS) != SD_SYSCTL_ICS);

	/* Send CMD0/GO_IDLE_STATE to reset the SD card connected to MMC0 interface */
	arg = 0x00000000;
	cmd = SD_CMD_CMD0_GO_IDLE_STATE | SD_CMD_CMD_TYPE_NORMAL | SD_CMD_DP_NO_DATA_PRESENT |
		SD_CMD_CICE_DISABLE | SD_CMD_CCCE_DISABLE | SD_CMD_RSP_TYPE_NO_RESPONSE;
	if (sdcmd(cmd, arg, resp) == -1)
		return(-1);

	/* Send CMD8/SEND_IF_COND to verify that the SD card satisfies MMC/SD controller
	   requirements */
	arg = 0x00000188;
	cmd = SD_CMD_CMD8_SEND_IF_COND | SD_CMD_CMD_TYPE_NORMAL | SD_CMD_DP_NO_DATA_PRESENT |
		SD_CMD_CICE_ENABLE | SD_CMD_CCCE_ENABLE | SD_CMD_RSP_TYPE_R7;
	if (sdcmd(cmd, arg, resp) == -1)
		return(-1);

	/* Determine what type of SD card is connected, i.e. standard capacity, high capacity, etc. 
	 * We perform a CMD55/ACMD41 loop until the "Card power up status bit" is set in the OCR
	 * register from the SD card to determine when we have a valid response */
	do {
		arg = 0x00000000;
		cmd = SD_CMD_CMD55_APP_CMD | SD_CMD_CMD_TYPE_NORMAL | SD_CMD_DP_NO_DATA_PRESENT |
			SD_CMD_CICE_ENABLE | SD_CMD_CCCE_ENABLE | SD_CMD_RSP_TYPE_R1;
		if (sdcmd(cmd, arg, resp) == -1)
			return(-1);

		arg = 0x40060000;
		cmd = SD_CMD_ACMD41_SD_SEND_OP_COND | SD_CMD_CMD_TYPE_NORMAL |
			SD_CMD_DP_NO_DATA_PRESENT | SD_CMD_CICE_DISABLE | SD_CMD_CCCE_DISABLE |
			SD_CMD_RSP_TYPE_R3;
		if (sdcmd(cmd, arg, resp) == -1)
			return(-1);
	} while (!(resp[0] & SD_RSP10_R3_CARD_POWER_UP_STATUS));

	/* Check SD_RSP10 to determine whether the card connected is high capacity or not */
	if (resp[0] & SD_RSP10_R3_CARD_CAPACITY_STATUS)
		sdInfoTbl[interface].highcapacity = 1;
	else
		sdInfoTbl[interface].highcapacity = 0;

	/* Send CMD2 to get SD's CID and to put the card into Identification State */
	arg = 0x00000000;
	cmd = SD_CMD_CMD2_ALL_SEND_CID | SD_CMD_CMD_TYPE_NORMAL | SD_CMD_DP_NO_DATA_PRESENT |
		SD_CMD_CICE_DISABLE | SD_CMD_CCCE_ENABLE | SD_CMD_RSP_TYPE_R2;
	if (sdcmd(cmd, arg, resp) == -1)
		return(-1);

	/* Send CMD3, i.e. request new relative address (RCA) and to put the card into
	   Stand-by State */
	arg = 0x00000000;
	cmd = SD_CMD_CMD3_SEND_RELATIVE_ADDR | SD_CMD_CMD_TYPE_NORMAL | SD_CMD_DP_NO_DATA_PRESENT |
		SD_CMD_CICE_ENABLE | SD_CMD_CCCE_ENABLE | SD_CMD_RSP_TYPE_R6;
	if (sdcmd(cmd, arg, resp) == -1)
		return(-1);

	/* Save the RCA published from the SD card, this will be used in future CMDx commands */
	sdrca = (MMCHS0_REG(SD_RSP10) >> 16) & 0xFFFF;

	/* Wait for the SD card to enter Stand-by State */
	do {
		arg = (sdrca << 16) & 0xFFFF0000;
		cmd = SD_CMD_CMD13_SEND_STATUS | SD_CMD_CMD_TYPE_NORMAL | SD_CMD_DP_NO_DATA_PRESENT |
			SD_CMD_CICE_ENABLE | SD_CMD_CCCE_ENABLE | SD_CMD_RSP_TYPE_R1;
		if (sdcmd(cmd, arg, resp) == -1)
			return(-1);
	} while ((resp[0] & SD_RSP10_R1_CURRENT_STATE) != SD_RSP10_R1_CURRENT_STATE_STANDBY);

	/* Put the card with the RCA obtained previously into Transfer State via CMD7 */
	arg = (sdrca << 16) & 0xFFFF0000;
	cmd = SD_CMD_CMD7_SELECT_DESELECT_CARD | SD_CMD_CMD_TYPE_NORMAL | SD_CMD_DP_NO_DATA_PRESENT |
		SD_CMD_CICE_ENABLE | SD_CMD_CCCE_ENABLE | SD_CMD_RSP_TYPE_R1B;
	if (sdcmd(cmd, arg, resp) == -1)
		return(-1);

	/* Wait for the SD card to enter Transfer State */
	do {
		arg = (sdrca << 16) & 0xFFFF0000;
		cmd = SD_CMD_CMD13_SEND_STATUS | SD_CMD_CMD_TYPE_NORMAL | SD_CMD_DP_NO_DATA_PRESENT |
			SD_CMD_CICE_ENABLE | SD_CMD_CCCE_ENABLE | SD_CMD_RSP_TYPE_R1;
		if (sdcmd(cmd, arg, resp) == -1)
			return(-1);
	} while ((resp[0] & SD_RSP10_R1_CURRENT_STATE) != SD_RSP10_R1_CURRENT_STATE_TRANSFER);

	/* Set the bus-width to 4-bits */

	/* Send CMD55 to get ready to configure the bus-width via ACMD6 */
	arg = (sdrca << 16) & 0xFFFF0000;
	cmd = SD_CMD_CMD55_APP_CMD | SD_CMD_CMD_TYPE_NORMAL | SD_CMD_DP_NO_DATA_PRESENT |
		SD_CMD_CICE_ENABLE | SD_CMD_CCCE_ENABLE | SD_CMD_RSP_TYPE_R1;
	if (sdcmd(cmd, arg, resp) == -1)
		return(-1);

	/* Send ACMD6, SET_BUS_WIDTH to set the bus-width to 4-bits */
	arg = 0x00000002;
	cmd = SD_CMD_ACMD6_SET_BUS_WIDTH | SD_CMD_CMD_TYPE_NORMAL | SD_CMD_DP_NO_DATA_PRESENT |
		SD_CMD_CICE_ENABLE | SD_CMD_CCCE_ENABLE | SD_CMD_RSP_TYPE_R1;
	if (sdcmd(cmd, arg, resp) == -1)
		return(-1);

	/* Put the SD card into Stand-by State */
	arg = 0x00000000;
	cmd = SD_CMD_CMD7_SELECT_DESELECT_CARD | SD_CMD_CMD_TYPE_NORMAL | SD_CMD_DP_NO_DATA_PRESENT |
		SD_CMD_CICE_ENABLE | SD_CMD_CCCE_ENABLE | SD_CMD_RSP_TYPE_NO_RESPONSE;
	if (sdcmd(cmd, arg, resp) == -1)
		return(-1);

	/* Wait for the SD card to enter Stand-by State */
	do {
		arg = (sdrca << 16) & 0xFFFF0000;
		cmd = SD_CMD_CMD13_SEND_STATUS | SD_CMD_CMD_TYPE_NORMAL | SD_CMD_DP_NO_DATA_PRESENT |
			SD_CMD_CICE_ENABLE | SD_CMD_CCCE_ENABLE | SD_CMD_RSP_TYPE_R1;
		if (sdcmd(cmd, arg, resp) == -1)
			return(-1);
	} while ((resp[0] & SD_RSP10_R1_CURRENT_STATE) != SD_RSP10_R1_CURRENT_STATE_STANDBY);

	/* This point is reached when SD controller initialization and SD card
	   communication is successful */
	return(0);
}

int
sdRead(int interface, char *buf, int blknum, int blkcount)
{
	return(-1);
}

int
sdWrite(int interface, char *buf, int blknum, int blkcount)
{
	return(-1);
}

int
sdInstalled(int interface)
{
	return(-1);
}

int
sdPowerup(int tot)
{
	return(-1);
}

int
sdReadCxD(int interface, unsigned char *buf, int cmd)
{
	return(-1);
}
