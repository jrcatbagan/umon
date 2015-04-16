/* strace.c:
 *	General notice:
 *	This code is part of a boot-monitor package developed as a generic base
 *	platform for embedded system designs.  As such, it is likely to be
 *	distributed to various projects beyond the control of the original
 *	author.  Please notify the author of any enhancements made or bugs found
 *	so that all may benefit from the changes.  In addition, notification back
 *	to the author will allow the new user to pick up changes that may have
 *	been made by other users after this version of the code was distributed.
 *
 *	Note1: the majority of this code was edited with 4-space tabs.
 *	Note2: as more and more contributions are accepted, the term "author"
 *		   is becoming a mis-representation of credit.
 *
 *	Original author:	Ed Sutter
 *	Email:				esutter@lucent.com
 *	Phone:				908-582-2351
 *
 */
#include "config.h"
#if INCLUDE_STRACE
#include "tfs.h"
#include "tfsprivate.h"
#include "ctype.h"
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
