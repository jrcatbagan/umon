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
 * lineedit.c:
 *
 * This code supports the monitor's command line editing capability and
 * command history log.  The command line editing is a subset of KSH's
 * vi-mode editing.
 *
 * This code includes a few suggestions/fixed from Martin Carroll.
 *
 * Original author:     Ed Sutter (ed.sutter@alcatel-lucent.com)
 *
 */

#include "config.h"
#if INCLUDE_LINEEDIT
#include <ctype.h>
#include "genlib.h"
#include "stddefs.h"
#include "cli.h"

#define HMAX            16
#define ESC             0x1B
#define CTLC            0x03
#define BACKSPACE       0x08
#define CMD             100
#define EDIT            101
#define EDIT1           102
#define INSERT          103
#define NEXT            104
#define PREV            105
#define NEITHER         106

#define EDITFILELINE    1
#define EDITCMDLINE     2

static int      stridx;         /* store index */
static int      shwidx;         /* show index */
static int      srchidx;        /* search index */
static int      lastsize;       /* size of last command */
static char     curChar;        /* latest input character */
static char     *curPos;        /* current position on command line */
static char     *startOfLine;   /* start of command line */
static int      lineLen;        /* length of line */
static int      lMode;          /* present mode of entry */
static char     cmdhistory[HMAX+1][CMDLINESIZE];/* array for command history */

static void     ledit(), lcmd(), showprev(), ldelete(), linsert(), lerase();
static void     gotobegin(), gotoend(), backup(), shownext(), linsertbs();
static void     delete_to_end(), delete_something();
static void     historysearch(void), ledit1(void);
static char     *lineeditor();

/* line_edit() & file_line_edit():
 * These two functions are simply front-ends to the lineeditor()
 * function.  The line_edit() function is called by the command line
 * interface and file_line_edit() is called by the flash file editor
 * to provide a convenient single line editor when modifying a file.
 */

char *
line_edit(char *line_to_edit)
{
    return(lineeditor(line_to_edit,EDITCMDLINE));
}

char *
file_line_edit(char *line_to_edit)
{
    return(lineeditor(line_to_edit,EDITFILELINE));
}

/* lineeditor():
 * This function is fed a pointer to a command line.
 * It sets up a command line editor for that particular line.
 * The line is modified in place so, if successful, the function
 * returns the same pointer but with its contents modified based
 * on the editor commands executed.
 * If failure, the function returns (char *)0.
 */
static char *
lineeditor(char *line_to_edit,int type)
{
    lMode = CMD;
    startOfLine = line_to_edit;
    curPos = line_to_edit;
    while(*curPos != ESC) {
        curPos++;
    }
    *curPos = 0;    /* Remove the escape character from the line */
    lineLen = (ulong)curPos - (ulong)startOfLine;
    if(lineLen > 0) {
        curPos--;
        putstr(" \b\b");
    } else {
        putstr(" \b");
    }
    lastsize = 0;
    shwidx = stridx;
    srchidx = stridx;
    while(1) {
        curChar = getchar();
        switch(curChar) {
        case ESC:
            if(lMode != CMD) {
                lMode = CMD;
                continue;
            } else {
                putchar('\n');
                return((char *)0);
            }
        case '\r':
        case '\n':
            putchar('\n');
            if(lineLen == 0) {
                return((char *)0);
            }
            *(char *)(startOfLine + lineLen) = '\0';
            return(startOfLine);
        case CTLC:
            putchar('\n');
            *startOfLine = 0;
            lineLen = 0;
            return((char *)0);
        }
        switch(lMode) {
        case CMD:
            lcmd(type);
            if(lMode == NEITHER) {
                return((char *)0);
            }
            break;
        case INSERT:
            linsert();
            break;
        case EDIT1:
            ledit1();
            break;
        case EDIT:
            ledit();
            break;
        }
        if(lineLen >= (CMDLINESIZE - 2)) {
            printf("line overflow\n");
            return((char *)0);
        }
    }
}

static void
ledit(void)
{
    int len;

    if(curChar == '\b') {
        curPos--;
    } else {
        *curPos++ = curChar;
    }

    len = (curPos - startOfLine);   /* line may get longer than original */
    if(len > lineLen) {
        lineLen = len;
    }

    putchar(curChar);
    switch(lMode) {
    case EDIT1:
        lMode = CMD;
        break;
    }
}

static void
ledit1(void)
{
    *curPos = curChar;
    putchar(curChar);
    putchar('\b');
    lMode = CMD;
}

static void
lcmd(int type)
{
    switch(curChar) {
    case '0':
        gotobegin();
        return;
    case '$':
        gotoend();
        return;
    case 'A':
        gotoend();
        putchar(*curPos++);
        lMode = INSERT;
        return;
    case 'a':
        if(curPos != startOfLine) {
            putchar(*curPos++);
        }
        lMode = INSERT;
        return;
    case 'i':
        lMode = INSERT;
        return;
    case 'D':
        delete_to_end();
        return;
    case 'd':
        delete_something();
        return;
    case 'c':
        delete_something();
        lMode = INSERT;
        return;
    case 'x':
        ldelete();
        return;
    case ' ':
    case 'l':
        if(curPos < startOfLine+lineLen-1) {
            putchar(*curPos);
            curPos++;
        }
        return;
    case '\b':
    case 'h':
        if(curPos > startOfLine) {
            putchar('\b');
            curPos--;
        }
        return;
    case 'r':
        lMode = EDIT1;
        return;
    case 'R':
        lMode = EDIT;
        return;
    }

    /* The remaining commands should only be processed if we are editing
     * a command line.  For editing a line in a file, the commands that
     * relate to command line history are not applicable and should be
     * blocked.
     */
    if(type == EDITCMDLINE) {
        switch(curChar) {
        case '/':
            putchar('/');
            historysearch();
            return;
        case '+':
        case 'j':
            shownext();
            return;
        case '-':
        case 'k':
            showprev();
            return;
        }
    }

    /* Beep to indicate an error. */
    putchar(0x07);
}

static void
linsert()
{
    char    string[CMDLINESIZE];

    if(curChar == BACKSPACE) {
        linsertbs();
        return;
    }
    if(!isprint(curChar)) {
        return;
    }
    putchar(curChar);
    putstr(curPos);
    backup((int)strlen(curPos));
    strncpy(string,curPos,CMDLINESIZE-1);
    *curPos++ = curChar;
    strcpy(curPos,string);
    lineLen++;
}

/* linsertbs():
 * Handle the backspace when in 'INSERT' mode.
 */

static void
linsertbs()
{
    char    *eol, *now;
    int cnt;

    if(curPos == startOfLine) {
        return;
    }
    backup(1);
    curPos--;
    cnt = 0;
    eol = startOfLine + lineLen - 1;
    now = curPos;
    while(curPos <= eol) {
        *curPos = *(curPos+1);
        curPos++;
        cnt++;
    }
    putbytes(now,cnt-1);
    putchar(' ');
    backup((int)cnt);
    curPos = now;
    lineLen--;
}

static void
delete_something()
{
    char    *eol, *now, C, *new;
    int cnt;

    C = getchar();
    if(C == 'w') {      /* word */
        now = curPos;
        eol = startOfLine + lineLen -1;
        while(curPos <= eol) {
            if(*curPos == ' ') {
                break;
            }
            curPos++;
        }
        if(curPos < eol) {
            new = curPos;
            strcpy(now,new);
            cnt = strlen(now);
            putbytes(now,cnt);
            curPos = now + cnt;
            while(curPos <= eol) {
                putchar(' ');
                curPos++;
                cnt++;
            }
            backup(cnt);
        } else {
            curPos = now;
            delete_to_end();
            return;
        }
        curPos = now;
        lineLen = strlen(startOfLine);
    } else if(C == 'f') {       /* find */
        C = getchar();
        now = curPos;
        eol = startOfLine + lineLen -1;
        while(curPos <= eol) {
            if(*curPos == C) {
                break;
            }
            curPos++;
        }
        if(curPos < eol) {
            new = curPos+1;
            strcpy(now,new);
            cnt = strlen(now);
            putbytes(now,cnt);
            curPos = now + cnt;
            while(curPos <= eol) {
                putchar(' ');
                curPos++;
                cnt++;
            }
            backup(cnt);
        } else {
            curPos = now;
            delete_to_end();
            return;
        }
        curPos = now;
        lineLen = strlen(startOfLine);
    }
}

static void
delete_to_end()
{
    char    *eol, *now, *sol;

    eol = startOfLine + lineLen;
    now = curPos;
    if(curPos == eol) {
        return;
    }
    while(curPos < eol) {
        putchar(' ');
        curPos++;
    }
    if(now == startOfLine) {
        sol = now+1;
    } else {
        sol = now;
    }
    while(curPos >= sol) {
        putchar('\b');
        curPos--;
    }
    lineLen = now - startOfLine;
    if(lineLen == 0) {
        curPos = startOfLine;
        *curPos = 0;
    } else {
        *(curPos+1) = '\0';
    }
}

static void
ldelete()
{
    char    *eol, *now;
    int cnt;

    if(lineLen == 0) {
        return;
    }
    cnt = 0;
    eol = startOfLine + lineLen - 1;
    now = curPos;
    if(curPos != eol) {
        while(curPos <= eol) {
            *curPos = *(curPos+1);
            curPos++;
            cnt++;
        }
        putbytes(now,cnt-1);
        putchar(' ');
        backup((int)cnt);
    } else {
        putstr(" \b\b");
        *eol = '\0';
        now--;
    }
    curPos = now;
    lineLen--;
    if(lineLen == 0) {
        curPos = startOfLine;
    }
}

/* showdone():
 * Used as common completion code for the showprev() and
 * shownext() functions below.
 */
static void
showdone(int idx)
{
    if(idx == HMAX) {
        printf("History buffer empty.\007\n");
        lMode = NEITHER;
        return;
    }

    backup((int)(curPos - startOfLine));
    lineLen = strlen(cmdhistory[shwidx]);
    putbytes(cmdhistory[shwidx],lineLen);
    lerase((int)(lastsize-lineLen));
    backup((int)(lineLen));
    strcpy(startOfLine,cmdhistory[shwidx]);
    curPos = startOfLine;
    lastsize = lineLen;
}

/* showprev() & shownext():
 * Show previous or next command in history list based on
 * the current position in the list being established by
 * the shwidx variable.
 */
static void
showprev()
{
    int i;

    if(shwidx == 0) {
        shwidx = HMAX-1;
    } else {
        shwidx--;
    }

    for(i=0; i<HMAX; i++) {
        if(*cmdhistory[shwidx]) {
            break;
        }
        if(shwidx == 0) {
            shwidx = HMAX-1;
        } else {
            shwidx--;
        }
    }
    showdone(i);
}


static void
shownext()
{
    int i;

    if(shwidx == HMAX-1) {
        shwidx = 0;
    } else {
        shwidx++;
    }

    for(i=0; i<HMAX; i++) {
        if(*cmdhistory[shwidx]) {
            break;
        }
        if(shwidx == HMAX) {
            shwidx = 0;
        } else {
            shwidx++;
        }
    }
    showdone(i);
}

static void
backup(count)
int count;
{
    char    string[CMDLINESIZE];
    int i;

    if(count <= 0) {
        return;
    }
    *string = '\0';
    for(i=0; i<count; i++) {
        strcat(string,"\b");
    }
    putbytes(string,count);
}

static void
lerase(int count)
{
    char    string[CMDLINESIZE];
    int i;

    if(count <= 0) {
        return;
    }
    *string = '\0';
    for(i=0; i<count; i++) {
        strcat(string," ");
    }
    for(i=0; i<count; i++) {
        strcat(string,"\b");
    }
    putbytes(string,count*2);
}

static void
gotoend()
{
    char    string[CMDLINESIZE], *eol;
    int i;

    eol = startOfLine + lineLen -1;
    for(i=0; i<CMDLINESIZE-1; i++) {
        if(curPos == eol) {
            break;
        }
        string[i] = *curPos++;
    }
    if(i) {
        putbytes(string,i);
    }
}

static void
gotobegin()
{
    char    string[CMDLINESIZE];
    int i;

    i = 0;
    while(curPos != startOfLine) {
        string[i++] = '\b';
        curPos--;
    }
    putbytes(string,i);
}

/* History():
 * Command used at the CLI to allow the user to dump the content
 * of the history buffer.
 */
char *HistoryHelp[] = {
    "Display command history",
    "",
    0,
};

int
History(int argc,char *argv[])
{
    int i;

    for(i=stridx; i<HMAX; i++) {
        if(cmdhistory[i][0]) {
            printf("%s\n",cmdhistory[i]);
        }
    }
    if(stridx) {
        for(i=0; i<stridx; i++) {
            if(cmdhistory[i][0]) {
                printf("%s\n",cmdhistory[i]);
            }
        }
    }
    return(CMD_SUCCESS);
}

/* historyinit():
 * Initialize the command history...
 */
void
historyinit()
{
    int i;

    shwidx = stridx = 0;
    for(i=0; i<HMAX; i++) {
        cmdhistory[i][0] = 0;
    }
}

/* historylog():
 * This function is called by the CLI retrieval code to store away
 * the command in the CLI history list, a circular queue
 * (size HMAX) of most recent commands.
 */
void
historylog(char *cmdline)
{
    int idx;

    if(strlen(cmdline) >= CMDLINESIZE) {
        return;
    }

    if(stridx == 0) {
        idx = HMAX-1;
    } else {
        idx = stridx -1;
    }

    /* don't store if this command is same as last command */
    if(strcmp(cmdhistory[idx],cmdline) == 0) {
        return;
    }

    if(stridx == HMAX) {
        stridx = 0;
    }

    strcpy(cmdhistory[stridx++],cmdline);
}


static void
historysearch(void)
{
    static char string[100];
    char    *s, *ptr, *last, C;
    int size, len, count;

    lerase((int)lastsize);
    s = string;
    size = 0;
    while(1) {
        C = getchar();
        if(C == '\b') {
            if(size == 0) {
                continue;
            }
            putstr("\b \b");
            s--;
            size--;
            continue;
        }
        if((C == '\r') || (C == '\n')) {
            break;
        }
        putchar(C);
        size++;
        *s++ = C;
    }
    backup((int)(size+1));
    if(size != 0) {
        *s = '\0';
    } else {
        size = strlen(string);
    }

    count = 0;

    while(1) {
        ptr = cmdhistory[srchidx];
        len = strlen(ptr);
        last = ptr + len - size + 1;
        while(ptr < last) {
            if(strncmp(ptr,string,size) == 0) {
                goto gotmatch;
            }
            ptr++;
        }
        if(srchidx == 0) {
            srchidx = HMAX-1;
        } else {
            srchidx--;
        }
        if(++count == HMAX) {
            backup((int)(curPos - startOfLine));
            lineLen = 0;
            lerase((int)(size+1));
            backup((int)(lineLen));
            *startOfLine = '\0';
            curPos = startOfLine;
            lastsize = lineLen;
            return;
        }
    }
gotmatch:
    backup((int)(curPos - startOfLine));
    lineLen = strlen(cmdhistory[srchidx]);
    putbytes(cmdhistory[srchidx],lineLen);
    lerase((int)(lastsize-lineLen));
    backup((int)(lineLen));
    strcpy(startOfLine,cmdhistory[srchidx]);
    curPos = startOfLine;
    lastsize = lineLen;
    if(srchidx == 0) {
        srchidx = HMAX-1;
    } else {
        srchidx--;
    }
    return;
}

#endif

