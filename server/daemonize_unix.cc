#include "daemonize.hh"

// Unix/Linux implementation
#include <cstdlib>

void daemonize (const bool, const uintptr_t )
{
	daemon(1,0);
}


void daemonize_commit (const int retval)
{
	// nothing to do anymore. It was totally BS.
}
