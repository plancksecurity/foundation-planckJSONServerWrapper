#include "daemonize.hh"

#ifdef _WIN32

#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <strsafe.h>
#include <inttypes.h>

#include <thread>
#include <iostream>
#include <stdexcept>
#include <system_error>

HANDLE gPipeRd = NULL;
HANDLE gPipeWr = NULL;

void daemonize (const bool daemonize, const void * winsrv)
{
	TCHAR * tzWinFrontCmdline, *tzWinSrvCmdline;
	SECURITY_ATTRIBUTES saAttr;
	PROCESS_INFORMATION piProcInfo;
	STARTUPINFO siStartInfo;
	HANDLE hwinsrv = (HANDLE) winsrv;
	BOOL bStatusReceived = FALSE;
	DWORD dwRead = 0, dwTotRead = 0;
	DWORD retval = 1, newRetval = 0;
	BOOL bSuccess = FALSE;

	/*
	 * On Windows there is no need and possibility for a daemonization fork-dance.
	 * Instead we launch the same binary ("image") again, passing it
	 * additional open file handles. But the new process must learn about the
	 * value of the filehandles somehow; only handles dedicated to stderr/stdout/stdin
	 * redirection can be passed "internally" via system call. So we simply pass the
	 * handle as a string over the command line; as "winsrv" argument. See CreateProcess
	 * documentation for more information.
	 */

	gPipeWr = hwinsrv;

	if (hwinsrv == NULL)
	{
		saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
		saAttr.bInheritHandle = TRUE;
		saAttr.lpSecurityDescriptor = NULL;

		if (!CreatePipe(&gPipeRd, &gPipeWr, &saAttr, 0))
			throw std::runtime_error("Cannot create pipe!");

		/* Ensure the read handle to the pipe is not inherited */
		if (!SetHandleInformation(gPipeRd, HANDLE_FLAG_INHERIT, 0))
			throw std::runtime_error("Cannot configure pipe handle!");

		/* Add " --winsrv <handle>" to the current command line */
		tzWinFrontCmdline = GetCommandLine();   // FIXME: Unicode GetCommandLineW, and wmain()
		if (!(tzWinSrvCmdline = (TCHAR *)calloc(MAX_PATH + 1, sizeof(TCHAR))))
			throw std::runtime_error("Memory allocation for background process command line failed!");
		_stprintf(tzWinSrvCmdline, _T("%s --winsrv %" PRIuPTR), tzWinFrontCmdline,
			                                                    ((uintptr_t)(void*)gPipeWr));

		ZeroMemory(&piProcInfo, sizeof(PROCESS_INFORMATION));

		ZeroMemory(&siStartInfo, sizeof(STARTUPINFO));
		siStartInfo.cb = sizeof(STARTUPINFO);
		siStartInfo.hStdError = GetStdHandle(STD_ERROR_HANDLE);
		siStartInfo.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
		siStartInfo.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
		siStartInfo.dwFlags |= /* STARTF_USESTDHANDLES | */ STARTF_USESHOWWINDOW;
		siStartInfo.wShowWindow = SW_HIDE;

		bSuccess = CreateProcess(
			NULL, 		      // application name
			tzWinSrvCmdline,  // command line
			&saAttr,          // process security attributes
			NULL,             // primary thread security attributes
			TRUE,             // handles are inherited
			0,                // creation flags, e.g. CREATE_UNICODE_ENVIRONMENT
			NULL,             // use parent's environment
			NULL,             // use parent's current directory
			&siStartInfo,     // STARTUPINFO pointer
			&piProcInfo       // receives PROCESS_INFORMATION
		);

		free(tzWinSrvCmdline);

		if (!bSuccess)
			throw std::runtime_error("Failed to start background process!");

		/*
		 * Now wait for the background process to give status feedback
		 * via the pipe or for the process to have exited.
		 *
		 * Close the writing side in the foreground process so that we get EOF when
		 * the background process crashes
		 */
		if (!CloseHandle(gPipeWr))
			throw std::runtime_error("Can not close pipe's write end in foreground process!");

		do
		{
			bSuccess = ReadFile(gPipeRd, &newRetval + dwTotRead, sizeof(DWORD) - dwTotRead, &dwRead, NULL);
			dwTotRead += dwRead;
			if (bSuccess == FALSE) {
				if (GetLastError() == ERROR_BROKEN_PIPE) {
					break;
				}
				throw std::runtime_error("Error while reading pipe in foreground process!");
			}
			if (dwTotRead < sizeof(DWORD))
				continue;
			retval = newRetval;
			bStatusReceived = TRUE;
		} while (1);

		/*
		 * If status reporting was not complete, wait for the process to exit, and proxy
		 * the return value.
		 */
		newRetval = 0;   /* STILL_ACTIVE = 259 */
		do
		{
			if (newRetval == STILL_ACTIVE)
				std::this_thread::sleep_for(std::chrono::milliseconds(250));
			if (!(bSuccess = GetExitCodeProcess(piProcInfo.hProcess, &newRetval)))
				throw std::runtime_error("Can not read exit code from background process!");
		} while (bStatusReceived == FALSE && newRetval == STILL_ACTIVE);
		if (bStatusReceived == FALSE && newRetval != STILL_ACTIVE)
			retval = newRetval;

		exit(retval);
	}

}

void daemonize_commit (int retval)
{
	DWORD dwWritten = 0;
	DWORD dwRetval = 0;
	BOOL bSuccess = FALSE;

	dwRetval = (DWORD) retval;

	if (gPipeWr == NULL)
	{
		if (dwRetval) exit(dwRetval);   /* FIXME: see comment in UNIX implementation */
		return;
	}

	bSuccess = WriteFile(gPipeWr, &dwRetval, sizeof(DWORD), &dwWritten, NULL);
	if (bSuccess == FALSE && GetLastError() != ERROR_BROKEN_PIPE)
		throw std::runtime_error("Can not write to pipe's write end in background process!");

	if (!(bSuccess = FreeConsole()))
		throw std::runtime_error("Can not detach from console in background process!");

	if (!CloseHandle(gPipeWr))
		throw std::runtime_error("Can not close pipe's write end in background process!");
}

#else

// Unix/Linux implementation
#include <cstdlib>
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/wait.h>

#include <thread>
#include <iostream>
#include <stdexcept>
#include <system_error>


volatile sig_atomic_t sig_term_recv = 0;
int retval_pipefd[2];  /* read fd, write fd */
pid_t fpid = 1;         /* frontend process PID */

void handle_sig_term (int signal)
{
    sig_term_recv = 1;
}

void daemonize (const bool daemonize, const void * winsrv)
{
	int retval = EXIT_FAILURE;
	pid_t pid, sid, daemon_pid;

	struct sigaction sig_action = {};
	sig_action.sa_handler = handle_sig_term;
	bool this_process_should_term = 0;
	int child_status = 0;

	FILE * ipipe;
	int c;

	pid = getpid();
	if ( daemonize == true && fpid == 1 ) fpid = pid;

	/* only the frontend process shall continue */
	if ( fpid == 1 || fpid != pid ) return;

	if (pipe (retval_pipefd))
		throw std::runtime_error ("Cannot create return-value pipe!");
	
	/* now fork the intermediate process */
	pid = fork ();
	if (pid < 0)
		throw std::runtime_error ("Cannot fork child process!");
	
	if (pid > 0)
	{
		/*
		 * This is the foreground process which needs to stick around until the
		 * background process has comminicated success or failure or has crashed.
		 * As soon as a feedback is obtained from the background process, inform
		 * the intermediate process to exit (kill TERM) and eventually "copy up"
		 * the exit value of the background process.
		 */

		/* close the write end of the return value pipe */
		close (retval_pipefd[1]);

		/* read pipe until closed or crashed on writing side */
		ipipe = fdopen (retval_pipefd[0], "r");
		while ((c = getw (ipipe)) != EOF)
		{
			retval = c;
			this_process_should_term = 1;
		}
		fclose (ipipe);
		if (c == EOF)
			this_process_should_term = 1;

		do
		{
			kill (pid, SIGTERM);
			if (!this_process_should_term && (waitpid (pid, &child_status, WUNTRACED) > 0))
			{
				if (WIFEXITED(child_status) || WIFSIGNALED(child_status))
				{
					retval = WEXITSTATUS(child_status);
					this_process_should_term = 1;
				}
			}
		} while (!this_process_should_term);
		
		exit (retval); /* exit the foreground process */
	}
	
	/*
	 * At this point we are the intermediate child process,
	 * which now needs to fork the daemon process.
	 */
	
	/* close the read end of the pipe, belonging to the foreground process */
	close (retval_pipefd[0]);

	/* create a new SID for the child process */
	sid = setsid();
	if (sid < 0)
		throw std::runtime_error ("Cannot call setsid()");

	daemon_pid = fork();
	if (daemon_pid < 0)
		throw std::runtime_error ("Cannot fork again!");

	if (daemon_pid > 0)
	{
	
		/*
		 * This now sticks around until it gets the TERM signal from the
		 * foreground process. At that point, it will see if the daemon
		 * process is still running, and if not, proxy it's retval (which
		 * will be again proxied by the frontend process).
		 */

		close (retval_pipefd[1]);

		do {
			if (!this_process_should_term && (waitpid (daemon_pid, &child_status, WUNTRACED) > 0))
			{
				if ((WIFEXITED(child_status) || WIFSIGNALED(child_status)))
				{
					retval = WEXITSTATUS(child_status);
					this_process_should_term = 1;
				}
			}
		}
		while (!(sig_term_recv || this_process_should_term));

		_exit (retval);
	}

	/* At this point we are a daemon process */

	sig_action.sa_handler = SIG_DFL;
	sigaction (SIGTERM, &sig_action, NULL);

	umask (0007);

	/* daemons normally chdir("/"), we don't do this until our code has evolved */
	/* chdir("/"); */

	/* We delay closing the terminal to daemonize_commit(). */

	const int fd = open ("/dev/null", O_RDONLY, 0);
	if (fd == -1)
		throw std::runtime_error ("Cannot open /dev/null!");
    
	dup2 (fd, STDIN_FILENO);
}


void daemonize_commit (const int retval)
{
	static int commited = 0;
	FILE * retvalfd;
	pid_t pid;

	pid = getpid();

	/* nothing to do if not the daemon process */
	if ( fpid == 1 || fpid == pid ) {
		/* FIXME: throw in json-adapter.cc is not catched in main ... */
		if (retval) exit(retval);          /*   ... so, we exit here. */
		return;
	}

	if (commited) return;
	commited = 1;

	const int fd = open ("/dev/null", O_WRONLY, 0);
	if (fd == -1)
		throw std::runtime_error ("Cannot open /dev/null!");

	retvalfd = fdopen (retval_pipefd[1], "a");
	putw (retval, retvalfd);
	fclose (retvalfd);
	close (retval_pipefd[1]);

	dup2 (fd, STDOUT_FILENO);
	dup2 (fd, STDERR_FILENO);
}

#endif // ! _WIN32
