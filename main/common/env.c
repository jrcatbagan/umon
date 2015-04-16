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
 * env.c:
 * Shell variable functions used to load or retrieve shell variable
 * information from the shell variable table.
 *
 * Original author:     Ed Sutter (ed.sutter@alcatel-lucent.com)
 *
 */
#include <stdarg.h>
#include "config.h"
#include "tfs.h"
#include "tfsprivate.h"
#include "ether.h"
#include "genlib.h"
#include "stddefs.h"
#include "cli.h"
#include "version.h"
#include "boardinfo.h"
#include "timer.h"

#if INCLUDE_SHELLVARS

char *whatplatform = "@(#)PLATFORM=" PLATFORM_NAME;

#ifdef TARGET_ENV_SETUP
extern void TARGET_ENV_SETUP(void);
#endif

#ifndef PROMPT
#define PROMPT "uMON>"
#endif

int	shell_print(void);
int	envToExec(char *);
void clearenv(void);

/* Structure used for the shell variables: */
struct s_shell {
	char	*val;		/* Value stored in shell variable */
	char	*name;		/* Name of shell variable */
	int		vsize;		/* Size of storage allocated for value */
	struct	s_shell *next;
};

struct s_shell *shell_vars;

/* If no malloc, then use locally defined env_alloc() and env_free()...
 */
#if INCLUDE_MALLOC

#define env_alloc	malloc
#define env_free	free

#else

#define ENV_ALLOC_TOT	48
#define ENV_ALLOC_SIZE	(sizeof(struct s_shell)+8)

struct env_space {
	int inuse;
	char space[ENV_ALLOC_SIZE];
} envSpace[ENV_ALLOC_TOT];


char *
env_alloc(int size)
{
	int	 i;

	if (size > ENV_ALLOC_SIZE)
		return(0);

	for(i=0;i<ENV_ALLOC_TOT;i++) {
		if (envSpace[i].inuse == 0) {
			envSpace[i].inuse = 1;
			memset(envSpace[i].space,0,ENV_ALLOC_SIZE);
			return(envSpace[i].space);
		}
	}
	return(0);
}

void
env_free(char *space)
{
	int i;

	for(i=0;i<ENV_ALLOC_TOT;i++) {
		if (envSpace[i].space == space) {
			envSpace[i].inuse = 0;
			break;
		}
	}
	return;
}
#endif

/*
 *  Set()
 *
 *	Syntax:
 *		set var				clears the variable 'var'
 *		set var value		assign "value" to variable 'var'
 *		set -a var value	AND 'var' with 'value'
 *		set -o var value	OR 'var' with 'value'
 *		set -i var [value]	increment 'var' by 'value' (or 1 if no value)
 *		set -d var [value]	decrement 'var' by 'value' (or 1 if no value)
 *		set -x 				result of -i/-d is in hex
 */
char *SetHelp[] = {
	"Shell variable operations",
#if INCLUDE_EE
	"-[ab:cdef:iox] [varname[=expression]] [value]",
#else
	"-[ab:cdef:iox] [varname] [value]",
#endif
#if INCLUDE_VERBOSEHELP
	" -a        AND var with value",
	" -b        set console baudrate",
	" -c        clear the environment",
	" -d        decrease var by value (or 1)",
	" -e        build an environ string",
#if INCLUDE_TFS
	" -f{file}  create script from environment",
#endif
	" -i        increase var by value (or 1)",
	" -o        OR var with value",
	" -x        result in hex (NA with expressions, use hex())",
#endif
	0,
};

#define SET_NOOP	0
#define SET_INCR	1
#define SET_DECR	2
#define SET_OR		3
#define SET_AND		4

int
Set(int argc,char *argv[])
{
	char	*envp, buf[CMDLINESIZE];
	int		opt, decimal, setop, i;

	setop = SET_NOOP;
	envp = (char *)0;
	decimal = 1;
	while((opt=getopt(argc,argv,"ab:cdef:iox")) != -1) {
		switch(opt) {
		case 'a':		/* logical and */
			setop = SET_AND;
			decimal = 0;
			break;
		case 'b':
			ChangeConsoleBaudrate(atoi(optarg));
			return(CMD_SUCCESS);
		case 'c':		/* clear environment */
			clearenv();
			break;
		case 'd':		/* decrement */
			setop = SET_DECR;
			break;
		case 'e':	
			envp = getenvp();
			break;
#if INCLUDE_TFS	
		case 'f':		/* build script from environment */
			envToExec(optarg);
			return(0);
#endif
		case 'i':		/* increment */
			setop = SET_INCR;
			break;
		case 'o':		/* logical or */
			setop = SET_OR;
			decimal = 0;
			break;
		case 'x':
			decimal = 0;
			break;
		default:
			return(CMD_PARAM_ERROR);
		}
	}

	if (!shell_vars) {
		printf("No memory allocated for environment.\n");
		return(CMD_FAILURE);
	}

	if (setop != SET_NOOP) {	/* Do some operation on a shell variable */
		char	*varval;
		unsigned long	value, opval;

		/* For -i & -d, if value is not specified, then assume 1. */
		if (argc == optind+1) {
			if ((setop == SET_INCR) || (setop == SET_DECR))
				opval = 1;
			else
				return(CMD_PARAM_ERROR);
		}
		else if (argc == optind+2)
			opval = strtoul(argv[optind+1],0,0);
		else
			return(CMD_PARAM_ERROR);

		varval = getenv(argv[optind]);
		if (!varval) {
			printf("%s: not found\n", argv[optind]);
			return(CMD_FAILURE);
		}
			
		value = strtoul(varval,(char **)0,0);
		switch(setop) {
			case SET_INCR:
				value += opval;
				break;
			case SET_DECR:
				value -= opval;
				break;
			case SET_AND:
				value &= opval;
				break;
			case SET_OR:
				value |= opval;
				break;
		}
		if (decimal)
			sprintf(buf,"%ld",value);
		else
			sprintf(buf,"0x%lx",value);
		setenv(argv[optind],buf);
	}
	else if (argc == optind) {	/* display all variables */
		shell_print();
	}
	else if (argc == (optind+1)) {	/* run EE or clear one var or set envp */
#if INCLUDE_EE
		switch(setEE(argv[optind])) {
			case 1:
				return(CMD_SUCCESS);
			case -1:
				return(CMD_FAILURE);
		}
#endif
		if (envp) 
			shell_sprintf(argv[optind],"0x%lx",(ulong)envp);
		else
			setenv(argv[optind],0);
	}
	else if (argc >= (optind+2)) {	/* Set a specific variable */
		buf[0] = 0;
		for(i=optind+1;i<argc;i++) {
			if ((strlen(buf) + strlen(argv[i]) + 2) >= sizeof(buf)) {
				printf("String too large\n");
				break;
			}
			strcat(buf,argv[i]);
			if (i != (argc-1))
				strcat(buf," ");
		}
		if (!decimal)
			shell_sprintf(argv[optind],"0x%lx",atoi(buf));
		else
			setenv(argv[optind],buf);
	}
	else 
		return(CMD_PARAM_ERROR);

	return(CMD_SUCCESS);
}

/* Shell variable support routines...
 *	The basic scheme is to malloc in the space needed for the variable
 *	name and the value of that variable.  For each variable that 
 *	exists there is one s_shell structure that is in the linked list.
 *	As shell variables are removed, their corresonding s_shell structure
 *	is NOT removed, but the data pointed to within the structure is
 *	freed.  This keeps the linked list implementation extremely simple
 *	but maintains the versatility gained by using malloc for the 
 *	variables instead of some limted set of static arrays.
 */


/* shell_alloc():
 *	First scan through the entire list to see if the requested
 *	shell variable name already exists in the list; if it does,
 *	then just use the same s_shell entry but change the value.
 *	Also, if the new value fits in the same space as the older value,
 *	then just use the same memory space (don't do the free/malloc).
 *	If it doesn't, then scan through the list again.  If there
 *	is one that has no variable assigned to it (name = 0), then
 *	use it for the new allocation.  If all s_shell structures do 
 *	have valid entries, then malloc a new s_shell structure and then
 *	place the new shell variable data in that structure.
 */

static int
shell_alloc(char *name,char *value)
{
	int	namelen, valuelen;
	struct s_shell *sp;

	sp = shell_vars;
	namelen = strlen(name);
	valuelen = strlen(value);
	while(1) {
		if (sp->name == (char *)0) {
			if (sp->next != (struct s_shell *)0) {
				sp = sp->next;
				continue;
			}
			else
				break;
		}
		if (strcmp(sp->name,name) == 0) {
			if (sp->vsize < valuelen+1) {		/* If new value is smaller	*/
				env_free(sp->val);				/* than the old value, then */
				sp->val = env_alloc(valuelen+1);/* don't re-allocate any	*/
				if (!sp->val)					/* memory, just copy into	*/
					return(-1);					/* the space used by the	*/
				sp->vsize = valuelen+1;			/* previous value.			*/
			}
			strcpy(sp->val,value);
			return(0);
		}
		if (sp->next == (struct s_shell *)0) 
			break;
		sp = sp->next;
	}
	sp = shell_vars;
	while(1) {
		if (sp->name == (char *)0) {
			sp->name = env_alloc(namelen+1);
			if (!sp->name)
				return(-1);
			strcpy(sp->name,name);
			sp->val = env_alloc(valuelen+1);
			if (!sp->val)
				return(-1);
			sp->vsize = valuelen+1;
			strcpy(sp->val,value);
			return(0);
		}
		if (sp->next != (struct s_shell *)0)
			sp = sp->next;
		else {
			sp->next = (struct s_shell *)env_alloc(sizeof(struct s_shell));
			if (!sp->next)
				return(-1);
			sp = sp->next;
			sp->name = env_alloc(namelen+1);
			if (!sp->name)
				return(-1);
			strcpy(sp->name,name);
			sp->val = env_alloc(valuelen+1);
			if (!sp->val)
				return(-1);
			sp->vsize = valuelen+1;
			strcpy(sp->val,value);
			sp->next = (struct s_shell *)0;
			return(0);
		}
	}
}

/* shell_dealloc():
 *	Remove the requested shell variable from the list.  Return 0 if
 *	the variable was removed successfully, otherwise return -1.
 */
static int
shell_dealloc(char *name)
{
	struct	s_shell *sp;

	sp = shell_vars;
	while(1) {
		if (sp->name == (char *)0) {
			if (sp->next == (struct s_shell *)0)
				return(-1);
			else {
				sp = sp->next;
				continue;
			}
		}
		if (strcmp(name,sp->name) == 0) {
			env_free(sp->name);
			env_free(sp->val);
			sp->name = (char *)0;
			sp->val = (char *)0;
			return(0);
		}
		
		if (sp->next == (struct s_shell *)0)
			return(-1);
		else
			sp = sp->next;
	}
}

/* ConsoleBaudEnvSet():
 * Called by to load/reload the CONSOLEBAUD shell variable based on
 * the content of the global variable 'ConsoleBaudRate'.
 */
void
ConsoleBaudEnvSet(void)
{
	char	buf[16];

	sprintf(buf,"%d",ConsoleBaudRate);
	setenv("CONSOLEBAUD",buf);
}

/* ShellVarInit();
 *	Setup the shell_vars pointer appropriately for additional
 *	shell variable assignments that will be made through shell_alloc().
 */
int
ShellVarInit()
{
	char	buf[16];

#if !INCLUDE_MALLOC
	memset((char *)&envSpace,0,sizeof(envSpace));
#endif

	shell_vars = (struct s_shell *)env_alloc(sizeof(struct s_shell));
	if (!shell_vars) {
		printf("No memory for environment initialization\n");
		return(-1);
	}
	shell_vars->next = (struct s_shell *)0;
	shell_vars->name = (char *)0;
	setenv("PROMPT",PROMPT);
	sprintf(buf,"0x%lx",APPLICATION_RAMSTART);
	setenv("APPRAMBASE",buf);
	sprintf(buf,"0x%lx",BOOTROM_BASE);
	setenv("BOOTROMBASE",buf);
	setenv("PLATFORM",PLATFORM_NAME);
	setenv("MONITORBUILT",monBuilt());
	shell_sprintf("MONCOMPTR","0x%lx",(ulong)&moncomptr);
#if INCLUDE_HWTMR
	shell_sprintf("TARGETTIMER","0x%x",target_timer);
	shell_sprintf("TICKSPERMSEC","0x%x",TIMER_TICKS_PER_MSEC);
#endif

	/* Support the ability to have additional target-specific 
	 * shell variables initialized at startup...
	 */
#ifdef TARGET_ENV_SETUP
	TARGET_ENV_SETUP();
#endif

	shell_sprintf("VERSION_MAJ","%d",MAJOR_VERSION);
	shell_sprintf("VERSION_MIN","%d",MINOR_VERSION);
	shell_sprintf("VERSION_TGT","%d",TARGET_VERSION);
	return(0);
}

/* getenv:
 *	Return the pointer to the value entry if the shell variable
 *	name is currently set; otherwise, return a null pointer.
 */
char *
getenv(char *name)
{
	register struct	s_shell *sp;

	for(sp = shell_vars;sp != (struct s_shell *)0;sp = sp->next) {
		if (sp->name != (char *)0) {
			if (strcmp(sp->name,name) == 0)
				return(sp->val);
		}
	}
	return((char *)0);
}

/* getenvp:
 *	Build an environment string consisting of all shell variables and
 *	their values concatenated into one string.  The format is
 *
 *	  NAME=VALUE LF NAME=VALUE LF NAME=VALUE LF NULL
 *
 *	with the limit in size being driven only by the space
 *	available on the heap.  Note that this uses malloc, and it
 *	the responsibility of the caller to free the pointer when done.
 */
char *
getenvp(void)
{
	int size;
	char *envp, *cp;
	register struct	s_shell *sp;

	size = 0;

	/* Get total size of the current environment vars */
	for(sp = shell_vars;sp != (struct s_shell *)0;sp = sp->next) {
		if (sp->name != (char *)0) {
			size += (strlen(sp->name) + strlen(sp->val) + 2);
		}
	}
	if (size == 0)
		return((char *)0);

	envp = env_alloc(size+1);	/* leave room for final NULL */
	if (envp == 0)
		return((char *)0);

	cp = envp;
	for(sp = shell_vars;sp != (struct s_shell *)0;sp = sp->next) {
		if (sp->name != (char *)0)
			cp += sprintf(cp,"%s=%s\n",sp->name,sp->val);
	}
	*cp = 0;		/* Append NULL after final separator */
	return(envp);
}

/* clearenv():
 * Clear out the entire environment.
 */
void
clearenv(void)
{
	struct	s_shell *sp;

	for(sp = shell_vars;sp != (struct s_shell *)0;sp = sp->next) {
		if (sp->name != (char *)0) {
			env_free(sp->name);
			env_free(sp->val);
			sp->name = (char *)0;
			sp->val = (char *)0;
		}
	}
}

/* setenv:
 *	Interface to shell_dealloc() and shell_alloc().
 */
int
setenv(char *name,char *value)
{
	if (!shell_vars)
		return(-1);
	if ((value == (char *)0) || (*value == 0))
		return(shell_dealloc(name));
	else
		return(shell_alloc(name,value));
}

/* shell_print():
 *	Print out all of the current shell variables and their values.
 */
int
shell_print(void)
{
	int	maxlen, len;
	char format[8];
	register struct	s_shell *sp;

	/* Before printing the list, pass through the list to determine the
	 * largest variable name.  This is used to create a format string
	 * that is then passed to printf() when printing the list of
	 * name/value pairs.  It guarantees that regardless of the length
	 * of the name, the format of the printed out put will be consistent
	 * for all variables.
	 */
	maxlen = 0;
	sp = shell_vars;
	while(1) {
		if (sp->name) {
			len = strlen(sp->name);
			if (len > maxlen)
				maxlen = len;
		}
		if (sp->next != (struct s_shell *)0)
			sp = sp->next;
		else
			break;
	}
	sprintf(format,"%%%ds = ",maxlen+1);

	/* Now that we know the size of the largest variable, we can 
	 * print the list cleanly...
	 */
	sp = shell_vars;
	while(1) {
		if (sp->name != (char *)0) {
			printf(format, sp->name);
			puts(sp->val);		/* sp->val may overflow printf, so use puts */
		}
		if (sp->next != (struct s_shell *)0)
			sp = sp->next;
		else
			break;
	}
	return(0);
}

/* shell_sprintf():
 *	Simple way to turn a printf-like formatted string into a shell variable.
 */
int
shell_sprintf(char *varname, char *fmt, ...)
{
	int	tot;
	char buf[CMDLINESIZE];
	va_list argp;

	va_start(argp,fmt);
	tot = vsnprintf(buf,CMDLINESIZE-1,fmt,argp);
	va_end(argp);
	setenv(varname,buf);
	return(tot);
}


#if INCLUDE_TFS
/* validEnvToExecVar():
 * Return 1 if the variable should be included in the script
 * generated by envToExec(); else return 0.
 * Specifically... if the variable is generated internally
 * then we don't want to include it in the script.
 */
int
validEnvToExecVar(char *varname)
{
	char **vp;
	static char *invalid_varprefixes[] = {
		"ARG",			"TFS_PREFIX_",	"TFS_START_",
		"TFS_END_",		"TFS_SPARE_",	"TFS_SPARESZ_",
		"TFS_SCNT_",	"TFS_DEVINFO_", "FLASH_BASE_",
		"FLASH_SCNT_",	"FLASH_END_",
		0
	};
	static char *invalid_varnames[] = {
		"APPRAMBASE",	"BOOTROMBASE",	"CMDSTAT",
		"CONSOLEBAUD",	"MALLOC",		"MONCOMPTR",
		"MONITORBUILT",	"PLATFORM",		"PROMPT",
		"TFS_DEVTOT",	"FLASH_DEVTOT",	"PROMPT",
		"VERSION_MAJ",	"VERSION_MIN",	"VERSION_TGT",
		"MONCMD_SRCIP",	"MONCMD_SRCPORT",
#if INCLUDE_HWTMR
		"TARGETTIMER",	"TICKSPERMSEC",
#endif
		0
	};

	if (varname == 0)
		return(0);

	if (strncmp(varname,"ARG",3) == 0)
		return(0);

#if INCLUDE_BOARDINFO
	if (BoardInfoVar(varname))
		return(0);
#endif

	for(vp=invalid_varnames;*vp;vp++) {
		if (!strcmp(varname,*vp))
			return(0);
	}
	for(vp=invalid_varprefixes;*vp;vp++) {
		if (!strncmp(varname,*vp,strlen(*vp)))
			return(0);
	}
	return(1);
}

/* envToExec():
   Create a file of "set" commands that can be run to recreate the
   current environment.
   Changed Oct 2008 to eliminate use of getAppRamStart().
*/
int
envToExec(char *filename)
{
	int		err, vartot, size, rc;
	char	*buf, *bp, *cp;
	register struct	s_shell *sp;

	sp = shell_vars;
	vartot = size = rc = 0;

	/* First go through the list to see how much space we need
	 * to allocate...
	 */
	while(1) {
		if (validEnvToExecVar(sp->name)) {
			size += strlen(sp->name) + 6;
			cp = sp->val;
			while(*cp) {
				if (*cp == '$')
					size++;
				size++;
				cp++;
			}
			size += 3;
			vartot++;
		}
		if (sp->next != (struct s_shell *)0)
			sp = sp->next;
		else
			break;
	}
	if (size == 0)
		return(0);

	/* Now that we know the space needed (stored in 'size' variable),
	 * allocate it and build the new file in that space, then use tfsadd()
	 * to create the file...
	 */
	vartot = 0;
	sp = shell_vars;
	buf = bp = (char *)env_alloc(size);
	while(1) {
		/* Note: if this code changes, then the code above that is used to
		 * allocate the buffer size may also need to change...
		 */
		if (validEnvToExecVar(sp->name)) {
			bp += sprintf(bp,"set %s \"",sp->name);
			cp = sp->val;
			while(*cp) {
				if (*cp == '$')
					*bp++ = '\\';
				*bp++ = *cp++;
			}
			*bp++ = '\"';
			*bp++ = '\n';
			*bp = 0;
			vartot++;
		}
		if (sp->next != (struct s_shell *)0)
			sp = sp->next;
		else
			break;
	}
	if (vartot > 0) {
		err = tfsadd(filename,"envsetup","e",(unsigned char *)buf,strlen(buf));
		if (err != TFS_OKAY) {
			printf("%s: %s\n",filename,(char *)tfsctrl(TFS_ERRMSG,err,0));
			rc = -1;
		}
	}
	env_free(buf);
	return(rc);
}
#endif

#else

/* The 'set' command is part of the build even if INCLUDE_SHELLVARS
 * is false.  This allows the user to still access teh "set -b ###"
 * facility for changing the baudrate.
 */
char *SetHelp[] = {
	"Set baud",
	"-[b:] (no args)",
#if INCLUDE_VERBOSEHELP
	" -b        set console baudrate",
#endif
	0,
};

int
Set(int argc,char *argv[])
{
	int		opt;

	while((opt=getopt(argc,argv,"b:")) != -1) {
		switch(opt) {
		case 'b':
			ChangeConsoleBaudrate(atoi(optarg));
			return(CMD_SUCCESS);
			break;
		default:
			return(CMD_PARAM_ERROR);
		}
	}
	printf("Shell vars not included in build.\n");
	return(CMD_FAILURE);
}

int
setenv(char *name,char *value)
{
	return(-1);
}

char *
getenv(char *name)
{
	return(0);
}

int
shell_sprintf(char *varname, char *fmt, ...)
{
	return(0);
}

void
ConsoleBaudEnvSet(void)
{
}

char *
getenvp(void)
{
	return(0);
}

#endif

/* ChangeConsoleBaudrate():
 * Called to attempt to adjust the console baudrate.
 * Support 2 special cases:
 *   if baud == 0, then just turn off console echo;
 *   if baud == 1, turn it back on.
 */
int
ChangeConsoleBaudrate(int baud)
{
	if (baud == 0)
		console_echo(0);
	else if (baud == 1)
		console_echo(1);
	else {
		if (ConsoleBaudSet(baud) < 0) {
			printf("Baud=%d failed\n",baud);
			return(-1);
		}
		ConsoleBaudRate = baud;
		ConsoleBaudEnvSet();
	}
	return(0);
}

