#include "config.h"
#include <ctype.h>
#include "genlib.h"
#include "stddefs.h"

/* s_memset():
 *  Superset of memset() (used to be tfsmemset()).
 *  Includes verbose option, test to make sure the destination is not
 *  within MicroMonitor's own space, plus verification after set.
 */
int
s_memset(uchar *to,uchar val,int count,int verbose,int verifyonly)
{
    int     failed;
    uchar   *end;

    failed = 0;

#ifndef MONAPP
    if(inUmonBssSpace((char *)to,(char *)to+count)) {
        return(-1);
    }
#endif

    if(verbose) {
        printf("%s %7d bytes  at  0x%08lx to 0x%02x",
               verifyonly ? "vrfy" : "set ",count,(ulong)to,val);
    }

    if((count == 0) || (verbose > 1)) {
        goto done;
    }

    end = to+count;

    if(verifyonly) {
        while(to < end) {
#ifdef WATCHDOG_ENABLED
            if(((ulong)to & 0xff) == 0) {
                WATCHDOG_MACRO;
            }
#endif
            if(*to++ != val) {
                failed = 1;
                break;
            }
        }
    } else {
        while(to < end) {
#ifdef WATCHDOG_ENABLED
            if(((ulong)to & 0xff) == 0) {
                WATCHDOG_MACRO;
            }
#endif
            *to = val;
            if(*to++ != val) {
                failed = 1;
                break;
            }
        }
    }
done:
    if(verbose) {
        if(failed) {
            printf(" failed");
        } else if(verifyonly) {
            printf(" OK");
        }
        printf("\n");
    }
    if(failed) {
        return(-1);
    } else {
        return(0);
    }
}
