#include "config.h"
#include <ctype.h>
#include "genlib.h"
#include "stddefs.h"

/* strtolower():
 *  In-place modification of a string to be all lower case.
 */
char *
strtolower(char *string)
{
    char *cp;

    cp = string;
    while(*cp) {
        *cp = tolower(*cp);
        cp++;
    }
    return(string);
}

char *
strtoupper(char *string)
{
    char *cp;

    cp = string;
    while(*cp) {
        *cp = toupper(*cp);
        cp++;
    }
    return(string);
}
