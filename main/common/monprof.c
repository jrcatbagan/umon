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
 * monprof.c:
 *
 * This command and support code allow the monitor to provide an application
 * with some system profiling capabilities.  The function "profiler"
 * is part of the API to allow the application to take advantage of this.
 *
 * The basic assumption is that the application has the ability to insert
 * a call to mon_profiler() in its system tick handler (actually the call
 * can be put anywhere in the application, but this is the most logical
 * place to put it).  The content of the structure passed in to
 * mon_profiler() depends on what kind of profiling the application wants
 * to do.  The tool works partially through calls made by the application
 * into the monitor's api (mon_profiler()) and partially through
 * interaction with the user at the monitor command line.  The runtime
 * profiling calls are done in application space, but the initial setup of
 * the profiling session is done at the CLI.
 * 
 * There are a few different profiling mechanisms in place...
 *
 * FUNCTION LOGGING:
 * This capability makes the same assumption regarding the symtbl file
 * as does strace... That the symbols are listed in the file in ascending
 * address order.  Ideally, the only symbols in the file would be the
 * functions, so some processing can be done on the symtbl file prior to
 * putting it on the target, this would help (not mandatory).  At the CLI,
 * the user issues the "prof init [address]" command.  This initializes
 * a table in the monitor that contains one pdata structure for each entry
 * in the symtbl file.  The pdata structure simply contains the starting
 * address and a pass count for each entry in symtbl. After this
 * initialization, runtime profiling can start.  This is used by setting
 * the MONPROF_FUNCLOG (see monprof.h) flag in the 'type' member of the
 * monprof structure.  Upon entry into the profiler, the 'pc' member of
 * the incoming structure is assumed to contain the address that was
 * executing at the time of the interrupt.  The profiler uses a simple
 * binary search to scan through all entries in the table to find the 
 * symbol (function) that the address falls within.  When this is found,
 * that entry's pass count variable is incremented.  At some point later,
 * profiling is completed and the user can dump the results to show the 
 * user what functions were hogging the CPU during the profiling session.
 *
 *    Example setup:
 *		prof init			# Initialize internals.
 *		prof funccfg		# Configure function logging using symtbl.
 *		prof on				# Enable profiling.
 *  
 *
 * PC LOGGING:
 * This capability assumes that every instruction is the same size (typical
 * for RISC CPUs).  It uses a block of memory equal in size to the .text
 * section of the application and treats that block as a table of counters.
 * Each time the profiler is called, the PC (along with some delta) is
 * used as an offset into the table and that offset is incremented.
 * This may be used as a better alternative to the FUNCTION LOGGING
 * mechanism described above; but it does have some additional requirements,
 * (fixed-width instruction & extra ram space == .text size) so it may not
 * be an option.
 *
 *    Example setup:
 *	prof init			# Initialize internals.
 *	prof pccfg 4 0x22000 0x10000		# Configure pc logging for 32-bit
 *										# instructions. The .text space of
 *										# the application starts at 0x22000
 *										# with a size of 0x10000 bytes.
 *		prof on							# Enable profiling.
 *
 * TASK ID LOGGING:
 * Similar to function logging except that the MONPROF_TIDLOG flag is set
 * in the 'type' member, and the 'tid' member is used...
 * The user initializes with the "prof tinit {count}" command.  The count
 * is the maximum number of unique task ids expected.  The monitor builds
 * another table and as each tid comes in through mon_profiler, if this is
 * the first time mon_profiler() is being called with the specific incoming
 * tid value, then it is added to the list of pdata structures and the pass
 * count is set to 1; otherwise if it has already been logged, only the
 * pass count is incremented.
 *
 *    Example setup:
 *		prof init			# Initialize internals.
 *		prof tidcfg	16		# Configure tid logging for 16 unique tids.
 *		prof on				# Enable profiling.
 *  
 * 
 * Original author:     Ed Sutter (ed.sutter@alcatel-lucent.com)
 *
 */
#include "config.h"
#include "monprof.h"
#if INCLUDE_PROFILER
#include "stddefs.h"
#include "genlib.h"
#include <ctype.h>
#include "cli.h"
#include "tfs.h"
#include "tfsprivate.h"

#define HALF(m)	(m >> 1)

/* pdata:
 * One of these represents each symbol (or tid) in the profiling session. 
 * For MONPROF_FUNCLOG, the data member is the starting address of the symbol
 * and for MONPROF_TIDLOG, the data member is the tid value.
 */
struct pdata {
	ulong	data;			/* Start of symbol or tid. */
	int		pcount;			/* Pass count. */
};

static int prof_Enabled;		/* If set, profiler runs; else return. */ 
static int prof_BadSymCnt;		/* Number of hits not within a symbol. */
static int prof_CallCnt;		/* Number of times profiler was called. */
static int prof_FuncTot;		/* Number of functions being profiled. */
static int prof_TidTot;			/* Number of TIDs being profiled. */
static int prof_TidTally;		/* Number of unique TIDs logged so far. */
static int prof_TidOverflow;	/* More TIDs than the table was built for. */
static int prof_PcTot;			/* Number of instructions being profiled. */
static int prof_PcDelta;		/* Delta between .text space and PC table. */
static int prof_PcWidth;		/* Width (2 or 4) of PC table elements. */
static int prof_PcOORCnt;		/* Out-of-range hit count for PC profiler */
static ulong prof_PcTxtEnd;		/* End of .text area being profiled */
static ulong prof_PcTxtBase;	/* Base of .text area being profiled */

static struct pdata *prof_FuncTbl;
static struct pdata *prof_TidTbl;
static uchar  *prof_PcTbl;
static char prof_SymFile[TFSNAMESIZE+1];

void
profiler(struct monprof *mpp)
{
	struct pdata *current, *base;
	int nmem;

	if (prof_Enabled == 0)
		return;

	if (mpp->type & MONPROF_FUNCLOG) {
		nmem = prof_FuncTot;
		base = prof_FuncTbl;
		while(nmem) {
			current = &base[HALF(nmem)];
			if (mpp->pc < current->data)
				nmem = HALF(nmem);
			else if (mpp->pc > current->data) {
				if (mpp->pc < (current+1)->data) {
					current->pcount++;
					goto tidlog;
				}
				else {
					base = current + 1;
					nmem = (HALF(nmem)) - (nmem ? 0 : 1);
				}
			}
			else {
				current->pcount++;
				goto tidlog;
			}
		}
		prof_BadSymCnt++;
	}
tidlog:
	if (mpp->type & MONPROF_TIDLOG) {
		/* First see if the tid is already in the table.  If it is,
		 * increment the pcount.  If it isn't add it to the table.
		 */
		nmem = prof_TidTally;
		base = prof_TidTbl;
		while(nmem) {
			current = &base[HALF(nmem)];
			if (mpp->tid < current->data)
				nmem = HALF(nmem);
			else if (mpp->tid > current->data) {
				base = current + 1;
				nmem = (HALF(nmem)) - (nmem ? 0 : 1);
			}
			else {
				current->pcount++;
				goto pclog;
			}
		}
		/* Since we got here, the tid must not be in the table, so
		 * do an insertion into the table.  Items are in the table in
		 * ascending order.
		 */
		if (prof_TidTally == 0) {
			prof_TidTbl->pcount = 1;
			prof_TidTbl->data = mpp->tid;
			prof_TidTally++;
		}
		else if (prof_TidTally >= prof_TidTot) {
			prof_TidOverflow++;
		}
		else {
			current = prof_TidTbl + prof_TidTally - 1;
			while(current >= prof_TidTbl) {
				if (mpp->tid > current->data) {
					current++;
					current->pcount = 1;
					current->data = mpp->tid;
					break;
				}
				else {
					*(current+1) = *current;
					if (current == prof_TidTbl) {
						current->pcount = 1;
						current->data = mpp->tid;
						break;
					}
				}
				current--;
			}
			prof_TidTally++;
		}
	}
pclog:
	if (mpp->type & MONPROF_PCLOG) {
		ulong offset;

		if ((mpp->pc > prof_PcTxtEnd) || (mpp->pc < prof_PcTxtBase)) {
			prof_PcOORCnt++;
		}
		else {
			offset = mpp->pc - prof_PcDelta;
			switch(prof_PcWidth) {
				case 2:
					(*(ushort *)offset)++;
					break;
				case 4:
					(*(ulong *)offset)++;
					break;
			}
		}
	}
	prof_CallCnt++;
	return;
}

int
prof_GetSymFile(void)
{
	int	tfd;

	if (prof_SymFile[0] == 0) {
		tfd = SymFileFd(1);
	}
	else {
		tfd = tfsopen(prof_SymFile,TFS_RDONLY,0);
		if (tfd < 0) {
			printf("%s: %s\n",prof_SymFile,(char *)tfsctrl(TFS_ERRMSG,tfd,0));
		}
	}
	return(tfd);
}

void
prof_ShowStats(int minhit, int more)
{
	int		i, tfd, linecount;
	ulong	notused;
	char	symname[64];
	struct 	pdata	*pptr;

	printf("FuncCount Cfg: tbl: 0x%08lx, size: 0x%x\n",
		(ulong)prof_FuncTbl, prof_FuncTot);
	printf("TidCount  Cfg: tbl: 0x%08lx, size: 0x%x\n",
		(ulong)prof_TidTbl, prof_TidTot);
	printf("PcCount   Cfg: tbl: 0x%08lx, size: 0x%x\n",
		(ulong)prof_PcTbl, prof_PcTot*prof_PcWidth);

	if (prof_CallCnt == 0) {
		printf("No data collected%s",
			prof_Enabled == 0 ? " (profiling disabled)\n" : "\n");
		return;
	}
	linecount = 0;
	tfd = prof_GetSymFile();
	if ((prof_FuncTbl) && (prof_FuncTot > 0)) {
		printf("\nFUNC_PROF stats:\n");
		pptr = prof_FuncTbl;
		for(i=0;i<prof_FuncTot;pptr++,i++) {
			if (pptr->pcount < minhit)
				continue;
			if ((tfd < 0) ||
				(AddrToSym(tfd,pptr->data,symname,&notused) == 0)) {
				printf(" %08lx    :  %d\n",pptr->data,pptr->pcount);
			}
			else {
				printf(" %-25s:  %d\n",symname,pptr->pcount);
			}
			if ((more) && (++linecount >= more)) {
				linecount = 0;
				if (More() == 0)
					goto showdone;
			}
		}
	}
	if ((prof_TidTbl) && (prof_TidTot > 0)) {
		printf("\nTID_PROF stats:\n");
		pptr = prof_TidTbl;
		for(i=0;i<prof_TidTot;pptr++,i++) {
			if (pptr->pcount < minhit)
				continue;
			printf(" %08lx    :  %d\n",pptr->data,pptr->pcount);
			if ((more) && (++linecount >= more)) {
				linecount = 0;
				if (More() == 0)
					goto showdone;
			}
		}
	}
	if (prof_PcTbl) {
		ushort *sp;
		ulong  *lp;

		sp = (ushort *)prof_PcTbl;
		lp = (ulong *)prof_PcTbl;
		printf("\nPC_PROF stats:\n");
		for(i=0;i<prof_PcTot;i++) {
			switch(prof_PcWidth) {
				case 2:
					if (*sp >= minhit) {
						printf(" %08x    :  %d\n",
							(int)sp + prof_PcDelta,*sp);
						linecount++;
					}
					sp++;
					break;
				case 4:
					if (*lp >= minhit) {
						printf(" %08x    :  %ld\n",
							(int)lp + prof_PcDelta,*lp);
						linecount++;
					}
					lp++;
					break;
			}
			if ((more) && (linecount >= more)) {
				linecount = 0;
				if (More() == 0)
					goto showdone;
			}
		}
	}
showdone:
	putchar('\n');
	if (prof_BadSymCnt)
		printf("%d out-of-range symbols\n",prof_BadSymCnt);
	if (prof_TidOverflow)
		printf("%d tid overflow attempts\n",prof_TidOverflow);
	if (prof_PcOORCnt)
		printf("%d pc out-of-range hits\n",prof_PcOORCnt);
	printf("%d total profiler calls\n",prof_CallCnt);

	if (tfd >= 0)
		tfsclose(tfd,0);
	return;
}

/* prof_FuncConfig():
 * This function builds a table of pdata structures based on the 
 * content of the symbol table.
 * It assumes the file is a list of symbols and addresses listed
 * in ascending address order.
 */
void
prof_FuncConfig(void)
{
	int		tfd, i;
	struct	pdata *pfp;
	char	line[80], *space;

	tfd = prof_GetSymFile();
	if (tfd < 0)
		return;

	prof_FuncTot = 0;
	pfp = prof_FuncTbl;
	
	while(tfsgetline(tfd,line,sizeof(line)-1)) {
		space = strpbrk(line,"\t ");
		if (!space)
			continue;
		*space++ = 0;
		while(isspace(*space))
			space++;
		pfp->data = strtoul(space,0,0);
		pfp->pcount = 0;
		pfp++;
		prof_FuncTot++;
	}
	tfsclose(tfd,0);

	/* Add one last item to the list so that there is an upper limit for
	 * the final symbol in the table:
	 */
	pfp->data = 0xffffffff;
	pfp->pcount = 0;

	/* Test to verify that all symbols are in ascending address order...
	 */
	for (i=0;i<prof_FuncTot;i++) {
		if (prof_FuncTbl[i].data > prof_FuncTbl[i+1].data) {
			printf("Warning: function addresses not in order\n");
			break;
		}
	}
	prof_FuncTot++;
}

/* prof():
 */

char *ProfHelp[] = {
	"Profiler configuration and result display",
	"-[h:m:s:] [operation] [op-specific args]",
#if INCLUDE_VERBOSEHELP
	"Operations:",
	" on                  enable profiler",
	" off                 disable profiler",
	" show                dump stats",
	" init                clear internal tables and runtime stats",
	" call {type pc tid}  call profiler from CLI",
	"                     ('type' can be any combination of 't', 'f' & 'p')",
	" restart             clear runtime stats only",
	" tidcfg {tidtot}     init tid profiler based on number of task ids",
	" funccfg             init function profiler from symtbl file",
	" pccfg {wid add siz} init pc profiler with instruction width (2 or 4)",
	"                     plus size and addr of text area",
	"",
	"Options:",
	" -a{#}        address to use for table",
	" -h{#}        minimum hit count for show",
	" -m{#}        line count for output throttling in show",
	" -s{symfile}  use this file for symbols instead of default",
#endif
	0,
};

int
Prof(int argc,char *argv[])
{
	char	*arg1, *arg2, *arg3, *arg4;
	ulong	address;
	int		i, opt, minhit, ret, more;

	ret = CMD_SUCCESS;
	minhit = 1;
	more = 0;
	address = 0;
	while((opt=getopt(argc,argv,"a:h:m:s:")) != -1) {
		switch(opt) {
		case 'a':
			address = strtoul(optarg,0,0);
			break;
		case 'h':
			minhit = atoi(optarg);
			break;
		case 'm':
			more = atoi(optarg);
			break;
		case 's':
			strncpy(prof_SymFile,optarg,TFSNAMESIZE);
			prof_SymFile[TFSNAMESIZE] = 0;
			break;
		default:
			return(CMD_PARAM_ERROR);
		}
	}

	arg1 = argv[optind];
	arg2 = argv[optind+1];
	arg3 = argv[optind+2];
	arg4 = argv[optind+3];

	if (argc == optind+1) {
		if (!strcmp(arg1,"on")) {
			prof_Enabled = 1;
		}
		else if (!strcmp(arg1,"off")) {
			prof_Enabled = 0;
		}
		else if (!strcmp(arg1,"show")) {
			prof_ShowStats(minhit, more);
		}
		else if (!strcmp(arg1,"init")) {
			prof_BadSymCnt = 0;
			prof_CallCnt = 0;
			prof_TidTally = 0;
			prof_Enabled = 0;
			prof_FuncTot = 0;
			prof_TidTot = 0;
			prof_PcTot = 0;
			prof_FuncTbl = (struct pdata *)0;
			prof_TidTbl = (struct pdata *)0;
			prof_PcTbl = (uchar *)0;
			prof_PcWidth = 0;
			prof_PcDelta = 0;
			prof_SymFile[0] = 0;
		}
		else if (!strcmp(arg1,"restart")) {
			prof_BadSymCnt = 0;
			prof_CallCnt = 0;
			prof_TidTally = 0;
			if (prof_FuncTbl) {
				for(i=0;i<prof_FuncTot;i++) 
					prof_FuncTbl[i].pcount = 0;
			}
			if (prof_TidTbl) {
				for(i=0;i<prof_TidTot;i++) 
					prof_TidTbl[i].pcount = 0;
					prof_TidTbl[i].data = 0;
			}
			if (prof_PcTbl) {
				memset((char *)prof_PcTbl,0,prof_PcTot*prof_PcWidth);
			}
		}
		else if (!strcmp(arg1,"funccfg")) {
			if (prof_FuncTbl) {
				printf("Already configured, run init to re-configure\n");
				return(CMD_FAILURE);
			}
			else if (prof_PcTbl) {
				printf("Can't use PC and FUNC profiling simultaneously\n");
				return(CMD_FAILURE);
			}
			else if (address) {
				prof_FuncTbl = (struct pdata *)address;
			}
			else if (prof_TidTbl) {
				prof_FuncTbl = &prof_TidTbl[prof_TidTot];
			}
			else
				prof_FuncTbl = (struct pdata *)getAppRamStart();
			
			prof_FuncConfig();
		}
		else
			ret = CMD_PARAM_ERROR;
	}
	else if (argc == optind+2) {
		if (!strcmp(arg1,"tidcfg")) {
			if (prof_TidTbl) {
				printf("Already configured, run init to re-configure\n");
				return(CMD_FAILURE);
			}
			else if (address) {
				prof_TidTbl = (struct pdata *)address;
			}
			else if (prof_FuncTbl) {
				prof_TidTbl = &prof_FuncTbl[prof_FuncTot];
			}
			else if (prof_PcTbl) {
				prof_TidTbl = (struct pdata *)&prof_PcTbl[prof_PcTot];
			}
			else
				prof_TidTbl = (struct pdata *)getAppRamStart();
			prof_TidTot = strtoul(arg2,0,0);
			for(i=0;i<prof_TidTot;i++) {
				prof_TidTbl[i].data = 0;
				prof_TidTbl[i].pcount = 0;
			}
		}
		else
			ret = CMD_PARAM_ERROR;
	}
	else if (argc == optind+4) {
		if (!strcmp(arg1,"call")) {
			struct	monprof	mp;

			mp.type = 0;

			if (strchr(arg2,'f'))
				mp.type |= MONPROF_FUNCLOG;
			if (strchr(arg2,'p'))
				mp.type |= MONPROF_PCLOG;
			if (strchr(arg2,'t'))
				mp.type |= MONPROF_TIDLOG;

			mp.pc = strtoul(arg3,0,0);
			mp.tid = strtoul(arg4,0,0);

			profiler(&mp);
		}
		else if (!strcmp(arg1,"pccfg")) {
			int size;

			if (prof_PcTbl) {
				printf("Already configured, run init to re-configure\n");
				return(CMD_FAILURE);
			}
			else if (prof_FuncTbl) {
				printf("Can't use PC and FUNC profiling simultaneously\n");
				return(CMD_FAILURE);
			}
			else if (address) {
				prof_PcTbl = (uchar *)address;
			}
			else if (prof_TidTbl) {
				prof_PcTbl = (uchar *)&prof_TidTbl[prof_TidTot];
			}
			else
				prof_PcTbl = (uchar *)getAppRamStart();
			prof_PcWidth = strtol(arg2,0,0);	/* instruction width */
			prof_PcTxtBase = strtol(arg3,0,0);	/* address of .text */
			size = strtol(arg4,0,0);			/* size of .text */
			prof_PcTxtEnd = prof_PcTxtBase + size;
			prof_PcTot = size / prof_PcWidth;
			prof_PcDelta = (ulong)prof_PcTxtBase - (ulong)prof_PcTbl;
			memset((char *)prof_PcTbl,0,prof_PcTot*prof_PcWidth);

		}
		else
			ret = CMD_PARAM_ERROR;
	}
	else
		ret = CMD_PARAM_ERROR;
		
	return(ret);
}
#else

void
profiler(struct monprof *mpp)
{
}

#endif
