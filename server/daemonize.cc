#include "daemonize.hh"

#ifdef _WIN32

	void daemonize()
	{
		// do nothing
	}

	void daemonize_end()
	{
		// do nothing
	}

#else

// Unix/Linux implementation
#include <cstdlib>
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <stdexcept>


int daemon_pipefd[2];  /* read fd, write fd */

void daemonize()
{
	/* already a daemon */
	if ( getppid() == 1 ) return;

	/* Open a pipe(2) to transfer output from children to parent */
	if (pipe(daemon_pipefd))
	{
		throw std::runtime_error("Cannot create pipe!");
	}
	
	/* Fork off the parent process */
	pid_t pid = fork();
	if (pid < 0)
	{
		throw std::runtime_error("Cannot fork!");
	}
	
	if (pid > 0)
	{
		close(daemon_pipefd[1]);

		/* consume pipe until it is closed by the children */
		FILE * ipipe = fdopen (daemon_pipefd[0], "r");
		FILE * opipe = fdopen (STDERR_FILENO, "a");
		int c;
		while ((c = getc (ipipe)) != EOF)
			putc (c, opipe);
		fclose (ipipe);
		fclose (opipe);

		exit(EXIT_SUCCESS); /*Killing the Parent Process*/
	}
	
	/* At this point we are executing as the child process */
	
	/* close the read end of the pipe, belonging to the parent */
	close(daemon_pipefd[0]);

	/* Create a new SID for the child process */
	pid_t sid = setsid();
	if (sid < 0)
	{
		throw std::runtime_error("Cannot call setsid()");
	}

	pid_t pid2 = fork();

	if (pid2 < 0)
	{
		throw std::runtime_error("Cannot fork again!");
	}

	if (pid2 > 0)
	{
		_exit(EXIT_SUCCESS);
	}

	/* At this point we are a daemon process */

	umask(0007);

	/* daemons normally chdir("/"), we don't do thus until our code has evolved */
	/* chdir("/"); */

	dup2 (daemon_pipefd[1], STDOUT_FILENO);
	dup2 (daemon_pipefd[1], STDERR_FILENO);

	const int fd = open("/dev/null", O_RDONLY, 0);
	
	if (fd != -1)
	{
		dup2 (fd, STDIN_FILENO);
	}
}


void daemonize_end()
{
	if ( getppid() != 1 ) return;

	const int fd = open("/dev/null", O_WRONLY, 0);

	if (fd != -1)
	{
		dup2 (fd, STDOUT_FILENO);
		dup2 (fd, STDERR_FILENO);
	}

	close(daemon_pipefd[1]);
}

#endif // ! _WIN32
