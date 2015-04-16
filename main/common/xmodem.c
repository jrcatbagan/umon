/**************************************************************************
 *
 * Copyright (c) 2013 Alcatel-Lucent
 * 
 * Alcatel Lucent licenses this file to You under the Apache License,
 * Version 2.0 (the "License"); you may not use this file except in
 * compliance with the License.  A copy of the License is contained the
 * file LICENSE at the top level of this repository.
 * You may also obtain a copy of the License at:
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 **************************************************************************
 *
 * xmodem.c:
 *
 * Command to upload or download via XMODEM protocol.  Xmodem is quite
 * limited, but adequate for simple file up/down load.
 * This also supports XMODEM-1K and YMODEM.  YMODEM is an extension to XMODEM
 * that uses a 1K packet size, CRC and the first packet (seqno=0) contains
 * information about the file being downloaded (in partiuclar, the name).
 * YMODEM also supports BATCH downloads (multiple files downloaded in one
 * transaction).  This code supports incoming BATCH downloads (not tested
 * because I can't find any terminal emulators that do it), but not for 
 * uploads.

 * Minicom notes:
 * Minicom is the terminal emulation program that comes with Linux.
 * For some reason I had to make some adjustments to the xmodem.c 
 * to accommodate Minicom...
 * - Minicom sends an initial 0x1a (EOF). Not sure why, so I just absorb
 *   that now.
 * - Minicom doesn't like a fast nak-resend rate, so each additional
 *   'd' option on the xmodem command line now causes the delay
 *   between outgoing NAKs (used to start up the protocol in Xdown())
 *   to double.  As a result, when running with minicom, a file download
 *   may need "xmodem -ddd".  Not exactly sure why this is necessary but
 *   here's my hunch...
 *   To do an xmodem transfer to the target running uMon, the "xmodem -d"
 *   command is issued on the uMon command line.  This causes uMon to start
 *   generating periodic NAKs (part of the xmodem startup handshake).
 *   Then via CTRL-A Z, the minicom side of the transaction starts up.
 *   Apparently, minicom doesn't flush the input buffer when the xmodem
 *   transfer is started on its side.  As a result there may be several
 *   NAKs buffered up on the TTY.  The end result is that if too many
 *   NAKs have been queued up in the TTY's buffer, Minicom chokes on
 *   them at startup.
 *   Adding the ability to slow down the NAK send rate keeps uMon from 
 *   sending too many NAKs prior to Minicom actually starting up its
 *   side of the protocol; hence, Minicom doesn't choke and the transfer
 *   works ok.
 *
 *   By default, this code will disable the ethernet interface
 *   during the xmodem transaction.  This is done because in most
 *   cases the monitor runs with no flow control; and getchar polls
 *   the ethernet interface.  If the poll results in an incoming
 *   packet, then it may cause the serial port to lose characters.
 *   If this is not desireable (or if the uart has a large input fifo),
 *   then this can be changed with the DONT_DISABLE_ENET_IN_XMODEM
 *   #define set up in config.h
 *
 * Original author:     Ed Sutter (ed.sutter@alcatel-lucent.com)
 *
 */
#include "config.h"
#include "genlib.h"
#include "stddefs.h"
#include "flash.h"
#include "tfs.h"
#include "tfsprivate.h"
#include "cli.h"
#include "ether.h"
#include "timer.h"

#if INCLUDE_XMODEM

#define PUTCHAR putchar


/* struct xinfo:
 * Used to contain information pertaining to the current transaction.
 * The structure is built by the command Xmodem, then passed to the other
 * support functions (Xup, Xdown, etc..) for reference and update.
 */
struct xinfo {
	uchar	sno;			/* Sequence number. */
	uchar	pad;			/* Unused, padding. */
	int		xfertot;		/* Running total of transfer. */
	int		pktlen;			/* Length of packet (128 or 1024). */
	int		pktcnt;			/* Running tally of number of packets processed. */
	int		filcnt;			/* Number of files transferred by ymodem. */
	long	size;			/* Size of upload. */
	ulong	flags;			/* Storage for various runtime flags. */
	ulong	base;			/* Starting address for data transfer. */
	ulong	dataddr;		/* Running address for data transfer. */
	int		errcnt;			/* Keep track of errors (used in verify mode). */
	int		nakresend;		/* Time between each NAK sent at Xdown startup. */
	char	*firsterrat;	/* Pointer to location of error detected when */
							/* transfer is in verify mode. */
	char	fname[TFSNAMESIZE];
};

/* Runtime flags: */
#define	USECRC	(1<<0)
#define VERIFY	(1<<1)
#define YMODEM	(1<<2)

/* Current xmodem operation: */
#define XNULL	0
#define XUP		1
#define XDOWN	2

/* X/Ymodem protocol: */
#define SOH		0x01
#define STX		0x02
#define EOT		0x04
#define ACK		0x06
#define NAK		0x15
#define CAN		0x18
#define EOF		0x1a
#define ESC		0x1b

#define PKTLEN_128	128
#define PKTLEN_1K	1024

#define BYTE_TIMEOUT	1000

static int Xup(struct xinfo *);
static int Xdown(struct xinfo *);
static int getPacket(uchar *,struct xinfo *);
static int putPacket(uchar *,struct xinfo *);

char *XmodemHelp[] = {
	"Xmodem file transfer",
	"-[a:BcdF:f:i:ks:uvy]",
#if INCLUDE_VERBOSEHELP
	"Options:",
	" -a{##}     address (overrides default of APPRAMBASE)",
#if INCLUDE_FLASH
	" -B         boot sector reload",
#endif
	" -c         use crc (default = checksum)",
	" -d         download",
#if INCLUDE_TFS
	" -F{name}   filename",
	" -f{flags}  file flags (see tfs)",
	" -i{info}   file info (see tfs)",
#endif
	" -k         force packet length to 1K",
	" -s{##}     size (overrides computed size)",
	" -u         upload",
	" -v         verify only",
#if INCLUDE_TFS
	" -y         use Ymodem extensions",
#endif
	"Notes:",
	" * Either -d or -u must be specified (-B implies -d).",
	" * Each additional 'd' option cuts the nak-resend rate in half.",
	" * XMODEM forces a 128-byte modulo on file size.  The -s option",
	"   can be used to override this when transferring a file to TFS.",
	" * File upload requires no address or size (size will be mod 128).",
	" * When using -B, it should be the ONLY command line option,",
	"   it's purpose is to reprogram the boot sector, so be careful!",
#endif
	(char *)0,
};

int
Xmodem(int argc,char *argv[])
{
	int	opt, xop;
	struct	xinfo xi;
	char	*info, *flags;
#if INCLUDE_TFS
	TFILE	*tfp;
#endif
#if INCLUDE_FLASH
	int	newboot = 0;
#endif
#if INCLUDE_ETHERNET
	int eon;
#endif

	xop = XNULL;
	info = (char *)0;
	flags = (char *)0;
	xi.fname[0] = 0;
	xi.size = 0;
	xi.flags = 0;
	xi.filcnt = 0;
	xi.nakresend = 2;
	xi.pktlen = PKTLEN_128;
	xi.base = xi.dataddr = getAppRamStart();
	while ((opt=getopt(argc,argv,"a:BcdF:f:i:ks:uvy")) != -1) {
		switch(opt) {
		case 'a':
			xi.dataddr = xi.base = strtoul(optarg,(char **)0,0);
			break;
		case 'B':
			xop = XDOWN;
#if INCLUDE_FLASH
			newboot = 1;
#endif
			break;
		case 'c':
			xi.flags |= USECRC;
			break;
		case 'd':
			xi.nakresend *= 2;		/* -ddd increases Xdown() resend delay */
			xop = XDOWN;
			break;
#if INCLUDE_TFS
		case 'F':
			strncpy(xi.fname,optarg,TFSNAMESIZE);
			break;
		case 'f':
			flags = optarg;
			break;
		case 'i':
			info = optarg;
			break;
#endif
		case 'k':
			xi.pktlen = PKTLEN_1K;
			break;
		case 's':
			xi.size = strtol(optarg,(char **)0,0);
			break;
		case 'u':
			xop = XUP;
			break;
		case 'v':
			xi.flags |= VERIFY;
			break;
#if INCLUDE_TFS
		case 'y':
			xi.flags |= (YMODEM | USECRC);
			xi.pktlen = PKTLEN_1K;
			break;
#endif
		default:
			return(CMD_PARAM_ERROR);
		}
	}

	/* There should be no arguments after the option list. */
	if (argc != optind)
		return(CMD_PARAM_ERROR);

	if (xop == XUP) {
		if ((xi.flags & YMODEM) && !(xi.fname[0]))
			printf("Ymodem upload needs filename\n");
		else {
			if (xi.fname[0]) {	/* Causes -a and -s options to be ignored. */
#if INCLUDE_TFS
				tfp = tfsstat(xi.fname);
				if (!tfp) {
					printf("%s: file not found\n",xi.fname);
					return(CMD_FAILURE);
				}
				xi.base = xi.dataddr = (ulong)TFS_BASE(tfp);
				xi.size = TFS_SIZE(tfp);
#endif
			}
#ifdef DONT_DISABLE_ENET_IN_XMODEM		/* See note at top of file */
			Xup(&xi);
#else
#if INCLUDE_ETHERNET
			eon = DisableEthernet();
#endif
			Xup(&xi);
#if INCLUDE_ETHERNET
			if (eon)
				EthernetStartup(0,0);
#endif
#endif
		}
	}
	else if (xop == XDOWN) {
		long	tmpsize;

		if ((xi.flags & YMODEM) && (xi.fname[0]))
			printf("Ymodem download gets name from protocol, '%s' ignored\n",
				xi.fname);

#ifdef DONT_DISABLE_ENET_IN_XMODEM		/* See note at top of file */
		tmpsize = (long)Xdown(&xi);
#else
#if INCLUDE_ETHERNET
		eon = DisableEthernet();
#endif
		tmpsize = (long)Xdown(&xi);
#if INCLUDE_ETHERNET
		if (eon)
			EthernetStartup(0,0);
#endif
#endif
		if (tmpsize == -1)
			return(CMD_FAILURE);
		if ((!xi.size) || (tmpsize < 0))
			xi.size = tmpsize;

#if INCLUDE_TFS
		if ((xi.fname[0]) && (xi.size > 0)) {
			int	err;

			printf("Writing to file '%s'...\n",xi.fname);
			err = tfsadd(xi.fname,info,flags,(uchar *)xi.base,xi.size);
			if (err != TFS_OKAY) {
				printf("%s: %s\n",xi.fname,(char *)tfsctrl(TFS_ERRMSG,err,0));
				return(CMD_FAILURE);
			}
		}
#if INCLUDE_FLASH
		else
#endif
#endif
#if INCLUDE_FLASH
		if ((newboot) && (xi.size > 0)) {
			extern	int FlashProtectWindow;
			int		rc;
			char	*bb;
			ulong	bootbase;

			bb = getenv("BOOTROMBASE");
			if (bb)
				bootbase = strtoul(bb,0,0);
			else
				bootbase = BOOTROM_BASE;
#ifdef TO_FLASH_ADDR
			/* The address the program is linked to is not necessarily the
			 * physical address where flash operations can be performed on.
			 * A typical use could be that the program is linked to run in
			 * a cacheable region but writing to the flash can only be done
			 * in an uncached region.
			 * "config.h" is a good place to define the TO_FLASH_ADDR macro.
			 */
			bootbase = (ulong)TO_FLASH_ADDR(bootbase);
#endif
			printf("Reprogramming boot @ 0x%lx from 0x%lx, %ld bytes.\n",
				bootbase,xi.base,xi.size);
			if (askuser("OK?")) {
				int first, last;

				/* If sector(s) need to be unlocked, do that now...
				 */
				if (addrtosector((uchar *)bootbase,&first,0,0) == -1)
					return(CMD_FAILURE);

				if (addrtosector((uchar *)bootbase+xi.size-1,&last,0,0) == -1)
					return(CMD_FAILURE);

				if (flashlock(first,FLASH_LOCKABLE) != -1) {
					FlashProtectWindow = 1;
					while(first < last)
						flashlock(first++,FLASH_UNLOCK);
				}

				FlashProtectWindow = 1;
				rc = flashewrite((uchar *)bootbase,(uchar *)xi.base,xi.size);
				if (rc == -1) {
					printf("failed\n");
					return(CMD_FAILURE);
				}
			}
		}
#endif
	}
	else
		return(CMD_PARAM_ERROR);
	shell_sprintf("XMODEMGET","%ld",xi.size);
	return(CMD_SUCCESS);
}

/* xgetchar():
 * Wrap getchar with a timer, so that after 5 seconds of waiting
 * we giveup...
 */
int
xgetchar(char *cp, int lno)
{
	struct	elapsed_tmr tmr;

	startElapsedTimer(&tmr,5000);
	while(!msecElapsed(&tmr) && !gotachar());
	if (!gotachar()) {
		Mtrace("Xgetchar tmt %d",lno);
		return(-1);
	}
	*cp = getchar();
	return(0);
}

/* putPacket():
 * Used by Xup to send packets.
 */
static int
putPacket(uchar *tmppkt, struct xinfo *xip)
{
	char	c;
	int		i;
	ushort	chksm;

	chksm = 0;
	
	if (xip->pktlen == PKTLEN_128)
		PUTCHAR(SOH);
	else
		PUTCHAR(STX);

	PUTCHAR((char)(xip->sno));
	PUTCHAR((char)~(xip->sno));

	if (xip->flags & USECRC) {
		for(i=0;i<xip->pktlen;i++) {
			PUTCHAR((char)*tmppkt);
			chksm = (chksm<<8)^xcrc16tab[(chksm>>8)^*tmppkt++];
		}
    	/* An "endian independent way to extract the CRC bytes. */
		PUTCHAR((char)(chksm >> 8));
		PUTCHAR((char)chksm);
	}
	else {
		for(i=0;i<xip->pktlen;i++) {
			PUTCHAR((char)*tmppkt);
			chksm = ((chksm+*tmppkt++)&0xff);
		}
		PUTCHAR((char)(chksm&0x00ff));
	}

	if (xgetchar(&c,__LINE__) == -1)			/* Wait for ack */
		return(-1);

	/* If pktcnt == -1, then this is the first packet sent by
	 * YMODEM (filename) and we must wait for one additional
	 * character in the response.
	 */
	if (xip->pktcnt == -1) {
		if (xgetchar(&c,__LINE__) == -1)
			return(-1);
	}
	return((int)c);
}

/* getPacket():
 * Used by Xdown to retrieve packets.
 */
static int
getPacket(uchar *tmppkt, struct xinfo *xip)
{
	char	*pkt;
	int		i,rcvd;
	uchar	seq[2];

	if ((rcvd = getbytes_t((char *)seq,2,BYTE_TIMEOUT)) != 2) {
		PUTCHAR(NAK);
		Mtrace("RCVD %d != 2",rcvd);
		return(0);
	}

	if (xip->flags & VERIFY) {
		rcvd = getbytes_t((char *)tmppkt,xip->pktlen,BYTE_TIMEOUT);
		if (rcvd != xip->pktlen) {
			PUTCHAR(NAK);
			Mtrace("RCVD %d != %d",rcvd,xip->pktlen);
			return(0);
		}
		for(i=0;i<xip->pktlen;i++) {
			if (tmppkt[i] != ((char *)xip->dataddr)[i]) {
				if (xip->errcnt++ == 0)
					xip->firsterrat = (char *)(xip->dataddr+i);
			}
		}
		pkt = (char *)tmppkt;
	}
	else {
		rcvd = getbytes_t((char *)xip->dataddr,xip->pktlen,BYTE_TIMEOUT);
		if (rcvd != xip->pktlen) {
			PUTCHAR(NAK);
			Mtrace("RCVD %d != %d",rcvd,xip->pktlen);
			return(0);
		}
		pkt = (char *)xip->dataddr;
	}

	if (xip->flags & USECRC) {
		char c;
		ushort	crc, xcrc;
		
    	/* An "endian independent way to combine the CRC bytes. */
		if (xgetchar(&c,__LINE__) == -1)
			return(0);
		crc = (ushort)c;
		crc <<= 8;
		if (xgetchar(&c,__LINE__) == -1)
			return(0);
	    crc += (ushort)c;
		xcrc = xcrc16((uchar *)pkt,(ulong)(xip->pktlen));
		if (crc != xcrc) {
			PUTCHAR(NAK);
			Mtrace("CRC %04x != %04x",crc,xcrc);
			return(0);
		}
	}
	else {
		uchar	csum, xcsum;

		if (xgetchar((char *)&xcsum,__LINE__) == -1)
			return(0);
		csum = 0;
		for(i=0;i<xip->pktlen;i++)
			csum += *pkt++;
		if (csum != xcsum) {
			PUTCHAR(NAK);
			Mtrace("CSUM %02x != %02x (%d)",csum,xcsum,xip->pktlen);
			return(0);
		}
		Mtrace("CSUM %02x (%d)",csum,xip->pktlen);
	}

	/* Test the sequence number compliment...
	 */
	if ((uchar)seq[0] != (uchar)~seq[1]) {
		PUTCHAR(NAK);
		Mtrace("SNOCMP %02x != %02x",(uchar)seq[0],(uchar)~(seq[1]));
		return(0);
	}

	/* Verify that the incoming sequence number is the expected value...
	 */
	if ((uchar)seq[0] !=  xip->sno) {
		/* If the incoming sequence number is one less than the expected
		 * sequence number, then we assume that the sender did not recieve
		 * our previous ACK, and they are resending the previously received
		 * packet.  In that case, we send  ACK and don't process the
		 * incoming packet...
	 	 */
		if ((uchar)seq[0] == xip->sno-1) {
			Mtrace("R_ACK");
			PUTCHAR(ACK);
			return(0);
		}

		/* Otherwise, something's messed up...
		 */
		PUTCHAR(CAN);
		Mtrace("SNO: %02x != %02x",seq[0],xip->sno);
		return(-1);
	}

	/* First packet of YMODEM contains information about the transfer:
	 * FILENAME SP FILESIZE SP MOD_DATE SP FILEMODE SP FILE_SNO
	 * Only the FILENAME is required and if others are present, then none
	 * can be skipped.
	 */
	if ((xip->flags & YMODEM) && (xip->pktcnt == 0)) {
		char *slash, *space, *fname;

		slash = strrchr((char *)(xip->dataddr),'/');
		space = strchr((char *)(xip->dataddr),' ');
		if (slash)
			fname = slash+1;
		else
			fname = (char *)(xip->dataddr);
		Mtrace("<fname=%s>",fname);
		if (space) {
			*space = 0;
			xip->size = atoi(space+1);
		}
		strcpy(xip->fname,fname);
		if (fname[0])
			xip->filcnt++;
	}
	else
		xip->dataddr += xip->pktlen;
	xip->sno++;
	xip->pktcnt++;
	xip->xfertot += xip->pktlen;
	Mtrace("ACK");
	PUTCHAR(ACK);
	if (xip->flags & YMODEM) {
		if (xip->fname[0] == 0) {
			printf("\nRcvd %d file%c\n",
				xip->filcnt,xip->filcnt > 1 ? 's' : ' ');
			return(1);
		}
	}
	return(0);
}

/* Xup():
 * Called when a transfer from target to host is being made (considered
 * an upload).
 */
static int
Xup(struct xinfo *xip)
{
	uchar	c, buf[PKTLEN_128];
	int		tmp, done, pktlen;

	Mtrace("Xup starting");

	if (xip->size & 0x7f) {
		xip->size += 128;
		xip->size &= 0xffffff80L;
	}

	printf("Upload %ld bytes from 0x%lx\n",xip->size,(ulong)xip->base);

	/* Startup synchronization... */
	/* Wait to receive a NAK or 'C' from receiver. */
	done = 0;
	while(!done) {
		if (xgetchar((char *)&c,__LINE__) == -1)
			return(0);
		switch(c) {
		case NAK:
			done = 1;
			Mtrace("CSM");
			break;
		case 'C':
			xip->flags |= USECRC;
			done = 1;
			Mtrace("CRC");
			break;
		case 'q':	/* ELS addition, not part of XMODEM spec. */
			return(0);
		default:
			break;
		}
	}

	rawon();

	if (xip->flags & YMODEM) {
		Mtrace("SNO_0");
		xip->sno = 0;
		xip->pktcnt = -1;
		memset((char *)buf,0,PKTLEN_128);
		sprintf((char *)buf,"%s",xip->fname);
		pktlen = xip->pktlen;
		xip->pktlen = PKTLEN_128;
		if (putPacket(buf,xip) == -1)
			return(0);
		xip->pktlen = pktlen;
	}

	done = 0;
	xip->sno = 1;
	xip->pktcnt = 0;
	while(!done) {
		if ((tmp = putPacket((uchar *)(xip->dataddr),xip)) == -1)
			return(0);
		c = (uchar)tmp;
		switch(c) {
		case ACK:
			xip->sno++;
			xip->pktcnt++;
			xip->size -= xip->pktlen;
			xip->dataddr += xip->pktlen;
			Mtrace("A");
			break;
		case NAK:
			Mtrace("N");
			break;
		case CAN:
			done = -1;
			Mtrace("C");
			break;
		case EOT:
			done = -1;
			Mtrace("E");
			break;
		default:
			done = -1;
			Mtrace("<%02x>",c);
			break;
		}
		if (xip->size <= 0) {
			char tmp;

			PUTCHAR(EOT);
			if (xgetchar(&tmp,__LINE__) == -1)	/* Flush the ACK */
				return(0);
			break;
		}
		Mtrace("!");
	}

	Mtrace("Xup_almost");
	if ((done != -1) && (xip->flags & YMODEM)) { 
		xip->sno = 0;
		memset((char *)buf,0,PKTLEN_128);
		pktlen = xip->pktlen;
		xip->pktlen = PKTLEN_128;
		if (putPacket(buf,xip) == -1)
			return(0);
		xip->pktlen = pktlen;
	}
	Mtrace("Xup_done.");
	rawoff();
	return(0);
}


/* Xdown():
 * Called when a transfer from host to target is being made (considered
 * an download).
 * Note that if we don't have INCLUDE_MALLOC set (in config.h), then
 * we allocate a 128-byte static buffer and only support the 128-byte
 * packet size here.
 */

static int
Xdown(struct xinfo *xip)
{
	struct	elapsed_tmr tmr;
	char	c, *tmppkt;
	int		i, done;
#if !INCLUDE_MALLOC
	static char pkt[PKTLEN_128];

	tmppkt = pkt;
#else
	tmppkt = malloc(PKTLEN_1K);
	if (!tmppkt) {
		Mtrace("malloc failed");
		return(-1);
	}
#endif

	rawon();

nextfile:
	if (xip->flags & YMODEM)
		xip->sno = 0x00;
	else
		xip->sno = 0x01;
	xip->pktcnt = 0;
	xip->errcnt = 0;
	xip->xfertot = 0;
	xip->firsterrat = 0;

	/* Startup synchronization... */
	/* Continuously send NAK or 'C' until sender responds. */
restart:
	Mtrace("Xdown");
	for(i=0;i<32;i++) {
		if (xip->flags & USECRC)
			PUTCHAR('C');
		else
			PUTCHAR(NAK);

		startElapsedTimer(&tmr,xip->nakresend * 1000);
		while(!msecElapsed(&tmr) && !gotachar());
		if (gotachar())
			break;
	}
	if (i == 32) {
		Mtrace("Giveup @ %d",__LINE__);
		return(-1);
	}

	done = 0;
	Mtrace("Got response");
	while(done == 0) {
		if (xgetchar(&c,__LINE__) == -1)
			return(-1);
		switch(c) {
		case SOH:				/* 128-byte incoming packet */
			Mtrace("O");
			xip->pktlen = 128;
			done = getPacket((uchar *)tmppkt,xip);
			if (done < 0)
				Mtrace("GP_%d",done);
			if (!done && (xip->pktcnt == 1) && (xip->flags & YMODEM))
				goto restart;
			break;
#if INCLUDE_MALLOC
		case STX:				/* 1024-byte incoming packet */
			Mtrace("T");
			xip->pktlen = 1024;
			done = getPacket((uchar *)tmppkt,xip);
			if (done < 0)
				Mtrace("GP_%d",done);
			if (!done && (xip->pktcnt == 1) && (xip->flags & YMODEM))
				goto restart;
			break;
#endif
		case CAN:
			Mtrace("C");
			done = -1;
			break;
		case EOT:
			Mtrace("E");
			PUTCHAR(ACK);
			if (xip->flags & YMODEM) {
#if INCLUDE_TFS
				if (!xip->size)
					xip->size = xip->pktcnt * xip->pktlen;
				if (xip->fname[0])
					tfsadd(xip->fname,0,0,(uchar *)xip->base,xip->size);
				xip->dataddr = xip->base;
#endif
				goto nextfile;
			}
			else {
				done = xip->xfertot;
				rawoff();
				printf("\nRcvd %d pkt%c (%d bytes)\n",xip->pktcnt,
						xip->pktcnt > 1 ? 's' : ' ',xip->xfertot);

		 		/* If the transfer is complete and no file add is to
				 * be done, then we flush d-cache and invalidate
				 * i-cache across the memory space that was just
				 * copied to.  This is necessary in case the
		 		 * binary data that was just transferred is code.
				 */
				flushDcache((char *)xip->base,xip->xfertot);
				invalidateIcache((char *)xip->base,xip->xfertot);
			}
			break;
		case ESC:		/* User-invoked abort */
			Mtrace("X");
			done = -1;
			break;
		case EOF:		/* 0x1a sent by MiniCom, just ignore it. */
			break;
		default:
			Mtrace("<%02x>",c);
			done = -1;
			break;
		}
		Mtrace("!");
	}
	rawoff();
	if (xip->flags & VERIFY) {
		if (xip->errcnt)
			printf("%d errors, first at 0x%lx\n",
				xip->errcnt,(ulong)(xip->firsterrat));
		else
			printf("verification passed\n");
	}
	free(tmppkt);
	return(done);
}


#endif
