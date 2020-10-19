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

#include "Utils.h"
#include <Tegenaria/Mutex.h>

namespace Tegenaria
{
  #ifdef WIN32

  //
  // Create fake anonymous pipe pair ready for asynchronous read.
  //
  // pipe      - buffer to store read and write instances of pipe (OUT).
  // sa        - security attributes to set (IN).
  // bufSize   - buffer size in bytes (IN).
  // readMode  - flags for read pipe (IN).
  // writeMode - flags for write pipe (IN).
  //
  // RETURNS: 0 if OK.
  //

  int CreatePipeEx(int pipe[2], SECURITY_ATTRIBUTES sa[2], int bufSize,
                       DWORD readMode, DWORD writeMode, int timeout)
  {
    DBG_ENTER("CreatePipeEx");

    int exitCode = -1;

    char pipeName[MAX_PATH];

    static unsigned int pipeCount = 0;

    static Mutex pipeNoMutex("pipeNoMutex");

    HANDLE pipeHandle[2] = {NULL, NULL};

    unsigned int pipeNo = 0;

    //
    // Generate unique, thread safe ID.
    //

    pipeNoMutex.lock();
    pipeNo = pipeCount;
    pipeCount ++;
    pipeNoMutex.unlock();

    readMode  |= PIPE_ACCESS_INBOUND | GENERIC_READ;
    writeMode |= FILE_ATTRIBUTE_NORMAL;

    if (bufSize == 0)
    {
      bufSize = 4096;
    }

    sprintf(pipeName, "\\\\.\\Pipe\\pipe-anon.%08x.%08x",
                (unsigned int) GetCurrentProcessId(), pipeNo);

    //
    // Create read pipe.
    //

    DBG_MSG("Creating read pipe...\n");

    pipeHandle[1] = CreateNamedPipe(pipeName, readMode,
                                        PIPE_TYPE_BYTE, 1,
                                            bufSize, bufSize, timeout, &sa[0]);

    FAIL(pipeHandle[1] == INVALID_HANDLE_VALUE);

    //
    // Create write pipe.
    //

    DBG_MSG("Creating write pipe...\n");

    pipeHandle[0] = CreateFile(pipeName, GENERIC_WRITE, 0, &sa[1], OPEN_EXISTING,
                                   writeMode, NULL);

    FAIL(pipeHandle[0] == INVALID_HANDLE_VALUE);

    //
    // Wrap HANDLES into CRT FDs.
    //

    pipe[1] = _open_osfhandle(intptr_t(pipeHandle[0]), 0);
    pipe[0] = _open_osfhandle(intptr_t(pipeHandle[1]), 0);

    //
    // Error handler.
    //

    exitCode = 0;

    fail:

    if (exitCode)
    {
      int err = GetLastError();

      DBG_MSG("ERROR: Cannot create pipe pair.\n"
                  "Error code is : %d.\n", err);

      if (err != 0)
      {
        exitCode = err;
      }

      CloseHandle(pipeHandle[0]);
      CloseHandle(pipeHandle[1]);
    }

    DBG_LEAVE("CreatePipeEx");

    return exitCode;
  }

  #endif /* WIN32 */

} /* namespace Tegenaria */
