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

void daemonize (const bool daemonize, const uintptr_t status_handle)
{
    TCHAR * szWinFrontCmdline, * szWinSrvCmdline;
    DWORD dwWinFrontCmdlineLen;
    SECURITY_ATTRIBUTES saAttr;
    PROCESS_INFORMATION piProcInfo;
    STARTUPINFO siStartInfo;
    BOOL bStatusReceived = FALSE;
    DWORD dwRead = 0, dwTotRead = 0;
    DWORD dwRetVal = 1, dwNewRetVal = 0;
    BOOL bSuccess = FALSE;

    /*
     * On Windows there is no need and possibility for a daemonization fork-dance.
     * Instead we launch the same binary ("image") again, passing it
     * additional open file handles. But the new process must learn about the
     * value of the filehandles somehow; only handles dedicated to stderr/stdout/stdin
     * redirection can be passed "internally" via system call. So we simply pass the
     * handle as a string over the command line; #defined STATUS_HANDLE "status-handle".
     * See CreateProcess documentation for more information.
     */

    gPipeWr = (HANDLE) status_handle;

    if (gPipeWr == NULL)
    {
        ZeroMemory(&saAttr, sizeof(SECURITY_ATTRIBUTES));
        saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
        saAttr.bInheritHandle = TRUE;
        saAttr.lpSecurityDescriptor = NULL;

        if (!CreatePipe(&gPipeRd, &gPipeWr, &saAttr, 0))
            throw std::runtime_error("Cannot create pipe!");

        if (!SetHandleInformation(gPipeRd, HANDLE_FLAG_INHERIT, 0))
            throw std::runtime_error("Cannot configure pipe read handle!");

        /*
         * Create new command line with appended " --status-handle <handle>"
         */
        szWinFrontCmdline = GetCommandLine();           // FIXME: Unicode GetCommandLineW, and wmain()
        dwWinFrontCmdlineLen = _tcslen(szWinFrontCmdline) + _tcslen(_T(" -- ")) + _tcslen(_T(STATUS_HANDLE));
        if (dwWinFrontCmdlineLen + 40 >= MAX_PATH)      // + 40 to accommodate PRIuPRT
            throw std::runtime_error("Command line too long to be extend!");

        if (!(szWinSrvCmdline = (TCHAR *)calloc(dwWinFrontCmdlineLen + 40 + 1, sizeof(TCHAR))))
            throw std::runtime_error("Memory allocation for background process command line failed!");

        _stprintf_s(szWinSrvCmdline, dwWinFrontCmdlineLen + 40 + 0,
                    _T("%s --" STATUS_HANDLE " %") PRIuPTR, szWinFrontCmdline,
                                                            ((uintptr_t)gPipeWr));

        ZeroMemory(&piProcInfo, sizeof(PROCESS_INFORMATION));

        ZeroMemory(&siStartInfo, sizeof(STARTUPINFO));
        siStartInfo.cb = sizeof(STARTUPINFO);
        siStartInfo.hStdError = GetStdHandle(STD_ERROR_HANDLE);
        siStartInfo.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
        siStartInfo.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
        siStartInfo.dwFlags |= /* STARTF_USESTDHANDLES | */ STARTF_USESHOWWINDOW;
        siStartInfo.wShowWindow = SW_HIDE;

        bSuccess = CreateProcess(
            NULL,               // application name
            szWinSrvCmdline,    // command line (len+NULL <= MAX_PATH | if appl. name == NULL)
            &saAttr,            // process security attributes
            NULL,               // primary thread security attributes
            TRUE,               // handles are inherited
            0,                  // creation flags, e.g. CREATE_UNICODE_ENVIRONMENT (FIXME: wmain)
            NULL,               // use parent's environment
            NULL,               // use parent's current directory
            &siStartInfo,       // STARTUPINFO pointer
            &piProcInfo         // receives PROCESS_INFORMATION
        );

        free(szWinSrvCmdline);

        if (!bSuccess)
            throw std::runtime_error("Failed to start background process!");

        /*
         * Now wait for the background process to give status feedback
         * via the pipe or for the process to exit (unexpectendly).
         *
         * Close the writing side in the foreground process so that we get EOF when
         * the background process crashes.
         */
        if (!CloseHandle(gPipeWr))
            throw std::runtime_error("Foreground process could not close pipe's write end!");

        do
        {
            bSuccess = ReadFile(gPipeRd, &dwNewRetVal + dwTotRead, sizeof(DWORD) - dwTotRead, &dwRead, NULL);
            dwTotRead += dwRead;
            if (bSuccess == FALSE) {
                if (GetLastError() == ERROR_BROKEN_PIPE)
                    break;

                throw std::runtime_error("Error in foreground process while reading pipe!");
            }

            if (dwTotRead < sizeof(DWORD)) continue;

            dwRetVal = dwNewRetVal;
            bStatusReceived = TRUE;
        } while (1);

        /*
         * If status reporting was not complete, wait for the process to exit,
         * and proxy the return value.
         */
        if (bStatusReceived == FALSE)
        {
            dwNewRetVal = STILL_ACTIVE;   /* STILL_ACTIVE = 259 */
            while (dwNewRetVal == STILL_ACTIVE)
            {
                if (!(bSuccess = GetExitCodeProcess(piProcInfo.hProcess, &dwNewRetVal)))
                    throw std::runtime_error("Can not read exit code from background process!");

                if (dwNewRetVal == STILL_ACTIVE)
                    std::this_thread::sleep_for(std::chrono::milliseconds(250));
            }
            dwRetVal = dwNewRetVal;
        }

        exit(dwRetVal);
    }

}

void daemonize_commit (int retval)
{
    DWORD dwWritten = 0;
    DWORD dwRetVal;
    BOOL bSuccess = FALSE;

    dwRetVal = (DWORD) retval;

    if (gPipeWr == NULL)
        return;

    bSuccess = WriteFile(gPipeWr, &dwRetVal, sizeof(DWORD), &dwWritten, NULL);

    if (bSuccess == FALSE && GetLastError() != ERROR_BROKEN_PIPE)
        throw std::runtime_error("Can not write to pipe's write end in background process!");

    if (!(bSuccess = FreeConsole()))
        throw std::runtime_error("Can not detach from console in background process!");

    if (!CloseHandle(gPipeWr))
        throw std::runtime_error("Can not close pipe's write end in background process!");
}

#endif //  _WIN32
