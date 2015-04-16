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
 * reg_cache.c:
 *
 *	Allow the user to display CPU registers that are locally cached
 *	when an exception is hit.   These registers are not the currently
 *	active registers in the CPU context; rather, a copy of the context at
 *	the time of the most recent breakpoint or exception.
 *
 * Original author:     Ed Sutter (ed.sutter@alcatel-lucent.com)
 *
 */
#include "config.h"
#include "cpu.h"
#include "genlib.h"
#include <ctype.h>
#include "stddefs.h"
#include "regnames.c"
#include "cli.h"

/* The file regnames.c will contain a table of character pointers that
 * corresond to registers that are stored away by the target's exception
 * handler.  The table of names and the order in which the registers are
 * stored by the exception handler must be synchronized.
 * An example table is as follows (taken from FADS-MPC860 target)...
 *
 * static char  *regnames[] = {
 * 		"R0",		"R1",		"R2",		"R3",
 * 		"R4",		"R5",		"R6",		"R7",
 * 		"R8",		"R9",		"R10",		"R11",
 * 		"R12",		"R13",		"R14",		"R15",
 * 		"R16",		"R17",		"R18",		"R19",
 * 		"R20",		"R21",		"R22",		"R23",
 * 		"R24",		"R25",		"R26",		"R27",
 * 		"R28",		"R29",		"R30",		"R31",
 * 		"MSR",		"CR",		"LR",		"XER",
 * 		"DAR",		"DSISR",	"SRR0",		"SRR1",
 * 		"CTR",		"DEC",		"TBL",		"TBU",
 * 		"SPRG0",	"SPRG1",	"SPRG2",	"SPRG3",
 * };
 *
 * The real definition of this array would be in the file regnames.c in 
 * the target-specific directory.
 */

#define REGTOT	(sizeof regnames/sizeof(char *))

/* regtbl[]:
 * This array is used to store the actual register values.  Storage
 * is done by the target's exception handler and must match the order
 * of the register names in the regnames[] array.
 */
ulong regtbl[REGTOT];

void
flushRegtbl(void)
{
	flushDcache((char *)regtbl,sizeof(regtbl));
}

/* regidx():
 * Return an index into the regnames[] array that matches the
 * incoming register name.
 * If no match is found, print an error message and return -1.
 */
static int
regidx(char *name)
{
	int	i;

	strtoupper(name);
	for(i=0;i<REGTOT;i++) {
		if (!strcmp(name,regnames[i])) {
			return(i);
		}
	}
	printf("Bad reg: '%s'\n",name);
	return(-1);
}

/* putreg():
 * Put the specified value into the specified register
 * storage location.
 */
int
putreg(char *name,ulong value)
{
	int	idx;

	if ((idx = regidx(name)) == -1)
		return(-1);
	
	regtbl[idx] = value;
	return(0);
}

/* getreg():
 * Retrieve the value of the specified register.
 */
int
getreg(char *name,ulong *value)
{
	int	idx;

	if ((idx = regidx(name)) == -1)
		return(-1);
	
	*value = regtbl[idx];
	return(0);
}

/* showregs():
 * Dump the content of the register cache in a tabular format
 * showing the entry in the regnames[] array and the corresponding
 * entry in the regtbl[] array.
 */
void
showregs(void)
{
	int	i, j;

	for(i=0;i<REGTOT;) {
		for(j=0;((j<4) && (i<REGTOT));j++,i++)
			printf("%6s=0x%08lx ",regnames[i],regtbl[i]);
		printf("\n");
	}
}

/* reginit():
 * Clear the register cache.
 */
void
reginit(void)
{
	int	i;

	for(i=0;i<REGTOT;i++)
		regtbl[i] = 0;
}

#if INCLUDE_STRACE
char *RegHelp[] = {
	"Display/modify content of monitor's register cache",
	"-[v:] [regname] [value]",
#if INCLUDE_VERBOSEHELP
	"Options:",
	" -v {var} quietly load 'var' with register content",
#endif
	0,
};

int
Reg(int argc,char *argv[])
{
	ulong	reg;
	int		opt, i, forscript;
	char	*varname, buf[32];

	forscript = 0;
	varname = (char *)0;
	while((opt=getopt(argc,argv,"sv:")) != -1) {
		switch(opt) {
		case 's':
			forscript = 1;
			break;
		case 'v':
			varname = optarg;
			break;
		default:
			return(CMD_PARAM_ERROR);
		}
	}

	if (argc == optind) {
		if (forscript) {
			for(i=0;i<REGTOT;i++) 
				printf("reg %s 0x%lx\n",regnames[i],regtbl[i]);
		}
		else
			showregs();
		return(CMD_SUCCESS);
	}
	else if (argc == optind + 1) {
		if (getreg(argv[optind],&reg) != -1) {
			sprintf(buf,"0x%lx",reg);
			if (varname)
				setenv(varname,buf);
			else
				printf("%s = %s\n",argv[optind],buf);
		}
	}
	else if (argc == optind + 2) {
    	putreg(argv[1],strtol(argv[optind+1],(char **)0,0));
	}
	else {
		return(CMD_PARAM_ERROR);
	}
	return(CMD_SUCCESS);
}
#endif
