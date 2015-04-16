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
 * strace_template.c:
 *
 * Template for creating cpu-specific stack-trace command.
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
#include "cpu.h"

char *StraceHelp[] = {
    "Stack trace",
    "-[d:F:P:rs:v]",
    " -d #   max depth count (def=20)",
    " -F #   specify frame-pointer (don't use content of A6)",
    " -P #   specify PC (don't use content of PC)",
    " -r     dump regs",
    " -v     verbose",
    0,
};

int
Strace(int argc,char *argv[])
{
	char	*symfile, fname[64];
	TFILE	*tfp;
	ulong	*framepointer, pc, fp, offset;
    int		tfd, opt, maxdepth, pass, verbose, bullseye;

	tfd = fp = 0;
	maxdepth = 20;
	verbose = 0;
	pc = ExceptionAddr;
    while ((opt=getopt(argc,argv,"d:F:P:rs:v")) != -1) {
		switch(opt) {
		case 'd':
			maxdepth = atoi(optarg);
			break;
		case 'F':
			fp = strtoul(optarg,0,0);
			break;
		case 'P':
			pc = strtoul(optarg,0,0);
			break;
		case 'r':
			showregs();
			break;
		case 'v':
			verbose = 1;
			break;
	    default:
			return(0);
		}
	}
	
	if (!fp)
		getreg("A6", (ulong *)&framepointer);
	else
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
	bullseye = pass = 0;
	while(maxdepth) {
		/* ADD_CODE_HERE */
	}

	if (!maxdepth)
		printf("Max depth termination\n");
	
	if (tfp) {
		tfsclose(tfd,0);
	}
    return(0);
}

#endif
