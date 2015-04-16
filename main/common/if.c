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
 * if.c:
 *
 * This file started off as the file for the "if" command in the monitor,
 * and has since grown into the file that contains the commands that only
 * make sense if they are used in scripts; hence, a prerequisite to these
 * commands is that TFS is included in the build.
 * It also contains the script runner portion of TFS.  The name of this
 * file should have been changed to tfsscript.c!
 *	
 * if:
 * Supports the monitor's ability to do conditional branching within
 * scripts executed through TFS.
 *
 * goto:
 * Tag based jumping in a script.
 *
 * item:
 * A simple way to process a list (similar to KSH's 'for' construct).
 *
 * Original author:     Ed Sutter (ed.sutter@alcatel-lucent.com)
 *
 */
#include "config.h"
#include "genlib.h"
#include "stddefs.h"
#include "tfs.h"
#include "tfsprivate.h"
#include "ether.h"
#include "cli.h"
#include <ctype.h>

#if INCLUDE_TFSSCRIPT

/* Subroutine variables:
 * The definition of MAXGOSUBDEPTH (15) determines the maximum number
 * of subroutines that can be nested within any one script invocation.
 * ReturnToTellTbl[]
 *  Used by script runner to keep track of the location in the file that
 *  the subroutine is supposed to return to.
 * ReturnToDepth
 *  The current subroutine nesting depth of the script runner.
 */
#define MAXGOSUBDEPTH	15
static int	ReturnToDepth;
static long	ReturnToTellTbl[MAXGOSUBDEPTH+1];
static int	CurrentScriptfdTbl[TFS_MAXOPEN+1];

/* ScriptIsRunning:
 * Non-zero if a script is active, else zero.
 */
static int	ScriptIsRunning;

/* ScriptGotoTag:
 * Points to the string that is the most recent target of
 * a goto or gosub command.
 */
static char	*ScriptGotoTag;

/* ScriptExitFlag:
 * If non-zero, then the script must exit.  
 * The 'exit' command sets this flag based on options provided to exit.
 */
int	ScriptExitFlag;

/* ExecuteAfterNext:
 * If the ScriptExitFlag has the EXECUTE_AFTER_EXIT flag set, then the content
 * of this array is assumed to be the file that is to be executed 
 * automatically after the exit of the current script.
 */
char	ExecuteAfterExit[TFSNAMESIZE+1];

/*****************************************************************************/

/* If:
 *	A simple test/action statement.
 *	Currently, the only action supported is "goto tag". 
 *	Syntax:
 *		if ARG1 compar ARG2 {action} [else action]
 *		if -t TEST {action} [else action]
 */
char *IfHelp[] = {
	"Conditional branching",
	"-[t:v] [{arg1} {compar} {arg2}] {action} [else action]",
#if INCLUDE_VERBOSEHELP
	" Options:",
	" -t{test} see below",
	" -v       verbose mode (print TRUE or FALSE result)",
	" Notes:",
	" * Numerical/logical/string compare:",
	"   seq sec sne sin gt lt le ge eq ne and or xor",
	" * Action:",
	"   goto tag | gosub tag | exit | return",
	" * Other tests (-t args):",
	"   gc, ngc, iscmp {filename}",
#endif
	0,
};

int
If(int argc, char *argv[])
{
	int		opt, arg, true, if_else, offset, verbose;
	void	(*iffunc)(char *), (*elsefunc)(char *);
	long	var1, var2;
	char	*testtype, *arg1, *arg2, *iftag, *elsetag;

	verbose = 0;
	testtype = 0;
	while((opt=getopt(argc,argv,"vt:")) != -1) {
		switch(opt) {
		case 'v':
			verbose = 1;
			break;
		case 't':
			testtype = optarg;
			break;
		default:
			return(CMD_PARAM_ERROR);
		}
	}

	elsetag = 0;
	elsefunc = 0;
	offset = true = if_else = 0;

	/* First see if there is an 'else' present... */
	for (arg=optind;arg<argc;arg++) {
		if (!strcmp(argv[arg],"else")) {
			if_else = 1;
			break;
		}
	}

	if (if_else) {
		elsetag = argv[argc-1];
		if (!strcmp(argv[argc-1],"exit")) {
			offset = 2;
			elsefunc = exitscript;
		}
		else if (!strcmp(argv[argc-1],"return")) {
			offset = 2;
			elsefunc = gosubret;
		}
		else if (!strcmp(argv[argc-2],"goto")) {
			offset = 3;
			elsefunc = gototag;
		}
		else if (!strcmp(argv[argc-2],"gosub")) {
			offset = 3;
			elsefunc = gosubtag;
		}
		else
			return(CMD_PARAM_ERROR);
	}
	
	iftag = argv[argc-offset-1];
	if (!strcmp(argv[argc-offset-1],"exit"))
		iffunc = exitscript;
	else if (!strcmp(argv[argc-offset-1],"return"))
		iffunc = gosubret;
	else if (!strcmp(argv[argc-offset-2],"goto"))
		iffunc = gototag;
	else if (!strcmp(argv[argc-offset-2],"gosub"))
		iffunc = gosubtag;
	else
		return(CMD_PARAM_ERROR);

	if (testtype) {
		if (!strcmp(testtype,"gc")) {
			if (gotachar())
				true=1;
		}
		else if (!strcmp(testtype,"ngc")) {
			if (!gotachar())
				true=1;
		}
		else
			return(CMD_PARAM_ERROR);
	}
	else {
		arg1 = argv[optind];
		testtype = argv[optind+1];
		arg2 = argv[optind+2];

		var1 = strtoul(arg1,(char **)0,0);
		var2 = strtoul(arg2,(char **)0,0);

		if (!strcmp(testtype,"gt")) {
			if (var1 > var2)
				true = 1;
		}
		else if (!strcmp(testtype,"lt")) {
			if (var1 < var2)
				true = 1;
		}
		else if (!strcmp(testtype,"le")) {
			if (var1 <= var2)
				true = 1;
		}
		else if (!strcmp(testtype,"ge")) {
			if (var1 >= var2)
				true = 1;
		}
		else if (!strcmp(testtype,"eq")) {
			if (var1 == var2)
				true = 1;
		}
		else if (!strcmp(testtype,"ne")) {
			if (var1 != var2)
				true = 1;
		}
		else if (!strcmp(testtype,"and")) {
			if (var1 & var2)
				true = 1;
		}
		else if (!strcmp(testtype,"xor")) {
			if (var1 ^ var2)
				true = 1;
		}
		else if (!strcmp(testtype,"or")) {
			if (var1 | var2)
				true = 1;
		}
		else if (!strcmp(testtype,"sec")) {
			if (!strcasecmp(arg1,arg2))
				true = 1;
		}
		else if (!strcmp(testtype,"seq")) {
			if (!strcmp(arg1,arg2))
				true = 1;
		}
		else if (!strcmp(testtype,"sin")) {
			if (strstr(arg2,arg1))
				true = 1;
		}
		else if (!strcmp(testtype,"sne")) {
			if (strcmp(arg1,arg2))
				true = 1;
		}
		else
			return(CMD_PARAM_ERROR);
	}
	
	/* If the true flag is set, call the 'if' function.
	 * If the true flag is clear, and "else" was found on the command
	 * line, then call the 'else' function...
	 */
	if (true) {
		if (verbose)
			printf("TRUE\n");
		iffunc(iftag);
	}
	else {
		if (verbose)
			printf("FALSE\n");
		if (if_else)
			elsefunc(elsetag);
	}

	return(CMD_SUCCESS);
}

int
InAScript(void)
{
	if (ScriptIsRunning)
		return(1);
	else {
		printf("Invalid from outside a script\n");
		return(0);
	}
}

void
exitscript(char *ignored)
{
	if (InAScript()) {
		ScriptExitFlag = EXIT_SCRIPT;
	}
}


char *ExitHelp[] = {
	"Exit currently running script",
	"-[ae:r]",
#if INCLUDE_VERBOSEHELP
	"Options:",
	" -a       exit all scripts (if nested)",
	" -e {exe} automatically execute 'exe' after exit",
	" -r       remove script after exit",
#endif
	0,
};

int
Exit(int argc, char *argv[])
{
	int opt;
	ScriptExitFlag = EXIT_SCRIPT;

	while((opt=getopt(argc,argv,"ae:r")) != -1) {
		switch(opt) {
		case 'a':
			ScriptExitFlag |= EXIT_ALL_SCRIPTS;
			break;
		case 'e':
			if (strlen(optarg) >= sizeof(ExecuteAfterExit))
				return(CMD_PARAM_ERROR);
			ScriptExitFlag |= EXECUTE_AFTER_EXIT;
			strcpy(ExecuteAfterExit,optarg);
			break;
		case 'r':
			ScriptExitFlag |= REMOVE_SCRIPT;
			break;
		}
	}
	return(CMD_SUCCESS);
}

char *GotoHelp[] = {
	"Branch to file tag",
	"{tagname}",
	0,
};

int
Goto(int argc, char *argv[])
{
	if (argc != 2)
		return(CMD_PARAM_ERROR);
	gototag(argv[1]);
	return(CMD_SUCCESS);
}


char *GosubHelp[] = {
	"Call a subroutine",
	"{tagname}",
	0,
};

int
Gosub(int argc, char *argv[])
{
	if (argc != 2)
		return(CMD_PARAM_ERROR);
	gosubtag(argv[1]);
	return(CMD_SUCCESS);
}

char *ReturnHelp[] = {
	"Return from subroutine",
	"",
	0,
};

int
Return(int argc, char *argv[])
{
	if (argc != 1)
		return(CMD_PARAM_ERROR);
	gosubret(0);
	return(CMD_SUCCESS);
}

/* Item:
 *	This is a simple replacement for the KSH "for" construct...
 *	It allows the user to build a list of strings and retrieve one at a time.
 *	Basically, the items can be thought of as a table.  The value of idx
 *	(starting with 1) is used to index into the list of items and place
 *	that item in the shell variable "var".
 *	Syntax:
 *		item {idx} {var} {item1} {item2} {item3} ....
 */
char *ItemHelp[] = {
	"Extract an item from a list",
	"{idx} {stor_var} [item1] [item2] ...",
#if INCLUDE_VERBOSEHELP
	"Note: idx=1 retrieves item1",
#endif
	0,
};

int
Item(int argc, char *argv[])
{
	int	idx;
	char *varname;

	if (argc < 3)
		return(CMD_PARAM_ERROR);

	idx = atoi(argv[1]);
	varname = argv[2];

	if ((idx < 1) || (idx > (argc-3))) {
		setenv(varname,0);
		return(CMD_SUCCESS);
	}

	idx += 2;
	setenv(varname,argv[idx]);
	return(CMD_SUCCESS);
}

/* Read():
 *	Simple interactive shell variable entry.  
 *	Syntax:
 *	read [options] [var1] [var2] [...]
 *	Options:
 *		-twww   timeout after 'www' milliseconds (approximate) of 
 *				waiting for input.
 *
 */

char *ReadHelp[] = {
	"Interactive shellvar entry",
#if INCLUDE_VERBOSEHELP
	"-[fnp:t:T:] [var1] [var2] ... ",
	" -f      flush console input",
	" -n      no echo during read",
	" -p ***  prefill string",
	" -t ###  msec timeout per char",
	" -T ###  msec timeout cumulative",
	"Notes:",
	"  This command waits for console input terminated by ENTER.",
	"  Input tokens are whitespace delimited.",
	"Examples:",
	" * read name                    # wait forever",
	"   If user responds with ENTER, shell var 'name' is cleared;",
	"   else it is loaded with the first input token.",
	" * read -t2000 name             # timeout after 2 seconds",
	"   If timeout, shell var 'name' is untouched; else if timeout", 
	"   doesn't occur, but user only types ENTER 'var' is cleared;",
	"   else 'var' will contain token.",
#endif
	0,
};

int
Read(int argc,char *argv[])
{
	int		i, reached_eol, opt, waitfor, noecho;
	char	*prefill, buf[64], *space, *bp;

	prefill = 0;
	waitfor = noecho = 0;
	while((opt=getopt(argc,argv,"fnp:t:T:")) != -1) {
		switch(opt) {
		case 'f':
			flush_console_in();
			return(CMD_SUCCESS);
		case 'n':
			noecho = 1;
			break;
		case 'p':
			prefill = optarg;
			break;
		case 't':
			waitfor = strtol(optarg,(char **)0,0);
			break;
		case 'T':
			waitfor = strtol(optarg,(char **)0,0);
			waitfor = -waitfor;
			break;
		default:
			return(CMD_PARAM_ERROR);
		}
	}

#if INCLUDE_MONCMD
	/* Since this command blocks waiting for user input on the console,
	 * check to see if we are running under the context of MONCMD, and,
	 * if yes, return CMD_FAILURE to avoid a lockup...
	 */
	if (IPMonCmdActive) {
		printf("Invalid under moncmd context.\n");
		return(CMD_FAILURE);
	}
#endif

	/* If -t, then restart the timeout for each character.
	 * If -T, then the timeout accumulates over all characters.
	 * If a timeout does occur, then do nothing to the shell variables
	 * that may be specified as arguments.  This allows the script
	 * writer to distinguish between a timeout, and an empty line...
	 * Timeout has no affect, empty line will NULL out the variable.
	 */
	if (getfullline(buf,sizeof(buf)-1,0,waitfor,prefill,noecho?0:1) == -1)
		return(CMD_SUCCESS);

	/* If there were no args after the option, then there is no need to
	 * consider populating a shell variable, so just return here.
	 */
	if (argc == optind) {
		return(CMD_SUCCESS);
	}

	bp = buf;
	reached_eol = 0;
	for(i=optind;i<argc;i++) {
		space=strpbrk(bp," \t");
		if (space) {
			*space = 0;
			setenv(argv[i],bp);
			bp = space+1;
		}
		else {
			if ((reached_eol) || (*bp == 0)) {
				setenv(argv[i],0);
			}
			else {
				setenv(argv[i],bp);
				reached_eol = 1;
			}
		}
	}
	return(CMD_SUCCESS);
}

int
currentScriptfd(void)
{
	return(CurrentScriptfdTbl[ScriptIsRunning]);
}

/* gototag():
 *	Used with tfsscript to allow a command to adjust the pointer into the
 *	script that is currently being executed.  It simply populates the
 *	"ScriptGotoTag" pointer with the tag that should be branched to next.
 */
void
gototag(char *tag)
{
	if (InAScript()) {
		if (ScriptGotoTag)
			free(ScriptGotoTag);
		ScriptGotoTag = malloc(strlen(tag)+8);
		sprintf(ScriptGotoTag,"# %s",tag);
	}
}

/* gosubtag():
 *	Similar in basic use to gototag(), except that we keep a copy of the
 *	current position in the active script file so that it can be returned
 *	to later.
 */
void
gosubtag(char *tag)
{
	if (InAScript()) {
		if (ReturnToDepth >= MAXGOSUBDEPTH) {
			printf("Max return-to depth reached\n");
			return;
		}
		ReturnToTellTbl[ReturnToDepth++] = tfstell(currentScriptfd());
		gototag(tag);
	}
}

void
gosubret(char *ignored)
{
	int	offset, err;

	if (InAScript()) {
		err = 0;

		if (ReturnToDepth <= 0) {
			printf("Nothing to return to\n");
			err = 1;
		}
		else {
			ReturnToDepth--;
			offset = tfsseek(currentScriptfd(),
				ReturnToTellTbl[ReturnToDepth],TFS_BEGIN);
			if (offset <= 0) {
				err = 1;
				printf("return error: %s\n",tfserrmsg(offset));
			}
		}
		if (err)
			printf("Possible gosub/return imbalance.\n");
	}
}

/* tfsscript():
 *	Treat the file as a list of commands that should be executed
 *	as monitor commands.  Step through each line and execute it.
 *	The tfsDocommand() function pointer is used so that the application
 *	can assign a different command interpreter to the script capbilities
 *	of the monitor.  To really take advantage of this, the command interpreter
 *	that is reassigned, should allow monitor commands to run if the command
 *	does not exist in the application's command list.
 *	
 *	Scripts can call other scripts that call other scripts (etc...) and
 *	that works fine because tfsscript() will just use more stack space but
 *	it eventually returns through the same function call tree.  A script can
 *	also call a COFF, ELF or AOUT file and expect it to return as long as
 *	the executable returns through the same point into which it was started
 *	(the entrypoint).  If the executable uses mon_appexit() to terminate, 
 *	then the calling script will not regain control.
 */

int
tfsscript(TFILE *fp,int verbose)
{
	char	lcpy[CMDLINESIZE], *sv;
	int		tfd, lnsize, lno, verbosity, ignoreerror, cmdstat;

	tfd = tfsopen(fp->name,TFS_RDONLY,0);
	if (tfd < 0)
		return(tfd);

	lno = 0;

	/* If ScriptIsRunning is zero, then we know that this is the top-level
	 * script, so we can initialize state here...
	 */
	if (ScriptIsRunning == 0)
		ReturnToDepth = 0;

	CurrentScriptfdTbl[++ScriptIsRunning] = tfd;
	
	while(1) {
		lno++;
		lnsize = tfsgetline(tfd,lcpy,CMDLINESIZE);
		if (lnsize == 0)	/* end of file? */
			break;
		if (lnsize < 0) {
			printf("tfsscript(): %s\n",tfserrmsg(lnsize));
			break;
		}
		if ((lcpy[0] == '\r') || (lcpy[0] == '\n'))	/* empty line? */
			continue;

		lcpy[lnsize-1] = 0;			/* Remove the newline */

		/* Just in case the goto tag was set outside a script, */
		/* clear it now. */
		if (ScriptGotoTag) {
			free(ScriptGotoTag);
			ScriptGotoTag = (char *)0;
		}

		ScriptExitFlag = 0;

		/* Execute the command line.
		 * If the shell variable "SCRIPTVERBOSE" is set, then enable
		 * verbosity for this command; else use what was passed in 
		 * the parameter list of the function.  Note that the variable
		 * is tested for each command so that verbosity can be enabled
		 * or disabled within a script.
		 * If the command returns a status that indicates that there was
		 * some parameter error, then exit the script.
		 */
		sv = getenv("SCRIPTVERBOSE");
		if (sv)
			verbosity = atoi(sv);
		else
			verbosity = verbose;

		if ((lcpy[0] == '-') || (getenv("SCRIPT_IGNORE_ERROR")))
			ignoreerror = 1;
		else
			ignoreerror = 0;

		if (verbosity)
			printf("[%02d]: %s\n",lno,lcpy);

		cmdstat = tfsDocommand(lcpy[0] == '-' ? lcpy+1 : lcpy, 0);

		if (cmdstat != CMD_SUCCESS) {
			setenv("CMDSTAT","FAIL");
			if (ignoreerror == 0) {
				printf("Terminating script '%s' at line %d\n",
					TFS_NAME(fp),lno);
				ScriptExitFlag = EXIT_SCRIPT;
				break;
			}
		}
		else {
			setenv("CMDSTAT","PASS");
		}

		/* Check for exit flag.  If set, then in addition to terminating the
		 * script, clear the return depth here so that the "missing return"
		 * warning  is not printed.  This is done because there is likely
		 * to be a subroutine with an exit in it and this should not
		 * cause a warning.
		 */
		if (ScriptExitFlag) {
			ReturnToDepth = 0;
			break;
		}

		/* If ScriptGotoTag is set, then attempt to reposition the line 
		 * pointer to the line that contains the tag.
		 */
		if (ScriptGotoTag) {
			int		tlen;

			tlen = strlen(ScriptGotoTag);
			lno = 0;
			tfsseek(tfd,0,TFS_BEGIN);
			while(1) {
				lnsize = tfsgetline(tfd,lcpy,CMDLINESIZE);
				if (lnsize == 0) {
					printf("Tag '%s' not found\n",ScriptGotoTag+2);
					free(ScriptGotoTag);
					ScriptGotoTag = (char *)0;
					tfsclose(tfd,0);
					return(TFS_OKAY);
				}
				lno++;
				if (!strncmp(lcpy,ScriptGotoTag,tlen) &&
					(isspace(lcpy[tlen]) || (lcpy[tlen] == ':'))) {
					free(ScriptGotoTag);
					ScriptGotoTag = (char *)0;
					break;
				}
			}
		}
		/* After each line, poll ethernet interface. */
		pollethernet();
	}
	tfsclose(tfd,0);
	if (ScriptExitFlag & REMOVE_SCRIPT)
		tfsunlink(fp->name);
	if (ScriptIsRunning > 0) {
		ScriptIsRunning--;
		if ((ScriptIsRunning == 0) && (ReturnToDepth != 0)) {
			printf("Error: script is done, but return-to-depth != 0\n");
			printf("(possible gosub/return imbalance)\n");
		}
	}
	else  {
		printf("Script run-depth error\n");
	}

	/* If the EXECUTE_AFTER_EXIT flag is set (by exit -e), then automatically
	 * start up the file specified in ExecuteAfterExit[]...
	 */
	if (ScriptExitFlag & EXECUTE_AFTER_EXIT) {
		char	*argv[2];

		argv[0] = ExecuteAfterExit;
		argv[1] = 0;
		ScriptExitFlag = 0;
		tfsrun(argv,0);
	}

	/* Upon completion of a script, clear the ScriptExitFlag variable so
	 * that it is not accidentally applied to another script that may have
	 * called this script.
	 */
	if ((ScriptExitFlag & EXIT_ALL_SCRIPTS) != EXIT_ALL_SCRIPTS)
		ScriptExitFlag = 0;
	return(TFS_OKAY);
}

/* tfsscriptname():
 * Return the currently running script name; else return an empty string.
 */
char *
tfsscriptname(void)
{
	struct	tfsdat *slot;

	if (ScriptIsRunning) {
		slot = &tfsSlots[CurrentScriptfdTbl[ScriptIsRunning]];
		if (slot->offset != (ulong)-1)
			return(slot->hdr.name);
	}
	return("");
}

#else	/* INCLUDE_TFSSCRIPT */

int
tfsscript(TFILE *fp,int verbose)
{
	return(TFSERR_NOTAVAILABLE);
}

char *
tfsscriptname(void)
{
	return(0);
}

#endif
