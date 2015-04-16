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
 * gdb.c:
 *
 * The code in this file allows a gdb debugger on a host to connect to the
 * monitor.  It supports both serial and network (udp) connections. 
 * Note that this is only the cpu-independent portion of the GDB debug
 * protocol.  The following commands in gdb have been verified to work
 * with this code:
 *
 *	- target remote com1
 *	- target remote udp:135.222.140.68:1234
 *	- load and c 
 *	- info registers
 *	- x/16x 0xff800000
 *	- print varname
 *
 *I'm sure other commands work, but these are the ones I've tested.
 *
 * This interface was written from scratch.
 * References used were:
 * Bill Gatliff's ESP article and a description of the GDB Remote
 * Serial Protocol I found at:
 * http://developer/apple.com/documentation/DeveloperTools/gdb/gdb/gdb_32.htm
 *
 * Original author:     Ed Sutter (ed.sutter@alcatel-lucent.com)
 *
 */
#include "config.h"
#include "genlib.h"
#include "ether.h"
#include "stddefs.h"
#include "endian.h"
#include "cli.h"

#if INCLUDE_GDB

#include "gdbregs.c"	/* CPU-specific register name table */

#define REGTBL_SIZE			(sizeof(gdb_regtbl)/sizeof(char *))

#define GDBERR_NOHASH		1	/* no '#' in command */
#define GDBERR_BADCSUM		2	/* bad checksum */
#define GDBERR_GENERR		3	/* general confusion */
#define GDBERR_BADXFMT		4	/* unexpected 'X' command format */
#define GDBERR_RNUMOOR		5	/* register number out of range */
#define GDBERR_NOSPACE		6	/* buffer not big enough for response */

/* gdbIbuf[]:
 * Input buffer used for storage of the incoming command from
 * the gdb debugger.
 */
#if INCLUDE_ETHERNET
static	uchar gdbIbuf[512];
#endif

/* gdbRbuf[]:
 * Response buffer used for the complete response destined
 * for the gdb host.
 */
static	uchar	gdbRbuf[1024];
static	int		gdbRlen;
static	void	(*gdbContinueFptr)();

/* gdbUdp:
 * Set if the gdb interaction is via UDP, else zero.
 * Obviously this code assumes there is no reentrancy to deal with.
 */
static int gdbUdp;

/* gdbTrace():
 * This is a function pointer that is loaded with either printf
 * or Mtrace... Use printf if gdb is running via UDP; else use
 * printf.
 */
static int	(*gdbTrace)(char *, ...);

/* gdb_response():
 * Utility function used by the other response functions to format
 * the complete response back to the gdb host.  All interaction with
 * the host is in ASCII.  The response message is preceded by '+$'
 * and and terminated with '#CC' where 'CC' is a checksum.
 */
int
gdb_response(char *line)
{
	uchar	csum, *resp;
	
	csum = 0;
	resp = gdbRbuf;
	*resp++ = '$';
	while(*line) {
		csum += *line;
		*resp++ = *line++;
	}
	resp += sprintf((char *)resp,"#%02x",csum);
	*resp = 0;

	/* If gdbUdp is clear, then we assume that the gdb host is tied
	 * to the target's serial port, so just use printf to send the
	 * response. 
	 * If gdbUdp is set, then we assume the calling function will
	 * send the response (via ethernet).
	 */
	if (!gdbUdp)
		printf((char *)gdbRbuf);

	gdbRlen = strlen((char *)gdbRbuf);

	if (gdbRlen < 128)
		gdbTrace("GDB_RSP: %s\n",gdbRbuf);
	else
		gdbTrace("GDB_RSP: BIG\n");

	return(gdbRlen);
}

int
gdb_ok(void)
{
	return(gdb_response("OK"));
}

int
gdb_sig(int signal)
{
	char buf[8];

	sprintf(buf,"S%02d",signal);
	return(gdb_response(buf));
}

int
gdb_err(int errno)
{
	char buf[8];

	sprintf(buf,"E%02d",errno);
	return(gdb_response(buf));
}

/* gdb_m():
 * GDB memory read command...
 * Incoming command format is...
 *
 *		mADDR,LEN#CC
 *
 * where:
 *	'm'		is the "memory read" request
 *	'ADDR'	is the address from which the data is to be read
 *	'LEN'	is the number of bytes to be read
 *
 */
int
gdb_m(char *line)
{
	int		len, i;
	char 	*lp;
	uchar	*addr, *resp, buf[128];
	
	addr = (uchar *)strtol(line+1,&lp,16);
	len = (int)strtol(lp+1,0,16);
	if (len) {
		if (len*2 >= sizeof(buf)) {
			gdb_err(GDBERR_NOSPACE);
		}
		else {
			resp = buf;
			for(i=0;i<len;i++,addr++)
				resp += sprintf((char *)resp,"%02x",*addr);
			gdb_response((char *)buf);
		}
	}
	else
		gdb_ok();
	return(0);
}

/* gdb_M():
 * GDB memory write command...
 * Incoming command format is...
 *
 *		MADDR,LEN:DATA#CC
 *
 * where:
 *	'M'		is the "memory read" request
 *	'ADDR'	is the address from which the data is to be read
 *	'LEN'	is the number of bytes to be read
 *	'DATA'	is the ascii data
 *
 * STATUS: This function has been tested with m68k-elf-gdb (xtools)
 * and appears to work ok.
 */
int
gdb_M(char *line)
{
	int		len, i;
	char	*lp;
	uchar	*addr, buf[3];
	
	addr = (uchar *)strtol(line+1,&lp,16);
	len = (int)strtol(lp+1,&lp,16);
	lp++;

	buf[2] = 0;
	for(i=0;i<len;i++) {
		buf[0] = *lp++;
		buf[1] = *lp++;
		*addr++ = (uchar)strtol((char *)buf,0,16);
	}
	gdb_ok();
	return(0);
}

/* gdb_X():
 * Similar to gdb_M() except that the data is in binary.
 * The characters '$', '#' and 0x7d are escaped using 0x7d.
 */
int
gdb_X(char *line)
{
	int		len, i;
	char	*lp;
	uchar	*addr;
	
	addr = (uchar *)strtol(line+1,&lp,16);
	len = (int)strtol(lp+1,&lp,16);
	lp++;

	for(i=0;i<len;i++) {
		if ((*lp == 0x7d) && 
			((*(lp+1) == 0x03) || (*(lp+1) == 0x04) || (*(lp+1) == 0x5d))) {
			*addr++ = *(lp+1) | 0x20;
			lp += 2;
		}
		else
			*addr++ = *lp++;
	}

	gdb_ok();
	return(0);
}

/* gdb_q():
 * Query.  Not sure what this is for, but according to Gatliff's
 * article, just do it...
 */
int
gdb_q(char *line)
{
	line++;

	if (strncmp(line,"Offsets",7) == 0)
		return(gdb_response("Text=0;Data=0;Bss=0"));
	else
		return(gdb_ok());
}

/* gdb_g():
 * Get all registers. 
 * GDB at the host expects to see a buffer of ASCII-coded hex values
 * with each register being 8-bytes (forming one 32-bit value).
 * The order of these registers is defined by the table included
 * in the file gdbregs.c above.
 */

int
gdb_g(char *line)
{
	int		i;
	ulong	reg;
	char	*resp, buf[(REGTBL_SIZE * 8) + 1];

	resp = buf;
	for(i=0;i<REGTBL_SIZE;i++) {
		if (gdb_regtbl[i] != 0)
			getreg(gdb_regtbl[i],&reg);
		else
			reg = 0;
		self_ecl(reg);
		resp += sprintf(resp,"%08lx",reg);
	}
	return(gdb_response(buf));
}

/* gdb_P():
 * Store to a register.
 */
int
gdb_P(char *line)
{
	char *lp;
	int rnum;
	ulong rval;

	line++;
	rnum = strtol(line,&lp,16);
	if (rnum >= REGTBL_SIZE) {
		gdb_err(GDBERR_RNUMOOR);	
		return(-1);
	}
	lp++;
	rval = strtol(lp,0,16);
	self_ecl(rval);
	putreg(gdb_regtbl[rnum],rval);

	gdb_ok();
	return(0);
}

/* gdb_c():
 * This is the function that is called as a result of the 'c' (continue)
 * command. 
 */ 
int
gdb_c(char *line)
{
	ulong	addr;
	void	(*func)();

	line++;
	if (*line == '#')
		getreg(CPU_PC_REG,&addr);
	else
		addr = strtol(line,0,16);

	func = (void(*)())addr;
	func();
	return(0);
}

/* gdb_cmd():
 *	First function called out of the monitor's command interpreter.  It
 *	does a basic syntax verification and then passes parameters to the
 *	appropriate handler above.
 *	Incoming syntax is
 *
 *		$ CMD # CSUM (of CMD)
 *
 *	where:
 *		$		is the ascii '$' character (0x24)
 *		#		is the ascii '#' character (0x23)
 *		CMD		is some command line consisting of a command and arguments
 *		CSUM	is the checksum of the characters in CMD
 *
 *	for example:
 *		
 *		$m4015bc,2#5a
 *
 *	Returns...
 *		 0 if command is not processed;
 *		 1 if command is processed;
 *		-1 if command is processed but has an error;
 *
 *	If this code detects an error, then send an error code back to GDB.
 *	According to the article, there are no defined error codes in GDB so
 *	we will use the following...
 *		1 	indicates a missing '#' at the end of the incoming cmd string.
 *		2 	indicates a bad checksum calculation.
 *		3 	indicates some command processing error.
 *		4 	indicates bad 'X' command parsing.
 */
int
gdb_cmd(uchar *line)
{
	char 	*comma, *colon, *cp, *bp, buf[32];
	int		len, clen, err, i;
	uchar	mycsum, incsum;

	gdbContinueFptr = (void(*)())0;

	/* If the command is 'X', then we have to treat it "special" because
	 * it contains binary data...
	 */
	if (line[1] == 'X') {
		comma = strchr((char *)line,',');
		colon = strchr((char *)line,':');
		if ((comma) && (colon)) {
			bp = buf;
			cp = (char *)line;
			while(cp <= colon)
				*bp++ = *cp++;
			*bp = 0;
			gdbTrace("GDB_CMD: '%s'\n",buf);
		}
		else {
			gdbTrace("GDB_CMD: 'X'\n");
			gdb_err(GDBERR_BADXFMT);	/* Unexpected 'X' command format */
		}
	}
	else if (line[0] == 0x03) {
		gdbTrace("GDB_CTRLC\n");
		gdb_sig(2);
		return(1);
	}
	else {
		gdbTrace("GDB_CMD: '%s'\n",line);
		len = strlen((char *)line);

		if (line[len-3] != '#') {
			gdb_err(GDBERR_NOHASH);		/* Missing ending '#' */
			return(-1);
		}

		clen = len - 3;
		mycsum = 0;
		for(i=1;i<clen;i++)
			mycsum += line[i];

		incsum = (uchar)strtol((char *)line+len-2,(char **)0,16);
		if (mycsum != incsum) {
			gdb_err(GDBERR_BADCSUM);	/* Checksum failure */
			return(-1);
		}
	}

	err = 0;
	line++;
	switch(*line) {
		case 'm':		/* Memory read */
			err = gdb_m((char *)line);
			break;
		case 'M':		/* Memory write (Ascii-coded-hex) */
			err = gdb_M((char *)line);
			break;
		case 'X':		/* Memory write (Binary) */
			err = gdb_X((char *)line);
			break;
		case 's':		/* Step */
			gdb_response("S05");
			break;
		case 'c':		/* Continue */
			gdb_c((char *)line);
			break;
		case '?':		/* Last signal */
			gdb_response("S05");
			break;
		case 'g':		/* get all registers */
			gdb_g((char *)line);
			break;
		case 'q':		/* Query */
			gdb_q((char *)line);
			break;
		case 'P':		/* PRR=HHHHHHHH... reg*/
			gdb_P((char *)line);
			break;
		case 'H':		/* Thread */
			gdb_ok();
			break;
		case 'k':		/* Quit */
			gdb_ok();
			break;
		default:		/* Unknown... return empty response. */
			gdb_response("");
			break;
	}
	if (err) {
		gdb_err(GDBERR_GENERR);			/* Command processing error */
	}
	return(1);
}

/* Gdb():
 * This is the command at the CLI that allows the monitor to connect
 * to a gdb debugger via the serial port.  It currently assumes that
 * the console port is the same port as is being used for the gdb
 * connection.  Eventually this needs to be modified to provide an 
 * option for the gdb protocol to run on some serial port other than
 * the console.
 *
 * The connection command in gdb to connect to via serial port (PC)
 * is:
 *	target remote com1
 */
char *GdbHelp[] = {
	"Enter gdb mode",
	"(no options)",
	0,
};

int
Gdb(int argc, char *argv)
{
	int		state, quit;
	uchar	*lp, c;
	static	uchar	line[1024];

	printf("Entering GDB mode, to exit manually type: '$k#00'\n");

	lp = (uchar *)0;
	quit = 0;
	state = 0;
	gdbUdp = 0;
	gdbTrace = Mtrace;
	while(!quit) {
		c = getchar();
		switch(state) {
			case 0:				/* Wait for start of message */
				if (c == '$') {
					lp = line;
					*lp++ = c;
					state = 1;
				}
				break;
			case 1:	
				*lp++ = c;		/* This is the command character */
				state = 2;
				break;
			case 2:
				if (c == '#') {
					state = 3;
					*lp++ = c;
				}
				else {
					*lp++ = c;
				}
				break;
			case 3:
				*lp++ = c;
				state = 4;
				break;
			case 4:
				*lp++ = c;
				*lp = 0;
				state = 0;
				if (line[1] == 'k')
					quit = 1;
				gdb_cmd(line);
				break;
			default:
				break;
		}
	}
	putchar('\n');
	return(CMD_SUCCESS);
}

#if INCLUDE_ETHERNET
/* processGDB():
 * This is the function that allows a remote gdb host to connect to
 * the monitor with gdb at the udp level.  The connection command in
 * gdb to do this is:
 *
 *	target remote udp:TARGET_IP:TARGET_PORT
 */
int
processGDB(struct ether_header *ehdr,ushort size)
{
	char	*gdbp;
	struct	ip *ihdr, *ti, *ri;
	struct	Udphdr *uhdr, *tu, *ru;
	struct	ether_header *te;

	/* If SHOW_GDB is set (via ether -vg), then we dump the trace to
	 * the console; otherwise, we use mtrace.
	 */
#if INCLUDE_ETHERVERBOSE
	if (EtherVerbose & SHOW_GDB)
		gdbTrace = printf;
	else
#endif
		gdbTrace = Mtrace;

	ihdr = (struct ip *)(ehdr + 1);
	uhdr = (struct Udphdr *)((char *)ihdr + IP_HLEN(ihdr));
	gdbp = (char *)(uhdr + 1);
	size = ecs(uhdr->uh_ulen) - sizeof(struct Udphdr);

	/* Check for ACK/NAK here:
	 */
	if (size == 1) {
		if ((*gdbp == '+') || (*gdbp == '-')) {
			gdbTrace("GDB_%s\n",*gdbp == '+' ? "ACK" : "NAK");
			return(0);
		}
	}

	/* Copy the incoming udp payload (the gdb command) to gdbIbuf[]
	 * and NULL terminate it...
	 */
	memcpy((char *)gdbIbuf,(char *)gdbp,size);
	gdbIbuf[size] = 0;

	/* Now that we've stored away the GDB command request, we
	 * initially respond with the GDB acknowledgement ('+')...
	 */
	te = EtherCopy(ehdr);
	ti = (struct ip *) (te + 1);
	ri = (struct ip *) (ehdr + 1);
	ti->ip_vhl = ri->ip_vhl;
	ti->ip_tos = ri->ip_tos;
	ti->ip_len = ecs((1 + (sizeof(struct ip) + sizeof(struct Udphdr))));
	ti->ip_id = ipId();
	ti->ip_off = ri->ip_off;
	ti->ip_ttl = UDP_TTL;
	ti->ip_p = IP_UDP;
	memcpy((char *)&(ti->ip_src.s_addr),(char *)BinIpAddr,
		sizeof(struct in_addr));
	memcpy((char *)&(ti->ip_dst.s_addr),(char *)&(ri->ip_src.s_addr),
		sizeof(struct in_addr));
	tu = (struct Udphdr *) (ti + 1);
	ru = (struct Udphdr *) (ri + 1);
	tu->uh_sport = ru->uh_dport;
	tu->uh_dport = ru->uh_sport;
	tu->uh_ulen = ecs((ushort)(sizeof(struct Udphdr) + 1));
	gdbp = (char *)(tu+1);
	*gdbp = '+';

	ipChksum(ti);		/* Compute checksum of ip hdr */
	udpChksum(ti);		/* Compute UDP checksum */

	sendBuffer(sizeof(struct ether_header) + sizeof(struct ip) + 
		sizeof(struct Udphdr) + 1);

	/* Wrap the processing of the incoming packet with a set/clear
	 * of the gdbUdp flag so that the other gdb code can act 
	 * accordingly.
	 */
	gdbUdp = 1;
	if (gdb_cmd(gdbIbuf) == 0) {
		gdbUdp = 0;
		return(0);
	}
	gdbUdp = 0;

	/* Add 1 to the gdbRlen to include the NULL termination.
	 */
	gdbRlen++;	


	/* The second respons is only done if gdb_cmd returns non-zero.
	 * It is the response to the gdb command issued by the debugger
	 * on the host.
	 */
	te = EtherCopy(ehdr);
	ti = (struct ip *) (te + 1);
	ri = (struct ip *) (ehdr + 1);
	ti->ip_vhl = ri->ip_vhl;
	ti->ip_tos = ri->ip_tos;
	ti->ip_len = ecs((gdbRlen + (sizeof(struct ip) + sizeof(struct Udphdr))));
	ti->ip_id = ipId();
	ti->ip_off = ri->ip_off;
	ti->ip_ttl = UDP_TTL;
	ti->ip_p = IP_UDP;
	memcpy((char *)&(ti->ip_src.s_addr),(char *)BinIpAddr,
		sizeof(struct in_addr));
	memcpy((char *)&(ti->ip_dst.s_addr),(char *)&(ri->ip_src.s_addr),
		sizeof(struct in_addr));

	tu = (struct Udphdr *) (ti + 1);
	ru = (struct Udphdr *) (ri + 1);
	tu->uh_sport = ru->uh_dport;
	tu->uh_dport = ru->uh_sport;
	tu->uh_ulen = ecs((ushort)(sizeof(struct Udphdr) + gdbRlen));
	memcpy((char *)(tu+1),(char *)gdbRbuf,gdbRlen);

	ipChksum(ti);		/* Compute checksum of ip hdr */
	udpChksum(ti);		/* Compute UDP checksum */

	sendBuffer(sizeof(struct ether_header) + sizeof(struct ip) + 
		sizeof(struct Udphdr) + gdbRlen);

	return(1);
}
#endif

#endif

