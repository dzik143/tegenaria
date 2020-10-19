/******************************************************************************/
/*                                                                            */
/* Copyright (c) 2010, 2014 Sylwester Wysocki <sw143@wp.pl>                   */
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
  #include <windows.h>
#else
# include <errno.h>
# include <stdio.h>
# include <unistd.h>
# include <signal.h>
# include <wait.h>
# include <pthread.h>
#endif

#include <Tegenaria/Debug.h>

#include "Thread.h"
#include "Internal.h"

namespace Tegenaria
{
  //
  // Allocate new ProcessHandle_t struct basing on system handle/pid data.
  //
  // handle - underlying system handle (IN).
  // id     - underlying thread id (IN).
  //
  // RETURNS: Pointer to new allocated handle,
  //          or NULL if error.
  //

  static ThreadHandle_t *_ThreadHandleAlloc(void *handle, int id)
  {
    ThreadHandle_t *rv = (ThreadHandle_t *) calloc(sizeof(ThreadHandle_t), 1);

    if (rv)
    {
      #ifdef WIN32
      rv -> handle_ = handle;
      #else
      rv -> handle_ = (pthread_t) handle;
      #endif

      rv -> id_ = id;
    }
    else
    {
      Error("ERROR: Can't allocate ThreadHandle_t. Out of memory?\n");
    }

    return rv;
  }

  //
  // Internal wrapper over caller entry point to:
  // - hide OS differences (entry point on Linux has different proto)
  // - get signal, when thread finished in easy way
  //
  //
  //

  static void *_ThreadEntryWrapperLinux(void *rawCtx)
  {
    ThreadCtx_t *ctx = (ThreadCtx_t *) rawCtx;

    //
    // TODO: Handle thread result.
    //

    if (ctx)
    {
      int result = ctx -> callerEntry_(ctx -> callerCtx_);

      free(ctx);
    }
  }

  //
  // Create new thread.
  //
  // entry - thread entry point i.e. pointer to function, where code execution
  //         will be started (MUST be static) (IN).
  //
  // ctx   - arbitrary data passed directly to the thread entry point. Can
  //         be NULL if not needed (IN/OPT).
  //
  // RETURNS: handle to new thread,
  //          or NULL if error.
  //


  ThreadHandle_t *ThreadCreate(ThreadEntryProto entry, void *ctx)
  {
    int exitCode = -1;

    ThreadHandle_t *rv = NULL;

    //
    // Check args.
    //

    FAILEX(entry == NULL, "ERROR: 'entry' can't be NULL in ThreadCreate().\n");

    //
    // Allocate new thread handle.
    //

    rv = _ThreadHandleAlloc(NULL, -1);

    FAILEX(rv == NULL, "ERROR: Can't alloc thread space.\n");

    //
    // Dispatch to proper OS.
    //

    #ifdef WIN32
    {
      //
      // Windows.
      //

      HANDLE handle = NULL;

      DWORD id = 0;

      handle = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) entry, ctx, 0, &id);

      FAIL(handle == NULL);

      //
      // Wrap low level {handle, threadId} into ThreadHandle_t.
      //

      rv -> id_        = id;
      rv -> handle_    = handle;
      rv -> isRunning_ = 1;

      DBG_SET_ADD("thread", rv, "%d", id);

      DBG_MSG("Created thread ID# '%d', PTR '%0x', .\n", id, rv);
    }
    #else
    {
      //
      // Linux, MacOS.
      //

      pthread_t handle;

      ThreadCtx_t *threadCtx = NULL;

      //
      // Allocate thread context to group all info in one structure.
      //

      threadCtx = (ThreadCtx_t *) calloc(1, sizeof(ThreadCtx_t));

      FAILEX(threadCtx == NULL, "ERROR: Can't allocate thread context.");

      //
      // Fill up thread context structure.
      //

      threadCtx -> th_          = rv;
      threadCtx -> callerEntry_ = entry;
      threadCtx -> callerCtx_   = ctx;

      //
      // Create thread using wrapper entry point to hide different between
      // Linux and Windows.
      //

      FAILEX(pthread_create(&handle, NULL, _ThreadEntryWrapperLinux, (void *) threadCtx),
               "ERROR: Can't create thread.");

      //
      // Wrap low level {handle, threadId} into ThreadHandle_t.
      // TODO: Get ThreadId on Linux.
      //

      rv -> id_        = -1;
      rv -> handle_    = handle;
      rv -> isRunning_ = 1;

      DBG_SET_ADD("thread", rv, "%d", id);

      DBG_MSG("Created thread ID# '%d', PTR '%0x', .\n", -1, rv);
    }
    #endif

    //
    // Error handler.
    //

    exitCode = 0;

    fail:

    if (exitCode)
    {
      Error("ERROR: Cannot create thread with entry '%p'.\n"
                  "System error code is : %d.\n", entry, GetLastError());

      ThreadClose(rv);
    }

    return rv;
  }

  //
  // Check is thread running.
  //
  // handle - thread handle returned by threadCreate() before (IN).
  //
  // RETURN:  1 if thread running,
  //          0 if thread dead,
  //         -1 if error.
  //

  int ThreadIsRunning(ThreadHandle_t *th)
  {
    #ifdef WIN32
    {
      //
      // Windows.
      //

      if (WaitForSingleObject(th -> handle_, 0) == WAIT_TIMEOUT)
      {
        return 1;
      }
      else
      {
        th -> isRunning_ = 0;

        DBG_SET_MOVE("thread_terminated", "thread", th);
      }

      return 0;
    }
    #else
    {
      //
      // Linux, MacOS.
      //

      Fatal("ThreadIsRunning() not implemented on this platform.\n");
    }
    #endif
  }

  //
  // Close handle retrieved from ThreadCreate() before.
  //
  // WARNING: Function does NOT terminate thread if it still running.
  //
  // TIP#1: Use ThreadKill() if you want to terminate thread before close.
  // TIP#2: Use ThreadWait() if you want to wait until thread finished before close.
  //
  // th - handle to thread retrieved from ThreadCreate() before (IN).
  //
  // RETURNS:  0 if thread finished before timeout,
  //          -1 if thread still working.
  //

  int ThreadClose(ThreadHandle_t *th)
  {
    int exitCode = -1;

    //
    // Check args.
    //

    FAILEX(th == NULL, "ERROR: 'th' can't be NULL in ThreadClose().\n");

    //
    // Dispatch to proper OS.
    //

    #ifdef WIN32
    {
      //
      // Windows.
      //

      CloseHandle(th -> handle_);

      if (th -> id_ == GetCurrentThreadId())
      {
        DBG_MSG("WARNING: Self join by thread ID#%d.\n", th -> id_);
      }

      free(th);

      DBG_SET_DEL("thread", th);
      DBG_SET_DEL("thread_terminated", th);
    }

    //
    // Linux, MacOS.
    //

    #else
    {
      Fatal("ThreadClose() not implemented on this platform.\n");
    }
    #endif

    //
    // Error handler.
    //

    exitCode = 0;

    fail:

    if (exitCode)
    {
      Error("ERROR: Cannot close handle '%p'.\n"
              "System error code is : %d.\n", th, GetLastError());
    }

    return exitCode;
  }

  //
  // Unconditionaly terminate thread.
  //
  // th - thread handle retrieved from ThreadCrate() before (IN).
  //
  // RETURNS:  0 if thread terminated,
  //          -1 otherwise.
  //

  int ThreadKill(ThreadHandle_t *th)
  {
    int exitCode = -1;

    //
    // Check args.
    //

    FAILEX(th == NULL, "ERROR: 'th' can't be NULL in ThreadKill().\n");

    //
    // Dispatch to proper OS.
    //

    #ifdef WIN32
    {
      //
      // Windows.
      //

      DBG_MSG3("Terminating thread handle '%p' ID #%d...\n", th, th -> id_);

      FAIL(TerminateThread(th -> handle_, 1) == FALSE);

      DBG_MSG("Thread handle '%p', ID #%d terminated.\n", th, th -> id_);
    }
    #else
    {
      //
      // Linux, MacOS.
      //

      Fatal("ThreadKill() not implemented on this platform.\n");
    }
    #endif

    //
    // Success, thread isn't running longer.
    //

    th -> isRunning_ = 0;

    DBG_SET_MOVE("thread_terminated", "thread", th);

    exitCode = 0;

    //
    // Error handler.
    //

    fail:

    if (exitCode)
    {
      Error("ERROR: Cannot terminate thread '%p'.\n"
                "System error code is : %d.\n", th, GetLastError());
    }

    return exitCode;
  }

  //
  // Wait until thread finished work or timeout.
  //
  // WARNING: Function DOES not clear resources allocated by thread event
  //           if thread terminated before funtion return. Use ThreadClose()
  //          to free thread handle, when no needed longer.
  //
  // th      - thread handle retrieved from ThreadCreate() before (IN).
  //
  // result  - buffer, where to store exit code returned by thread.
  //           Can be NULL if not needed (OUT/OPT).
  //
  // timeout - maximum time to wait im milisecond. Defaulted to infinite if
  //           skipped or set to -1. (IN/OPT).
  //
  // RETURNS: 0 if thread finished before timeout,
  //          1 if thread still working,
  //         -1 if error.
  //

  int ThreadWait(ThreadHandle_t *th, int *result, int timeoutMs)
  {
    int exitCode = -1;

    //
    // Check args.
    //

    FAILEX(th == NULL, "ERROR: 'th' can't be NULL in ThreadWait().\n");

    //
    // Dispatch to proper OS.
    //

    #ifdef WIN32
    {
      //
      // Windows.
      //

      DWORD waitResult = 0;

      //
      // Fail if attemp to wait for current running thread detected.
      //

      FAILEX(th -> id_ == GetCurrentThreadId(),
                 "ERROR: Can't wait for current running thread.\n");

      DBG_MSG("Waiting for thread handle '%p', id #%d with timeout '%d' ms...\n",
                  th, th -> id_, timeoutMs);

      //
      // Faill into wait loop.
      //

      waitResult = WaitForSingleObject(th -> handle_, timeoutMs);

      //
      // Wait loop finished.
      // Dispatch basing on wait result.
      //

      switch(waitResult)
      {
        //
        // Thread finished.
        //

        case WAIT_OBJECT_0:
        {
          DWORD code = 0;

          GetExitCodeThread(th -> handle_, &code);

          DBG_MSG("Thread handle '%p', ID#%d finished with result #%d.\n",
                      th, th -> id_, int(code));

          DBG_SET_MOVE("thread_terminated", "thread", th);

          th -> isRunning_   = 0;
          th -> result_      = (int) code;
          th -> isResultSet_ = 1;

          if (result)
          {
            *result = code;
          }

          exitCode = 0;

          break;
        }

        //
        // Timeout.
        //

        case WAIT_TIMEOUT:
        {
          DBG_MSG("Thread handle '%p', ID#%d timeout.\n", th, th -> id_);

          exitCode = 1;

          break;
        }

        //
        // Unexpected error.
        //

        default:
        {
          Error("ERROR: Unexpected ThreadWait() failed with code 0x%x.\n"
                "Thread handle '%p', ID#%d.\n",
                waitResult, th, th -> id_);

          exitCode = -1;
        }
      }
    }
    #else
    {
      //
      // Linux, MacOS.
      //

      Fatal("ThreadWait() not implemented on this platform.\n");
    }
    #endif

    //
    // Error handler.
    //

    fail:

    if (exitCode == -1)
    {
      Error("ERROR: Cannot wait for thread handle '%p'.\n"
                "System error code is : %d.\n", th, GetLastError());
    }

    return exitCode;
  }

  //
  // Get id of current running thread.
  //

  int ThreadGetCurrentId()
  {
    #ifdef WIN32
    return GetCurrentThreadId();
    #else
    Fatal("ThreadGetCurrentId() not implemented on this platform.\n");
    #endif
  }

} /* namespace Tegenaria */
