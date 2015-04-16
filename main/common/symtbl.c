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
 * symtbl.c:
 *
 * This file contains functions related to the symbol table option in
 * the monitor.
 *
 * Original author:     Ed Sutter (ed.sutter@alcatel-lucent.com)
 *
 */
#include "config.h"
#include "stddefs.h"
#include "genlib.h"
#if INCLUDE_TFSSYMTBL
#include <ctype.h>
#include "tfs.h"
#include "tfsprivate.h"

#ifndef SYMFILE
#define SYMFILE "symtbl"
#endif

/* SymFileFd():
 * Attempt to open the symbol table file.  First look to the SYMFILE env var;
 * else default to SYMFILE definition.  If the file exists, open it and return
 * the file descriptor; else return TFSERR_NOFILE.
 */
int
SymFileFd(int verbose)
{
	TFILE	*tfp;
	int		tfd;
	char	*symfile;

	/* Load symbol table file name.  If SYMFILE is not a variable, default
	 * to the string defined by SYMFILE.
	 */
	symfile = getenv("SYMFILE");
	if (!symfile)
		symfile = SYMFILE;

	tfp = tfsstat(symfile);
	if (!tfp) 
		return(TFSERR_NOFILE);

	tfd = tfsopen(symfile,TFS_RDONLY,0);
	if (tfd < 0) {
		if (verbose)
			printf("%s: %s\n",symfile,(char *)tfsctrl(TFS_ERRMSG,tfd,0));
		return(TFSERR_NOFILE);
	}
	return(tfd);
}

/* AddrToSym():
 *	Assumes each line of symfile is formatted as...
 *		synmame SP hex_address
 *	and that the symbols are sorted from lowest to highest address.
 *	Using the file specified by the incoming TFS file descriptor, 
 *	determine what symbol's address range covers the incoming address.
 *	If found, store the name of the symbol as well as the offset between
 *	the address of the symbol and the incoming address.
 *	Note, if the incoming file descriptor is -1, then we open (and later
 *	close) the file	here.
 *
 *	Return 1 if a match is found, else 0.
 */
int
AddrToSym(int tfdin,ulong addr,char *name,ulong *offset)
{
	int		lno, tfd;
	char	*space;
	ulong	thisaddr, lastaddr;
	char	thisline[84];
	char	lastline[sizeof(thisline)];

	lno = 1;
	if (offset)
		*offset = 0;
	lastaddr = 0;
	if (tfdin == -1) {
		tfd = SymFileFd(0);
		if (tfd == TFSERR_NOFILE)
			return(0);
	}
	else
		tfd = tfdin;
	tfsseek(tfd,0,TFS_BEGIN);
	while(tfsgetline(tfd,thisline,sizeof(thisline)-1)) {
		space = strpbrk(thisline,"\t ");
		if (!space)
			continue;
		*space++ = 0;
		while(isspace(*space))
			space++;
	
		thisaddr = strtoul(space,0,0);	/* Compute address from entry in	*/
										/* symfile. 						*/

		if (thisaddr == addr) {			/* Exact match, use this entry		*/
			strcpy(name,thisline);		/* in symfile.						*/
			if (tfdin == -1)
				tfsclose(tfd,0);
			return(1);
		}
		else if (thisaddr > addr) {		/* Address in symfile is greater	*/	
			if (lno == 1)				/* than incoming address...			*/
				break;					/* If this is first line of symfile */
			strcpy(name,lastline);		/* then return error.				*/
			if (offset)
				*offset = addr-lastaddr;/* Otherwise return the symfile		*/
			if (tfdin == -1)
				tfsclose(tfd,0);
			return(1);					/* entry previous to this one.		*/
		}
		else {							/* Address in symfile is less than	*/
			lastaddr = thisaddr;		/* incoming address, so just keep	*/
			strcpy(lastline,thisline);	/* a copy of this line and go to 	*/
			lno++;						/* the next.						*/
		}
	}
	if (tfdin == -1)
		tfsclose(tfd,0);
	sprintf(name,"0x%lx",addr);
	return(0);
}

/* getsym():
 * Provides a similar capability to shell variables on the command line
 * except that here, the variable replacement is based on the content of
 * a symbol file.  The file would be be loaded into TFS to provide
 * a potentially large number of symbols.
 * Looks for the file SYMFILE.  If non-existent, just return NULL.
 * If file exists, then search through the file for the incoming
 * symbol name and, if found, return the replacement for the symbol.
 * else return NULL.
 * The text file is formatted such that no line is greater than 40 characters.
 * Each line has 2 whitespace delimited strings.  The first string is
 * the symbol name and the second string is the replacement value for that
 * symbol.  The first string is assumed to start at the beginning of the
 * line, and the second string is separated from the first string by
 * spaces and/or tabs.
 * For example, here are a few lines:
 *
 *	   main		0x10400
 *	   func		0x10440
 *
 * With the above lines in SYMFILE, if %main were on the command line, it
 * would be replaced with 0x10400.
 */
char *
getsym(char *symname,char *line,int sizeofline)
{
	int		tfd;
	char	*space;

	if ((tfd = SymFileFd(1)) < 0) {
		return((char *)0);
	}

	while(tfsgetline(tfd,line,sizeofline)) {
		char *eol;
		eol = strpbrk(line,"\r\n");
		if (eol)
			*eol = 0;
		space = strpbrk(line,"\t ");
		if (!space)
			continue;
		*space = 0;
		if (!strcmp(line,symname)) {
			tfsclose(tfd,0);
			space++;
			while((*space == ' ') || (*space == '\t'))
				space++;
			return(space);
		}
	}
	tfsclose(tfd,0);
	return((char *)0);
}

#else

char *
getsym(char *symname,char *line,int sizeofline)
{
	return((char *)0);
}

int
AddrToSym(int tfd,ulong addr,char *name,ulong *offset)
{
	sprintf(name,"0x%lx",addr);
	return(0);
}

#endif
