//==========================================================================
//
// smsc911x.c
//
// 
//
// Author(s):    Jay Monkman <jtm@lopingdog.com>
// Contributors: 
// Date:         06-07-2007
// Description:  Driver for the SMSC 911x and 912x families of
//               ethernet controllers
//
//--------------------------------------------------------------------------

#include "config.h"
#if INCLUDE_ETHERNET
#include "cpuio.h"
#include "stddefs.h"
#include "genlib.h"
#include "ether.h"
#include "smsc911x.h"


//--------------------------------------------------------------------------
//  smsc911x_mac_read()
//
//  Reads a register mapped through the MAC_CSR register
//--------------------------------------------------------------------------
static ulong smsc911x_mac_read(int reg)
{
    while (MAC_CSR_CMD & MAC_CSR_CMD_CSR_BUSY) {
        continue;
    }

    MAC_CSR_CMD = MAC_RD_CMD(reg);

    while (MAC_CSR_CMD & MAC_CSR_CMD_CSR_BUSY) {
        continue;
    }

    return MAC_CSR_DATA;
}

//--------------------------------------------------------------------------
//  smsc911x_mac_write()
//
//  Writes a register mapped through the MAC_CSR register
//--------------------------------------------------------------------------
static void smsc911x_mac_write(int reg, ulong val)
{
    while (MAC_CSR_CMD & MAC_CSR_CMD_CSR_BUSY) {
        continue;
    }

    MAC_CSR_DATA = val;
    MAC_CSR_CMD = MAC_WR_CMD(reg);

    while (MAC_CSR_CMD & MAC_CSR_CMD_CSR_BUSY) {
        continue;
    }
}

//--------------------------------------------------------------------------
//  smsc911x_phy_read()
//
//  Reads a PHY register 
//--------------------------------------------------------------------------
#if 0
static ulong smsc911x_phy_read(int reg)
{
    while (smsc911x_mac_read(MII_ACC) & MII_ACC_BUSY) {
        continue;
    }

    smsc911x_mac_write(MII_ACC, MII_ACC_ADDR(1) | MII_ACC_REG(reg));

    while (smsc911x_mac_read(MII_ACC) & MII_ACC_BUSY) {
        continue;
    }

    return smsc911x_mac_read(MII_DATA);
}
#endif

//--------------------------------------------------------------------------
//  smsc911x_phy_write()
//
//  Writes a PHY register
//--------------------------------------------------------------------------
static void smsc911x_phy_write(int reg, ushort val)
{
    while (smsc911x_mac_read(MII_ACC) & MII_ACC_BUSY) {
        continue;
    }

    smsc911x_mac_write(MII_DATA, val);

    smsc911x_mac_write(MII_ACC, (MII_ACC_ADDR(1) | 
                                 MII_ACC_REG(reg) | 
                                 MII_ACC_WR));

    while (smsc911x_mac_read(MII_ACC) & MII_ACC_BUSY) {
        continue;
    }
}

//--------------------------------------------------------------------------
// smsc911x_reset()
//
// This function performs a soft reset of the controller
//--------------------------------------------------------------------------
void smsc911x_reset(void)
{

    HW_CFG |= HW_CFG_SRST;

    /* Wait for it */
    while (HW_CFG & HW_CFG_SRST) {
        continue;
    }

    PMT_CTRL |= PMT_CTRL_PHY_RST;

    /* Wait for it */
    while (PMT_CTRL & PMT_CTRL_PHY_RST) {
        continue;
    }
}

//--------------------------------------------------------------------------
// smsc911x_set_mac()
//
// This function puts the mac address from BinEnetAddr into the smsc911x.
//--------------------------------------------------------------------------
static void smsc911x_set_mac(void)
{
    ulong i;
	
    i = ((BinEnetAddr[5] << 8) |
         (BinEnetAddr[4] << 0));
    smsc911x_mac_write(ADDRH, i);


    i = ((BinEnetAddr[3] << 24) |
         (BinEnetAddr[2] << 16) |
         (BinEnetAddr[1] <<  8) |
         (BinEnetAddr[0] <<  0));
    smsc911x_mac_write(ADDRL, i);

}

//--------------------------------------------------------------------------
// smsc911x_init()
//
//  This code initializes the ethernet controller.
//--------------------------------------------------------------------------
int smsc911x_init(void)
{
    ulong val;

    /* Set the PHY clock config */
    HW_CFG = (HW_CFG & ~HW_CFG_PHY_CLK_MASK) | HW_CFG_PHY_CLK_INT;

    /* reset the controller, PHY */
    smsc911x_reset();

    switch(ID_REV) {
    case ID_REV_CHIP_9118:
	if (EtherVerbose & SHOW_DRIVER_DEBUG)
		printf("Found SMSC9118\n");
	break;
    case ID_REV_CHIP_9211:
	if (EtherVerbose & SHOW_DRIVER_DEBUG)
		printf("Found SMSC9211\n");
	break;
    case ID_REV_CHIP_9215:
	if (EtherVerbose & SHOW_DRIVER_DEBUG)
		printf("Found SMSC9215\n");
	break;
    case ID_REV_CHIP_9218:
	if (EtherVerbose & SHOW_DRIVER_DEBUG)
		printf("Found SMSC9218\n");
	break;
    default:
        printf("Unidentified Ethernet controller: 0x%x\n", ID_REV);
        return -1;
    }

    // set the mac address
    smsc911x_set_mac();

    /* These came from SMSC's example driver */
    HW_CFG = (HW_CFG & ~HW_CFG_TX_FIF_MASK) | HW_CFG_TX_FIF_SZ(5);

    AFC_CFG = (AFC_CFG_AFC_HI(0x6e) |
               AFC_CFG_AFC_LO(0x37) |
               AFC_CFG_BACK_DUR(0x4));

    /* Configure the GPIOs as LEDs */
    GPIO_CFG = (GPIO_CFG_LED3_EN | 
                GPIO_CFG_LED2_EN | 
                GPIO_CFG_LED1_EN | 
                GPIO_CFG_GPIOBUF(2) |
                GPIO_CFG_GPIOBUF(1) |
                GPIO_CFG_GPIOBUF(0));

    /* Configure PHY to advertise all speeds */
    smsc911x_phy_write(PHY_ANAR, (PHY_ANAR_100TX_FD | 
                                  PHY_ANAR_100TX |
                                  PHY_ANAR_10T_FD |
                                  PHY_ANAR_10T |
                                  PHY_ANAR_SF));

    /* Enable auto negotiation */
    smsc911x_phy_write(PHY_BCR, (PHY_BCR_ANE | 
                                 PHY_BCR_RAN));

    /* Set the controller to buffer entire packets */
    HW_CFG |= HW_CFG_SF;

    /* Configure FIFO thresholds */
    FIFO_INT = FIFO_INT_TDAL(0xff);

    /* Enable the MAC */
    val = smsc911x_mac_read(MAC_CR);
    val  |= (MAC_CR_TXEN | MAC_CR_RXEN);
    smsc911x_mac_write(MAC_CR, val);

    TX_CFG |= TX_CFG_TX_ON;

    RX_CFG = 0;

    return 0;	
}

void
smsc911x_enable_multicast_reception(void)
{
    ulong val;
    val = smsc911x_mac_read(MAC_CR);
    val  |= MAC_CR_MCPAS;
    smsc911x_mac_write(MAC_CR, val);
}

void
smsc911x_disable_multicast_reception(void)
{
    ulong val;
    val = smsc911x_mac_read(MAC_CR);
    val  &= ~MAC_CR_MCPAS;
    smsc911x_mac_write(MAC_CR, val);
}

void
smsc911x_enable_promiscuous_reception(void)
{
    ulong val;
    val = smsc911x_mac_read(MAC_CR);
    val  |= MAC_CR_PRMS;
    smsc911x_mac_write(MAC_CR, val);
}

void
smsc911x_disable_promiscuous_reception(void)
{
    ulong val;
    val = smsc911x_mac_read(MAC_CR);
    val  &= ~MAC_CR_PRMS;
    smsc911x_mac_write(MAC_CR, val);
}

void
smsc911x_enable_broadcast_reception(void)
{
    ulong val;
    val = smsc911x_mac_read(MAC_CR);
    val  |= MAC_CR_BCAST;
    smsc911x_mac_write(MAC_CR, val);
}

void
smsc911x_disable_broadcast_reception(void)
{
    ulong val;
    val = smsc911x_mac_read(MAC_CR);
    val  &= ~MAC_CR_BCAST;
    smsc911x_mac_write(MAC_CR, val);
}


//--------------------------------------------------------------------------
// smsc911x_tx()
//
// This function transmits a packet
//--------------------------------------------------------------------------
int smsc911x_tx(uchar *txbuf, ulong len)
{
    int avail;
    ulong cmda;
    ulong cmdb;
    int i;
    ulong *p;
    vulong tmp __attribute__((unused));

    tmp = TX_STATUS_FIFO_PORT;

    /* Wait until space is available for the packet */
    do {
        avail = TX_FIFO_INF & TX_FIFO_TDFREE_MASK;
    } while (avail < len);

   cmda = (TX_CMD_FS |
           TX_CMD_LS |
           TX_CMD_BS(len));

   cmdb = TX_CMD_PKTLEN(len);

   TX_FIFO_PORT = cmda;
   TX_FIFO_PORT = cmdb;
   p = (ulong*)txbuf;

   for (i = 0; i < (len/4); i++) {
       TX_FIFO_PORT = p[i];
       
   }

   if ((len & 0x3) != 0) {
       int index = len & ~3;
       int num = len & 3;
       ulong last = 0;

       for (i = 0; i < num; i++) {
           last |= (txbuf[index + i] << (i * 8));
       }

       TX_FIFO_PORT = last;
   }


   return 0;
}

//--------------------------------------------------------------------------
// smsc911x_rx()
//
// This function checks to see if the smsc911x has a receive buffer
// ready.  If so, it copies it into the buffer pointed to by pktbuf
//--------------------------------------------------------------------------
int smsc911x_rx(uchar *pktbuf)
{
    ulong status;
    int i;
    int size;
    ulong inf = RX_FIFO_INF;
    ulong *p = (ulong *)pktbuf;

    if (((inf >> 16) & 0xffff) == 0) {
        return 0;
    }

    status = RX_STATUS_FIFO_PORT;
    size = (status & RX_STATUS_PL_MASK) >> RX_STATUS_PL_SHIFT;
    if (size == 0) {
        return 0;
    }
    if ((status & RX_STATUS_ES) == 0) {
        for (i = 0; i < ((size + 3) / 4); i++) {
            p[i] = RX_FIFO_PORT;
        }

        return size - 4;
    } else {
        /* Fast forward */
        if (size >= 16) {
            RX_DP_CTL = RX_DP_RX_FFWD;
            while (RX_DP_CTL & RX_DP_RX_FFWD) {
                continue;
            }
        } else {
            ulong tmp;
            for (i = 0; i < ((size + 3)/4); i++) {
                tmp = RX_FIFO_PORT;
            }
			tmp = tmp;	// eliminate 'set-but-not-unused' warning
        }
        return 0;
    }
}
				   	

#endif // INCLUDE_ETHERNET

