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

#ifndef Tegenaria_Core_Debug_Config_H
#define Tegenaria_Core_Debug_Config_H

#include <cstdio>
#include <cstdlib>
#include <cstdarg>

#ifdef WIN32
# include <io.h>
# include <process.h>
# include <windows.h>
#else
# include <semaphore.h>
# include <sys/time.h>
# include <sys/syscall.h>
# include <stdarg.h>
# include <signal.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <cstring>
#include <unistd.h>
#include <ctime>
#include <cstdio>
#include <map>
#include <set>
#include <string>
#include <stdint.h>

namespace Tegenaria
{
  //
  // Defines.
  //

  #define DEBUG_CONFIG_PATTERN "%s/.debugConfig.%d"

  //
  // Structs.
  //

  struct DebugConfig_t
  {
    int init_;       // Already inited?
    int logLevel_;   // Log level, see DBG_LEVEL_XXX in Debug.h.
    int logFd_;      // FD to opened main log (defaulted to 2 = stderr).
    int flags_;      // Combination of DBG_XXX flags from Debug.h.
    int stateFd_;    // FD to opened state.<pid>.
    int stateHisFd_; // FD to opened state-history.<pid>.
    int ioFd_;       // FD to write IO related messages.
    int pid_;        // pid of current process.
  };

  //
  // Exported functions.
  //

  int DbgLoadConfig(DebugConfig_t *debugConfig);
  int DbgSaveConfig(DebugConfig_t *debugConfig);

} /* namespace Tegenaria */

#endif /* Tegenaria_Core_Debug_Config_H */
