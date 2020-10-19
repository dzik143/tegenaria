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

//
// WARNING: Not used anywhere.
//          Review how to inform loop about output queue changes
//          when loop is blocked in read wait.

//
// Purpose: Give common scheme to handle main I/O loop with
//          many input/outputs.
//

#include "IOLoop.h"
#include <Tegenaria/Debug.h>

#ifdef WIN32
# include <windows.h>
# include <io.h>
#endif

namespace Tegenaria
{
  //
  // Template routine for main I/O loop.
  //
  // count    - number of FDs to read/write. MUST much with number of
  //            elements in fd[], direct[] and queue[] tables (IN).
  //
  // fd       - table with FDs to read or write (IN).
  //
  // direct   - 0 at index i menns fd[i] is input device,
  //            1 at index i means fd[i] is output device (IN).
  //
  // queue    - table with IOFifo objects related with related fd.
  //            Used to collect data incoming from fd (direct=0),
  //            or as data source written to fd (direct=1) (IN/OUT).
  //
  // callback - routine called when portion of data received or send.
  //            Can be NULL if not needed. (IN/OPT).
  //
  // RETURNS: 0 if loop finished in usual way,
  //         -1 if abnormal exit.
  //

  int IOLoop(int count, int *fd, int *direct,
                 IOFifo *queue, IOCompletedProto callback)
  {
    DBG_ENTER3("IOLoop");

    int goOn = 1;

    int exitCode = -1;

    /*
     * FIXME: Cross initialiation.
     */

    //FAILEX(count <= 0, "ERROR: count <= 0 passed to IOLoop.\n");
    //FAILEX(fd == NULL, "ERROR: fd cannot be NULL in IOLoop.\n");
    //FAILEX(direct == NULL, "ERROR: direct cannot be NULL in IOLoop.\n");
    //FAILEX(queue == NULL, "ERROR: queue cannot be NULL in IOLoop.\n");

    //
    // Windows.
    // Overlapped I/O based loop.
    //

    #ifdef WIN32
    {
      HANDLE *handle = (HANDLE *) calloc(count, sizeof(HANDLE));
      HANDLE *event  = (HANDLE *) calloc(count, sizeof(HANDLE));

      OVERLAPPED *ov = (OVERLAPPED *) calloc(count, sizeof(OVERLAPPED));

      int *pending = (int *) calloc(count, sizeof(int));

      void **buf = (void **) calloc(count, sizeof(void *));

      DWORD *bufSize = (DWORD *) calloc(count, sizeof(DWORD));

      DWORD waitResult  = 0;
      DWORD transferred = 0;

      int id = 0;

      //
      // For every CRT FD:
      //
      // - Get underlying HANDLE.
      // - Prepare overlapped I/O.
      // - Allocate work buffers to read/write data.
      //

      DEBUG2("IOLoop: Preparing OS Handles...\n");

      for (int i = 0; i < count; i++)
      {
        handle[i] = (HANDLE) _get_osfhandle(fd[i]);

        event[i] = CreateEvent(NULL, FALSE, FALSE, NULL);

        ov[i].hEvent = event[i];

        bufSize[i] = queue[i].capacity();

        buf[i] = malloc(bufSize[i]);
      }

      //
      // Fall into main loop.
      //

      DEBUG1("IOLoop: Falling into main I/O loop...\n");

      while(goOn)
      {
        //
        // Start asynchronous read/write.
        //

        for (int i = 0; i < count; i++)
        {
          //
          // Start asynchronous read for every input handles if:
          //
          // - we have space to store data in queue.
          // - there is no pending operation from last call.
          //

          if (direct[i] == 0)
          {
            if (queue[i].bytesLeft() > 0)
            {
              DEBUG3("Going to read up to [%d] bytes from FD #%d HANDLE #%d.",
                         queue[i].bytesLeft(), fd[i], handle[i]);

              ReadFile(handle[i], buf[i], queue[i].bytesLeft(), NULL, &ov[i]);

              pending[i] = 1;
            }
          }

          //
          // Start asynchronous write for every output handle if:
          //
          // - we have something to write in queue.
          // - there is no pending operation from last call.
          //

          else
          {
            if (queue[i].size() > 0)
            {
              DEBUG3("Going to write [%d] bytes to FD #%d HANDLE #%d.",
                         queue[i].size(), fd[i], handle[i]);

              queue[i].pop(buf[i]);

              WriteFile(handle[i], buf[i], bufSize[i], NULL, &ov[i]);

              pending[i] = 1;
            }
          }
        }

        //
        // Wait until one of asyncrhonous operation finished.
        //

        waitResult = WaitForMultipleObjects(count, event, FALSE, INFINITE);

        id = waitResult - WAIT_OBJECT_0;

        FAILEX(id > count,
                   "ERROR: WaitForMultipleObjects() failed with"
                       " code [%d].\n", waitResult);

        GetOverlappedResult(handle[id], &ov[id], &transferred, TRUE);

        //
        // One of asynchronous read finished.
        //

        if (direct[id] == 0)
        {
          FAILEX(transferred == 0,
                     "ERROR: Cannot read from FD #%d handle #%d.\n",
                         fd[id], handle[id]);

          DEBUG3("Readed [%d] bytes from FD #%d handle #%d.\n",
                     transferred, fd[id], handle[id]);

          queue[id].push(buf[id], transferred);
        }

        //
        // One of asynchronous write finished.
        //

        else
        {
          FAILEX(transferred == 0,
                     "ERROR: Cannot write to FD #%d handle #%d.\n",
                         fd[id], handle[id]);

          DEBUG3("Written [%d] bytes to FD #%d handle #%d.\n",
                     transferred, fd[id], handle[id]);
        }

        //
        // Disable pending flag.
        // Move device pointr for block devices (e.g. files).
        //

        pending[0] = 0;

        ov[id].Offset += transferred;

        //
        // Inform caller that next portion of data received
        // or sent.
        //

        if (callback)
        {
          callback(fd[id], transferred, direct[id], &queue[id]);
        }
      }

      exitCode = 0;

      //
      // Clean up.
      //

      fail:

      DEBUG1("IOLoop: Main I/O loop finished.\n");

      for (int i = 0; i < count; i++)
      {
        CloseHandle(event[i]);

        free(buf[i]);
      }

      free(handle);
      free(event);
      free(ov);
      free(pending);
      free(buf);
      free(bufSize);
    }

    //
    // Linux, MacOS.
    // Select based loop.
    //

    #else
    {
      Error("ERROR: IOLoop is not implemented.\n");
    }
    #endif

    return exitCode;
  }
} /* namespace Tegenaria */
