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
 * docmd:
 * This code supports the command line interface (CLI) portion of the
 * monitor.  It is a table-driven CLI that uses the table in cmdtbl.c.
 * A limited amount of "shell-like" capabilities are supported...
 *   shell variables, symbol-table lookup, command history,
 *   command line editiing, command output redirection, etc...
 *
 * Original author:     Ed Sutter (ed.sutter@alcatel-lucent.com)
 *
 */
#include "config.h"
#include "genlib.h"
#include "tfs.h"
#include "tfsprivate.h"
#include "ether.h"
#include "cli.h"
#include "term.h"
#include <ctype.h>

/* appCmdlist:
 * 	This is a pointer to a list of commands that can be added to the 
 * 	monitor's list by the application using addcommand().
 */
struct	monCommand *appCmdlist;
char	*appcmdUlvl;

extern	struct monCommand cmdlist[];
extern	char cmdUlvl[];

void
showusage(struct monCommand *cmdptr)
{
	char	*usage;

	usage = cmdptr->helptxt[1];
	printf("Usage: %s %s\n",
	    cmdptr->name,*usage ? usage : "(no args/opts)");
}

void
paramerr(struct monCommand *cmdptr)
{
	printf("Command parameter error...\n");
	showusage(cmdptr);
}

int
addcommand(struct monCommand *cmdlist, char *cmdlvl)
{
	appCmdlist = cmdlist;
#if INCLUDE_USRLVL
	appcmdUlvl = cmdlvl;
#endif
	return(0);
}

#if INCLUDE_SHELLVARS

char *
shellsym_chk(char type, char *string,int *size,char *buf,int bufsize)
{
	char	*p1, *p2, varname[CMDLINESIZE], *val;

	p1 = string;
	p2 = varname;
	/* If incoming check is for a symbol, we apply a somewhat more
	 * flexible syntax check for the symbol name...
	 */
	if (type == '%') {
		while(*p1 && (*p1 != '}') && (*p1 != ' ') && (*p1 != '\t'))
			*p2++ = *p1++;
	}
	else {
		while(1) {
			if (((*p1 >= '0') && (*p1 <= '9')) ||
			    ((*p1 >= 'a') && (*p1 <= 'z')) ||
			    ((*p1 >= 'A') && (*p1 <= 'Z')) ||
			    (*p1 == '_')) {
				*p2++ = *p1++;
			}
			else
				break;
		}
	}
	*p2 = '\0';

	if (type == '%')
		val = getsym(varname,buf,bufsize);
	else
		val = getenv(varname);

	if ((val) && (size))
		*size = strlen(varname);

	return(val);
}

/* braceimbalance():
 *	Return non-zero (the index into the src string at the point of the 
 *	imbalance) if the incoming string does not have a balanced set
 *	of braces; else return 0.
 */
static int
braceimbalance(char *src, int *idx, int *ndp)
{
	int		bnest;
	char	*base;

	bnest = 0;
	base = src;
	while((*src) && (bnest >= 0)) {
		if (((*src == '$') || (*src == '%')) && (*(src+1) == '{')) {
			bnest++;
			src++;
		}
		else if (*src == '}') {
			if (bnest)
				bnest--;
		}
		else if (*src == '{') {
			*ndp += 1;	/* Indicate that there is a brace with no '$' prefix */
		}
		else if (*src == '\\') {
			if ((*(src+1) == '$') || (*(src+1) == '%') ||
			    (*(src+1) == '\\') ||
			    (*(src+1) == '{') || (*(src+1) == '}')) {
				src++;
			}
		}
		src++;
	}

	/* If there is a '{}' mismatch, bnest will be non-zero... */
	*idx = src - base - 1;
	return(bnest);
}

/* processprefixes():
 *	Process the '$' for shell variables and '%' for symbols.
 * 	Look for the last '$' (or '%') in the incoming string and attempt to
 *	make a shell variable (or symbol) substitution.  Return 0 if no '$'
 *	(or '%') is found.  Note that '$' and '%' are processed interchangeably
 *	to support symbols and shell variables in the same way.
 */
static int
processprefixes(char *src)
{
	int		namesize, srclen;
	char	*varname, *value;
	char	buf[CMDLINESIZE], buf1[CMDLINESIZE];


	srclen = strlen(src);

	while(*src) {
		if (((*src == '$') || (*src == '%')) && (*(src-1) != '\\')) {
			varname = src+1;
			value = shellsym_chk(*src,varname,&namesize,buf1,sizeof(buf1));
			if (value) {
				if (((srclen - namesize) + strlen(value)) >= CMDLINESIZE) {
					printf("Cmd line expansion overflow\n");
					return(-1);
				}
				if ((*value == '$') &&
					(strncmp(value+1,varname,strlen(value+1)) == 0))
					return(0);
				strcpy(buf,varname+namesize);
				sprintf(varname-1,"%s%s",value,buf);
				return(1);
			}
		}
		src++;
	}
	return(0);
}

/* processbraces():
 *	Look into the incoming string for the deepest set of braces and
 *	substitute that with the value stored in the corresponding shell
 *	variable.  Return 1 if a set of braces was processed; else 0 indicating
 *	that all braces have been processed.  Return -1 if there is some kind
 *	of processing error (buffer overflow).
 */
static int
processbraces(char *src)
{
	int		namesize, srclen, result, opentot;
	char	*cp1, *cp2, *varname, *value, type;
	char	buf[CMDLINESIZE], buf1[CMDLINESIZE], buf2[CMDLINESIZE];

	type = 0;
	opentot = 0;
	varname = src;
	srclen = strlen(src);

	while(*src) {
		if (((*src == '$') || (*src == '%')) && (*(src+1) == '{')) {
			opentot++;
			type = *src;
			varname = src+2;
			src++;
		}
		else if ((*src == '}')  && (opentot)) {
			cp1 = varname;
			cp2 = buf1;
			while(cp1 < src)
				*cp2++ = *cp1++;
			*cp2 = 0;
			while((result = processprefixes(buf1)) == 1);
			if (result == -1)
				return(-1);

			strcpy(buf,src);
			sprintf(varname,"%s%s",buf1,buf);
			value = shellsym_chk(type,varname,&namesize,buf2,sizeof(buf2));
			/* If the shellvar or symbol exists, replace it; else remove it. */
			if (value) {
				if (((srclen-(namesize+3))+strlen(value)+1) > CMDLINESIZE) {
					printf("Cmd line expansion overflow\n");
					return(-1);
				}
				strcpy(buf1,varname+namesize+1);
				sprintf(varname-2,"%s%s",value,buf1);
			}
			else {
				strcpy(varname-2,src+1);
			}
			return(1);
		}
		else if (*src == '\\') {
			if ((*(src+1) == '$') || (*(src+1) == '%') ||
				(*(src+1) == '\\') ||
			    (*(src+1) == '{') || (*(src+1) == '}')) {
				src++;
			}
		}
		else if ((isspace(*src)) && (opentot)) {
			printf("Cmd line expansion error\n");
			return(-1);
		}
		src++;
	}
	return(0);
}

/* expandshellvars():
 *	Passed a string that is to be expanded with all shell variables converted.
 *	This function supports variables of type $VARNAME and ${VARNAME}.
 *	It also allows variables to be embedded within variables.  For example...
 *	${VAR${NAME}} will be a 2-pass expansion in which ${NAME} is evaluated
 *	and then ${VARXXX} (where XXX is whatever was in variable NAME) is
 *	processed.
 */
static int
expandshellvars(char *newstring)
{
	char	*cp;
	int		result, cno, ndp;

	/* Verify that there is a balanced set of braces in the incoming
	 * string...
	 */
	ndp = 0;
	if (braceimbalance(newstring,&cno,&ndp)) {
		printf("Brace imbalance @ %d%s.\n",
			cno,ndp ? " ({ missing $ or %)" : "");
		return(-1);
	}
		
	/* Process the variable names within braces... */
	while((result = processbraces(newstring)) == 1);
	if (result == -1)
		return(-1);

	/* Process dollar signs (left-most first)...	*/
	while((result = processprefixes(newstring)) == 1);
	if (result == -1)
		return(-1);

	/* Cleanup any remaining "\{", "\}" or "\$" strings... */
	cp = newstring+1; 
	while(*cp) {
		if (*cp == '{' || *cp == '}' || *cp == '$' || *cp == '%') {
			if (*(cp-1) == '\\') {
				strcpy(cp-1,cp);
				cp -= 2;
			}
		}
		cp++;
	}
	return(0);
}
#else
static int
expandshellvars(char *newstring)
{
	return(0);
}
#endif


/* tokenize():
 *	Take the incoming string and create an argv[] array from that.  The
 *	incoming string is assumed to be writeable.  The argv[] array is simple
 *	a set of pointers into that string, where the whitespace delimited
 *	character sets are each NULL terminated.
 */
int
tokenize(char *string,char *argv[])
{
	int	argc, done;

	/* Null out the incoming argv array. */
	for(argc=0;argc<ARGCNT;argc++)
		argv[argc] = (char *)0;

	argc = 0;
	while(1) {
		while ((*string == ' ') || (*string == '\t'))
			string++;
		if (*string == 0)
			break;
		argv[argc] = string;
		while ((*string != ' ') && (*string != '\t')) {
			if ((*string == '\\') && (*(string+1) == '"')) {
				strcpy(string,string+1);
			}
			else if (*string == '"') {
				strcpy(string,string+1);
				while(*string != '"') {
					if ((*string == '\\') && (*(string+1) == '"'))
						strcpy(string,string+1);
					if (*string == 0)
						return(-1);
					string++;
				}
				strcpy(string,string+1);
				continue;
			}
			if (*string == 0)
				break;
			string++;
		}
		if (*string == 0)
			done = 1;
		else {
			done = 0;
			*string++ = 0;
		}
		argc++;
		if (done)
			break;
		if (argc >= ARGCNT) {
			argc = -1;
			break;
		}
	}
	return(argc);
}

/* showhelp():
 *	Called by Help() when it is time to print out some verbosity level of
 *	a command's help text.
 *	if...
 *		verbose == 2, then print all the help text;
 *		verbose == 1, then print the command name and abstract;
 *		verbose == 0, then print only the command name;
 */
int
showhelp(struct monCommand *list,int index,int verbose)
{
	char **hp;
	struct monCommand *cptr = &list[index];

#if INCLUDE_USRLVL
	char  *lvltbl = 0;

	/* Get command list in sync with user-level table:
	 */
	if (list == cmdlist) 
		lvltbl = cmdUlvl;
	else if (list == appCmdlist)
		lvltbl = appcmdUlvl;
	else
		return(-1);
	
	/* Verify user level:
	 */
	if ((lvltbl[index] > getUsrLvl()))
		return(0);
#endif

	if (verbose == 2) {
		printf("%s\n", cptr->helptxt[0]);
		showusage(cptr);

		hp = &cptr->helptxt[2];
		while(*hp) 
			printf("%s\n",*hp++);
		
#if INCLUDE_USRLVL
		printf("\nRequired user level: %d\n",lvltbl[index]);
#endif
	}
	else if (verbose == 1) {
#if INCLUDE_USRLVL
		printf(" %-12s %d %s\n", cptr->name, lvltbl[index],cptr->helptxt[0]);
#else
		printf(" %-12s %s\n", cptr->name, cptr->helptxt[0]);
#endif
	}
	else {
		printf("%-12s",cptr->name);
	}
	return(1);
}

/* Command list headers can be defined in config.h or just
 * omitted...
 */
#ifndef APP_CMDLIST_HEADER
#define APP_CMDLIST_HEADER "\nApplication-Installed Command Set:"
#endif

#ifndef MON_CMDLIST_HEADER
#define MON_CMDLIST_HEADER "\nMicro-Monitor Command Set:"
#endif

/* Help():
 * This command displays each commands help text.
 * The help text is assumed to be formatted as an array of strings
 * where...
 *	the first string is the command description;
 *	the second string is command usage;
 *	and all remaining strings up to the NULL are just printable text.
 */
char *HelpHelp[] = {
	"Display command set",
	"-[di] [commandname]",
#if INCLUDE_VERBOSEHELP
	"Options:",
	" -d   list commands and descriptions",
	" -i   configuration info",
#endif
	0,
};

int
Help(int argc,char *argv[])
{
	char	*cp;
	char	*args[2];
	struct	monCommand *cptr, *acptr;
	int		j, i, foundit, opt, descriptions, info;

	descriptions = info = 0;
	while((opt=getopt(argc,argv,"di")) != -1) {
		switch(opt) {
		case 'd':
			descriptions = 1;
			break;
		case 'i':
			info = 1;
			break;
		default:
			return(CMD_PARAM_ERROR);
		}
	}

	if (info) {
		monHeader(0);
		stkusage();
		printf("Moncomptr:    0x%08lx\n",(long)&moncomptr);
#if INCLUDE_ETHERNET
		printf("Etheradd_ptr: 0x%08lx\n",(long)etheraddr);
#endif
#if INCLUDE_TFS
#ifdef TFS_ALTDEVTBL_BASE
		if (alt_tfsdevtbl != (struct tfsdev *)0xffffffff)
			printf("AltTFSdevtbl: 0x%08lx\n",(long)alt_tfsdevtbl);
#endif
#endif
#if INCLUDE_TERM
		{
		int row, col;
		if (term_getsize(&row,&col) != -1)
			printf("ROW/COL: %d/%d\n",row,col);
		}
#endif

		return(CMD_SUCCESS);
	}

	cptr = cmdlist;
	if (argc == optind) {
		foundit = 1;
		if (descriptions) {
			if (appCmdlist) {
				printf("%s\n",APP_CMDLIST_HEADER);
				acptr = appCmdlist;
				for(i=0;acptr->name;acptr++,i++)
					showhelp(appCmdlist,i,1);
			}
			printf("%s\n",MON_CMDLIST_HEADER);
			while(cptr->name) {
				showhelp(cmdlist,cptr-cmdlist,1);
				cptr++;
			}
			putchar('\n');
		}
		else {
			i = 0;
			if (appCmdlist) {
				acptr = appCmdlist;
				printf("%s\n",APP_CMDLIST_HEADER);
				for(j=0;acptr->name;acptr++,j++) {
					if (showhelp(appCmdlist,j,0)) { 
						if ((++i%6) == 0)
							putchar('\n');
					}
				}
				putchar('\n');
				i = 0;
			}
			printf("%s\n",MON_CMDLIST_HEADER);
			while(cptr->name) {
				if (showhelp(cmdlist,cptr-cmdlist,0)) {
					if ((++i%6) == 0)
						putchar('\n');
				}
				cptr++; 
			}
			putchar('\n');
		}
	}
	else {
		foundit = 0;
		cp = argv[1];
		if (appCmdlist) {
			acptr = appCmdlist;
			for(i=0;acptr->name;acptr++,i++) {
				if (strcmp(acptr->name,cp) == 0) {
					foundit = showhelp(appCmdlist,i,2);
					break;
				}
			}
		}
		if (!foundit) {
			if (*cp == '_')
				cp++;
			while(cptr->name) {
				if (strcmp(cptr->name,cp) == 0) {
					foundit = showhelp(cmdlist,cptr-cmdlist,2);
					break;
				}
				cptr++;
			}
		}
	}

	if (!foundit) {
		TFILE *tfp;

		/* If the command is not found in the command table, then see if
		 * it is an executable in TFS.  If it is, run it with a single
		 * argument "help"...
		 */
		tfp = tfsstat(argv[1]);
		if (tfp && (TFS_ISEXEC(tfp))) {
			args[0] = argv[1];
			args[1] = argv[0];
			tfsrun(args,0);
		}
		else {
			printf("\"%s\" not found\n",argv[1]);
			return(CMD_FAILURE);
		}
	}
	return(CMD_SUCCESS);
}

#if INCLUDE_SHELLVARS
/* getPathEntry():
 * Copy the next path entry pointed to by 'path' into 'entry'.
 */
char *
getPathEntry(char *path, char *entry, int entrysize)
{
	int i;
	char *base = entry;

	entrysize--;

	while(*path == ':')
		path++;

	for(i=0;(i < entrysize) && (*path != ':') && (*path != 0); i++)
		*entry++ = *path++;

	if (entry == base)
		return(0);

	*entry++ = 0;
	return(path);
}

/* findPath():
 * If PATH is set, then step through each colon-delimited entry looking
 * for a valid executable.
 */
int
findPath(char *name,char *fpath,int pathsize)
{
	int elen;
	TFILE *tfp;
	char *path;
	char entry[TFSNAMESIZE+1];

	if ((tfp = tfsstat(name)))
		strcpy(fpath,name);

	if ((path = getenv("PATH"))) {
		if ((*path == ':') && tfp)
			return(1);
		while((path = getPathEntry(path,entry,sizeof(entry)))) {
			elen = strlen(entry);
			if ((elen + strlen(name)) < (pathsize-1)) {
				if (entry[elen-1] != '/')
					strcat(entry,"/");
				strcat(entry,name);
				if (tfsstat(entry)) {
					strcpy(fpath,entry);
					return(1);
				}
			}
		}
	}
	if (tfp)
		return(1);
	return(0);
}
#else
int
findPath(char *name, char *fpath, int pathsize)
{
	if (tfsstat(name)) {
		strcpy(fpath,name);
		return(1);
	}
	return(0);
}
#endif

/* fsCmdAlias():
 * Just allow "cat" to be used instead of "tfs cat" (for exammple)...
 */
char *
fsCmdAlias(char *cmdcpy, char *cmd, int cpysize)
{
#if INCLUDE_TFS
	if ((strlen(cmd) + 16) >= cpysize) {
		strcpy(cmdcpy,cmd);
	}
	else if ((strncmp(cmd,"cat ",4) == 0) ||
			 (strncmp(cmd,"ls",2) == 0) ||
			 (strncmp(cmd,"rm ",3) == 0) ||
			 (strncmp(cmd,"cp ",3) == 0) ||
			 (strncmp(cmd,"ld ",3) == 0)) {
		sprintf(cmdcpy,"tfs %s", cmd);
	}
	else
		strcpy(cmdcpy,cmd);
#else
	strcpy(cmdcpy,cmd);
#endif
	return(cmdcpy);
}

/*	_docommand():
 *  Called by docommand() (below) to process what it thinks is a
 *  single command.
 *
 *	Assume the incoming string is a null terminated character string
 *	that is made up of whitespace delimited tokens that will be parsed
 *	into command line arguments.  The first argument is the command name
 *	and all following arguments (if any) are specific to that command.
 * If verbose is non-zero, print the list of arguments after tokenization.
 */
int
_docommand(char *cmdline,int verbose)
{
	int	ret, argc, i, err;
	struct	monCommand	*cmdptr, *cmdptrbase;
	char	*argv[ARGCNT], cmdcpy[CMDLINESIZE], path[TFSNAMESIZE+1];
#if INCLUDE_USRLVL
	char	*ulvlptr = 0;
#endif

	cmdline = fsCmdAlias(cmdcpy,cmdline,CMDLINESIZE);

	/* Redirection check is done prior to shell variable expansion, so
	 * the code within RedirectionCheck() itself must deal with the '$'
	 * after the redirection arrow...
	 */
	if (RedirectionCheck(cmdline) == -1)
		return(CMD_LINE_ERROR);

	/* If there are any instances if a dollar or percent sign within the 
	 * command line, then expand any shell variables (or symbols) that may
	 * be present.
	 */
	if (strpbrk(cmdline,"$%")) {
		if (expandshellvars(cmdline) < 0)
			return(CMD_LINE_ERROR);
	}

	/* Build argc/argv structure based on incoming command line.
	 */
	argc = tokenize(cmdline,argv);
	if (argc == 0)
		return(CMD_SUCCESS);	/* Empty line is ok */
	if (argc < 0) {
		printf("Command line error\n");
		return(CMD_LINE_ERROR);
	}

	/* If verbosity is enabled, print the processed command line.
	 */
	if (verbose) {
		for(i=0;i<argc;i++)
			printf("%s ",argv[i]);
		printf("\n");
	}

	/* Initialize static data used by getopt().
	 */
	getoptinit();

	/* At this point all CLI processing has been done.  We've tokenized
	 * and converted shell variables where necessary.  Now its time to
	 * scan through the command table(s) looking for a match between
	 * the first token of the incoming command string and a command name
	 * in the table(s).
	 * The monitor allows the application to insert a set of commands 
	 * that will be in addition to the commands that are in the monitor's
	 * command list, this is done with the API function mon_addcommand()
	 * which ultimately calls the function addcommand() in this file.
	 * That function is handed a pointer to two tables: a command structure
	 * table and a user level table.  
	 * If the application-supplied command table is present, then we scan
	 * through it first.  This is done so that when we scan the monitor-owned
	 * command table, we can strip off a leading underscore to support the
	 * ability to have a command in each table (applcation-supplied and 	
	 * monitor built-ins) with the same name.
	 */
	if (appCmdlist) {
		cmdptrbase = appCmdlist;
#if INCLUDE_USRLVL
		ulvlptr = appcmdUlvl;
#endif
	}
	else {
		cmdptrbase = cmdlist;
#if INCLUDE_USRLVL
		ulvlptr = cmdUlvl;
#endif
	}

	while(1) {
		/* If we are processing the monitor-owned command table, then
		 * we want to eliminate the leading underscore of argv[0] (if 
		 * there is one).
		 */
		if ((cmdptrbase == cmdlist) && (argv[0][0] == '_'))
			strcpy(argv[0],&argv[0][1]);

		for(cmdptr = cmdptrbase; cmdptr->name; cmdptr++) {
			if (strcmp(argv[0],cmdptr->name) == 0)
				break;
		}
		if (cmdptr->name) {
#if INCLUDE_USRLVL
			/* If command exists, but we are not at the required user
			 * level, then just pretend there was no command match...
			 */
			if (ulvlptr[cmdptr-cmdptrbase] > getUsrLvl())
				break;
#endif
			/* Do not run this command if monrc is active and the 
			 * NOMONRC flag bit is set for this command...
			 */
			if ((cmdptr->flags & CMDFLAG_NOMONRC) && tfsRunningMonrc()) {
				printf("%s: illegal within monrc.\n",cmdptr->name);
				return(CMD_MONRC_DENIED);
			}

			/* Execute the command's function...
			 */
			ret = cmdptr->func(argc,argv);
	
			/* If command returns parameter error, then print the second
			 * string in that commands help text as the usage text.  If
			 * the second string is a null pointer, then print a generic
			 * "no arguments" string as the usage message.
			 */
			if (ret == CMD_PARAM_ERROR)
				paramerr(cmdptr);
			
			RedirectionCmdDone();
			return(ret);
		}
		if (cmdptrbase == cmdlist)
			break;

		cmdptrbase = cmdlist;
#if INCLUDE_USRLVL
		ulvlptr = cmdUlvl;
#endif
	}

	/* If we get here, then the first token does not match on any of
	 * the command names in ether of the command tables.  As a last
	 * resort, we look to see if the first token matches an executable
	 * file name in TFS.
	 */
	ret = CMD_NOT_FOUND;

	if (findPath(argv[0],path,sizeof(path))) {
		argv[0] = path;
		err = tfsrun(argv,0);
		if (err == TFS_OKAY)
			ret = CMD_SUCCESS;
		else 
			printf("%s: %s\n",path,(char *)tfsctrl(TFS_ERRMSG,err,0));
	}
	else {
		printf("Command not found: %s\n",argv[0]);
	}
	RedirectionCmdDone();
	return(ret);
}

/* docommand():
 * This is a relatively new (Jan 2006) front end to _docommand().
 * It deals with three things (two of which are new uMon command line
 * capabilities):
 *
 *   1. Terminates the line at the sight of a poundsign (moved from
 *      original docommand() function).
 *   2. Looks for the left arros ('<') at the end of the line and if
 *      found, it is used to indicate that the line should be repeated
 *      until interrupted.
 *   3. Allow multiple, semicolon-delimited commands to be put on a 
 *      single command line.
 */
int
docommand(char *line, int verbose)
{
	int	ret, loop, len;
	char *lp, *base, *backslash, cmdcpy[CMDLINESIZE];

	loop = 0;
	len = strlen(line);
	if (len >= CMDLINESIZE)
		return(CMD_LINE_ERROR);

	lp = line + len - 1;
	if (*lp == '<')  {
		loop = 1;
		*lp = 0;
	}

repeat:
	backslash = 0;
	lp = base = cmdcpy;
	strcpy(lp,line);

	while (*lp) {
		if (*lp == ';') {
			if (backslash) {
				strcpy(backslash,lp);	
				backslash = 0;
				continue;
			}
			*lp = 0;
			ret = _docommand(base,verbose);
			*lp++ = ';';
			base = lp;
			if (ret != CMD_SUCCESS)
				return(ret);
			continue;
		}
		if (*lp == '#') {
			if (backslash) {
				strcpy(backslash,lp);	
				backslash = 0;
				continue;
			}
			*lp = 0;
			loop = 0;
			break;
		}
		if (*lp == '\\') {
			backslash = lp;
			lp++;
			continue;
		}
		backslash = 0;
		lp++;
	}
	ret = _docommand(base,verbose);

	if ((loop) && !gotachar()) 
		goto repeat;

	return(ret);
}
