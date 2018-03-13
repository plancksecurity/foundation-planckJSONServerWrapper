#include "daemonize.hh"

#ifdef _WIN32

	void daemonize(std::function<void()> child_fn = std::function<void()>() )
	{
		if(child_fn)
			child_fn();
	}

#else

// Unix/Linux implementation
#include <cstdlib>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <stdexcept>

void daemonize(std::function<void()> child_fn )
{
	/* already a daemon */
	if ( getppid() == 1 ) return;
	
	/* Fork off the parent process */
	pid_t pid = fork();
	if (pid < 0)
	{
		throw std::runtime_error("Cannot fork!");
	}
	
	if (pid > 0)
	{
		exit(EXIT_SUCCESS); /*Killing the Parent Process*/
	}
	
	/* At this point we are executing as the child process */
	if(child_fn)
		child_fn();
	
	/* Create a new SID for the child process */
	pid_t sid = setsid();
	if (sid < 0)
	{
		throw std::runtime_error("Cannot call setsid()");
	}
	
	const int fd = open("/dev/null",O_RDWR, 0);
	
	if (fd != -1)
	{
		dup2 (fd, STDIN_FILENO);
		dup2 (fd, STDOUT_FILENO);
		dup2 (fd, STDERR_FILENO);
		
		if (fd > 2)
		{
			close (fd);
		}
	}
}

#endif // ! _WIN32
