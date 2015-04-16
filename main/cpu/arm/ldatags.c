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
 * ldatags:
 *
 * Load (install) ARM Tags for booting Linux.
 *
 * Original author:     Ed Sutter (ed.sutter@alcatel-lucent.com)
 *
 */
#include "config.h"
#include "stddefs.h"
#include "genlib.h"
#include "ether.h"
#include "cli.h"
#include "tfs.h"
#include "tfsprivate.h"

#define __PACKED__	__attribute__ ((packed))
#define TAG_SIZE(a)		((sizeof(a) + sizeof(struct tag_hdr))/4)
#define STR_LEN(string)	(sizeof(string) - 1)

#define MEM32_SIZE		"mem32_size="
#define MEM32_START		"mem32_start="
#define CORE_FLAGS		"core_flags="
#define CORE_PGSIZE		"core_pgsize="
#define CORE_ROOTDEV	"core_rootdev="
#define INITRD_SIZE		"initrd_size="
#define INITRD_START	"initrd_start="
#define RAMDISK_FLAGS	"ramdisk_flags="
#define RAMDISK_SIZE	"ramdisk_size="
#define RAMDISK_START	"ramdisk_start="
#define SERIAL_HI		"serial_hi="
#define SERIAL_LO		"serial_lo="
#define CMDLINE			"cmdline="
#define CMDLINE_APP		"cmdline_append="
#define REVISION		"revision="

void sno_mac_warning(int);
void initrd_file_warning(int);

#ifdef ATAG_SPACE_DEFINED
extern int atag_space, end_atag_space;
#endif

#define CLISIZE			1024

/* Tag specification defaults:
 */
#define RAMDISK_MAJOR	1
#define PAGE_SIZE		4096
#define PHYS_OFFSET		0x20000000
#define MEM_SIZE		0x01e00000

/* Tag identifiers:
 */
#define ATAG_NONE		0x00000000
#define ATAG_CORE		0x54410001
#define ATAG_MEM		0x54410002
#define ATAG_VIDEOTEXT	0x54410003
#define ATAG_RAMDISK	0x54410004
#define ATAG_INITRD		0x54410005
#define ATAG_INITRD2	0x54420005
#define ATAG_SERIAL		0x54410006
#define ATAG_REVISION	0x54410007
#define ATAG_VIDEOLFB	0x54410008
#define ATAG_CMDLINE	0x54410009
#define ATAG_ACORN		0x41000101
#define ATAG_MEMCLK		0x41000402
#define ATAG_SNOMAC		ATAG_SERIAL

/* Tag structures:
 */
struct tag_hdr {
	ulong size;
	ulong tag;
}__PACKED__;

struct tag_core {
	ulong flags;
	ulong pgsize;
	ulong rootdev;
}__PACKED__;

struct tag_mem32 {
	ulong size;
	ulong start;
}__PACKED__;

union tag_snomac {
	struct tag_serial {
		ulong hi;
		ulong lo;
	} serial;
	struct tag_mac {
		char mac[8];	/* only use first 6 bytes */
	} mac;
}__PACKED__;

struct tag_initrd {
	ulong start;
	ulong size;
}__PACKED__;

struct tag_ramdisk {
	ulong flags;
	ulong size;
	ulong start;
}__PACKED__;

struct tag_cmdline {
	char cmdline[CLISIZE];
}__PACKED__;

struct tag_revno {
	ulong rev;
}__PACKED__;

struct init_tags {
	struct tag_hdr		hdr1;
	struct tag_core		core;
	struct tag_hdr		hdr2;
	struct tag_mem32	mem;
	struct tag_hdr		hdr3;
	union tag_snomac	snomac;
	struct tag_hdr		hdr4;
	struct tag_ramdisk	ramdisk;
	struct tag_hdr		hdr5;
	struct tag_initrd	initrd;
	struct tag_hdr		hdr6;
	struct tag_cmdline	cmdline;
	struct tag_hdr		hdr7;
	struct tag_revno	revno;
	struct tag_hdr		hdr_last;
}__PACKED__;

/* This is the default tag list.  All entries in this list can
 * be overridden by sub-commands within the ldatags command.
 */
static struct init_tags inittag = {
	{ TAG_SIZE(struct tag_core), ATAG_CORE },
	{ 1, PAGE_SIZE, 0xff },

	{ TAG_SIZE(struct tag_mem32), ATAG_MEM },
	{ MEM_SIZE, PHYS_OFFSET },

	{ TAG_SIZE(union tag_snomac), ATAG_SNOMAC },
	{ {0xffffffff, 0xffffffff} },

	{ TAG_SIZE(struct tag_ramdisk), ATAG_RAMDISK },
	{ 0, 0, 0 },

	{ TAG_SIZE(struct tag_initrd), ATAG_INITRD2 },
	{ 0, 0 },

	{ TAG_SIZE(struct tag_cmdline), ATAG_CMDLINE },
	{ {0} },

	{ TAG_SIZE(struct tag_revno), ATAG_REVISION },
	{ 0 },

	{ 0, ATAG_NONE },
};


char *ldatagsHelp[] = {
	"Install ARM tags for Linux startup.",
	"[-a:cf:imv] [sub-cmd1 sub-cmd2 ...]",
	"Options:",
	" -a {addr}    tag list address",
	" -c           clear tag list memory",
	" -f {fname}   initrd filename",
	" -i           init default tag list",
	" -m           load MAC address into serial_no tag",
	" -v           enable verbosity",
	"Sub-commands:",
	"   " CORE_FLAGS "{value}",
	"   " CORE_PGSIZE "{value}",
	"   " CORE_ROOTDEV "{value}",
	"   " MEM32_SIZE "{value}",
	"   " MEM32_START "{value}",
	"   " SERIAL_LO "{value}       (serial hi/lo overrides -m)",
	"   " SERIAL_HI "{value}",
	"   " INITRD_SIZE "{value}     (initrd size/start overrides -f)",
	"   " INITRD_START "{value}",
	"   " RAMDISK_FLAGS "{value}",
	"   " RAMDISK_SIZE "{value}",
	"   " RAMDISK_START "{value}",
	"   " REVISION "{value}",
	"   " CMDLINE "{string}",
	"   " CMDLINE_APP "{string}",
	0
};


int
ldatags(int argc,char *argv[])
{
	TFILE	*tfp;
	char	*eadd, *initrd_fname;
	struct init_tags *tagaddr;
	int		opt, verbose, arg, clear, init, mac, len;
	int		sno_mac_warned, initrd_file_warned;

	initrd_fname = 0;
	sno_mac_warned = initrd_file_warned = 0;
	mac = init = verbose = clear = 0;
#ifdef ATAG_SPACE_DEFINED
	tagaddr = (struct init_tags *)&atag_space;
#else
	tagaddr = (struct init_tags *)getAppRamStart();
#endif
	while((opt=getopt(argc,argv,"a:cf:imv")) != -1) {
		switch(opt) {
		case 'a':	/* Override default tag list address. */
			tagaddr = (struct init_tags *)strtoul(optarg,0,0);
			break;
		case 'c':	/* Clear the tag list space. */
			clear++;
			break;
		case 'f':	/* Initrd filename. */
			initrd_fname = optarg;
			break;
		case 'i':	/* Initialize tag list with defaults. */
			init++;
			break;
		case 'm':	/* Load MAC address into list. */
			mac++;
			break;
		case 'v':	/* Enable verbosity. */
			verbose++;
			break;
		default:
			return(CMD_PARAM_ERROR);
		}
	}

#ifdef ATAG_SPACE_DEFINED
	/* If the ATAG space is allocated externally (usually in rom.lnk file),
	 * then if at this point the tag address still points to that space, 
	 * we must start by verifying that the allocated space is large enough
	 * to hold the init_tags structure...
	 */
	if (tagaddr == (struct init_tags *)&atag_space) {
		if (((int)&end_atag_space - (int)&atag_space) < sizeof(struct init_tags)) {
			printf("Error: the size of struct init_tags (%d) is larger than\n",
				sizeof(struct init_tags));
			printf("       the size of the allocated atag space (%d bytes).\n",
				((int)&end_atag_space - (int)&atag_space));
			return(CMD_FAILURE);
		}
	}
#endif

	if (clear) {
		memset((char *)tagaddr,0,sizeof(struct init_tags));
		if (verbose)
			printf("Tagspace at 0x%lx cleared\n",(long)tagaddr);
		return(CMD_SUCCESS);
	}

	/* If -i specified, then load default tag list for starters...
	 */
	if (init) 
		memcpy((char *)tagaddr,(char *)&inittag,sizeof(struct init_tags));
		
	/* Insert this board's MAC address:
	 */
	if (mac) {
		memset(tagaddr->snomac.mac.mac,0,8);
		eadd = getenv("ETHERADD");
		if (eadd) {
			if (EtherToBin(eadd,(uchar *)tagaddr->snomac.mac.mac) < 0)
				return(CMD_FAILURE);
		}
		else {
			printf("ETHERADD shell var not set.\n");
			return(CMD_FAILURE);
		}
	}

	if (initrd_fname) {
		tfp = tfsstat(initrd_fname);
		if (!tfp) {
			printf("No such file: %s\n",initrd_fname);
			return(CMD_FAILURE);
		}
		tagaddr->initrd.size = (ulong)TFS_SIZE(tfp);
		tagaddr->initrd.start = (ulong)TFS_BASE(tfp);
	}

	/* Process the command line arguments:
	 */
	for(arg=optind;arg<argc;arg++) {
		if (strncmp(argv[arg],CORE_FLAGS,STR_LEN(CORE_FLAGS)) == 0) {
			tagaddr->core.flags =
				strtoul(argv[arg]+STR_LEN(CORE_FLAGS),0,0);
		}
		else if (strncmp(argv[arg],CORE_PGSIZE,STR_LEN(CORE_PGSIZE)) == 0) {
			tagaddr->core.pgsize =
				strtoul(argv[arg]+STR_LEN(CORE_PGSIZE),0,0);
		}
		else if (strncmp(argv[arg],CORE_ROOTDEV,STR_LEN(CORE_ROOTDEV)) == 0) {
			tagaddr->core.rootdev =
				strtoul(argv[arg]+STR_LEN(CORE_ROOTDEV),0,0);
		}
		else if (strncmp(argv[arg],MEM32_SIZE,STR_LEN(MEM32_SIZE)) == 0) {
			tagaddr->mem.size =
				strtoul(argv[arg]+STR_LEN(MEM32_SIZE),0,0);
		}
		else if (strncmp(argv[arg],MEM32_START,STR_LEN(MEM32_START)) == 0) {
			tagaddr->mem.start =
				strtoul(argv[arg]+STR_LEN(MEM32_START),0,0);
		}
		else if (strncmp(argv[arg],INITRD_SIZE,STR_LEN(INITRD_SIZE)) == 0) {
			if (initrd_fname)
				initrd_file_warning(initrd_file_warned++);
			tagaddr->initrd.size =
				strtoul(argv[arg]+STR_LEN(INITRD_SIZE),0,0);
		}
		else if (strncmp(argv[arg],INITRD_START,STR_LEN(INITRD_START)) == 0) {
			if (initrd_fname)
				initrd_file_warning(initrd_file_warned++);
			tagaddr->initrd.start =
				strtoul(argv[arg]+STR_LEN(INITRD_START),0,0);
		}
		else if (strncmp(argv[arg],RAMDISK_FLAGS,STR_LEN(RAMDISK_FLAGS)) == 0) {
			tagaddr->ramdisk.flags =
				strtoul(argv[arg]+STR_LEN(RAMDISK_FLAGS),0,0);
		}
		else if (strncmp(argv[arg],RAMDISK_SIZE,STR_LEN(RAMDISK_SIZE)) == 0) {
			tagaddr->ramdisk.size =
				strtoul(argv[arg]+STR_LEN(RAMDISK_SIZE),0,0);
		}
		else if (strncmp(argv[arg],RAMDISK_START,STR_LEN(RAMDISK_START)) == 0) {
			tagaddr->ramdisk.start =
				strtoul(argv[arg]+STR_LEN(RAMDISK_START),0,0);
		}
		else if (strncmp(argv[arg],SERIAL_HI,STR_LEN(SERIAL_HI)) == 0) {
			if (mac) 
				sno_mac_warning(sno_mac_warned++);
			mac = 0;
			tagaddr->snomac.serial.hi =
				strtoul(argv[arg]+STR_LEN(SERIAL_HI),0,0);
		}
		else if (strncmp(argv[arg],SERIAL_LO,STR_LEN(SERIAL_LO)) == 0) {
			if (mac) 
				sno_mac_warning(sno_mac_warned++);
			mac = 0;
			tagaddr->snomac.serial.lo =
				strtoul(argv[arg]+STR_LEN(SERIAL_LO),0,0);
		}
		else if (strncmp(argv[arg],CMDLINE,STR_LEN(CMDLINE)) == 0) {
			len = strlen(argv[arg]+STR_LEN(CMDLINE));
			if (len > CLISIZE-1)
				printf("Kernel cli too big (%d>%d)\n",len,CLISIZE);
			else
				strcpy(tagaddr->cmdline.cmdline,argv[arg]+STR_LEN(CMDLINE));
		}
		else if (strncmp(argv[arg],CMDLINE_APP,STR_LEN(CMDLINE_APP)) == 0) {
			len = strlen(argv[arg]+STR_LEN(CMDLINE_APP));
			len += strlen(tagaddr->cmdline.cmdline);
			if (len > CLISIZE-1)
				printf("Kernel cli too big (%d>%d)\n",len,CLISIZE);
			else
				strcat(tagaddr->cmdline.cmdline,argv[arg]+STR_LEN(CMDLINE_APP));
		}
		else if (strncmp(argv[arg],REVISION,STR_LEN(REVISION)) == 0) {
			tagaddr->revno.rev =
				strtoul(argv[arg]+STR_LEN(REVISION),0,0);
		}
		else {
			printf("Unrecognized sub-command: %s\n",argv[arg]);
			return(CMD_FAILURE);
		}
	}

	if (verbose) {
		printf("ATAGS (%d bytes) at 0x%lx...\n",
			sizeof(struct init_tags),(long)tagaddr);
		printf(" Core (flags/pgsize/rootdev) = 0x%lx/0x%0lx/0x%lx\n",
			tagaddr->core.flags,tagaddr->core.pgsize,tagaddr->core.rootdev);
		printf(" Mem32 (size/offset) = 0x%08lx/0x%08lx\n",
			tagaddr->mem.size,tagaddr->mem.start);
		if (mac) {
			printf(" Mac = %02x:%02x:%02x:%02x:%02x:%02x\n",
				tagaddr->snomac.mac.mac[0], tagaddr->snomac.mac.mac[1],
				tagaddr->snomac.mac.mac[2], tagaddr->snomac.mac.mac[3],
				tagaddr->snomac.mac.mac[4], tagaddr->snomac.mac.mac[5]);
		}
		else {
			printf(" Serial (hi/lo) = 0x%08lx/0x%08lx\n",
				tagaddr->snomac.serial.hi, tagaddr->snomac.serial.lo);
		}
		printf(" Ramdisk (flags/size/start) = 0x%lx/0x%lx/0x%lx\n",
			tagaddr->ramdisk.flags, tagaddr->ramdisk.size,
			tagaddr->ramdisk.start);

		printf(" Initrd (size/start) = 0x%lx/0x%lx\n",
			tagaddr->initrd.size, tagaddr->initrd.start);

		printf(" Cmdline = <%s>\n",tagaddr->cmdline.cmdline);
	}

	return(CMD_SUCCESS);
}

void
sno_mac_warning(int already_warned)
{
	if (already_warned)
		return;

	printf("Warning: serialno command overrides -m option.\n");
}

void
initrd_file_warning(int already_warned)
{
	if (already_warned)
		return;

	printf("Warning: initrd command overrides -f option.\n");
}
