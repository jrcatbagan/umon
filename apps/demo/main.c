/* 
 * This file is a simple example of an application that could be run
 * on top of the monitor.
 *
 * Cstart():
 * The Cstart() function depends on the setting of MONCOMPTR in config.h.
 * It demonstrates the use of monConnect and the first mon_XXX function
 * typically called by an application, mon_getargv().
 *
 * main():
 * The main() function demonstrates argument processing (thanks to
 * the call to mon_getargv() in start()), environment variables and
 * a simple use of TFS to dump the content of an ASCII file.
 * Also, if the first argument is "strace_demo", then the strace_demo()
 * function is called.  Refer to strace.c for details.
 */

#include <string.h>
#include <stdlib.h>
#include "monlib.h"
#include "tfs.h"
#include "cfg.h"
#include "timer.h"

unsigned long AppStack[APPSTACKSIZE/4];

extern void strace_demo(void);

void
hitakey(void)
{
	while(!mon_gotachar());
	mon_getchar();
}

/* timer_demo():
 * This function assumes that the underlying monitor has a relatively
 * new uMon API hook call mon_timer().
 * This demo shows how to do a few different elapsed timer operations
 * using the API.
 */
void
timer_demo(void)
{
	struct elapsed_tmr	tmr;
	unsigned long starttime, endtime;

	/* Here are a few different ways (using the uMon API) to wait for 
	 * three seconds...
	 *
	 * First make sure the underlying monitor has a Hardware-based
	 * timer.  If it doesn't then this is worthless.
	 */
	mon_timer(TIMER_QUERY,&tmr);
	if ((HWRTMR_IS_ENABLED(&tmr)) == 0) {
		mon_printf("This monitor doesn't have INCLUDE_HWTMR configured\n");
		return;
	}

	mon_printf("Timer demo ready to start, hit a key to continue\n");
	hitakey();

	/*****************************************************************
	 *
	 * The first one, also the simplest, but least versatile is to just
	 * use mon_delay().  The mon_delay function just takes the number
	 * of milliseconds you want to wait, and uses the underlying hardware
	 * to busy wait on that duration...
	 */
	mon_printf("1. Wait for 3 seconds...\n");
	mon_delay(3000);
	mon_printf("1. Done, hit a key to continue\n");
	hitakey();

	/*****************************************************************
	 *
	 * The second one, allows the user to specify the elapsed time 
	 * (in milliseconds), then allows the user to poll waiting for that
	 * time to expire.  Meanwhile, the application can do other things
	 * while it waits.
	 */
	mon_printf("2. Wait for 3 seconds...\n");
	tmr.start = 3000;
	mon_timer(TIMER_START,&tmr);
	while(mon_timer(TIMER_ELAPSED,&tmr) == 0) {
		/* Do whatever you like here. */
	}
	mon_printf("2. Done, hit a key to continue\n");
	hitakey();

	/*****************************************************************
	 *
	 * The third method, uses the granularity of the hardware clock
	 * and the returned value of "ticks-per-millisecond".  It provides
	 * the most versatility (without actually knowing the details of 
	 * the hardware clock), but requires the most work.  Note that we
	 * have to deal with the possiblity of 32-bit timer value wrap.
	 */
	mon_printf("3. Wait for 3 seconds...\n");
	mon_timer(TIMER_QUERY,&tmr);
	starttime = tmr.currenttmrval;
	endtime = starttime + 3000 * tmr.tpm;
	if (endtime < starttime) {
		do {
			mon_timer(TIMER_QUERY,&tmr);
		} while(tmr.currenttmrval > starttime);
		do {
			mon_timer(TIMER_QUERY,&tmr);
		} while(tmr.currenttmrval < endtime);
	}
	else {
		do {
			mon_timer(TIMER_QUERY,&tmr);
		} while((tmr.currenttmrval > starttime) &&
			(tmr.currenttmrval < endtime));
	}
	
	mon_printf("3. Done, hit a key to continue\n");
	while(!mon_gotachar());

	mon_printf("Timer demo done\n");
	mon_appexit(1);
}

/* Global variables added just to be able to use this
 * app to demonstrate hookup with gdb...
 */
int argtot;
long apprambase;

int
main(int argc,char *argv[])
{
	int		i, tfd;
	char	line[80], *ab, *filename;

	argtot = argc;

	/* If argument count is greater than one, then dump out the
	 * set of CLI arguments...
	 */
	if (argc > 1) {
		if ((argc == 2) && (strcmp(argv[1],"strace_demo") == 0))
			strace_demo();

		if ((argc == 2) && (strcmp(argv[1],"timer") == 0))
			timer_demo();
		
		mon_printf("Argument list...\n");
		for(i=0;i<argc;i++) {
			mon_printf("  arg[%d]: %s\n",i,argv[i]);
			if (strcmp(argv[i],"match") == 0)
				mon_printf("got a match!\n");
		}
	}

	/* Using the content of the shell variable APPRAMBASE, dump the
	 * memory starting at that location...
	 */
	ab = mon_getenv("APPRAMBASE");
	if (ab) {
		char *addr = (char *)strtoul(ab,0,0);
		apprambase = strtoul(ab,0,0);

		mon_printf("Dumping memory at 0x%lx...\n",addr);
		mon_printmem(addr,128,1);
	}

	filename = "monrc";

	/* If the 'monrc' file exists, the assume it is ASCII and dump it
	 * line by line...
	 */
	if (mon_tfsstat(filename)) {
		mon_printf("Dumping content of '%s'...\n",filename);

		tfd = mon_tfsopen(filename,TFS_RDONLY,0);
		if (tfd >= 0) {
			while(mon_tfsgetline(tfd,line,sizeof(line)))
				mon_printf("%s",line);
			mon_tfsclose(tfd,0);
		}
		else {
			mon_printf("TFS error: %s\n",
				(char *)mon_tfsctrl(TFS_ERRMSG,tfd,0));
		}
	}
	else {
		mon_printf("File '%s' not found\n",filename);
	}
	return(0);
}

void
__gccmain()
{
}

int
Cstart(void)
{
	char	**argv;
	int		argc;

	/* Connect the application to the monitor.  This must be done
	 * prior to the application making any other attempts to use the
	 * "mon_" functions provided by the monitor.
	 */
	monConnect((int(*)())(*(unsigned long *)MONCOMPTR),(void *)0,(void *)0);

	/* When the monitor starts up an application, it stores the argument
	 * list internally.  The call to mon_getargv() retrieves the arg list
	 * for use by this application...
	 */
	mon_getargv(&argc,&argv);

	/* Call main, then exit to monitor.
	 */
	main(argc,argv);

	mon_appexit(0);

	/* Won't get here. */
	return(0);
}

/* CstartAlt():
 * Demonstrates the use of the "call -a" command in uMon. 
 * For example, if for some reason you wanted to do this...
 * Load the application then load the symtbl file using
 * "make TARGET_IP=1.2.3.4 sym", then issue these commands:
 *
 *  tfs -v ld app
 *  call -a %CstartAlt one two three
 *
 * The "call -a" command in uMon correctly sets up the function
 * call parameters so that the following function would see 4
 * arguments (including arg0), with argv[1] thru argv[3] being
 * pointers to each of the number strings (i.e. "one", "two", "three")
 * and argv[0] being the ascii-coded-hex address of the function
 * CstartAlt.
 */
int
CstartAlt(int argc, char *argv[])
{
	monConnect((int(*)())(*(unsigned long *)MONCOMPTR),(void *)0,(void *)0);
	main(argc,argv);
	mon_appexit(0);
	return(0);
}
