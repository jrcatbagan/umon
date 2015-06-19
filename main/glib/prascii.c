#include "config.h"
#include <ctype.h>
#include "genlib.h"
#include "stddefs.h"

/* prascii():
 *	Print the incoming data stream as ascii if printable, else just
 *	print a dot.
 */
void
prascii(uchar *data,int cnt)
{
	int	i;

	for(i=0;i<cnt;i++) {
		if ((*data > 0x1f) && (*data < 0x7f))
			printf("%c",*data);
		else
			putchar('.');
		data++;
	}
}
