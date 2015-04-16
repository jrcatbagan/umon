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
 * cli.h:
 *
 * Header file for Command Line Interface related stuff.
 *
 * Original author:     Ed Sutter (ed.sutter@alcatel-lucent.com)
 *
 */
#ifndef _CLI_H_
#define _CLI_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Command table structure used by the monitor:
 */
struct	monCommand {
    char    *name;					/* Name of command seen by user. */
    int	    (*func)(int,char **);	/* Called when command is invoked. */
    char    **helptxt;				/* Help text (see notes below). */
	long	flags;					/* Single-bit flags for various uses */
									/* (see the CMDFLAG_XXX macros). */
};

#ifdef __cplusplus
}
#endif

/* Bits currently assigned to command flags used in the monCommand
 * structure...
 */
#define CMDFLAG_NOMONRC	1

/* Maximum size of a command line:
 */
#ifndef CMDLINESIZE
#define CMDLINESIZE	256
#endif

/* Maximum number of arguments in a command line:
 */
#define ARGCNT		24

/* Definitions for docommand() return values:
 *
 * Note that the CMD_SUCCESS, CMD_FAILURE and CMD_PARAM_ERROR are return
 * values used by the local command code also.  The remaining errors
 * (CMD_LINE_ERROR, CMD_ULVL_DENIED and CMD_NOT_FOUND) are used only by
 # the docommand() function.
 *
 *	CMD_SUCCESS:
 *		Everything worked ok.
 *	CMD_FAILURE:
 *		Command parameters were valid, but command itself failed for some other
 *		reason. The docommand() function does not print a message here, it
 *		is assumed that the error message was printed by the local function.
 *	CMD_PARAM_ERROR:
 *		Command line did not parse properly.  Control was passed to a 
 *		local command function, but argument syntax caused it to choke.
 *		In this case docommand() will print out the generic CLI syntax error
 *		message.
 *	CMD_LINE_ERROR:
 *		Command line itself was invalid.  Too many args, invalid shell var
 *		syntax, etc.. Somekind of command line error prior to checking for
 *		the command name-to-function match.
 *	CMD_ULVL_DENIED:
 *		Command's user level is higher than current user level, so access
 *		is denied.
 *	CMD_NOT_FOUND:
 *		Since these same return values are used for each command function
 *		plus the docommand() function, this error indicates that docommand()
 *		could not even find the command in the command table.
 *	CMD_MONRC_DENIED:
 *		The command cannot execute because it is considered illegal
 *		when run from within the monrc file.
 */
#define CMD_SUCCESS			0
#define CMD_FAILURE			-1
#define CMD_PARAM_ERROR		-2
#define CMD_LINE_ERROR		-3
#define CMD_ULVL_DENIED		-4
#define CMD_NOT_FOUND		-5
#define CMD_MONRC_DENIED	-6

/* Notes on help text array:
 * The monitor's CLI processor assumes that every command's help text 
 * array abides by a few basic rules...
 * First of all, it assumes that every array has AT LEAST two strings.
 * The first string in the array of strings is assumed to be a one-line
 * abstract describing the command.
 * The second string in the array of strings is assumed to be a usage
 * message that describes the syntax of the arguments needed by the command.
 * If this second string is an empty string (""), the docommand() prints out
 * a generic usage string indicating that there are no options or arguements
 * to apply to the command.
 * All remaining lines are formatted based on the needs of the individual
 * command and the final string is a null pointer to let the CLI processor
 * know where the end is.
 * Following is an example help text array...
 *
 *	char *HelpHelp[] = {
 *			"Display command set",
 *			"-[d] [commandname]",
 *			"Options:",
 *			" -d   list commands and descriptions",
 *			0,
 *	};
 *
 */
#endif
