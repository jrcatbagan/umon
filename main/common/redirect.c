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
 * redirect:
 *
 *	This code supports the monitor's ability to redirect what was destined for
 *	the console to a block of memory in RAM.  Enough data is maintained so
 *	that the block of memory can be transferred to a file using tfsadd().
 *
 *	Description of redirection CLI syntax:
 *		>  buffer_addr,buffer_size[,filename]
 *		>> [filename]
 *
 *	- Single arrow starts up a redirection to some specified block of memory
 *	  with a specified size.  If the filename is specified, then the output
 *	  of that single command is redirected to the buffer then transferred to
 *	  the specified file in TFS.  Any previously existent file of the same
 *	  name is overwritten.
 *
 *  - Double arrow with no argument says append output of this command to
 *	  the previously established buffer.
 *
 *	- Double arrow with an argument says append output of this command to
 *	  the previously established buffer and then write that buffer to the
 *	  specified filename.  Once again, any previously existent file of the
 *	  same name is overwritten.
 *
 * Original author:     Ed Sutter (ed.sutter@alcatel-lucent.com)
 *
 */
#include "config.h"
#include "genlib.h"
#include "stddefs.h"
#include <ctype.h>
#include "tfs.h"
#include "tfsprivate.h"

#if INCLUDE_REDIRECT

/* Redirect states:
 *	ACTIVE: the current command has issued some redirection, so
 *		when RedirectCharacter() is called, it should copy the
 *		character to the redirect buffer.
 *  IDLE: the current command has not issued any redirection, so
 *		when RedirectCharacter() is called, it simply returns.
 *	UNINITIALIZED: the first call to >base,size[,file] has not been
 *		made so we don't have a buffer; hence we can't redirect anything.
 */
#define REDIRECT_UNINITIALIZED	0
#define REDIRECT_ACTIVE			0x12345678
#define REDIRECT_IDLE			0x87654321

static int	RedirectSize, RedirectState;
static char *RedirectBase, *RedirectPtr, *RedirectEnd;
static char RedirectFile[TFSNAMESIZE];

/* getEnvVal():
 * This function is used under the context of the redirection code
 * because of the fact that the docommand() function processes the
 * redirection arrow prior to converting shell variables.  
 * This is important because we want to support the ability to do
 * something like...
 *
 *   echo hello >$APPRAMBASE,100
 *
 * but we also want to allow a shell variable to contain a right
 * arrow that is NOT processed by this redirection code.
 */ 
static ulong
getEnvVal(char *ptr)
{
#if INCLUDE_SHELLVARS
	char	*v;

	/* If the incoming string starts with a '$', then retrieve the
	 * value and then convert that to a long; otherwise, just convert
	 * the string to a long immediately.
	 */
	if (*ptr == '$') {
		v = shellsym_chk(*ptr,ptr+1,0,0,0);
		if (!v)
			return(0);
	}
	else
		v = ptr;
	return(strtoul(v,0,0));
#else
	return(0);
#endif
}

int
RedirectionCheck(char *cmdcpy)
{
	int	inquote;
	char *arrow, *base, *comma, *space;

	base = cmdcpy;
	arrow = (char *)0;

	/* Parse the incoming command line looking for a right arrow.
	 * This parsing assumes that there will be no negated arrows
	 * (preceding backslash) after a non-negated arrow is detected.
	 * Note that a redirection arrow within a double-quote set is
	 * ignored.  This allows a shell variable that contains a right arrow
	 * to be printed properly if put in double quotes.
	 *	For example...
	 *	set PROMPT "maint> "
	 *	echo $PROMPT	# This will generate a redirection syntax error
	 *  echo "$PROMPT"	# This won't.
	 */
	inquote = 0;
	while(*cmdcpy) {
		if ((*cmdcpy == '"') && ((cmdcpy == base) || (*(cmdcpy-1) != '\\'))) {
			inquote = inquote == 1 ? 0 : 1;
			cmdcpy++;
			continue;
		}	
		if (inquote == 1) {
			cmdcpy++;
			continue;
		}
		if (*cmdcpy == '>') {
			arrow = cmdcpy;
			if (*(arrow-1) == '\\') {
				strcpy(arrow-1,arrow);
				cmdcpy = arrow+1;
				arrow = (char *)0;
				continue;
			}
			break;
		}
		cmdcpy++;
	}
	if (arrow == (char *)0)
		return(0);

	/* Remove the remaining text from the command line because it is to
	 * be used only by the redirection mechanism.
	 */
	*arrow = 0;

	/* Now parse the text after the first non-negated arrow. */
	if (*(arrow+1) == '>') {
		if (RedirectState == REDIRECT_UNINITIALIZED) {
			printf("Redirection not initialized\n");
			return(-1);
		}
		arrow += 2;
		while(isspace(*arrow))
			arrow++;
		if (*arrow != 0) {
			strncpy(RedirectFile,*arrow == '$' ? getenv(arrow+1) : arrow,
				TFSNAMESIZE);
		}
	}
	else {
		RedirectPtr = RedirectBase = (char *)getEnvVal(arrow+1);
		comma = strchr(arrow+1,',');
		if (comma) {
			RedirectSize = (int)getEnvVal(comma+1);
			comma = strchr(comma+1,',');
			if (RedirectSize <= 0) {
				printf("Redirection size error: %d\n",RedirectSize);
				return(-1);
			}
			RedirectEnd = RedirectBase + RedirectSize;
			if (comma) {
				space = strpbrk(comma," \t\r\n");
				if (space)
					*space = 0;
				if (*(comma+1) == '$') {
					if (getenv(comma+2))
						strncpy(RedirectFile,getenv(comma+2),TFSNAMESIZE);
					else
						RedirectFile[0] = 0;
				}
				else
					strncpy(RedirectFile,comma+1,TFSNAMESIZE);
			}
			else
				RedirectFile[0] = 0;
		}
		else {
			printf("Redirection syntax error\n");
			return(-1);
		}
	}
	RedirectState = REDIRECT_ACTIVE;
	return(0);
}

/* RedirectCmdDone():
 * As each command completes, this redirection mechanism must be notified.
 * When this function is called (by docmd() after the command function has
 * completed), if the RedirectFile[] array is populated, it transfers the
 * buffer to the specified file; if the array is empty, then no TFS action
 * is taken.
 * The file specified in RedirectFile[] may have additional commas in it.
 * They allow the filename to contain flags and info field.
 */
void
RedirectionCmdDone(void)
{

	if (RedirectState != REDIRECT_UNINITIALIZED) {
		RedirectState = REDIRECT_IDLE;
		if (RedirectFile[0]) {
			char	*comma, *info, *flags;

			comma = info = flags = (char *)0;
			comma = strchr(RedirectFile,',');
			if (comma) {
				*comma = 0;
				flags = comma+1;
				comma = strchr(flags,',');
				if (comma) {
					*comma = 0;
					info = comma+1;
				}
			}
			tfsadd(RedirectFile,info,flags,(uchar *)RedirectBase,
				(int)(RedirectPtr-RedirectBase));
			RedirectFile[0] = 0;
			RedirectPtr = RedirectBase;
		}
	}
}

/* RedirectCharacter():
 * This function is called from putchar() in chario.c.  It simply
 * copies the incoming character to the redirection buffer.
 */
void
RedirectCharacter(char c)
{
	if (RedirectState == REDIRECT_ACTIVE) {
		if (RedirectPtr < RedirectEnd)
			*RedirectPtr++ = c;
	}
}
#else
int
RedirectionCheck(char *cmdcpy)
{
	return(0);
}
#endif
