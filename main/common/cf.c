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
 * cf.c:
 * This code is the user interface portion of the cf (compact flash)
 * command for uMon.
 * This command is intended to be the "interface" portion of some
 * other command (for example "fatfs").  Refer to the discussion in
 * fatfs.c for more details.
 *
 * Original author:     Ed Sutter (ed.sutter@alcatel-lucent.com)
 *
 */
 
#include "config.h"
#if INCLUDE_CF
#include "stddefs.h"
#include "genlib.h"
#include "cli.h"
#include "tfs.h"
#include "tfsprivate.h"
#include "cf.h"


char *CfHelp[] = {
	"Compact Flash Interface",
	"[options] {operation} [args]...",
#if INCLUDE_VERBOSEHELP
	"",
	"Options:",
	" -v     enable verbosity",
	"",
	"Operations:",
	" init [prefix]",
	" read {dest} {blk} {blktot}",
	" write {blk} {src} {blktot}",
#endif
	0
};

static int cfInum;		/* Interface number: to support multiple CF interfaces.
						 * Typically this will always be zero.
						 */

int
CfCmd(int argc, char *argv[])
{
	char *cmd, *buf, *prefix, varname[16];
	int opt, verbose, cfret, blknum, blkcnt;

	verbose = 0;
	while ((opt=getopt(argc,argv,"i:r:w:v")) != -1) {
		switch(opt) {
			case 'i':
				cfInum = atoi(optarg);	/* sticky */
				break;
			case 'v':
				verbose = 1;
				break;
			default:
				return(CMD_PARAM_ERROR);
		}
	}

	if (argc < optind + 1)
		return(CMD_PARAM_ERROR);

	cmd = argv[optind];

	if (strcmp(cmd,"init") == 0) {
		if (argc != optind+2) 
			return(CMD_PARAM_ERROR);

		prefix = argv[optind+1];
		if (strlen(prefix)+4 > sizeof(varname)) {
			printf("prefix %s too long\n",prefix);
			return(CMD_PARAM_ERROR);
		}

		cfret = cfInit(cfInum,verbose);
		if (cfret < 0) {
			printf("cfInit returned %d\n",cfret);
			return(CMD_FAILURE);
		}

		sprintf(varname,"%s_RD",prefix);
		shell_sprintf(varname,"0x%lx",(long)cfRead);

		sprintf(varname,"%s_WR",prefix);
		shell_sprintf(varname,"0x%lx",(long)cfWrite);

		shell_sprintf("CF_BLKSIZ","0x%lx",CF_BLKSIZE);

		if (verbose) {
			printf("read: 0x%lx, write: 0x%lx, blksiz: 0x%lx\n",
				(long)cfRead,(long)cfWrite,(long)CF_BLKSIZE,verbose);
		}
	}
	else if (strcmp(cmd,"read") == 0) {
		if (argc != optind+4) 
			return(CMD_PARAM_ERROR);

		buf = (char *)strtoul(argv[optind+1],0,0);
		blknum = strtoul(argv[optind+2],0,0);
		blkcnt = strtoul(argv[optind+3],0,0);

		cfret = cfRead(cfInum,buf,blknum,blkcnt,verbose);
		if (cfret < 0) {
			printf("cfRead returned %d\n",cfret);
			return(CMD_FAILURE);
		}
	}
	else if (strcmp(cmd,"write") == 0) {
		if (argc != optind+4) 
			return(CMD_PARAM_ERROR);
		buf = (char *)strtoul(argv[optind+1],0,0);
		blknum = strtoul(argv[optind+2],0,0);
		blkcnt = strtoul(argv[optind+3],0,0);

		cfret = cfWrite(cfInum,buf,blknum,blkcnt,verbose);
		if (cfret < 0) {
			printf("cfWrite returned %d\n",cfret);
			return(CMD_FAILURE);
		}
	}
	else {
		printf("cf op <%s> not found\n",cmd);
		return(CMD_FAILURE);
	}

	return(CMD_SUCCESS);
}

#ifdef INCLUDE_CF_DUMMY_FUNCS
/* This code is included here just for simulating the CF
 * interface (temporarily if a real one isn't ready.  In a real system,
 * the INCLUDE_CF_DUMMY_FUNCS definition would be off.
 */

int
cfInit(int interface, int verbose)
{
	if (interface != 0) {
		if (verbose)
			printf("cfInit bad interface %d\n",interface);
		return(-1);
	}

	return(0);
}

int
cfRead(int interface, char *buf, int blk, int blkcnt, int verbose)
{
	char *from;
	int	size;

	if (interface != 0) {
		if (verbose)
			printf("cfRead bad interface %d\n",interface);
		return(-1);
	}

	from = (char *)(blk * CF_BLKSIZE);
	size = blkcnt * CF_BLKSIZE;
	memcpy(buf,from,size);
	return(0);
}

int
cfWrite(int interface, char *buf, int blk, int blkcnt, int verbose)
{
	char *to;
	int	size;

	if (interface != 0) {
		if (verbose)
			printf("cfWrite bad interface %d\n",interface);
		return(-1);
	}

	to = (char *)(blk * CF_BLKSIZE);
	size = blkcnt * CF_BLKSIZE;
	memcpy(to,buf,size);
	return(0);
}

#endif

#endif
