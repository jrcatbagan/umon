/* This file supports the GCC  option '-finstrument-functions' to
 * insert per-function entry and exit calls...
 * Every function that is recompiled with that option on will call
 * __cyg_profile_func_enter() at the entrypoint and __cyg_profile_func_exit()
 * at the exitpoint.
 * This can be an extremely powerful capability for diagnosing various
 * problems with system because it provides a trace of all functions
 * executed.  The disadvantage is time and space (minor detail)!
 */
int cyg_prof_on;

void
__cyg_profile_func_enter (void *this_fn, void *call_site)
{
    if (cyg_prof_on) {
        mon_memtrace("IN:  %x %x\n", this_fn, call_site);
    }
}

void
__cyg_profile_func_exit (void *this_fn, void *call_site)
{
    if (cyg_prof_on) {
		mon_memtrace("OUT: %x %x\n", this_fn, call_site);
    }
}
