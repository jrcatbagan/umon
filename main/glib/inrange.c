#include "config.h"
#include <ctype.h>
#include "genlib.h"
#include "stddefs.h"

/* inRange():
 * This function is handed a range string and a value.
 * If the value is within the range of the string specified, then
 * return 1; else return 0.
 * The incoming string can be a mix of ranges and values with each
 * range and/or value separated by a comma and a range is specified
 * by 2 numbers separated by a dash.
 * Also, incoming ranges of "all" or "any" immediately return true
 * and an incoming range of "none" or a null or empty pointer will
 * return false.
 */
int
inRange(char *range, int value)
{
    int     lo, hi;
    char    rcopy[32], *rsp, *comma, *dash;

    /* If incoming range is NULL, return zero.
     */
    if((range == 0) || (*range == 0) || (strcmp(range,"none") == 0)) {
        return(0);
    }

    /* If the range string is "all" or "any", then immediately return true...
     */
    if((strcmp(range,"all") == 0) || (strcmp(range,"any") == 0)) {
        return(1);
    }

    /* Scan the range string for valid characters:
     */
    rsp = range;
    while(*rsp) {
        if((*rsp == ',') || (*rsp == '-') ||
                (*rsp == 'x') || isxdigit(*rsp)) {
            rsp++;
        } else {
            break;
        }
    }
    if(*rsp) {
        return(0);
    }

    /* If incoming range string exceeds size of copy buffer, return 0.
     */
    if(strlen(range) > sizeof(rcopy)-1) {
        return(0);
    }

    strcpy(rcopy,range);
    rsp = rcopy;
    do {
        comma = strchr(rsp,',');
        if(comma) {
            *comma = 0;
        }
        dash = strchr(rsp,'-');
        if(dash) {
            lo = strtol(rsp,0,0);
            hi = strtol(dash+1,0,0);
            if((value >= lo) && (value <= hi)) {
                return(1);
            }
        } else {
            if(value == strtol(rsp,0,0)) {
                return(1);
            }
        }
        rsp = comma+1;
    } while(comma);
    return(0);
}
