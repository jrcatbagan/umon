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
 * strace.c:
 *
 * Note that this stack trace can be used to trace REAL exceptions or
 * user-induced exceptions.  If one is user-induced, then the user is
 * typically just replacing an instruction at the point at which the break
 * is to occur with a SC (syscall) instruction.  The point at which this
 * insertion is made must be after the function sets up its stack frame
 * otherwise it is likely that the trace will be bogus.
 *
 * BTW... the SC instruction is 0x44000002.
 *
 * Original author:     Ed Sutter (ed.sutter@alcatel-lucent.com)
 *
 */
#include "config.h"
#if INCLUDE_STRACE
#include "tfs.h"
#include "tfsprivate.h"
#include <ctype.h>
#include "genlib.h"
#include "stddefs.h"

char *StraceHelp[] = {
    "Stack trace",
    "-[d:rs:v]",
    " -d #      max depth count (def=20)",
    " -r        dump regs",
    " -s {func} stop at function 'func'",
    " -v        verbose",
    0,
};

int
Strace(int argc,char *argv[])
{
	TFILE	*tfp;
	char	*symfile, *stopat, fname[64];
	ulong	*framepointer, pc, fp, offset;
    int		tfd, opt, maxdepth, pass, verbose;

	tfd = fp = 0;
	maxdepth = 20;
	verbose = 0;
	stopat = 0;
	pc = ExceptionAddr;
    while ((opt=getopt(argc,argv,"d:rs:v")) != -1) {
		switch(opt) {
		case 'd':
			maxdepth = atoi(optarg);
			break;
		case 'r':
			showregs();
			break;
		case 's':
			stopat = optarg;
			break;
		case 'v':
			verbose = 1;
			break;
	    default:
			return(0);
		}
	}
	
	if (!fp)
		getreg("R11", &fp);

	framepointer = (ulong *)fp;

	/* Start by detecting the presence of a symbol table file... */
	symfile = getenv("SYMFILE");
	if (!symfile)
		symfile = SYMFILE;

	tfp = tfsstat(symfile);
	if (tfp)  {
		tfd = tfsopen(symfile,TFS_RDONLY,0);
		if (tfd < 0)
			tfp = (TFILE *)0;
	}

	/* Show current position: */
	printf("   0x%08lx",pc);
	if (tfp) {
		AddrToSym(tfd,pc,fname,&offset);
		printf(": %s()",fname);
		if (offset)
			printf(" + 0x%lx",offset);
	}
	putchar('\n');

	/* Now step through the stack frame... */
	pass = 0;
	while(maxdepth) {
		if (pass != 0) 
			framepointer = (ulong *)*(framepointer - 3);
		
		pc = *(framepointer - 1);

		if (verbose) {
			printf("fp=0x%lx,*fp=0x%lx,pc=%lx\n", (ulong)framepointer,
				(ulong)*framepointer,pc);
		}

		if (((ulong)framepointer & 3) || (!framepointer) ||
			(!*framepointer) || (!pc)) {
			break;
		}

		printf("   0x%08lx",pc);
		if (tfp) {
			int match;

			match = AddrToSym(tfd,pc,fname,&offset);
			printf(": %s()",fname);
			if (offset)
				printf(" + 0x%lx",offset);
			if ((!match) || ((stopat != 0) && (strcmp(fname,stopat) == 0))) {
				putchar('\n');
				break;
			}
		}
		putchar('\n');
		maxdepth--;
		pass++;
	}

	if (!maxdepth)
		printf("Max depth termination\n");
	
	if (tfp) {
		tfsclose(tfd,0);
	}
    return(0);
}
#endif
