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
 * term.c:
 *
 * This file contains miscellaneous snippets of code that will allow
 * umon to talk to a vt100-compatible terminal.
 *
 * Original author:     Ed Sutter (ed.sutter@alcatel-lucent.com)
 *
 */

#include "config.h"
#include "genlib.h"
#include "timer.h"
#include "term.h"

#if INCLUDE_TERM

static int
gettermtype(void)
{
    char *term = getenv("TERM");
    int termtype = TERMTYPE_UNDEFINED;

    if(term != 0) {
        if(strcasecmp(term,"VT100") == 0) {
            termtype = TERMTYPE_VT100;
        }
    }

    return(termtype);
}

int
term_settextcolor(int fg)
{
    if(gettermtype() == TERMTYPE_VT100) {
        printf("%c[%dm", 0x1B, fg + 30);
        return(0);
    }
    return(-1);
}

int
term_setbgcolor(int bg)
{
    if(gettermtype() == TERMTYPE_VT100) {
        printf("%c[%dm", 0x1B, bg + 40);
        return(0);
    }
    return(-1);
}

int
term_settextattribute(int attr)
{
    if(gettermtype() == TERMTYPE_VT100) {
        printf("%c[%dm", 0x1B, attr);
        return(0);
    }
    return(-1);
}


int
term_resettext(void)
{
    if(gettermtype() == TERMTYPE_VT100) {
        printf("%c[0m",0x1b);
        return(0);
    }
    return(-1);
}

/* term_getsize():
 * Attempt to query the terminal for its row/col values...
 */
int
term_getsize(int *rows, int *cols)
{
    int c, i, rtot, ctot;
    char *semi, buf[16];
    struct elapsed_tmr tmr;

    if(gettermtype() == TERMTYPE_UNDEFINED) {
        return(-1);
    }

    setenv("ROWS",0);
    setenv("COLS",0);

    /* Send the "what is your terminal size?" request...
     */
    printf("\E[6n");

    /* Wait for a response that looks like: "ESC[rrr;cccR"
     * where 'rrr' is the number of rows, and 'ccc' is the
     * number of columns.
     */
    memset(buf,0,sizeof(buf));
    startElapsedTimer(&tmr,1000);
    for(i=0; i<sizeof(buf); i++) {
        while(!gotachar()) {
            if(msecElapsed(&tmr)) {
                return(-1);
            }
        }
        c = getchar();
        if((i == 0) && (c != 0x1b)) {
            return(-1);
        }
        if((i == 1) && (c != '[')) {
            return(-1);
        }
        if(c == 'R') {
            break;
        }
        buf[i] = c;
    }
    if(i == sizeof(buf)) {
        return(-1);
    }
    semi = strchr(buf,';');
    if(semi == (char *)0) {
        return(-1);
    }
    *semi = 0;
    buf[i] = 0;
    rtot = atoi(buf+2);
    ctot = atoi(semi+1);
    shell_sprintf("ROWS","%d",rtot);
    shell_sprintf("COLS","%d",ctot);
    if(rows) {
        *rows = rtot;
    }
    if(cols) {
        *cols = ctot;
    }
    return(0);
}

void
term_clearscreen(void)
{
    if(gettermtype() == TERMTYPE_VT100) {
        printf("\E[0J");
    }
}

#endif
