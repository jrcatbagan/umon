//=============================================================================
//
//      etherdev.c
//
//      Ethernet Abstraction Layer for Micro Monitor
//
// Author(s):    Michael Kelly, Cogent Computer Systems, Inc.
// Contributors: Luis Torrico, Cogent Computer Systems, Inc.
// Date:         05-26-2002
// Modified:	 06-26-2007
// Description:  This file contains the interface layer between Micro Monitor
//               and the Ethernet driver for the LAN921x on the CSB733.
//
//=============================================================================

#include "config.h"
#include "cpuio.h"
#include "genlib.h"
#include "stddefs.h"
#include "ether.h"

extern void smsc911x_reset(void);
extern ushort smsc911x_rx(uchar *pktbuf);
extern int smsc911x_init(void);
extern ulong smsc911x_tx(ulong txbuf, ulong length);
extern void smsc911x_enable_promiscuous_reception(void);
extern void smsc911x_disable_promiscuous_reception(void);
extern void smsc911x_enable_multicast_reception(void);
extern void smsc911x_disable_multicast_reception(void);
extern void smsc911x_enable_broadcast_reception(void);
extern void smsc911x_disable_broadcast_reception(void);

ulong tx_buf[400];

#if INCLUDE_ETHERNET

/*
 * enreset():
 *	Reset the PHY and MAC.
 */
void
enreset(void)
{
	smsc911x_reset();
}

/*
 * eninit():
 * This would include establishing buffer descriptor tables and
 * all the support code that will be used by the ethernet device.
 *
 * It can be assumed at this point that the array uchar BinEnetAddr[6]
 * contains the 6-byte MAC address.
 *
 * Return 0 if successful; else -1.
 */
int
eninit(void)
{
	return smsc911x_init();

}

int
EtherdevStartup(int verbose)
{
	/* Initialize local device error counts (if any) here. */
	/* OPT_ADD_CODE_HERE */

	/* Put ethernet controller in reset: */
	enreset();

	/* Initialize controller: */
	eninit();

	return(0);
}

/* disablePromiscuousReception():
 * Provide the code that disables promiscuous reception.
 */
void
disablePromiscuousReception(void)
{
	smsc911x_disable_promiscuous_reception();
}

/* enablePromiscuousReception():
 * Provide the code that enables promiscuous reception.
 */
void
enablePromiscuousReception(void)
{
	smsc911x_enable_promiscuous_reception();
}

/* disableBroadcastReception():
 * Provide the code that disables broadcast reception.
 */
void
disableBroadcastReception(void)
{
	smsc911x_disable_broadcast_reception();
}

/* enableBroadcastReception():
 * Provide the code that enables broadcast reception.
 */
void
enableBroadcastReception(void)
{
	smsc911x_enable_broadcast_reception();
}

void
disableMulticastReception(void)
{
	smsc911x_disable_multicast_reception();
}

void
enableMulticastReception(void)
{
	smsc911x_enable_multicast_reception();
}


/* 
 * enselftest():
 *	Run a self test of the ethernet device(s).  This can be stubbed
 *	with a return(1).
 *	Return 1 if success; else -1 if failure.
 */
int
enselftest(int verbose)
{
	return(1);
}

/* ShowEtherdevStats():
 * This function is used to display device-specific stats (error counts
 * usually).
 */
void
ShowEtherdevStats(void)
{
	/* OPT_ADD_CODE_HERE */
}

/* getXmitBuffer():
 * Return a pointer to the buffer that is to be used for transmission of
 * the next packet.  Since the monitor's driver is EXTREMELY basic,
 * there will only be one packet ever being transmitted.  No need to queue
 * up transmit packets.
 */
uchar *
getXmitBuffer(void)
{
	return((uchar *) tx_buf);
}

/* sendBuffer():
 * Send out the packet assumed to be built in the buffer returned by the
 * previous call to getXmitBuffer() above.
 */
int
sendBuffer(int length)
{
	ulong temp32;

    if (length < 64)
        length = 64;

	if (EtherVerbose &  SHOW_OUTGOING)
		printPkt((struct ether_header *)tx_buf,length,ETHER_OUTGOING);

	// tell the cs8900a to send the tx buffer pointed to by tx_buf
	temp32 = smsc911x_tx((ulong)tx_buf, (ulong)length);

	EtherXFRAMECnt++;
	if (temp32) {
		return -1;
	}
	else {
		return 0;
	}
}

/* DisableEtherdev():
 * Fine as it is...
 */
void
DisableEtherdev(void)
{
	enreset();
}

/* extGetIpAdd():
 * If there was some external mechanism (other than just using the
 * IPADD shell variable established in the monrc file) for retrieval of
 * the board's IP address, then do it here...
 */
char *
extGetIpAdd(void)
{
	return((char *)0);
}

/* extGetEtherAdd():
 * If there was some external mechanism (other than just using the
 * ETHERADD shell variable established in the monrc file) for retrieval of
 * the board's MAC address, then do it here...
 */
char *
extGetEtherAdd(void)
{
	return((char *)0);
}

/*
 * polletherdev():
 * Called continuously by the monitor (ethernet.c) to determine if there
 * is any incoming ethernet packets.
 */
int
polletherdev(void)
{
	ulong pktbuf[RBUFSIZE/4];
	int	pktlen, pktcnt = 0;

	pktlen = smsc911x_rx((uchar *)pktbuf);

	if(pktlen) {
		pktcnt = 1;
		EtherRFRAMECnt++;
		processPACKET((struct ether_header *)pktbuf, pktlen);
	}
	return(pktcnt);
}

#endif
