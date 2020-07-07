#include "daemonize.hh"

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
int retval_pipefd[2];   /* read fd, write fd */
pid_t fpid = 1;         /* frontend process PID */

void handle_sig_term (int signal)
{
    sig_term_recv = 1;
}

void daemonize (const bool daemonize, const uintptr_t status_handle)
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
    if ( fpid == 1 || fpid == pid )
        return;

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
