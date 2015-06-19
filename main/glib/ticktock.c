#include "config.h"
#include <ctype.h>
#include "ether.h"
#include "genlib.h"
#include "stddefs.h"

/* ticktock():
 * Put out a ticker...
 */
void
ticktock(void)
{
	static short tick;
	char	tock;

#if INCLUDE_MONCMD
	/* Don't do any ticker if the command was issued 
	 * from UDP (i.e. moncmd)...
	 */
	if (IPMonCmdActive)
		return;
#endif

	switch(tick) {
	case 1:
	case 5:
		tock = '|';
		break;
	case 2:
	case 6:
		tock = '/';
		break;
	case 3:
	case 7:
		tock = '-';
		break;
	case 4:
	case 8:
		tock = '\\';
		break;
	default:
		tock = '|';
		tick = 1;
		break;
	}
	tick++;
	printf("%c\b",tock);
}
