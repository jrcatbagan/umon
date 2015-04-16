#include "config.h"
#include <ctype.h>
#include "genlib.h"
#include "stddefs.h"

ushort
swap2(ushort sval_in)
{
	return(((sval_in & 0x00ff) << 8) | ((sval_in & 0xff00) >> 8));
}

ulong
swap4(ulong sval_in)
{
	return(((sval_in & 0x000000ff) << 24) | ((sval_in & 0x0000ff00) << 8) |
			((sval_in & 0x00ff0000) >> 8) | ((sval_in & 0xff000000) >> 24));
}

