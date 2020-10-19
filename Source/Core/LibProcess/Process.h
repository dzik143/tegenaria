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

#ifndef Tegenaria_Core_Process_H
#define Tegenaria_Core_Process_H

#include <cstdlib>

#ifdef WIN32
# include <windows.h>
#endif

namespace Tegenaria
{
  //
  // Defines.
  //

  #define PROCESS_FD_TYPE_CRT           0
  #define PROCESS_FD_TYPE_WINAPI_HANDLE 1
  #define PROCESS_FD_TYPE_SOCKET        2

  #define PROCESS_FD_INHERIT -1
  #define PROCESS_FD_NULL    -2

  #define PROCESS_OPTIONS_NO_WINDOW 1

  //
  // Structs.
  //

  struct ProcessHandle_t;

  //
  // Typedef.
  //

  typedef int (*ProcessWatchCbProto)(int pid, void *ctx);

  //
  // Fuctions.
  //

  // --------------------------------------------------------------------------
  //
  //                             Create process
  //
  // --------------------------------------------------------------------------

  ProcessHandle_t *ProcessCreate(const char *const argv[],
                                     int fdin = PROCESS_FD_INHERIT,
                                         int fdout = PROCESS_FD_INHERIT,
                                             int fderr = PROCESS_FD_INHERIT,
                                                 int fdType = PROCESS_FD_TYPE_CRT,
                                                     int options = 0);

  // --------------------------------------------------------------------------
  //
  //                       ProcessHandle_t based API
  //
  // --------------------------------------------------------------------------

  int ProcessIsRunning(ProcessHandle_t *proc);

  int ProcessWait(ProcessHandle_t *proc, int timeoutMs = -1, int *resultCode = NULL);

  int ProcessWatch(ProcessHandle_t *proc,
                       ProcessWatchCbProto callback,
                           int timeout = -1,
                               void *ctx = NULL);

  int ProcessCancelWatch(ProcessHandle_t *proc);

  int ProcessClose(ProcessHandle_t *proc);

  int ProcessKill(ProcessHandle_t *proc);

  // --------------------------------------------------------------------------
  //
  //                             PID based API
  //
  // --------------------------------------------------------------------------

  int ProcessIsRunningByPid(int pid);

  int ProcessWaitByPid(int pid, int timeoutMs = -1, int *resultCode = NULL);

  int ProcessWatchByPid(int pid, ProcessWatchCbProto callback,
                            int timeoutMs = -1, void *ctx = NULL);


  int ProcessCancelWatchByPid(int pid);

  int ProcessKillByPid(int pid);

  int ProcessGetPidByHandle(ProcessHandle_t *proc);

  // --------------------------------------------------------------------------
  //
  //                            Sleep functions
  //
  // --------------------------------------------------------------------------

  void ProcessSleepUs(int us);
  void ProcessSleepMs(int ms);
  void ProcessSleepSec(double seconds);

  // --------------------------------------------------------------------------
  //
  //                         General util functions
  //
  // --------------------------------------------------------------------------

  int ProcessGetCurrentPid();

  int ProcessGetParentPid();

  int ProcessGetModulePath(char *path, int pathSize);

  int ProcessExpandRelativePath(char *normPath, int pathSize, const char *relative);

  int ProcessDaemonize();

  int ProcessGetBinaryPath(char *binaryPath, int binaryPathSize, int pid);

} /* namespace Tegenaria */

#endif /* Tegenaria_Core_Process_H */
