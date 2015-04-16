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
 * edit.c:
 *
 * A simple buffer editor. It allows a user to pull a TFS file into RAM,
 * make some "ed-like" modifications to the ASCII data and then write
 * that data back to TFS.
 *
 * Original author:     Ed Sutter (ed.sutter@alcatel-lucent.com)
 *
 */
#include "config.h"
#if INCLUDE_EDIT
#include "stddefs.h"
#include "genlib.h"
#include "tfs.h"
#include "tfsprivate.h"
#include <ctype.h>
#include "cli.h"

extern char *file_line_edit(char *);
static int	rmCR(char *,int);
static int	gotoline(char *,char *,char **,char *,int);
static int	searchfor(char *,char *,char **,char *);
static int	whatlineisthis(char *,char *);
static int	getrange(char *,int *,int *,char *,char *,char *);
static void	lnprbuf(char *,char *,char *);
static void	prbuf(char *,char *);
static void prlno(int,int);
static void deletelines(char *,int,char *,char **,char **);
static char	*prline(char *,char *);
static char *skipwhite(char *);

#define BREAK			1
#define CONTINUE		2
#define FALLTHROUGH		3
#define INSERT			4
#define COMMAND			5
#define ESCAPE	 		0x1b

#define ILCMAX		8

void
efree(int size, char *buffer)
{
	if (size != 0)
		free(buffer);
}

/* Help text while in the editor: */
char *edithelp[] = {
	"Editor commands:",
	"d{LRNG}    delete line",
#if INCLUDE_LINEEDIT
	"e#         edit line # (uses 'ksh/vi-like' command line editing)",
#endif
	"i          enter insert mode (use '.' to exit insert mode)",
	"a          enter append mode (use '.' to exit append mode)",
	"j#         join line '#' with line '#+1'",
	"P[LRNG]    print buffer with line numbers",
	"p          print buffer",
	"q[fname]   quit edit, write file",
	"s[srchstr] goto line containing search string",
	"x          exit edit, do not write file",
	"#          goto line # (use '$' to go to last line)",
	".+/-#      goto line relative to current position",
	"",
	"Where...",
	"# represents a decimal number;",
	"LRNG represents a line number or inclusive line range (# or #-#);",
	0,
};

/* Help text outside the editor: */

char *EditHelp[] = {
	"Edit file or buffer",
	"-[b:c:f:i:m:rs:t] [filename]",
#if INCLUDE_VERBOSEHELP
	" -b {adr}  specify buffer base address",
	" -c {cmd}  in-line command",
	" -f {flgs} file flags (see tfs)",
	" -i {info} file info (see tfs)",
	" -m {size} specify allocation size for buffer",
	" -r        do not automatically remove carriage returns from file",
	" -s {size} size of buffer",
	" -t        convert tabs to spaces",
#endif
	0,
};

int
Edit(int argc,char *argv[])
{
	int		i, opt, tfd, quit, mode, bsize;
	char	ln[CMDLINESIZE], *lp, *cp, *cp1, *filename, *flags, *info;
	char	*buffer, *bp;	/* Buffer and pointer into buffer. */
	char	*eob;			/* Current EOF  pointer (max distance between */
							/* bp and buffer). */
	char	*inlinecmd[ILCMAX];	/* If non-NULL, it points to a list of */
								/* commands to be executed prior to going */
								/* into interactive mode. */
	int	ilcidx;				/* Index into in-line command table. */
	int	len;				/* Length of the incoming line. */
	int	size;				/* Size of buffer specified on command line. */
	int	cmdtot;				/* Running total of incoming lines. */
	int delcnt;				/* Used by :d when a range is specified. */
	int noCR;				/* Set by -r option to remove Carriage Returns */
	TFILE	tstruct, *tfp;

	tfp = (TFILE *)0;
	size = bsize = 0;
	flags = filename = (char*)0;
	buffer = (char *)getAppRamStart();
	info = (char *)0;
	noCR = 1;
	ilcidx = 0;
	for(i=0;i<8;i++)
		inlinecmd[i] = (char *)0;

	while ((opt = getopt(argc, argv, "b:c:f:i:m:rs:")) != -1) {
		switch (opt) {
		case 'b':
			buffer = (char *)strtol(optarg,(char **)0,0);
			break;
		case 'c':
			if (ilcidx < ILCMAX-1)
				inlinecmd[ilcidx++] = optarg;
			break;
		case 'f':
			flags = optarg;
			break;
		case 'i':
			info = optarg;
			break;
		case 'm':
			bsize = strtol(optarg,0,0);
			break;
		case 'r':
			noCR = 0;
			break;
		case 's':
			size = (int)strtol(optarg,(char **)0,0);
			break;
		default:
			return (CMD_PARAM_ERROR);
		}
	}

	if (bsize != 0) {
		buffer = malloc(bsize);
		if (!buffer)
			return(CMD_FAILURE);
	}

	if (argc == optind+1) {
		filename = argv[optind];
		if (tfsfstat(filename,&tstruct) != -1) {
			tfp = &tstruct;
			size = TFS_SIZE(tfp);
			if ((bsize > 0) && (size > bsize)) {
				printf("Allocation too small\n");
				efree(bsize,buffer);
				return(CMD_FAILURE);
			}
			tfd = tfsopen(filename,TFS_RDONLY,0);
			if (tfd < 0) {
				printf("%s: %s\n",filename,(char *)tfsctrl(TFS_ERRMSG,tfd,0));
				efree(bsize,buffer);
				return(CMD_FAILURE);
			}
			else {
				tfsread(tfd,buffer,size);
				tfsclose(tfd,0);
			}
		}
	}

	if ((noCR) && (size != 0)) {
		if ((buffer[size-1] != 0x0d) && (buffer[size-1] != 0x0a)) {
			printf("EOF linefeed inserted.\n");
			buffer[size++] =  0x0a;
			buffer[size] = 0;
		}
		size -= rmCR(buffer,size);
	}
	
	bp = buffer;
	eob = buffer + size;

	if (!ilcidx)
		printf("Edit buffer = 0x%lx.\nType '?' for help\n",(ulong)buffer);

	quit = 0;
	cmdtot = 0;
	mode = COMMAND;
	while(!quit) {
		lp = ln;
		if (ilcidx > cmdtot)
			strcpy(ln,inlinecmd[cmdtot]);
		else if (ilcidx) {		/* If ILC command set doesn't terminate */
								/* the edit session, force it here. */
			efree(bsize,buffer);
			return(CMD_SUCCESS);
		}
		else
			getline(ln,CMDLINESIZE,0);
		cmdtot++;
		if (mode == INSERT) {
			if (!strcmp(lp,"."))
				mode = COMMAND;
			else {
				len = strlen(lp) + 1;
				cp1 = eob + len;
				cp = eob;
				while(cp > bp) *--cp1 = *--cp;
				while(*lp) *bp++ = *lp++;
				*bp++ = '\n';
				eob += len;
			}
			continue;
		}
		if (!strcmp(ln,"x")) {				/* Exit editor now. */
			efree(bsize,buffer);
			return(CMD_SUCCESS);
		}
		else if (!strcmp(ln,"?")) {			/* Display help info. */
			char	**eh;

			eh = edithelp;
			while(*eh)
				printf("%s\n",*eh++);
		}
		else if (!strcmp(ln,"p"))			/* Print buffer. */
			prbuf(buffer,eob);
		else if (!strcmp(ln,"P"))			/* Print buffer with lines. */
			lnprbuf(buffer,bp,eob);
		else if (!strcmp(ln,"i"))			/* Enter insert mode. */
			mode = INSERT;
		else if (!strcmp(ln,"a")) {			/* Enter insert mode at */
			mode = INSERT;					/* next line */
			gotoline(".+1",buffer,&bp,eob,0);
		}
		else if (!strcmp(ln,"$")) {			/* Goto last line. */
			gotoline(ln,buffer,&bp,eob,0);
			gotoline(".-1",buffer,&bp,eob,1);
		}
		else if (isdigit(ln[0]) || (ln[0]=='.'))	/* Goto line. */
			gotoline(ln,buffer,&bp,eob,1);
		else if (ln[0] == 's') {			/* Go to line containing string. */
			char *str;
			str = skipwhite(&ln[1]);
			if (!*str)
				gotoline(".+1",buffer,&bp,eob,0);
			searchfor(str,buffer,&bp,eob);
		}
#if INCLUDE_LINEEDIT
		else if (ln[0] == 'e') {
			char	*copy, *eoc, *cp2, crlf[2];
			int		oldlnsize, newlnsize, toendsize;

			if (gotoline(&ln[1],buffer,&bp,eob,0) == -1)
				continue;
			copy = malloc(CMDLINESIZE);
			if (!copy)
				continue;
			eoc = copy + CMDLINESIZE - 3;
			cp = bp;
			cp1 = copy;
			oldlnsize = 0;
			while ((*cp != 0x0a) && (*cp != 0x0d) && (cp1 != eoc)) {
				if (*cp == '\t')	/* Tabs are replaced with spaces to */
					*cp = ' ';		/* eliminate whitespace confusion in */
				putchar(*cp);		/* the line editor. */
				*cp1++ = *cp++;
				oldlnsize++;
			}
			crlf[0] = *cp;
			if (*cp != *(cp+1)) {
				crlf[1] = *(cp+1);
				if (*(cp+1) == 0x0a || *(cp+1) == 0x0d)
					oldlnsize++;
			}
			else {
				crlf[1] = 0;
			}
			oldlnsize++;
			*cp1++ = ESCAPE;
			*cp1 = 0;
			if (file_line_edit(copy) != (char *)0) {
				newlnsize = strlen(copy);
				toendsize = eob - (bp+oldlnsize);
				copy[newlnsize++] = crlf[0];
				if (crlf[1] == 0x0d || crlf[1] == 0x0a)
					copy[newlnsize++] = crlf[1];
				copy[newlnsize] = 0;
				if (oldlnsize == newlnsize) {
					memcpy(bp,copy,newlnsize);
				}
				else if (oldlnsize > newlnsize) {
					strcpy(bp,copy);
					memcpy(bp+newlnsize,cp+1,toendsize);
				}
				else {
					cp1 = eob-1;
					cp2 = (char *)(eob+(newlnsize-oldlnsize)-1);
					for(i=0;i<toendsize;i++)
						*cp2-- = *cp1--;
					memcpy(bp,copy,newlnsize);
				}
				eob -= oldlnsize;
				eob += newlnsize;
			}
			free(copy);
		}
#endif
		else if (ln[0] == 'P') {
			int	range, first, last, currentline;

			range = getrange(&ln[1],&first,&last,buffer,bp,eob);
			if (range > 0) {
				char *bpcpy;

				bpcpy = bp;
				gotoline(".",buffer,&bpcpy,eob,0);
				currentline = whatlineisthis(buffer,bpcpy);
				if (gotoline(&ln[1],buffer,&bpcpy,eob,0) == -1)
					continue;
				for(i=first;i<=last;i++) {
					prlno(i,currentline);
					bpcpy = prline(bpcpy,eob);
				}
			}				
		}
		else if (ln[0] == 'j') {	/* Join line specified with next line */
			char *tmpeob;

			if (gotoline(&ln[1],buffer,&bp,eob,0) == -1)
				continue;
			tmpeob=eob-1;
			while(bp < tmpeob) {
				if (*bp ==  '\n') {
					memcpy(bp,bp+1,eob-bp);
					eob--;
					break;
				}
				bp++;
			}
		}
		else if (ln[0] == 'd') {	/* Delete line (or range of lines) */
			delcnt = getrange(&ln[1],0,0,buffer,bp,eob);
			deletelines(&ln[1],delcnt,buffer,&bp,&eob);
		}
		else if (ln[0] == 'q') { 	/* Quit edit, if filename is present */
			lp = &ln[1];			/* use it. */
			lp = skipwhite(lp);
			if (*lp)
				filename = lp;
			quit = 1;
		}
		else {
			printf("huh?\n");
		}
	}

	if (filename) {
		int err;
		char	buf[16];

		if (tfp) {
			int	link;

			/* If filename and TFS_NAME(tfp) differ, then we are editing
			 * a file through a symbolic link.  If it is a link, then we
			 * use the info/flags/filename from the source file (using the
			 * file pointer and ignoring user input stuff).
			 */
			link = strcmp(filename,TFS_NAME(tfp));

			if ((!flags) || (link))
				flags = (char *)tfsctrl(TFS_FBTOA,TFS_FLAGS(tfp),(long)buf);
			if ((!info) || (link))
				info = TFS_INFO(tfp);
			if (link) {
				printf("Updating source file '%s' thru link '%s'\n",
					TFS_NAME(tfp),filename);
				filename = TFS_NAME(tfp);
			}
		}
		if (!flags)
			flags = (char *)0;
		if (!info)
			info = (char *)0;
		err = tfsadd(filename,info,flags,(unsigned char *)buffer,eob-buffer);
		if (err != TFS_OKAY)
			printf("%s: %s\n",filename,(char *)tfsctrl(TFS_ERRMSG,err,0));
	}
	efree(bsize,buffer);
	return(CMD_SUCCESS);
}

static void
lnprbuf(char *buffer,char *bp,char *eob)
{
	int	lno, currentline;

	if (buffer == eob)
		return;

	lno = 1;
	currentline = whatlineisthis(buffer,bp);
	prlno(lno++,currentline);
	while(buffer < eob) {
		putchar(*buffer);
		if ((*buffer == '\n') && ((buffer + 1) != eob))
			prlno(lno++,currentline);
		buffer++;
	}
}

static void
prlno(int lno,int currentline)
{
	char	*fmt;

	if (lno == currentline)
		fmt = ">%2d: ";
	else
		fmt = "%3d: ";
	printf(fmt,lno);
}

static void
prbuf(char *bp,char *eob)
{
	while(bp < eob)
		putchar(*bp++);
}

char *
prline(char *bp,char *eob)
{
	while(bp < eob) {
		putchar(*bp);
		if (*bp == '\n')
			break;
		bp++;
	}
	return(bp+1);
}

/* searchfor():
 *	Step through the buffer starting at 'bp' and search for a memory match
 *	with the incoming string pointed to by 'srch'.  If not found, simply 
 *	return after the entire buffer has been searched (wrap to start if
 *	necessary).  If found, then adjust '*bp' to point to the beginning of
 *	of the line that contains the match.
 */
static int
searchfor(char *srch,char *buffer,char **bp,char *eob)
{
	static	char *lastsrchstring;
	char	*startedhere, *tbp;
	int	len;

	tbp = *bp;
	if (tbp < eob)
		startedhere = *bp;
	else
		tbp = startedhere = buffer;

	if (*srch) {
		len = strlen(srch);
		if (lastsrchstring)
			free(lastsrchstring);
		lastsrchstring = malloc(len+1);	
		if (lastsrchstring)
			strcpy(lastsrchstring,srch);
	}
	else if (lastsrchstring) {
		len = strlen(lastsrchstring);
		srch = lastsrchstring;
	}
	else
		return(-1);
	do {
		if ((tbp + len) > eob) {
			tbp = buffer;
		}
		else {
			if (!memcmp(tbp,srch,len)) {
				while(1) {
					if (tbp <= buffer) {
						*bp = buffer;
						break;
					}
					if (*tbp == '\n') {
						*bp = tbp+1;
						break;
					}
					tbp--;
				}
				prline(*bp,eob);
				return(0);
			}
			else
				tbp++;
		}
	} while(tbp != startedhere);
	printf("'%s' not found\n",srch);
	return(-1);
}

static int
gotoline(char *ln,char *buffer,char **bp,char *eob,int verbose)
{
	int		lno, i, moveforward;
	char	*tbp, *base;

	base = buffer;
	moveforward = 1;

	/* If destination line number is '.', assume you're already there.
	 * If the '.' is followed by a '+' or '-' then the following number
	 * is considered an offset from the current line instead of from the
	 * start of the buffer.
	 */
	if (ln[0] == '.') {
		base = *bp;
		if (ln[1] == '+')
			lno = atoi(&ln[2]) + 1;
		else if (ln[1] == '-') {
			lno = atoi(&ln[2]) + 2;
			moveforward = 0;
		}
		else
			goto end;
	}
	else if (ln[0] == '$') {
		lno = 99999999;
	}
	else
		lno = atoi(ln);

	if (lno < 1) {
		printf("Invalid line\n");
		return(-1);
	}

	if (moveforward) {
		for(tbp=base,i=1;i<lno&&tbp<eob;tbp++)
			if (*tbp == '\n') i++;
		if (tbp == eob) {
			if (verbose)
				printf("Pointer set to end of buffer.\n");
			/* If out of range, set pointer to end of buffer. */
			*bp = eob;
			return(-1);
		}
	}
	else {
		for(tbp=base,i=1;i<lno&&tbp>=buffer;tbp--)
			if (*tbp == '\n') i++;
		if (tbp < buffer) {
			if (verbose)
				printf("Pointer set to start of buffer.\n");
			/* If out of range, set pointer to beginning of buffer. */
			*bp = buffer;
			return(-1);
		}
		tbp+=2;
	}
	*bp = tbp;

end:
	if (verbose) {
		printf("%3d: ",whatlineisthis(buffer,*bp));
		prline(*bp,eob);
	}
	return(0);
}

static int
whatlineisthis(char *buffer,char *bp)
{
	int		lno;
	char	*cp;
	cp = buffer;

	lno = 1;
	while(cp < bp) {
		if (*cp == '\n')
			lno++;
		cp++;
	}
	return(lno);
}

static int
getrange(char *cp,int *begin,int *end,char *buffer,char *bp,char *eob)
{
	char	*dash;
	int		b, e, lastline, thisline;

	if (!*cp)
		return(0);

	lastline = whatlineisthis(buffer,eob) - 1;
	thisline = whatlineisthis(buffer,bp);

	if (*cp == '.')
		b = thisline;
	else
		b = atoi(cp);
	dash = strchr(cp,'-');
	if (dash) {
		if (*(dash+1) == '$')
			e = lastline;
		else if (*(dash+1) == '.')
			e = thisline;
		else
			e = atoi(dash+1);
	}
	else
		e = b;

	if (e > lastline)
		e = lastline;
	if (begin)
		*begin = b;
	if (end)
		*end = e;
	if ((e <= 0) || (b <=0) || (e < b))
		printf("Bad range\n");
	return(e - b + 1);
}

static void
deletelines(char *lp,int lcnt,char *buffer,char **bp,char **eob)
{
	char	*cp, *cp1, *t_bp, *t_eob;

	if (lcnt <= 0)
		return;

	t_bp = *bp;
	t_eob = *eob;

	if (gotoline(lp,buffer,&t_bp,t_eob,0) == -1)
		return;
	cp = cp1 = t_bp;
	while(lcnt>0) {
		if (cp >= t_eob) {
			printf("Pointer set to end of buffer.\n");
			break;
		}
		if (*cp == '\n')
			lcnt--;
		cp++;
	}
	while(cp != t_eob)
		*cp1++ = *cp++;

	*eob = cp1;
	*bp = t_bp;
}

static char *
skipwhite(char *cp)
{
	while((*cp == ' ') || (*cp == '\t'))
		cp++;
	return(cp);
}

/* rmCR():
 *	Given the source and size of the buffer, remove all instances of 0x0d
 *	(carriage return) from the buffer.  Return the number of CRs removed.
*/
static int
rmCR(char *src,int size)
{
	int	i;				/* Index into src array. */
	int	tot;			/* Keep track of total # of 0x0d's removed. */
	int	remaining;		/* Keep track of how far to go. */
	
	tot = 0;
	remaining = size;
	for (i=0;i<size;i++) {
		remaining--;
		if (src[i] == 0x0d) {
			src[i] = 0;			/* Make sure memory is writeable. */
			if (src[i] != 0)
				continue;
			memcpy(&src[i],&src[i+1],remaining);
			tot++;
		}
	}
	if (tot)
		printf("Removed %d CRs\n",tot);
	return(tot);
}

#endif
