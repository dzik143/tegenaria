/******************************************************************************/
/*                                                                            */
/* Copyright (c) 2010, 2014 Lukasz Bienczyk <lukasz.bienczyk@gmail.com>,      */
/* Radoslaw Kolodziejczyk <radek.kolodziejczyk@gmail.com>,                    */
/* Sylwester Wysocki <sw143@wp.pl>                                            */
/*                                                                            */
/* Permission is hereby granted, free of charge, to any person obtaining a    */
/* copy of this software and associated documentation files (the "Software"), */
/* to deal in the Software without restriction, including without limitation  */
/* the rights to use, copy, modify, merge, publish, distribute, sublicense,   */
/* and/or sell copies of the Software, and to permit persons to whom the      */
/* Software is furnished to do so, subject to the following conditions:       */
/*                                                                            */
/* The above copyright notice and this permission notice shall be included in */
/* all copies or substantial portions of the Software.                        */
/*                                                                            */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR */
/* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,   */
/* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL    */
/* THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER */
/* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING    */
/* FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER        */
/* DEALINGS IN THE SOFTWARE.                                                  */
/*                                                                            */
/******************************************************************************/

//
// Windows includes.
//

#ifdef WIN32

# include <windows.h>
# include <io.h>
# include <fcntl.h>
# include <Psapi.h>
# include <shlwapi.h>
# include <tlhelp32.h>

//
// Linux includes.
//

#else

# include <errno.h>
# include <stdio.h>
# include <unistd.h>
# include <signal.h>
# include <wait.h>
# include <sys/stat.h>

#endif

#include <Tegenaria/Debug.h>
#include <Tegenaria/Str.h>
#include "Process.h"

namespace Tegenaria
{
  #ifdef WIN32

  //
  // Determine in/out/err handle basing on parameters given in ProcessCreate()
  // function.
  //
  // fd             - fd parameter passed to ProcessCreate() function (IN).
  // fdType         - fdType - parameter passed to ProcessCreate() function(IN).
  // parentHandleId - handle to use if inherit mode detected eg. handle to stdin (IN).
  //
  // RETURNS: Winapi handle, which should be set for new process.
  //

  HANDLE ProcessParseFdParameter(int fd, int fdType, DWORD parentHandleId)
  {
    #pragma qcbuild_set_private(1)

    HANDLE rv = NULL;

    if (fd == PROCESS_FD_INHERIT)
    {
      rv = GetStdHandle(parentHandleId);
    }
    else if (fd == PROCESS_FD_NULL)
    {
      rv = NULL;
    }
    else
    {
      switch(fdType)
      {
        //
        // CRT FD.
        //

        case PROCESS_FD_TYPE_CRT:
        {
          rv = HANDLE(_get_osfhandle(fd));

          break;
        }

        //
        // Raw win32 handle.
        //

        case PROCESS_FD_TYPE_WINAPI_HANDLE:
        case PROCESS_FD_TYPE_SOCKET:
        {
          rv = (HANDLE) fd;

          break;
        }
      }
    }

    return rv;
  }
  #endif /* WIN32 */

  //
  // USE:
  //   const char *argv[3 + argv_count];
  //   argv[0] = "path_to_process";
  //   argv[1] = "parameter#1";
  //   argv[2] = "parameter#2";
  //   argv[3] = "parameter#3";
  //   argv[4] = NULL;
  //
  // argv[]  - NULL terminated parameter list (IN).
  // fdin    - FD to set for std input (IN/OPT).
  // fdout   - FD to set for std output (IN/OPT).
  // fderr   - FD to set for std error (IN/OPT).
  //
  // fdType  - type of specified fdin/fdout/fderr values. One of
  //           PROCESS_TYPE_XXX values defined in LibProcess.h. Defaulted
  //           to PROCESS_TYPE_CRT if skipped. (IN/OPT).
  //
  // options - combination of PROCESS_OPTIONS_XXX flags (IN/OPT).
  //
  // TIP #1: Set fdin/fdout/fderr to -1 to inherit handle from parent.
  // TIP #2: Set fdin/fdout/fderr to -2 to redirect handle to nul.
  // TIP #3: Set fdType to PROCESS_TYPE_HANDLE if you have raw win32 handle on windows.
  //
  // RETURNS: pid of new process
  //          or -1 if error.
  //

  int ProcessCreate(const char *argv[],
                       int fdin, int fdout, int fderr,
                          int fdType, int options)
  {
    DBG_MSG("Creating process [%s]\n", argv[0]);

    for (int i = 1; argv[i]; i++)
    {
      DBG_MSG("With parameter [%d][%s]\n", i, argv[i]);
    }

    //
    // Windows.
    //

    #ifdef WIN32
    {
      PROCESS_INFORMATION pi = {0};

      STARTUPINFO si = {0};

      DWORD creationFlags = 0;

      char cmd[1024 * 4] = {0};

      int pid = -1;

      //
      // Check args.
      //

      SetLastError(ERROR_INVALID_PARAMETER);

      FAILEX(argv == NULL,    "ERROR: 'argv' cannot be NULL in ProcessCreate().\n");
      FAILEX(argv[0] == NULL, "ERROR: 'argv[0]' cannot be NULL in ProcessCreate().\n");

      //
      // Set std handles for child process.
      //
      // -1 = inherit from child
      // -2 = NULL
      //
      // Otherwise set to given CRT FD.
      //

      //
      // Set handles.
      //

      si.hStdInput  = ProcessParseFdParameter(fdin,  fdType, STD_INPUT_HANDLE);
      si.hStdOutput = ProcessParseFdParameter(fdout, fdType, STD_OUTPUT_HANDLE);
      si.hStdError  = ProcessParseFdParameter(fderr, fdType, STD_ERROR_HANDLE);

      //
      // Set up STARTUPINFO.
      //

      si.cb      = sizeof(STARTUPINFO);
      si.dwFlags = STARTF_USESTDHANDLES;

      //
      // Set up command to execute in one continous string.
      //

      for (int i = 1; argv[i]; i++)
      {
        strncat(cmd, argv[i], sizeof(cmd));
        strncat(cmd, " ", sizeof(cmd));
      }

      //
      // Set up creation flags
      //

      if (options & PROCESS_OPTIONS_NO_WINDOW)
      {
        creationFlags = CREATE_NO_WINDOW;
      }

      //
      // Create new process.
      //

      DBG_MSG
      (
        "Creating process...\n"
        "command : [%s]\n"
        "fdin    : [%d/%d]\n"
        "fdout   : [%d/%d]\n"
        "fderr   : [%d/%d]\n",
        cmd,
        fdin,  si.hStdInput,
        fdout, si.hStdOutput,
        fderr, si.hStdError
      );

      FAIL(CreateProcess(NULL, cmd, NULL, NULL, TRUE,
                             creationFlags, NULL, NULL, &si, &pi) == FALSE);

      pid = pi.dwProcessId;

      DBG_MSG("Created process '%d'.\n", pid);

      //
      // Error handler.
      //

      fail:

      if (pid == -1)
      {
        Error("ERROR: Cannot create process '%s'.\n"
                  "Error code is : %d.\n", argv[0],
                      GetLastError());
      }

      return pid;
    }

    //
    // Linux, MacOS.
    //

    #else
    {
      Fatal("ProcessCreate() not implemented on this platform.\n");
    }
    #endif
  }

  //
  // Check does given proces live.
  //
  // pid - pid of process to check (IN).
  //
  // RETURNS: 1 if process is running,
  //          0 if dead or error.
  //

  int ProcessIsRunning(int pid)
  {
    int running = 0;

    //
    // Windows.
    //

    #ifdef WIN32
    {
      HANDLE process = OpenProcess(SYNCHRONIZE, FALSE, pid);

      if (process != NULL)
      {
        if (WaitForSingleObject(process, 0) == WAIT_TIMEOUT)
        {
          running = 1;
        }

        CloseHandle(process);
      }
    }

    //
    // Linux, MacOS.
    //

    #else
    {
      if (pid > 0 && kill(pid, 0) != -1)
      {
        running = 1
      }
    }
    #endif

    return running;
  }

  void ProcessKill(int pid, int force)
  {
    #ifdef WIN32
    {
      //
      // Windows.
      //

      int exitCode = -1;

      DBG_MSG("Terminating process '%d'...\n", pid);

      HANDLE process = OpenProcess(PROCESS_TERMINATE, FALSE, pid);

      FAIL(process == NULL);

      FAIL(TerminateProcess(process, 1) == FALSE);

      exitCode = 0;

      //
      // Error handler.
      //

      fail:

      if (exitCode)
      {
        Error("ERROR: Cannot terminate process '%d'.\n"
                  "Error code is : %d.\n", pid, GetLastError());
      }

      CloseHandle(process);
    }
    #else
    {
      //
      // Linux, MacOS.
      //

      Fatal("ProcessKill() not implemented.\n");
    }
    #endif
  }

  //
  // Wait until process finished or timeout reached.
  //
  // WARNING#1: Function does NOT detect zoombi processes on Linux as long
  //            as resultCode parameter is NULL. Process become zoombi when
  //            finished its work, but parent didn't call waitpid to pop
  //            result code from it.
  //
  // pid        - pid of process to wait (IN).
  //
  // timeout    - maximum allowed wait time in miliseconds or -1 for inifinity (IN).
  //
  // resultCode - if process died before the timeout, this value returns
  //              its result code. Can be NULL if not needed. (OUT/OPT).
  //
  // RETURNS: 0 if process dead until timeout,
  //          1 if timeout reached,
  //         -1 if error.
  //
  // TIP #1: you can get resultCode only for child process.
  //

  int ProcessWait(int pid, int timeout, int *resultCode)
  {
    //
    // Windows.
    //

    #ifdef WIN32
    {
      int exitCode = -1;

      if (timeout < 0)
      {
        timeout = INFINITE;
      }

      //
      // Open process.
      //

      HANDLE process = OpenProcess(SYNCHRONIZE, FALSE, pid);

      FAILEX(process == NULL, "ERROR: Cannot open process '%d'.\n", pid);

      //
      // Wait for dead or timeout.
      //

      DBG_MSG("Waiting for process '%d' with timeout '%d' ms...\n", pid, timeout);

      switch(WaitForSingleObject(process, timeout))
      {
        //
        // Process dead.
        //

        case WAIT_OBJECT_0:
        {
          DBG_MSG("Process '%d' signaled.\n", pid);

          if (resultCode)
          {
            GetExitCodeProcess(process, PDWORD(resultCode));
          }

          exitCode = 0;

          break;
        }

        //
        // Timeout.
        //

        case WAIT_TIMEOUT:
        {
          DBG_MSG("WARNING: Timeout reached while waiting for process '%d'.\n", pid);

          exitCode = 1;

          break;
        }

        //
        // Error.
        //

        default:
        {
          goto fail;
        }
      }

      //
      // Error handler.
      //

      fail:

      if (exitCode == -1)
      {
        Error("ERROR: Cannot wait for process '%d'.\n"
                  "Error code is : %d.\n", pid, GetLastError());
      }

      CloseHandle(process);
    }
    #else

    //
    // Linux, MacOS.
    //

    {
      Fatal("ProcessWait() not implemented on Linux/MacOS.\n");
    }
    #endif
  }

  //
  // Find PID of parent process.
  //
  // RETURNS: PID of parent of current running process,
  //          or -1 if error.
  //

  int ProcessGetParentPid()
  {
    int parentPid = -1;

    //
    // Windows.
    //

    #ifdef WIN32
    {
      DWORD currentPid = GetCurrentProcessId();

      HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

      PROCESSENTRY32 procEntry = {0};

      //
      // Get snapshot of current running processes.
      //

      FAILEX(snap == NULL, "ERROR: Cannot create processes snapshot.\n");

      //
      // Get first process in snapshot.
      //

      procEntry.dwSize = sizeof(PROCESSENTRY32);

      FAIL(Process32First(snap, &procEntry) == FALSE);

      //
      // Search for current running process in processes list.
      //

      do
      {
        //
        // Get parent pid for current process.
        //

        if (currentPid == procEntry.th32ProcessID)
        {
          parentPid = (int) procEntry.th32ParentProcessID;
        }
      }
      while (parentPid == -1 && Process32Next(snap, &procEntry));

      FAILEX(parentPid < 0, "ERROR: Current running process"
                 " [%d] not found while searching for parent PID.\n", currentPid);
    }

    //
    // Linux.
    //

    #else
    {
      parentPid = getppid();
    }
    #endif

    //
    // Error handler.
    //

    fail:

    return parentPid;
  }

  //
  // Find PID of own process.
  //
  // RETURNS: PID of current running process,
  //          or -1 if error.
  //

  int ProcessGetCurrentPid()
  {
    int currentPid = -1;

    //
    // Windows.
    //

    #ifdef WIN32
    {
      currentPid = (int) GetCurrentProcessId();
    }

    //
    // Linux.
    //

    #else
    {
      currentPid = getpid();
    }
    #endif

    return currentPid;
  }

  //
  // Retrieve full, absolute path to current running binary
  // (e.g. c:/dirligo/dirligo.exe)
  //
  // path     - buffer, where to store retrieved path (OUT).
  // pathSize - size of path[] buffer in bytes (IN).
  //
  // RETURNS: 0 if OK.
  //

  int ProcessGetModulePath(char *path, int pathSize)
  {
    int exitCode = -1;

    //
    // Windows.
    //

    #ifdef WIN32
    {
      FAIL(GetModuleFileName(NULL, path, pathSize) == FALSE);
    }

    //
    // Linux, MacOS.
    //

    #else
    {
      Error("ERROR: ProcessGetModulePath() is not implemented on this platform.\n");

      goto fail;
    }
    #endif

    //
    // Error handler.
    //

    exitCode = 0;

    fail:

    if (exitCode)
    {
      Error("ERROR: Cannot retrieve path of current running module.\n"
                "Error code is : %d.\n", GetLastError());
    }

    return exitCode;
  }

  //
  // Transform relative (to where current binary stored) path into full path.
  // E.g. it transforms "/somedir" into "<where current process live>/somedir".
  // We use it becouse daemon (service) doesn't start in path, where current binary
  // stored until we don't set current directory manually.
  //
  // WARNING: If passed path is already absolute it returns the same string.
  //
  // normPath - full, generated path (OUT).
  // pathSize - size of path buffer in bytes (IN).
  // relative - relative path postfix to add to base path (IN).
  // quiet    - do not write logs if set to 1 (IN).
  //
  // RETURNS: 0 if OK.
  //

  int ProcessExpandRelativePath(char *normPath, int pathSize, const char *relative)
  {
    DBG_ENTER3("processExpandRelativePath");

    int exitCode = -1;

    int lastSlash = 0;

    char *path = (char *) malloc(pathSize);

    FAILEX(path == NULL, "ERROR: Out of memory.\n");

    //
    // Check is it already absolute?
    //

    if (relative && relative[0] && relative[1] == ':')
    {
      DEBUG1("Path [%s] is already absolute.\n", relative);

      strncpy(path, relative, pathSize);
    }
    else
    {
      DEBUG3("Expanding path [%s]...\n", relative);

      //
      // Get full path to nxdevice.exe binary.
      //

      FAIL(ProcessGetModulePath(path, pathSize));

      //
      // Remove filename from path.
      //

      for (int i = 0; path[i] && i < pathSize; i++)
      {
        if (path[i] == '\\' || path[i] == '/')
        {
          lastSlash = i;
        }
      }

      path[lastSlash] = 0;

      //
      // Add relative postfix if any.
      //

      if (relative)
      {
        strncat(path, "\\", pathSize);
        strncat(path, relative, pathSize);
      }
    }

    //
    // Normalize path for output (e.g. remove '..' entries).
    //

    for (int i = 0; path[i]; i++)
    {
      if (path[i] == '/')
      {
        path[i] = '\\';
      }
    }

    #ifdef WIN32
    PathCanonicalize(normPath, path);
    #endif

    exitCode = 0;

    DEBUG1("Path [%s] expanded to [%s].\n", relative, normPath);

    //
    // Error handler.
    //

    fail:

    if (exitCode)
    {
      int err = GetLastError();

      Error("ERROR: Cannot retrieve path of binary.\n"
                "Error code is : %d.\n", err);

      if (err != 0)
      {
        exitCode = err;
      }
    }

    if (path != NULL)
    {
      free(path);
    }

    DBG_LEAVE3("ProcessExpandRelativePath");

    return exitCode;
  }

  //
  // Create duplicate of current running process and leave it alone in background,
  // while parent procses (current running) is going to quit.
  //
  // WARNING#1: Linux only.
  //
  // Child process  : function exits with code 0
  // Parent process : end of function never reached, process exits in the middle.
  //
  // RETURNS: 0 for child when success,
  //         -1 for parent if error.
  //

  int ProcessDaemonize()
  {
    int exitCode = -1;

    //
    // Windows.
    //

    #ifdef WIN32
    {
      Error("ERROR: ProcessDaemonize() not implemented on Windows.\n");

      SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

      goto fail;
    }

    //
    // Linux, MacOS.
    //

    #else
    {
      pid_t childPid = 0;

      //
      // Create child process.
      //

      childPid = fork();

      FAIL(childPid == -1);

      //
      // Parent process.
      // Exit whole process with success.
      //

      if (childPid > 0)
      {
        DEBUG1("Process daemonized with PID#%d. Parent is going to exit now.\n", childPid);

        exit(0);
      }

      //
      // Child process.
      // Close std handles and go on.
      //

      else
      {
        DEBUG1("Daemon process started.\n");

        close(STDIN_FILENO);
        close(STDOUT_FILENO);
        close(STDERR_FILENO);
      }
    }
    #endif

    //
    // Exit code.
    //

    exitCode = 0;

    fail:

    if (exitCode)
    {
      Error("ERROR: Cannot to daemonize process.\n"
                "Error code is : %d.\n", GetLastError());
    }

    return exitCode;
  }

  //
  // Retrieve full, absolute path to binary related with running process.
  //
  // WARNING#1: Caller MUSTS have PROCESS_VM_READ right to target process
  //            on Windows OS. In other case function will fail with system code
  //            ACCESS_DENIED (5).
  //
  // Parameters:
  //
  // binaryPath     - buffer, where to store full path to binary file related with
  //                  given running process (e.g. c:\windows\explorer.exe) (OUT)
  //
  // binaryPathSize - size of binaryPath[] buffer in bytes (IN).
  //
  // pid            - pid of process to check (IN).
  //
  // RETURNS: 0 if OK.
  //

  int ProcessGetBinaryPathByPid(char *binaryPath, int binaryPathSize, int pid)
  {
    int exitCode = -1;

    //
    // Check parameters.
    //

    FAILEX(binaryPath == NULL, "ERROR: 'binaryPath' cannot be NULL in processGetBinaryPath().\n");
    FAILEX(binaryPathSize <= 1, "ERROR: 'binaryPathSize' cannot be <= 1 in processGetBinaryPath().\n");
    FAILEX(pid <= 0, "ERROR: 'pid' cannot be <= 0 in processGetBinaryPath().\n");

    //
    // Windows.
    //

    #ifdef WIN32
    {
      HANDLE handle = NULL;

      //
      // Open process with read rights.
      //

      handle = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);

      if (handle == NULL)
      {
        Error("ERROR: Can't open process with pid #%d.\n"
                  "Error code is %d.\n", pid, GetLastError());

        goto fail;
      }

      //
      // Read binary name.
      //

      GetModuleFileNameEx(handle, 0, binaryPath, binaryPathSize - 1);

      //
      // Clean up.
      //

      CloseHandle(handle);
    }

    //
    // Linux, MacOS.
    //

    #else
    {
      Error("ERROR: processGetBinaryPath() not implemented on this OS.\n");

      goto fail;
    }
    #endif

    //
    // Error handler.
    //

    exitCode = 0;

    fail:

    return exitCode;
  }

} /* namespace Tegenaria */
