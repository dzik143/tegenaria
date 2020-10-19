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

#ifdef WIN32
# include <windows.h>
#else
# include <errno.h>
# include <stdio.h>
# include <unistd.h>
# include <signal.h>
# include <wait.h>
# include <sys/stat.h>
#endif

#include <Tegenaria/Debug.h>
#include "Process.h"
#include "Monitor.h"
#include "Internal.h"

namespace Tegenaria
{
  // --------------------------------------------------------------------------
  //
  //                              PID based API
  //
  // --------------------------------------------------------------------------

  //
  // Check does given proces live.
  //
  // pid - pid of process to check (IN).
  //
  // RETURNS:  1 if process is running,
  //           0 if dead,
  //          -1 if error.
  //

  int ProcessIsRunningById(int pid)
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

  //
  // Unconditional terminate process.
  //
  // pid - pid of process, which we want to kill (IN)
  //
  // RETURNS:  0 if success,
  //          -1 otherwise.
  //

  int ProcessKillByPid(int pid)
  {
    int exitCode = -1;

    #ifdef WIN32
    {
      //
      // Windows.
      //

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

    return exitCode;
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

  int ProcessWaitByPid(int pid, int timeout, int *resultCode)
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
  // Add process to monitored processes.
  // After that process monitor will inform caller when process dead
  // or specified timeout reached.
  //
  // pid      - process ID to monitor (IN).
  //
  // callback - callback function to notify user when process dead or
  //            timeout reached (IN).
  //
  // timeout  - maximum allowed life time for process in ms or -1 to infinite (IN).
  //
  // ctx      - caller specified context passed directly to callback (IN).
  //
  // RETURNS: 0 if OK.
  //

  int ProcessWatchByPid(int pid, ProcessWatchCbProto callback, int timeout, void *ctx)
  {
    return _ProcessMonitorAddPid(pid, callback, timeout, ctx);
  }

  //
  // Remove process from monitored processes (added by ProcessWatchByPid() before).
  //
  // pid - process ID passed to ProcessWatchByPid() before (IN).
  //
  // RETURNS: 0 if OK.
  //

  int ProcessCancelWatchByPid(int pid)
  {
    return _ProcessMonitorRemovePid(pid);
  }

} /* namespace Tegenaria */
