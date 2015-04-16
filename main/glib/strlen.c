#include "config.h"
#include <ctype.h>
#include "genlib.h"
#include "stddefs.h"

/* strlen():
 * Returns the number of
 * non-NULL bytes in string argument.
 */
int
strlen(register char *s)
{
	register char *s0 = s + 1;

	while (*s++ != '\0')
		;
	return (s - s0);
}
