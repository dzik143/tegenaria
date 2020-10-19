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
// Purpose: Expand standard read/write functions with timeout functionality.
//          All below functions work with CRT FD, retrieved from standard
//          open() call (or equivalent).
//

#include <Tegenaria/Debug.h>
#include <Tegenaria/Thread.h>
#include "IOTime.h"

#ifdef WIN32
# include <windows.h>
# include <io.h>
#else
# include <unistd.h>
#endif

namespace Tegenaria
{
  //
  // Works like standard CRT read(), but with timeout functionality.
  //
  // fd      - CRT file descriptor (received from open()) (IN)
  // buf     - buffer where to store readed data (OUT)
  // size    - number of bytes to read (IN)
  // timeout - timeout in ms or -1 for infinite (IN)
  //
  // RETURNS: number of bytes readed or
  //          -1 if error.
  //

  int IOTimeoutRead(int fd, void *buf, int size, int timeout)
  {
    DBG_ENTER3("IOTimeoutRead");

    int exitCode = -1;

    DBG_IO_READ_BEGIN("FD", fd, buf, size);

    //
    // Windows.
    // FIXME: Use overlapped IO when possible.
    //

    #ifdef WIN32
    {
      //
      // Inifinite timeout. Just call read().
      //

      if (timeout < 0)
      {
        exitCode = read(fd, buf, size);
      }

      //
      // Timeout specified. Dispatch code depending on HANDLE type.
      //

      else
      {
        HANDLE handle = HANDLE(_get_osfhandle(fd));

        DWORD handleType = GetFileType(handle);

        switch(handleType)
        {
          //
          // Char device (console).
          //

          case FILE_TYPE_CHAR:
          {
            DEBUG3("IOTimeoutRead : FD#%d is CHAR device.\n", fd);

            int goOn = 1;

            int sleepTime = 5;

            FlushConsoleInputBuffer(handle);

            DEBUG3("IOTimeoutRead : Falling into wait loop for FD#%d...\n", fd);

            while(goOn)
            {
              INPUT_RECORD irec = {0};

              DWORD eventsReaded = 0;

              //
              // Peek input console.
              //

              FAILEX(PeekConsoleInput(handle, &irec, 1, &eventsReaded) == FALSE,
                         "ERROR: Cannot peek console input HANDLE #%p FD #%d\n",
                             handle, fd);

              //
              // Check is any events queued in buffer.
              //

              if (eventsReaded > 0)
              {
                //
                // Key event. It's signal to read.
                //

                if (irec.EventType == KEY_EVENT && irec.Event.KeyEvent.bKeyDown)
                {
                  DEBUG3("IOTimeoutRead : Reading from FD#%d...\n", fd);

                  exitCode = read(fd, buf, size);

                  goOn = 0;
                }

                //
                // Non key related event. Pop from queue and skip.
                //

                else
                {
                  ReadConsoleInput(handle, &irec, 1, &eventsReaded);
                }
              }

              //
              // Control timeout.
              //

              else
              {
                Sleep(sleepTime);

                timeout -= sleepTime;

                if (timeout <= 0)
                {
                  Error("ERROR: Timeout reached while reading from FD#%d.\n", fd);

                  SetLastError(WAIT_TIMEOUT);

                  goto fail;
                }

                if (sleepTime < 100)
                {
                  sleepTime *= 2;
                }
              }
            }

            break;
          }

          //
          // Pipe or socket.
          //

          case FILE_TYPE_PIPE:
          {
            int optVal = 0;
            int optLen = sizeof(optVal);

            //
            // Socket.
            //

            if (getsockopt((SOCKET) handle, SOL_SOCKET,
                               SO_ACCEPTCONN, (char *) &optVal, &optLen) == 0)
            {
              DEBUG3("IOTimeoutRead : FD#%d is a SOCKET device.\n", fd);

              fd_set rfd;

              struct timeval tv;

              FD_ZERO(&rfd);

              FD_SET(fd, &rfd);

              tv.tv_sec  = timeout / 1000;
              tv.tv_usec = (timeout % 1000) * 1000;

              DEBUG3("IOTimeoutRead : Falling into select loop for FD#%d...\n", fd);

              if (select((SOCKET) fd + 1, &rfd, NULL, NULL, &tv) > 0 && FD_ISSET(fd, &rfd))
              {
                DEBUG3("IOTimeoutRead : Reading from FD#%d...\n", fd);

                exitCode = read(fd, buf, size);
              }
              else
              {
                Error("ERROR: Timeout reached while reading from FD#%d.\n", fd);

                goto fail;
              }
            }

            //
            // Pipe.
            //

            else
            {
              DEBUG3("IOTimeoutRead : FD#%d is a PIPE device.\n", fd);

              DWORD readed = 0;

              DWORD pFlags = 0;

              DWORD sleepTime = 5;

              int goOn = 1;

              FAILEX(GetNamedPipeInfo(handle, &pFlags, NULL, NULL, NULL) == FALSE,
                         "ERROR: Cannot get pipe info for HANDLE #%p FD #%d.\n",
                             handle, fd);


              DEBUG3("IOTimeoutRead : Falling into wait loop for FD#%d...\n", fd);

              while(goOn)
              {
                FAILEX(PeekNamedPipe(handle, NULL, 0, NULL, &readed, NULL) == FALSE,
                           "ERROR: Cannot peek pipe HANDLE #%p FD #%p.\n",
                               handle, fd);


                //
                // Any bytes in buffer. It's signal to read.
                //

                if (readed > 0)
                {
                  DEBUG3("IOTimeoutRead : Reading from FD#%d...\n", fd);

                  exitCode = read(fd, buf, size);

                  goOn = 0;
                }

                //
                // No data in buffer, wait a moment and try again.
                //

                else
                {
                  Sleep(sleepTime);

                  timeout -= sleepTime;

                  if (timeout <= 0)
                  {
                    Error("ERROR: Timeout reached while reading from FD#%d.\n", fd);

                    SetLastError(WAIT_TIMEOUT);

                    goto fail;
                  }

                  if (sleepTime < 100)
                  {
                    sleepTime *= 2;
                  }
                }
              }
            }

            break;
          }

          //
          // File on disk.
          //

          case FILE_TYPE_DISK:
          {
            DEBUG3("IOTimeoutRead : FD#%d is DISK device.\n", fd);

            exitCode = read(fd, buf, size);

            break;
          }

          //
          // Unknown type.
          //

          default:
          {
            Error("ERROR: Unknown device type for FD#%d.\n", fd);

            goto fail;
          }
        }
      }
    }

    //
    // Linux, MacOS.
    // Select based.
    //

    #else
    {
      fd_set rfd;

      struct timeval tv;

      FD_ZERO(&rfd);

      FD_SET(fd, &rfd);

      tv.tv_sec  = timeout / 1000;
      tv.tv_usec = (timeout % 1000) * 1000;

      if (select(fd + 1, &rfd, NULL, NULL, &tv) > 0 && FD_ISSET(fd, &rfd))
      {
        exitCode = read(fd, buf, size);
      }
    }
    #endif

    //
    // Error handler.
    //

    fail:

    if (exitCode < 0)
    {
      Error("ERROR: Cannot read from FD #%d. Error is %d.\n", fd, GetLastError());
    }

    DBG_IO_READ_END("FD", fd, buf, size);

    DBG_LEAVE3("IOTimeoutRead");

    return exitCode;
  }
} /* namespace Tegenaria */
