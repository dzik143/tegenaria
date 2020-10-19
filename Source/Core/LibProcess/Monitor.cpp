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
// Purpose: Monitor selected process to inform caller when it dead
//          or specified timeout reached.
//

#pragma qcbuild_set_private(1)

#include "Monitor.h"
#include "Internal.h"
#include <Tegenaria/Semaphore.h>

#ifndef _WIN32
# include <signal.h>
# include <wait.h>
#endif

namespace Tegenaria
{
  //
  // Globabal variables.
  //

  static ThreadHandle_t *ProcessMonitorThread = NULL;

  static int ProcessMonitorEnabled = 0;

  //
  // List of monitored processes.
  //

  static list<ProcessMonitorEntry_t *> ProcessList;

  //
  // Mutex to lock ProcessList.
  //

  static Mutex ProcessMonitorMutex("ProcessMonitorMutex");

  static Semaphore ProcessMonitorSem;

  //
  // Reset event to let monitor thread know when
  // list of monitored processes changed.
  //

  #ifdef WIN32
  static HANDLE ProcessMonitorResetEvent = NULL;
  #else

  void sigchldHandler(int signal)
  {
    if (signal == SIGCHLD)
    {
      ProcessMonitorSem.signal();
    }
  }

  void setSignalHandler()
  {
    signal(SIGCHLD, &sigchldHandler);
  }

  #endif

  //
  // Main monitor loop working in background thread.
  // Monitor processes specified in ProcessList and call user
  // specified callback when one of below happen:
  //
  // - process dead
  // - specified timeout reached
  //
  // TIP: Use processWatch() to add process to monitor.
  //

  int _ProcessMonitorLoop(void *unused)
  {
    int exitCode = -1;

    DBG_ENTER("ProcessMonitorLoop");

    ProcessMonitorEnabled = 1;

    //
    // We don't need synchronise this thread with any else,
    // do selt join.
    //

    // threadClose(ProcessMonitorThread);

    //
    // Windows.
    //

    #ifdef WIN32
    {
      DWORD ret = 0;
      DWORD id  = 0;

      DWORD timeout = INFINITE;

      ProcessMonitorEntry_t *proc = NULL;

      vector<HANDLE> handles;

      vector<ProcessMonitorEntry_t *> handlesProcs;

      list<ProcessMonitorEntry_t *>::iterator it;

      while(ProcessMonitorEnabled)
      {
        //
        // Collect handles needed to be watched.
        // Set wait timeout to minimum from all processes.
        //

        handles.clear();
        handlesProcs.clear();

        timeout = INFINITE;

        DBG_MSG("ProcessMonitorLoop : Collecting handles...\n");

        ProcessMonitorMutex.lock();

        for (it = ProcessList.begin(); it != ProcessList.end(); it++)
        {
          proc = *it;

          handles.push_back(OpenProcess(SYNCHRONIZE, FALSE, proc -> pid_));

          handlesProcs.push_back(proc);

          //
          // Set wait timeout to minimum from every monitored processes.
          //

          if (proc -> timeout_ > 0 && DWORD(proc -> timeout_) < timeout)
          {
            timeout = proc -> timeout_;
          }
        }

        ProcessMonitorMutex.unlock();

        //
        // Add reset event at the end to know when new process arrived.
        //

        handles.push_back(ProcessMonitorResetEvent);

        //
        // Check is loop finished before falling into wait.
        //

        if (ProcessMonitorEnabled == 0)
        {
          break;
        }

        //
        // Wait until one of below happen:
        //
        // - one of process dead
        // - timeout (set to minimum from collected processes)
        // - reset event signaled
        //

        DBG_MSG("ProcessMonitorLoop : Waiting for"
                    " something happen with timeout [%d]...\n", timeout);

        ret = WaitForMultipleObjects(handles.size(), &handles[0], FALSE, timeout);

        //
        // Wait timeout.
        // At least one of processes reaches its timeout.
        //

        if (ret == WAIT_TIMEOUT)
        {
          ProcessMonitorMutex.lock();

          for (it = ProcessList.begin(); it != ProcessList.end(); it++)
          {
            ProcessMonitorEntry_t *proc = *it;

            //
            // If timeout set on given process decrease it with
            // time of last WaitForMultpleObject() time.
            //

            if (proc -> timeout_ > 0)
            {
              proc -> timeout_ -= timeout;

              if (proc -> timeout_ <= 0)
              {
                DBG_MSG("ProcessMonitorLoop : Process PID #%d timeout."
                            " Terminating...", proc -> pid_);

                ProcessKillByPid(proc -> pid_);
              }
            }
          }

          ProcessMonitorMutex.unlock();
        }

        //
        // One one of handle signaled or error.
        //

        else
        {
          //
          // Check which handle signaled.
          //

          id = ret - WAIT_OBJECT_0;

          if (id >= handles.size())
          {
            DEBUG1("ProcessMonitorLoop: WARNING: Wait failed with code [0x%x].\n",
                         (unsigned int) ret);

            continue;
          }

          //
          // Reset event. Only reset thread loop to collect
          // actual process list once again.
          //

          if (handles[id] == ProcessMonitorResetEvent)
          {
            DBG_MSG("ProcessMonitorLoop : Reset received.\n");
          }

          //
          // One of processes dead. Inform caller via callback
          // and remove process from list.
          //

          else
          {
            int found = 0;

            proc = handlesProcs[id];

            DBG_MSG("ProcessMonitorLoop : Process PID [%d] signaled.\n", proc -> pid_);

            //
            // Call callback specified in processWatch() function.
            //

            if (proc -> callback_)
            {
              proc -> callback_(proc -> pid_, proc -> ctx_);
            }

            //
            // Remove process from list.
            //

            ProcessMonitorMutex.lock();

            for (it = ProcessList.begin(); it != ProcessList.end(); it++)
            {
              if (proc == *it)
              {
                DBG_MSG("ProcessMonitorLoop : Removing process"
                            " PID [%d] from list...\n", proc -> pid_);

                ProcessList.erase(it);

                found = 1;

                break;
              }
            }

            //
            // Free related Process struct.
            //

            if (found)
            {
              DBG_MSG("ProcessMonitorThread : "
                          "Freeing process structure PID#%d PTR#%p...\n",
                              proc -> pid_, proc);

              free(proc);
            }

            ProcessMonitorMutex.unlock();
          }
        }

        //
        // Clean handles table before next epoch.
        // Skip last handle, becouse it's ready event.
        //

        for (int i = 0; i < handles.size() - 1; i++)
        {
          CloseHandle(handles[i]);
        }
      }
    }
    #else

    //
    // Linux, MacOS.
    //

    {
      Fatal("ProcessMonitorLoop() not implemented.");
    }

    #endif

    DBG_MSG("Process monitor loop finished.\n");

    //
    // Error handler.
    //

    exitCode = 0;

    fail:

    if (exitCode)
    {
      Error("ERROR: Cannot monitor processes.\n"
                "Error code is : %d.\n", GetLastError());
    }

    ProcessMonitorEnabled = 0;

    DBG_LEAVE("processMonitorLoop");

    return exitCode;
  };

  //
  // Stop ProcessMonitor thread inited by ProcessMonitorInit() function.
  // Called internally as atexit() when needed.
  //

  void _ProcessMonitorUninit()
  {
    if (ProcessMonitorThread)
    {
      ProcessMonitorEnabled = 0;

      #ifdef WIN32
      SetEvent(ProcessMonitorResetEvent);
      #endif

      ThreadWait(ProcessMonitorThread);
      ThreadClose(ProcessMonitorThread);

      ProcessMonitorThread = NULL;
    }
  }

  //
  // Init process monitor.
  // Called one time to init monitor thread.
  // Used internally only.
  //

  int _ProcessMonitorInit()
  {
    DBG_ENTER("ProcessMonitorInit");

    int exitCode = -1;

    //
    // Create monitor thread.
    //

    if (ProcessMonitorThread == 0)
    {
      ProcessMonitorThread = ThreadCreate(_ProcessMonitorLoop, NULL);

      FAILEX(ProcessMonitorThread == 0, "ERROR: Cannot create Process Monitor thread.\n");
    }

    DBG_SET_RENAME("thread", ProcessMonitorThread, "ProcessMonitor");

    //
    // Windows only.
    // Create reset event to let main thread
    // know when new process arrived.
    //

    #ifdef WIN32
    {
      if (ProcessMonitorResetEvent == NULL)
      {
        ProcessMonitorResetEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
      }
    }
    #endif

    atexit(_ProcessMonitorUninit);

    exitCode = 0;

    //
    // Error handler.
    //

    fail:

    if (exitCode)
    {
      Error("ERROR: Cannot initialize process monitor.\n"
                "Error code is : %d.\n", GetLastError());
    }

    DBG_LEAVE("ProcessMonitorInit");

    return exitCode;
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

  int _ProcessMonitorAddPid(int pid, ProcessWatchCbProto callback, int timeout, void *ctx)
  {
    DBG_ENTER("processWatch");

    int exitCode = -1;

    ProcessMonitorEntry_t *proc = NULL;

    list<ProcessMonitorEntry_t *>::iterator it;

    int found = 0;

    //
    // Check args.
    //

    FAILEX(pid < 0, "ERROR: Wrong PID in processWatch.\n");

    FAILEX(callback == NULL, "ERROR: Callback function cannot be NULL in processWatch.\n");

    //
    // Check is given PID not already watched.
    //

    ProcessMonitorMutex.lock();

    for (it = ProcessList.begin(); found == 0 && it != ProcessList.end(); it++)
    {
      if ((*it) -> pid_ == pid)
      {
        found = 1;

        DBG_MSG("ProcessWatch : PID #%d already monitored.\n", pid);
      }
    }

    ProcessMonitorMutex.unlock();

    if (found == 0)
    {
      //
      // Init process monitor thread if not inited yet.
      //

      _ProcessMonitorInit();

      //
      // Allocate new Process struct.
      //

      proc = (ProcessMonitorEntry_t *) malloc(sizeof(ProcessMonitorEntry_t));

      FAILEX(proc == NULL, "ERROR: Out of memory.\n");

      proc -> pid_      = pid;
      proc -> timeout_  = timeout;
      proc -> callback_ = callback;
      proc -> ctx_      = ctx;

      //
      // Add process to monitored list.
      //

      DBG_MSG("processWatch : Adding process PID [%d] with timeout [%d] ms"
                  " to monitored list...\n", pid, timeout);

      ProcessMonitorMutex.lock();

      ProcessList.push_back(proc);

      ProcessMonitorMutex.unlock();

      //
      // Reset process monitor thread on windows.
      //

      #ifdef WIN32
      SetEvent(ProcessMonitorResetEvent);
      #endif
    }

    exitCode = 0;

    //
    // Error handler.
    //

    fail:

    if (exitCode)
    {
      Error("ERROR: Cannot add process PID [%d] to monitored list.\n"
                "Error code is : %d.\n", GetLastError());
    }

    DBG_LEAVE("ProcessWatch");

    return exitCode;
  }

  //
  // Remove process from process monitor, added before by processWatch().
  //
  // pid - process ID to remove from monitor (IN).
  //
  // RETURNS: 0 if OK.
  //

  int _ProcessMonitorRemovePid(int pid)
  {
    list<ProcessMonitorEntry_t *>::iterator it;

    ProcessMonitorMutex.lock();

    for (it = ProcessList.begin(); it != ProcessList.end(); it++)
    {
      if ((*it) -> pid_ == pid)
      {
        DBG_MSG("ProcessCancelWatch : Removing PID #%d PTR #%p from process monitor...\n", pid, *it);

        free(*it);

        ProcessList.erase(it);

        break;
      }
    }

    ProcessMonitorMutex.unlock();

    //
    // Tell the process monitor that process list changed.
    //

    #ifdef WIN32
    {
      SetEvent(ProcessMonitorResetEvent);
    }
    #else
    {
      ProcessMonitorSem.signal();
    }
    #endif
  }
} /* namespace Tegenaria */
