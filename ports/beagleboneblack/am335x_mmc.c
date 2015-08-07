/*
 * Copyright (c) 2015 Jarielle Catbagan <jcatbagan93@gmail.com>
 *
 * The license and distribution terms for this file may be
 * found in the file LICENSE in this distribution or at
 * http://www.apache.org/licenses/LICENSE-2.0
 */

#include "genlib.h"
#include "cli.h"
#include "stddefs.h"
#include "am335x.h"
#include "am335x_mmc.h"

uint16_t mmcrca;
int mmcInum;

char *mmcHelp[] = {
    "MultiMediaCard Interface",
    "[options] {operation} [args]...",
#if INCLUDE_VERBOSEHELP
    "",
    "Options:",
    " -i ##	interface # (default is 0)",
    " -v 	additive verbosity",
    "",
    "Operations:",
    " init",
    " read {dest} {blk} {blktot}",
    " write {source} {blk} {blktot}",
#endif /* INCLUDE_VERBOSEHELP */
    0
};

int
mmccmd(uint32_t cmd, uint32_t arg, uint32_t resp[4])
{
    /* Clear the SD_STAT register for proper update of status bits after CMD invocation */
    MMC1_REG(SD_STAT) = 0xFFFFFFFF;

    MMC1_REG(SD_ARG) = arg;
    MMC1_REG(SD_CMD) = cmd;

    /* CMDx complete? */
    while(!(MMC1_REG(SD_STAT) & (SD_STAT_CC | SD_STAT_ERRI)));

    resp[0] = MMC1_REG(SD_RSP10);
    resp[1] = MMC1_REG(SD_RSP32);
    resp[2] = MMC1_REG(SD_RSP54);
    resp[3] = MMC1_REG(SD_RSP76);

    /* CMDx error? */
    if(MMC1_REG(SD_STAT) & SD_STAT_ERRI) {
        return(-1);
    } else {
        return(0);
    }
}

int
mmc(int argc, char *argv[])
{
    char *cmd, *buf;
    int opt, verbose, mmcret, blknum, blkcnt;

    verbose = 0;

    while((opt = getopt(argc, argv, "i:v")) != -1) {
        switch(opt) {
        case 'i':
            mmcInum = atoi(optarg);
            break;
        case 'v':
            verbose++;
            break;
        default:
            return(CMD_PARAM_ERROR);
        }
    }

    if(argc < optind + 1) {
        return(CMD_PARAM_ERROR);
    }

    cmd = argv[optind];

    if(mmcInstalled(mmcInum) == 0) {
        printf("MMC not installed\n");
        return(CMD_FAILURE);
    }

    if(strcmp(cmd, "init") == 0) {
        mmcret = mmcInit(mmcInum, verbose);
        if(mmcret < 0) {
            printf("mmcInit returned %d\n", mmcret);
            return(CMD_FAILURE);
        }
    } else if(strcmp(cmd, "read") == 0) {
        if(argc != (optind + 4)) {
            return(CMD_PARAM_ERROR);
        }

        buf = (char *)strtoul(argv[optind + 1], 0, 0);
        blknum = strtoul(argv[optind + 2], 0, 0);
        blkcnt = strtoul(argv[optind + 3], 0, 0);

        mmcret = mmcRead(mmcInum, buf, blknum, blkcnt);
        if(mmcret < 0) {
            printf("mmcRead returned %d\n", mmcret);
            return(CMD_FAILURE);
        }
    } else if(strcmp(cmd, "write") == 0) {
        if(argc != (optind + 4)) {
            return(CMD_PARAM_ERROR);
        }

        buf = (char *)strtoul(argv[optind + 1], 0, 0);
        blknum = strtoul(argv[optind + 2], 0, 0);
        blkcnt = strtoul(argv[optind + 3], 0, 0);

        mmcret = mmcWrite(mmcInum, buf, blknum, blkcnt);
        if(mmcret < 0) {
            printf("mmcWrite returned %d\n", mmcret);
            return(CMD_FAILURE);
        }
    } else {
        printf("mmc op <%s> not found\n", cmd);
        return(CMD_FAILURE);
    }

    return(CMD_SUCCESS);
}

int
mmcInit(int interface, int verbose)
{
    uint32_t cmd, arg, resp[4];

    /* Reset the MMC1 Controller */
    MMC1_REG(SD_SYSCONFIG) = SD_SYSCONFIG_SOFTRESET;
    while(!(MMC1_REG(SD_SYSSTATUS) & SD_SYSSTATUS_RESETDONE));

    /* Reset the command and data lines */
    MMC1_REG(SD_SYSCTL) |= SD_SYSCTL_SRA;
    while(MMC1_REG(SD_SYSCTL) & SD_SYSCTL_SRA);

    /* Configure the MMC1 controller capabilities to enable 3.0 V operating voltage */
    MMC1_REG(SD_CAPA) |= SD_CAPA_VS30;

    /* Configure SD_IE register to update certain status bits in SD_STAT */
    MMC1_REG(SD_IE) = SD_IE_BADA_ENABLE | SD_IE_CERR_ENABLE | SD_IE_ACE_ENABLE |
                      SD_IE_DEB_ENABLE | SD_IE_DCRC_ENABLE | SD_IE_DTO_ENABLE | SD_IE_CIE_ENABLE |
                      SD_IE_CEB_ENABLE | SD_IE_CCRC_ENABLE | SD_IE_CIRQ_ENABLE | SD_IE_CREM_ENABLE |
                      SD_IE_CINS_ENABLE | SD_IE_BRR_ENABLE | SD_IE_BWR_ENABLE |
                      SD_IE_TC_ENABLE | SD_IE_CC_ENABLE;

    /* Configure the operating voltage to 3.0 V */
    MMC1_REG(SD_HCTL) &= ~(SD_HCTL_SDVS);
    MMC1_REG(SD_HCTL) |= SD_HCTL_SDVS_VS30;

    /* Turn on the bus */
    MMC1_REG(SD_HCTL) |= SD_HCTL_SDBP;
    while(!(MMC1_REG(SD_HCTL) & SD_HCTL_SDBP));

    /* Enable the internal clock */
    MMC1_REG(SD_SYSCTL) |= SD_SYSCTL_ICE;

    /* Configure Clock Frequency Select to 100 KHz */
    MMC1_REG(SD_SYSCTL) = (MMC1_REG(SD_SYSCTL) & ~SD_SYSCTL_CLKD) | (960 << 6);

    /* Wait for clock to stabilize */
    while(!(MMC1_REG(SD_SYSCTL) & SD_SYSCTL_ICS));

    /* Configure SD_SYSCONFIG */
    MMC1_REG(SD_SYSCONFIG) &= ~(SD_SYSCONFIG_CLOCKACTIVITY | SD_SYSCONFIG_SIDLEMODE);
    MMC1_REG(SD_SYSCONFIG) |= SD_SYSCONFIG_SIDLEMODE_WKUP | SD_SYSCONFIG_ENAWAKEUP_ENABLE |
                              SD_SYSCONFIG_AUTOIDLE_AUTOGATE;

    /* Enable the clock to the eMMC */
    MMC1_REG(SD_SYSCTL) |= SD_SYSCTL_CEN;

    /* Perform the Initialization Stream as specified in the AM335x TRM, Section 18.3.3.2
       "Card Detection, Identification, and Selection" */
    MMC1_REG(SD_CON) |= SD_CON_INIT;
    /* Clear the SD_STAT register */
    MMC1_REG(SD_STAT) = 0xFFFFFFFF;
    MMC1_REG(SD_ARG) = 0x00000000;
    MMC1_REG(SD_CMD) = 0x00000000;
    while(!(MMC1_REG(SD_STAT) & SD_STAT_CC));
    /* Clear CC flag in SD_STAT */
    MMC1_REG(SD_STAT) |= SD_STAT_CC;
    MMC1_REG(SD_CON) &= ~SD_CON_INIT;

    /* Clear the SD_STAT register */
    MMC1_REG(SD_STAT) = 0xFFFFFFFF;

    /* Enable open-drain mode until we enter Stand-by State as illustrated in the
       JEDEC JESD84-A43 Embedded MultiMediaCard Product Standard specification, Table 5 */
    MMC1_REG(SD_CON) |= SD_CON_OD;

    /* Send CMD0/GO_IDLE_STATE to reset the eMMC on MMC1 interface */
    arg = 0x00000000;
    cmd = SD_CMD_CMD0_GO_IDLE_STATE | SD_CMD_CMD_TYPE_NORMAL | SD_CMD_DP_NO_DATA_PRESENT |
          SD_CMD_CICE_DISABLE | SD_CMD_CCCE_DISABLE | SD_CMD_RSP_TYPE_NO_RESPONSE;
    if(mmccmd(cmd, arg, resp) == -1) {
        return(-1);
    }

    /* Send CMD1 and poll busy bit in response */
    do {
        arg = 0x40FF8000;
        cmd = SD_CMD_CMD1_SEND_OP_COND | SD_CMD_CMD_TYPE_NORMAL | SD_CMD_DP_NO_DATA_PRESENT |
              SD_CMD_CICE_DISABLE | SD_CMD_CCCE_DISABLE | SD_CMD_RSP_TYPE_R3;
        if(mmccmd(cmd, arg, resp) == -1) {
            return(-1);
        }
    } while(!(MMC1_REG(SD_RSP10) & 0x80000000));

    /* Send CMD2, i.e. ALL_SEND_CID */
    arg = 0x00000000;
    cmd = SD_CMD_CMD2_ALL_SEND_CID | SD_CMD_CMD_TYPE_NORMAL | SD_CMD_DP_NO_DATA_PRESENT |
          SD_CMD_CICE_DISABLE | SD_CMD_CCCE_ENABLE | SD_CMD_RSP_TYPE_R2;
    if(mmccmd(cmd, arg, resp) == -1) {
        return(-1);
    }

    /* Set RCA of eMMC */
    mmcrca = 0x3A3A;

    /* Send CMD3 to set the relative card address (RCA) of the eMMC */
    arg = (mmcrca << 16) & 0xFFFF0000;
    cmd = SD_CMD_CMD3_SET_RELATIVE_ADDR | SD_CMD_CMD_TYPE_NORMAL | SD_CMD_DP_NO_DATA_PRESENT |
          SD_CMD_CICE_ENABLE | SD_CMD_CCCE_ENABLE | SD_CMD_RSP_TYPE_R1;
    if(mmccmd(cmd, arg, resp) == -1) {
        return(-1);
    }

    /* Wait for the eMMC to enter Stand-by State */
    do {
        /* Send CMD13 to get the status of the MMC */
        arg = (mmcrca << 16) & 0xFFFF0000;
        cmd = SD_CMD_CMD13_SEND_STATUS | SD_CMD_CMD_TYPE_NORMAL | SD_CMD_DP_NO_DATA_PRESENT |
              SD_CMD_CICE_ENABLE | SD_CMD_CCCE_ENABLE | SD_CMD_RSP_TYPE_R1;
        if(mmccmd(cmd, arg, resp) == -1) {
            return(-1);
        }
    } while((resp[0] & SD_RSP10_R1_CURRENT_STATE) != SD_RSP10_R1_CURRENT_STATE_STANDBY);

    /* Disable open-drain mode */
    MMC1_REG(SD_CON) &= ~SD_CON_OD;

    /* Send CMD7 to put the eMMC into Transfer State */
    arg = (mmcrca << 16) & 0xFFFF0000;
    cmd = SD_CMD_CMD7_SELECT_DESELECT_CARD | SD_CMD_CMD_TYPE_NORMAL | SD_CMD_DP_NO_DATA_PRESENT |
          SD_CMD_CICE_ENABLE | SD_CMD_CCCE_ENABLE | SD_CMD_RSP_TYPE_R1;
    if(mmccmd(cmd, arg, resp) == -1) {
        return(-1);
    }

    /* Wait for eMMC to enter Transfer State */
    do {
        /* Send CMD13 to get the status of the eMMC */
        arg = (mmcrca << 16) & 0xFFFF0000;
        cmd = SD_CMD_CMD13_SEND_STATUS | SD_CMD_CMD_TYPE_NORMAL | SD_CMD_DP_NO_DATA_PRESENT |
              SD_CMD_CICE_ENABLE | SD_CMD_CCCE_ENABLE | SD_CMD_RSP_TYPE_R1;
        if(mmccmd(cmd, arg, resp) == -1) {
            return(-1);
        }
    } while((resp[0] & SD_RSP10_R1_CURRENT_STATE) != SD_RSP10_R1_CURRENT_STATE_TRANSFER);

    /* Send CMD6 to change bus-width to 8-bits */
    arg = (3 << 24) | (183 << 16) | (2 << 8);
    cmd = SD_CMD_CMD6_SWITCH | SD_CMD_CMD_TYPE_NORMAL | SD_CMD_DP_NO_DATA_PRESENT |
          SD_CMD_CICE_ENABLE | SD_CMD_CCCE_ENABLE | SD_CMD_RSP_TYPE_R1B;
    if(mmccmd(cmd, arg, resp) == -1) {
        return(-1);
    }
    while(!(MMC1_REG(SD_STAT) & SD_STAT_TC));

    /* Wait while CMD6 is still in effect, i.e. while eMMC is not in Transfer State */
    do {
        arg = (mmcrca << 16) & 0xFFFF0000;
        cmd = SD_CMD_CMD13_SEND_STATUS | SD_CMD_CMD_TYPE_NORMAL | SD_CMD_DP_NO_DATA_PRESENT |
              SD_CMD_CICE_ENABLE | SD_CMD_CCCE_ENABLE | SD_CMD_RSP_TYPE_R1;
        if(mmccmd(cmd, arg, resp) == -1) {
            return(-1);
        }
    } while((resp[0] & SD_RSP10_R1_CURRENT_STATE) != SD_RSP10_R1_CURRENT_STATE_TRANSFER);

    /* Configure the MMC1 controller to use an 8-bit data width */
    MMC1_REG(SD_CON) |= SD_CON_DW8_8BIT;

    /* Send CMD6 to change to high-speed mode */
    arg = 0x03B90100;
    cmd = SD_CMD_CMD6_SWITCH | SD_CMD_CMD_TYPE_NORMAL | SD_CMD_DP_NO_DATA_PRESENT |
          SD_CMD_CICE_ENABLE | SD_CMD_CCCE_ENABLE | SD_CMD_RSP_TYPE_R1B;
    if(mmccmd(cmd, arg, resp) == -1) {
        return(-1);
    }
    while(!(MMC1_REG(SD_STAT) & SD_STAT_TC));

    /* Wait while CMD6 is still in effect, i.e. while eMMC is not in Transfer State */
    do {
        arg = (mmcrca << 16) & 0xFFFF0000;
        cmd = SD_CMD_CMD13_SEND_STATUS | SD_CMD_CMD_TYPE_NORMAL | SD_CMD_DP_NO_DATA_PRESENT |
              SD_CMD_CICE_ENABLE | SD_CMD_CCCE_ENABLE | SD_CMD_RSP_TYPE_R1;
        if(mmccmd(cmd, arg, resp) == -1) {
            return(-1);
        }
    } while((resp[0] & SD_RSP10_R1_CURRENT_STATE) != SD_RSP10_R1_CURRENT_STATE_TRANSFER);

    /* Change the clock frequency to 48 MHz and set the DTO to the maximum value setting */
    MMC1_REG(SD_SYSCTL) &= ~SD_SYSCTL_DTO;
    MMC1_REG(SD_SYSCTL) |= SD_SYSCTL_DTO_TCF_2_27;
    MMC1_REG(SD_SYSCTL) = (MMC1_REG(SD_SYSCTL) & ~SD_SYSCTL_CLKD) | (2 << 6);

    /* Wait for clock to stabilize */
    while((MMC1_REG(SD_SYSCTL) & SD_SYSCTL_ICS) != SD_SYSCTL_ICS);

    /* Put the eMMC into Stand-by State */
    arg = 0x00000000;
    cmd = SD_CMD_CMD7_SELECT_DESELECT_CARD | SD_CMD_CMD_TYPE_NORMAL | SD_CMD_DP_NO_DATA_PRESENT |
          SD_CMD_CICE_DISABLE | SD_CMD_CCCE_DISABLE | SD_CMD_RSP_TYPE_NO_RESPONSE;
    if(mmccmd(cmd, arg, resp) == -1) {
        return(-1);
    }

    /* Wait for the eMMC to enter Stand-by State */
    do {
        arg = (mmcrca << 16) & 0xFFFF0000;
        cmd = SD_CMD_CMD13_SEND_STATUS | SD_CMD_CMD_TYPE_NORMAL | SD_CMD_DP_NO_DATA_PRESENT |
              SD_CMD_CICE_ENABLE | SD_CMD_CCCE_ENABLE | SD_CMD_RSP_TYPE_R1;
        if(mmccmd(cmd, arg, resp) == -1) {
            return(-1);
        }
    } while((resp[0] & SD_RSP10_R1_CURRENT_STATE) != SD_RSP10_R1_CURRENT_STATE_STANDBY);

    return(0);
}

int
mmcRead(int interface, char *buf, int blknum, int blkcnt)
{
    uint32_t cmd, arg, resp[4];
    uint32_t *wordptr = (uint32_t *) buf;
    int byteindex;

    /* Get the SD card's status via CMD13 */
    arg = (mmcrca << 16) & 0xFFFF0000;
    cmd = SD_CMD_CMD13_SEND_STATUS | SD_CMD_CMD_TYPE_NORMAL | SD_CMD_DP_NO_DATA_PRESENT |
          SD_CMD_CICE_ENABLE | SD_CMD_CCCE_ENABLE | SD_CMD_RSP_TYPE_R1;
    if(mmccmd(cmd, arg, resp) == -1) {
        return(-1);
    }

    /* Ensure that the card is in Transfer State before proceeding */
    if((resp[0] & SD_RSP10_R1_CURRENT_STATE) != SD_RSP10_R1_CURRENT_STATE_TRANSFER) {
        arg = (mmcrca << 16) & 0xFFFF0000;
        cmd = SD_CMD_CMD7_SELECT_DESELECT_CARD | SD_CMD_CMD_TYPE_NORMAL |
              SD_CMD_DP_NO_DATA_PRESENT | SD_CMD_CICE_ENABLE | SD_CMD_CCCE_ENABLE |
              SD_CMD_RSP_TYPE_R1B;
        if(mmccmd(cmd, arg, resp) == -1) {
            return(-1);
        }

        /* Wait for the SD card to enter Transfer State */
        do {
            arg = (mmcrca << 16) & 0xFFFF0000;
            cmd = SD_CMD_CMD13_SEND_STATUS | SD_CMD_CMD_TYPE_NORMAL |
                  SD_CMD_DP_NO_DATA_PRESENT | SD_CMD_CICE_ENABLE | SD_CMD_CCCE_ENABLE |
                  SD_CMD_RSP_TYPE_R1;
            if(mmccmd(cmd, arg, resp) == -1) {
                return(-1);
            }
        } while((resp[0] & SD_RSP10_R1_CURRENT_STATE) != SD_RSP10_R1_CURRENT_STATE_TRANSFER);
    }

    /* Set the block length and the number of blocks to read */
    MMC1_REG(SD_BLK) = 0x200 | (blkcnt << 16);
    /* Read multiple blocks via CMD18 */
    arg = blknum;
    cmd = SD_CMD_CMD18_READ_MULTIPLE_BLOCK | SD_CMD_CMD_TYPE_NORMAL | SD_CMD_DP_DATA_PRESENT |
          SD_CMD_CICE_ENABLE | SD_CMD_CCCE_ENABLE | SD_CMD_RSP_TYPE_R1 | SD_CMD_MSBS_MULTIPLE |
          SD_CMD_DDIR_READ | SD_CMD_ACEN_CMD12_ENABLE | SD_CMD_BCE_ENABLE;
    if(mmccmd(cmd, arg, resp) == -1) {
        return(-1);
    }

    /* Check the data buffer to see if there is data to be read */
    do {
        while(!(MMC1_REG(SD_STAT) & SD_STAT_BRR));

        /* Clear the BRR status bit in SD_STAT */
        MMC1_REG(SD_STAT) |= SD_STAT_BRR;

        for(byteindex = 0; byteindex < (0x200 / 4); byteindex++) {
            *wordptr = MMC1_REG(SD_DATA);
            wordptr++;
        }
    } while(!(MMC1_REG(SD_STAT) & SD_STAT_TC));

    /* Put the eMMC into Stand-by State */
    arg = 0;
    cmd = SD_CMD_CMD7_SELECT_DESELECT_CARD | SD_CMD_CMD_TYPE_NORMAL | SD_CMD_DP_NO_DATA_PRESENT |
          SD_CMD_CICE_DISABLE | SD_CMD_CCCE_DISABLE | SD_CMD_RSP_TYPE_NO_RESPONSE;
    if(mmccmd(cmd, arg, resp) == -1) {
        return(-1);
    }

    /* Wait for the eMMC to enter Stand-by State */
    do {
        arg = (mmcrca << 16) & 0xFFFF0000;
        cmd = SD_CMD_CMD13_SEND_STATUS | SD_CMD_CMD_TYPE_NORMAL | SD_CMD_DP_NO_DATA_PRESENT |
              SD_CMD_CICE_ENABLE | SD_CMD_CCCE_ENABLE | SD_CMD_RSP_TYPE_R1;
        if(mmccmd(cmd, arg, resp) == -1) {
            return(-1);
        }
    } while((resp[0] & SD_RSP10_R1_CURRENT_STATE) != SD_RSP10_R1_CURRENT_STATE_STANDBY);

    return(0);
}

int
mmcWrite(int interface, char *buf, int blknum, int blkcnt)
{
    uint32_t cmd, arg, resp[4];
    uint32_t *wordptr = (uint32_t *) buf;
    int byteindex;

    /* Get the eMMC status by sending CMD13 */
    arg = (mmcrca << 16) & 0xFFFF0000;
    cmd = SD_CMD_CMD13_SEND_STATUS | SD_CMD_CMD_TYPE_NORMAL | SD_CMD_DP_NO_DATA_PRESENT |
          SD_CMD_CICE_ENABLE | SD_CMD_CCCE_ENABLE | SD_CMD_RSP_TYPE_R1;
    if(mmccmd(cmd, arg, resp) == -1) {
        return(-1);
    }

    /* Ensure that the eMMC is in the Transfer State before proceeding */
    if((resp[0] & SD_RSP10_R1_CURRENT_STATE) != SD_RSP10_R1_CURRENT_STATE_TRANSFER) {
        arg = (mmcrca << 16) & 0xFFFF0000;
        cmd  = SD_CMD_CMD7_SELECT_DESELECT_CARD | SD_CMD_CMD_TYPE_NORMAL |
               SD_CMD_DP_NO_DATA_PRESENT | SD_CMD_CICE_ENABLE | SD_CMD_CCCE_ENABLE |
               SD_CMD_RSP_TYPE_R1B;
        if(mmccmd(cmd, arg, resp) == -1) {
            return(-1);
        }

        /* Wait for eMMC to enter Transfer State */
        do {
            arg = (mmcrca << 16) & 0xFFFF0000;
            cmd = SD_CMD_CMD13_SEND_STATUS | SD_CMD_CMD_TYPE_NORMAL |
                  SD_CMD_DP_NO_DATA_PRESENT | SD_CMD_CICE_ENABLE | SD_CMD_CCCE_ENABLE |
                  SD_CMD_RSP_TYPE_R1;
            if(mmccmd(cmd, arg, resp) == -1) {
                return(-1);
            }
        } while((resp[0] & SD_RSP10_R1_CURRENT_STATE) != SD_RSP10_R1_CURRENT_STATE_TRANSFER);
    }

    /* Set the block length in bytes and the number of blocks to write to the SD card */
    MMC1_REG(SD_BLK) = 0x200 | (blkcnt << 16);
    /* Send CMD25, that is write the number of blocks specified in 'blkcount' from 'buf' to the
     * location that is 512 byte aligned in the SD card specified by the block number 'blknum'
     */
    arg = blknum;
    cmd = SD_CMD_CMD25_WRITE_MULTIPLE_BLOCK | SD_CMD_CMD_TYPE_NORMAL | SD_CMD_DP_DATA_PRESENT |
          SD_CMD_CICE_ENABLE | SD_CMD_CCCE_ENABLE | SD_CMD_RSP_TYPE_R1 | SD_CMD_MSBS_MULTIPLE |
          SD_CMD_DDIR_WRITE | SD_CMD_ACEN_CMD12_ENABLE | SD_CMD_BCE_ENABLE;
    if(mmccmd(cmd, arg, resp) == -1) {
        return(-1);
    }

    /* Write the data */
    do {
        /* Wait until data is ready to be written */
        while(!(MMC1_REG(SD_STAT) & (SD_STAT_BWR | SD_STAT_TC)));

        if(MMC1_REG(SD_STAT) & SD_STAT_TC) {
            break;
        }

        /* Clear the BWR status bit in SD_STAT */
        MMC1_REG(SD_STAT) |= SD_STAT_BWR;

        for(byteindex = 0; byteindex < (0x200 / 4); byteindex++) {
            MMC1_REG(SD_DATA) = *wordptr++;
        }
    } while(!(MMC1_REG(SD_STAT) & SD_STAT_TC));

    /* Put the eMMC into Stand-by State */
    arg = 0x00000000;
    cmd = SD_CMD_CMD7_SELECT_DESELECT_CARD | SD_CMD_CMD_TYPE_NORMAL | SD_CMD_DP_NO_DATA_PRESENT |
          SD_CMD_CICE_ENABLE | SD_CMD_CCCE_ENABLE | SD_CMD_RSP_TYPE_NO_RESPONSE;
    if(mmccmd(cmd, arg, resp) == -1) {
        return(-1);
    }

    /* Wait for eMMC to enter Stand-by State */
    do {
        arg = (mmcrca << 16) & 0xFFFF0000;
        cmd  = SD_CMD_CMD13_SEND_STATUS | SD_CMD_CMD_TYPE_NORMAL | SD_CMD_DP_NO_DATA_PRESENT |
               SD_CMD_CICE_ENABLE | SD_CMD_CCCE_ENABLE | SD_CMD_RSP_TYPE_R1;
        if(mmccmd(cmd, arg, resp) == -1) {
            return(-1);
        }
    } while((resp[0] & SD_RSP10_R1_CURRENT_STATE) != SD_RSP10_R1_CURRENT_STATE_STANDBY);

    return(0);
}

int
mmcInstalled(int interface)
{
    return(1);
}
