//==========================================================================
//
// smsc911x.h      
//
//
// Author(s):    Jay Monkman <jtm@lopingdog.com>
// Contributors: 
// Date:         06-07-2007
// Description:  This file contains definitions for the SMSC 911x and 912x
//               families of ethernet controllers.
//
//--------------------------------------------------------------------------

// ------------------------------------------------------------------------
// cpuio.h must provide SMSC911X_BASE_ADDRESS
// defines
#define SMSC_REG(_x_)         *(vulong *)(SMSC911X_BASE_ADDRESS + _x_)

// ------------------------------------------------------------------------
// Directly visible registers. 
#define RX_FIFO_PORT                    SMSC_REG(0x00)
#define TX_FIFO_PORT                    SMSC_REG(0x20)
#define RX_FIFO_PORT_INC                SMSC_REG(0x100)
#define TX_FIFO_PORT_INC                SMSC_REG(0x120)
#define RX_STATUS_FIFO_PORT             SMSC_REG(0x40)
#define RX_STATUS_FIFO_PEEK             SMSC_REG(0x44)
#define RX_STATUS_FF                    (1 << 30)
#define RX_STATUS_PL_MASK               (0x3fff << 16)
#define RX_STATUS_PL_SHIFT              (16)
#define RX_STATUS_ES                    (1 << 15)
#define RX_STATUS_BF                    (1 << 13)
#define RX_STATUS_LE                    (1 << 12)
#define RX_STATUS_RF                    (1 << 11)
#define RX_STATUS_MF                    (1 << 10)
#define RX_STATUS_FTL                   (1 <<  7)
#define RX_STATUS_CS                    (1 <<  6)
#define RX_STATUS_FT                    (1 <<  5)
#define RX_STATUS_RWTO                  (1 <<  4)
#define RX_STATUS_ME                    (1 <<  3)
#define RX_STATUS_DB                    (1 <<  2)
#define RX_STATUS_CE                    (1 <<  1)

#define TX_STATUS_FIFO_PORT             SMSC_REG(0x48)
#define TX_STATUS_FIFO_PEEK             SMSC_REG(0x4c)
#define TX_STATUS_FIFO_ES               (1 << 15)
#define TX_STATUS_FIFO_LOC              (1 << 11)
#define TX_STATUS_FIFO_NC               (1 << 10)
#define TX_STATUS_FIFO_LC               (1 <<  9)
#define TX_STATUS_FIFO_EC               (1 <<  8)
#define TX_STATUS_FIFO_ED               (1 <<  2)
#define TX_STATUS_FIFO_UE               (1 <<  1)
#define TX_STATUS_FIFO_D                (1 <<  0)
#define TX_STATUS_FIFO_TAG_MASK          (0xffff << 16)

#define ID_REV                          SMSC_REG(0x50)
#define ID_REV_ID_MASK                  (0xffff << 16)
#define ID_REV_CHIP_9118                (0x0115 << 16)
#define ID_REV_CHIP_9211                (0x9211 << 16)
#define ID_REV_CHIP_9215                (0x115A << 16)
#define ID_REV_CHIP_9218                (0x118A << 16)
#define ID_REV_REV_MASK                 (0xffff <<  0)

#define INT_STS                         SMSC_REG(0x58)
#define INT_STS_SW_INT                  (1 << 21)
#define INT_STS_TXSTOP_INT              (1 << 25)
#define INT_STS_RXSTOP_INT              (1 << 24)
#define INT_STS_RXDFH_INT               (1 << 23)
#define INT_STS_TIOC_INT                (1 << 21)
#define INT_STS_RXD_INT                 (1 << 20)
#define INT_STS_GPT_INT                 (1 << 19)
#define INT_STS_PHY_INT                 (1 << 18)
#define INT_STS_PMT_INT                 (1 << 17)
#define INT_STS_TXSO_INT                (1 << 16)
#define INT_STS_RWT_INT                 (1 << 15)
#define INT_STS_RXE_INT                 (1 << 14)
#define INT_STS_TXE_INT                 (1 << 13)
#define INT_STS_TDFU_INT                (1 << 11)
#define INT_STS_TDFO_INT                (1 << 10)
#define INT_STS_TDFA_INT                (1 <<  9)
#define INT_STS_TSFF_INT                (1 <<  8)
#define INT_STS_TSFL_INT                (1 <<  7)
#define INT_STS_RDXF_INT                (1 <<  6)
#define INT_STS_RDFL_INT                (1 <<  5)
#define INT_STS_RSFF_INT                (1 <<  4)
#define INT_STS_RSFL_INT                (1 <<  3)
#define INT_STS_GPIO2_INT               (1 <<  2)
#define INT_STS_GPIO1_INT               (1 <<  1)
#define INT_STS_GPIO0_INT               (1 <<  0)

#define BYTE_TEST                       SMSC_REG(0x64)
#define BYTE_TEST_VAL                   (0x87654321)

#define FIFO_INT                        SMSC_REG(0x68)
#define FIFO_INT_TDAL(x)                (((x) & 0xff) << 24)
#define FIFO_INT_TSL(x)                 (((x) & 0xff) << 16)
#define FIFO_INT_RDAL(x)                (((x) & 0xff) <<  8)
#define FIFO_INT_RSL(x)                 (((x) & 0xff) <<  0)

#define RX_CFG                          SMSC_REG(0x6c)
#define RX_CFG_END_ALIGN4               (0 << 30)
#define RX_CFG_END_ALIGN16              (1 << 30)
#define RX_CFG_END_ALIGN32              (2 << 30)
#define RX_CFG_FORCE_DISCARD            (1 << 15)
#define RX_CFG_RXDOFF(x)                (((x) & 0x1f) << 8)

#define TX_CFG                          SMSC_REG(0x70)
#define TX_CFG_TXS_DUMP                 (1 << 15)
#define TX_CFG_TXD_DUMP                 (1 << 14)
#define TX_CFG_TXSAO                    (1 <<  2)
#define TX_CFG_TX_ON                    (1 <<  1)
#define TX_CFG_STOP_TX                  (1 <<  0)

#define HW_CFG                          SMSC_REG(0x74)
#define HW_CFG_TTM                      (1 << 21)
#define HW_CFG_SF                       (1 << 20)
#define HW_CFG_TX_FIF_SZ(x)             (((x) & 0xf) << 16)
#define HW_CFG_TX_FIF_MASK              (0xf << 16)
#define HW_CFG_TR(x)                    (((x) & 0x3) << 12)
#define HW_CFG_PHY_CLK_MASK             (3 << 5)
#define HW_CFG_PHY_CLK_INT              (0 << 5)
#define HW_CFG_PHY_CLK_EXT              (1 << 5)
#define HW_CFG_PHY_CLK_DIS              (2 << 5)
#define HW_CFG_SMI_SEL                  (1 << 3)
#define HW_CFG_EXT_PHY_EN               (1 << 2)
#define HW_CFG_BITMD_32                 (1 << 2)
#define HW_CFG_SRST_TO                  (1 << 1)
#define HW_CFG_SRST                     (1 << 0)

#define RX_DP_CTL                       SMSC_REG(0x78)
#define RX_DP_RX_FFWD                   (1 << 31)

#define RX_FIFO_INF                     SMSC_REG(0x7c)
#define TX_FIFO_RXSUSED_MASK             (0x00ff << 16)
#define TX_FIFO_RXDUSED_MASK             (0xffff <<  0)

#define TX_FIFO_INF                     SMSC_REG(0x80)
#define TX_FIFO_TXSUSED_MASK             (0x00ff << 16)
#define TX_FIFO_TDFREE_MASK              (0xffff <<  0)

#define PMT_CTRL                        SMSC_REG(0x84)
#define PMT_CTRL_PM_MODE_D0             (0 << 12)
#define PMT_CTRL_PM_MODE_D1             (1 << 12)
#define PMT_CTRL_PM_MODE_D2             (2 << 12)
#define PMT_CTRL_PHY_RST                (1 << 10)
#define PMT_CTRL_WOL_EN                 (1 <<  9)
#define PMT_CTRL_ED_EN                  (1 <<  8)
#define PMT_CTRL_PME_TYPE               (1 <<  6)
#define PMT_CTRL_WUPS_NONE              (0 <<  4)
#define PMT_CTRL_WUPS_D2                (1 <<  4)
#define PMT_CTRL_WUPS_D1                (2 <<  4)
#define PMT_CTRL_WUPS_MULT              (3 <<  4)
#define PMT_CTRL_PME_IND                (1 <<  3)
#define PMT_CTRL_PME_POL                (1 <<  2)
#define PMT_CTRL_PME_EN                 (1 <<  1)
#define PMT_CTRL_PME_READY              (1 <<  0)

#define GPIO_CFG                        SMSC_REG(0x88)
#define GPIO_CFG_LED3_EN                (1 << 30)
#define GPIO_CFG_LED2_EN                (1 << 29)
#define GPIO_CFG_LED1_EN                (1 << 28)
#define GPIO_CFG_GPIO_INT_POL(x)        (1 << (((x) & 0x3) + 24))
#define GPIO_CFG_EEPR_EEPROM            (0 << 20)
#define GPIO_CFG_GPIOBUF(x)             (1 << (((x) & 0x3) + 16))
#define GPIO_CFG_GPIODIR(x)             (1 << (((x) & 0x3) + 8))

#define GPT_CFG                         SMSC_REG(0x8c)
#define GPT_CFG_TIMER_EN                (1 << 29)
#define GPT_CFG_GPT_LOAD(x)             (((x) & 0xffff) << 0)

#define GPT_CNT                         SMSC_REG(0x90)
#define GPT_CNT_MASK                     (0xffff << 0)

#define WORD_SWAP                       SMSC_REG(0x98)
#define WORD_SWAP_BIG                   (0xFFFFFFFF)

#define FREE_RUN                        SMSC_REG(0x9c)

#define RX_DROP                         SMSC_REG(0xa0)

#define MAC_CSR_CMD                    SMSC_REG(0xa4)
#define MAC_CSR_CMD_CSR_BUSY           (0x80000000)
#define MAC_CSR_CMD_RNW                (0x40000000)
#define MAC_RD_CMD(x)                  (((x) & 0xff) | (MAC_CSR_CMD_CSR_BUSY |\
                                                        MAC_CSR_CMD_RNW))
#define MAC_WR_CMD(x)                  (((x) & 0xff) | (MAC_CSR_CMD_CSR_BUSY))

#define MAC_CSR_DATA                   SMSC_REG(0xa8)

#define AFC_CFG                        SMSC_REG(0xac)
#define AFC_CFG_AFC_HI(x)              (((x) & 0xff) << 16)
#define AFC_CFG_AFC_LO(x)              (((x) & 0xff) <<  8)
#define AFC_CFG_BACK_DUR(x)            (((x) & 0xf) <<   4)
#define AFC_CFG_FCMULT                 (1 << 3)
#define AFC_CFG_FCBRD                  (1 << 2)
#define AFC_CFG_FCADD                  (1 << 1)
#define AFC_CFG_FCANY                  (1 << 0)

#define E2P_CMD                         SMSC_REG(0xb0)
#define E2P_DATA                        SMSC_REG(0xb4)

// ----------------------------------------------------------
// Registers available via MAC_CSR_CMD/MAC_CSR_DATA registers
#define MAC_CR                (0x1)
#define MAC_CR_RXALL          (1 << 31)
#define MAC_CR_RCVOWN         (1 << 23)
#define MAC_CR_LOOPBK         (1 << 21)
#define MAC_CR_FDPX           (1 << 20)
#define MAC_CR_MCPAS          (1 << 18)
#define MAC_CR_PRMS           (1 << 18)
#define MAC_CR_INVFILT        (1 << 17)
#define MAC_CR_PASSBAD        (1 << 16)
#define MAC_CR_HO             (1 << 15)
#define MAC_CR_HPFILT         (1 << 13)
#define MAC_CR_LCOLL          (1 << 12)
#define MAC_CR_BCAST          (1 << 11)
#define MAC_CR_DISRTY         (1 << 10)
#define MAC_CR_PADSTR         (1 <<  8)
#define MAC_CR_BOLMT10        (0 <<  6)
#define MAC_CR_BOLMT8         (1 <<  6)
#define MAC_CR_BOLMT4         (2 <<  6)
#define MAC_CR_BOLMT1         (3 <<  6)
#define MAC_CR_DFCHK          (1 <<  5)
#define MAC_CR_TXEN           (1 <<  3)
#define MAC_CR_RXEN           (1 <<  2)

#define ADDRH                 (0x2)
#define ADDRL                 (0x3)
#define HASHH                 (0x4)
#define HASHL                 (0x5)

#define MII_ACC               (0x6)
#define MII_ACC_ADDR(x)       (((x) & 0x1f) << 11)
#define MII_ACC_REG(x)        (((x) & 0x1f) << 6)
#define MII_ACC_WR            (1 << 1)
#define MII_ACC_BUSY          (1 << 0)

#define MII_DATA              (0x7)
#define FLOW                  (0x8)
#define VLAN1                 (0x9)
#define VLAN2                 (0xa)
#define WUFF                  (0xb)
#define WUCSR                 (0xc)

// ----------------------------------------------------------
// PHY Registers
#define PHY_BCR               0
#define PHY_BCR_RESET         (1 << 15)
#define PHY_BCR_LOOP          (1 << 14)
#define PHY_BCR_SPEED         (1 << 13)
#define PHY_BCR_ANE           (1 << 12)
#define PHY_BCR_PD            (1 << 11)
#define PHY_BCR_RAN           (1 <<  9)
#define PHY_BCR_FD            (1 <<  8)
#define PHY_BCR_CT            (1 <<  7)

#define PHY_BSR               1
#define PHY_PHY1              2
#define PHY_PHY2              3
#define PHY_ANAR              4
#define PHY_ANAR_NP           (1 << 15)
#define PHY_ANAR_RF           (1 << 13)
#define PHY_ANAR_PAUSE(x)     (((x) & 0x3) << 10)
#define PHY_ANAR_100T4        (1 << 9)
#define PHY_ANAR_100TX_FD     (1 << 8)
#define PHY_ANAR_100TX        (1 << 7)
#define PHY_ANAR_10T_FD       (1 << 6)
#define PHY_ANAR_10T          (1 << 5)
#define PHY_ANAR_SF           (0x01)


#define PHY_ANLPAR            5
#define PHY_ANER              6
#define PHY_MCSR             17
#define PHY_SMR              18
#define PHY_SCSI             27
#define PHY_ISR              29
#define PHY_IMR              30
#define PHY_PSCSR            31


// ----------------------------------------------------------
// TX and RX command definitions
#define TX_CMD_IC            (1 << 31)
#define TX_CMD_BEA(x)        (((x) & 0x3) << 24)
#define TX_CMD_DS(x)         (((x) & 0x1f) << 16)
#define TX_CMD_FS            (1 << 13)
#define TX_CMD_LS            (1 << 12)
#define TX_CMD_BS(x)         (((x) & 0x7ff) << 0)
#define TX_CMD_TAG(x)        (((x) & 0xffff) << 16)
#define TX_CMD_CRCDIS        (1 << 13)
#define TX_CMD_PKTLEN(x)     (((x) & 0x7ff) << 0)



void smsc911x_reset(void);
int smsc911x_rx(uchar *);
int smsc911x_tx(uchar *, ulong);
int smsc911x_init(void);
void smsc911x_enable_promiscuous_reception(void);
void smsc911x_disable_promiscuous_reception(void);
void smsc911x_enable_multicast_reception(void);
void smsc911x_disable_multicast_reception(void);
void smsc911x_enable_broadcast_reception(void);
void smsc911x_disable_broadcast_reception(void);
