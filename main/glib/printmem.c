#include "config.h"
#include <ctype.h>
#include "genlib.h"
#include "stddefs.h"

/*  printMem():
 *  Dump a block of memory.
 */
void
printMem(uchar *base, int size, int showascii)
{
    int i, col;
    uchar *cp, *cp1;

    cp = cp1 = base;
    printf("  ");
    for(col=1,i=0; i<size; i++,col++) {
        printf("%02x ",*cp++);
        if((col == 8) || (col == 16)) {
            printf("  ");
            if(col == 16) {
                col = 0;
                if(showascii) {
                    prascii(cp1,16);
                }
                cp1 += 16;
                printf("\n  ");
            }
        }
    }
    if((showascii) && (col > 1)) {
        int space;

        space = (3 * (17 - col)) + (col <= 8 ? 4 : 2);
        while(space--) {
            putchar(' ');
        }
        prascii(cp1,col-1);
    }
    printf("\n");
    return;
}
