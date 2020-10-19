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
  //                         ThreadHandle_t based API
  //
  // --------------------------------------------------------------------------

  //
  // Close handle retrieved from ProcessCreate() before.
  //
  // WARNING: Function does NOT terminate process if still running.
  //
  // proc - handle to process retrieved from ProcessCreate() before (IN).
  //
  // RETURNS: 0 if success,
  //         -1 otherwise.
  //

  int ProcessClose(ProcessHandle_t *proc)
  {
    int exitCode = -1;

    if (proc)
    {
      CloseHandle(proc -> handle_);

      free(proc);

      exitCode = 0;
    }
    else
    {
      Error("ERROR: 'proc' can't be NULL in ProcessClose()");
    }

    return exitCode;
  }

  //
  // Check does given proces live.
  //
  // proc - process handle retrievied from ProcessCreate() function before (IN).
  //
  // RETURNS:  1 if process is running,
  //           0 if dead,
  //          -1 if error.
  //

  int ProcessIsRunning(ProcessHandle_t *proc)
  {
    int running = 0;

    //
    // Windows.
    //

    #ifdef WIN32
    {
      if (proc)
      {
        if (WaitForSingleObject(proc -> handle_, 0) == WAIT_TIMEOUT)
        {
          running = 1;
        }
      }
    }

    //
    // Linux, MacOS.
    //

    #else
    {
      if (proc)
      {
        if (proc -> pid_ > 0 && kill(proc -> pid_, 0) != -1)
        {
          running = 1
        }
      }
    }
    #endif

    return running;
  }

  //
  // Unconditional terminate process.
  //
  // proc - process handle retrievied from ProcessCreate() function before (IN).
  //
  // RETURNS:  0 if success,
  //          -1 otherwise.

  int ProcessKill(ProcessHandle_t *proc)
  {
    int exitCode = -1;

    #ifdef WIN32
    {
      int pid = -1;

      //
      // Windows.
      //

      FAILEX(proc == NULL, "ERROR: Proc can't be NULL in ProcessKill().\n");

      pid = proc -> pid_;

      DBG_MSG("Terminating process '%d'...\n", pid);

      FAIL(TerminateProcess(proc -> handle_, 1) == FALSE);

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
  // proc       - process handle retrievied from ProcessCreate() function before (IN).
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
  // TIP #1: On Linux you can get resultCode only for child process.
  //

  int ProcessWait(ProcessHandle_t *proc, int timeout, int *resultCode)
  {
    //
    // Windows.
    //

    #ifdef WIN32
    {
      int exitCode = -1;
      int pid      = -1;

      //
      // Check args.
      //

      FAILEX(proc == NULL, "ERROR: 'proc' can't be NULL in ProcessWait().\n");

      if (timeout < 0)
      {
        timeout = INFINITE;
      }

      //
      // Wait for dead or timeout.
      //

      DBG_MSG("Waiting for process PID '%d' with timeout '%d' ms...\n", pid, timeout);

      switch(WaitForSingleObject(proc -> handle_, timeout))
      {
        //
        // Process dead.
        //

        case WAIT_OBJECT_0:
        {
          DBG_MSG("Process PID '%d' signaled.\n", pid);

          if (resultCode)
          {
            GetExitCodeProcess(proc -> handle_, PDWORD(resultCode));
          }

          exitCode = 0;

          break;
        }

        //
        // Timeout.
        //

        case WAIT_TIMEOUT:
        {
          DBG_MSG("WARNING: Timeout reached while waiting for process PID '%d'.\n", pid);

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
  // TIP#1: Use ProcessCancelWatch() to remove process from monitor.
  //
  //
  // proc     - process handle retrieved from ProcessCreate() before (IN).
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

  int ProcessWatch(ProcessHandle_t *proc,
                       ProcessWatchCbProto callback,
                           int timeout, void *ctx)
  {
    if (proc)
    {
      return _ProcessMonitorAddPid(proc -> pid_, callback, timeout, ctx);
    }
    else
    {
      return -1;
    }
  }

  //
  // Remove process from monitored processes (added by ProcessWatch() before).
  //
  // proc - process handle retrieved from ProcessCreate() before (IN).
  //
  // RETURNS: 0 if OK.
  //

  int ProcessCancelWatch(ProcessHandle_t *proc)
  {
    if (proc)
    {
      return _ProcessMonitorRemovePid(proc -> pid_);
    }
    else
    {
      return -1;
    }
  }

  //
  // Return pid of given process.
  //
  // proc - process handle retrieved from ProcessCreate() before (IN).
  //
  // RETURNS: PID of process given as proc parameter,
  //          or -1 if error.
  //

  int ProcessGetPidByHandle(ProcessHandle_t *proc)
  {
    if (proc)
    {
      return proc -> pid_;
    }
    else
    {
      return -1;
    }
  }
} /* namespace Tegenaria */
