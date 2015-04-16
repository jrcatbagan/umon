#include "config.h"
#include <ctype.h>
#include "genlib.h"
#include "stddefs.h"

/* s_memcpy():
 *	Superset of memcpy().  Note, this used to be tfsmemcpy() in tfs.c;
 *  however, since it has no real TFS dependencies, and code outside of
 *  TFS uses it, it has been moved here and the name is changed.
 *
 *  Includes verbose option plus verification after copy.
 *  Takes advantage of address alignment when possible.
 *
 *  Note:
 *  If verbose is greater than one, then this function doesn't
 *  do any memory transfer, it simply displays the memory range
 *  that is to be transferred.  This is used by TFS to dump a map without
 *  actually touching memory.
 * 
 *  Return:
 *  0 if successful, else -1 indicating some part of the copy failed.
 */
int
s_memcpy(char *_to,char *_from,int count, int verbose,int verifyonly)
{
	int	err;
	volatile register char *to, *from, *end;

	to = _to;
	from = _from;

	if (verbose)
		printf("%s %7d bytes from 0x%08lx to 0x%08lx",
			verifyonly ? "vrfy" : "copy", count,(ulong)from,(ulong)to);

	if (_to == _from) {
		if (verbose)
			printf("\n");
		return(0);
	}

	if (count < 0)
		return(-1);

	if (verifyonly) {
		while(count) {
			if (*to != *from)
				break;
			to++;
			from++;
			count--;
#ifdef WATCHDOG_ENABLED
			if ((count & 0xff) == 0)
				WATCHDOG_MACRO;
#endif
		}
		if (count) {
			if (verbose) {
				printf(" FAILED\n");
				printf("            (0x%02x @ 0x%08lx should be 0x%02x)\n",
					*to,(ulong)to,*from);
			}
			return(-1);
		}
		else
			if (verbose)
				printf(" OK\n");
			return(0);
	}

	/* If verbose is greater than 1, then we don't even do a memcpy,
	 * we just let the user know what we would have done...
	 */
	if ((count == 0) || (verbose > 1))
		goto done;

	if (to != from) {

		err = 0;
		if (!((int)to & 3) && !((int)from & 3) && !(count & 3)) {
			volatile register ulong	*lto, *lfrom, *lend;
	
			count >>= 2;
			lto = (ulong *)to;
			lfrom = (ulong *)from;
			lend = lto + count;
			while(lto < lend) {
				*lto = *lfrom;
				if (*lto != *lfrom) {
					err = 1;
					break;
				}
				lto++;
				lfrom++;
#ifdef WATCHDOG_ENABLED
				if (((int)lto & 0xff) == 0)
					WATCHDOG_MACRO;
#endif
			}
		}
		else if (!((int)to & 1) && !((int)from & 1) && !(count & 1)) {
			volatile register ushort	*sto, *sfrom, *send;
	
			count >>= 1;
			sto = (ushort *)to;
			sfrom = (ushort *)from;
			send = sto + count;
			while(sto < send) {
				*sto = *sfrom;
				if (*sto != *sfrom) {
					err = 1;
					break;
				}
				sto++;
				sfrom++;
#ifdef WATCHDOG_ENABLED
				if (((int)sto & 0xff) == 0)
					WATCHDOG_MACRO;
#endif
			}
		}
		else {
			end = to + count;
			while(to < end) {
				*to = *from;
				if (*to != *from) {
					err = 1;
					break;
				}
				to++;
				from++;
#ifdef WATCHDOG_ENABLED
				if (((int)to & 0xff) == 0)
					WATCHDOG_MACRO;
#endif
			}
		}
		if (err) {
			if (verbose)
				printf(" failed\n");
			return(-1);
		}
	}

done:
	if (verbose)
		printf("\n");

	return(0);
}
