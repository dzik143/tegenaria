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

#ifndef Tegenaria_Core_Ipc_H
#define Tegenaria_Core_Ipc_H

#ifdef WIN32
# include <io.h>
# include <windows.h>
#endif

#ifdef __linux__
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#endif

#include <stdarg.h>

#include <Tegenaria/Thread.h>

namespace Tegenaria
{
  //
  // Defines.
  //

  #define IPC_DEFAULT_TIMEOUT 1000
  #define IPC_LEN_MAX         (1024 * 256)

  //
  // Typedef.
  //

  typedef int (*IpcWorkerProto)(int fd, void *ctx);

  struct IpcJob
  {
    const char *pipeName_;

    IpcWorkerProto callback_;

    int ready_;

    void *ctx_;
  };

  //
  // Exported functions.
  //

  int IpcServerLoop(const char *name, IpcWorkerProto callback,
                        void *ctx = NULL, int *ready = NULL);

  int IpcServerCreate(const char *name, IpcWorkerProto callback, void *ctx = NULL);

  int IpcServerKill(const char *name);

  int IpcServerMarkLastRequest(const char *name);

  int IpcOpen(const char *name, int timeout = IPC_DEFAULT_TIMEOUT);

  int IpcRequest(const char *pipeName, int *serverCode,
                     char *serverMsg, int serverMsgSize, const char *fmt, ...);

  int IpcSendAnswer(int fd, int code, const char *msg);

} /* namespace Tegenaria */

#endif /* Tegenaria_Core_Ipc_H */
