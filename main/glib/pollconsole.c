#include "config.h"
#include <ctype.h>
#include "ether.h"
#include "genlib.h"
#include "stddefs.h"
#include "timer.h"

/* pollConsole():
 * Common function used to query the console port with a message.
 * If after about a 2-second period (or the time specified by POLLTIMEOUT),
 * there is no response from the console, then return 0; else return
 * the character that was received.
 */
int
pollConsole(char *msg)
{
    char    *env;
    int     pollval, msec;
    struct elapsed_tmr tmr;

    env = getenv("POLLTIMEOUT");
    if(env) {
        msec = strtol(env,0,0);
    } else {
        msec = 2000;
    }

    pollval = 0;
    printf("%s",msg);
    startElapsedTimer(&tmr,msec);
    while(!msecElapsed(&tmr)) {
        if(gotachar()) {
            while(gotachar()) {
                pollval = getchar();
            }
            break;
        }
        pollethernet();
    }
    putstr("\r\n");

    if(ELAPSED_TIMEOUT(&tmr)) {
        return(0);
    }

    return(pollval);
}
