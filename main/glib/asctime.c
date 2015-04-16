#include <time.h>
#include "genlib.h"

static const char *days[] =
  {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

static const char *months[] =
  { "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

char *asctime_r(const struct tm *t, char *ret)
{
  sprintf(ret, "%s %s %2d, %4d %2d:%02d:%02d",
	  days[t->tm_wday], months[t->tm_mon],
	  t->tm_mday, t->tm_year + 1970,
	  t->tm_hour, t->tm_min, t->tm_sec);
  return ret;
}

