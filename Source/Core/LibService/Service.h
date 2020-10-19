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

#ifndef Tegenaria_Core_Service_H
#define Tegenaria_Core_Service_H

#ifdef WIN32

//
// Includes.
//

#include <windows.h>
#include <Tegenaria/Debug.h>

namespace Tegenaria
{
  //
  // Defines.
  //

  //
  // Timeout for waiting until service init itself
  // after ServiceAdd.
  //
  //

  #define SERVICE_RUN_TIMEOUT 10000

  //
  // Structures.
  //

  struct StringIntPair
  {
    const char *string_;

    const int code_;
  };

  //
  // Exported functions.
  //

  int ServiceGetTypeCode(const char *serviceType);
  int ServiceGetStartTypeCode(const char *startType);

  int ServiceOpen(SC_HANDLE *serviceManager, SC_HANDLE *service,
                      const char *name, DWORD rights, int quiet = 0);

  int ServiceAdd(const char *name, const char *displayName, int type,
                     int startType, const char *path, bool failIfExists = true,
                         const char *obj = NULL, const char *pass = NULL,
                             int startAfter = 1, int quiet = 0);

  int ServiceChange(const char *name, const char *displayName, int type,
                        int startType, const char *path);

  int ServiceDelete(const char *name, int quiet = 0);
  int ServiceStart(const char *name, int argc = 0, const char **argv = NULL);
  int ServiceStop(const char *name, int timeout = SERVICE_RUN_TIMEOUT);

  int ServiceGetStatus(SERVICE_STATUS *status,
                           const char *name, int quiet = 0);

  int ServiceGetConfig(QUERY_SERVICE_CONFIG **config,
                           const char *name, int quiet = 0);

  int ServiceGetPid(const char *name);

  int ServiceExists(const char *name);

  int ServiceWaitUntilRunning(SC_HANDLE service, DWORD initState,
                                  DWORD targetState,
                                      int timeout = SERVICE_RUN_TIMEOUT,
                                          int quiet = 0);

  int ServiceKill(SC_HANDLE service, int quiet = 0);

} /* namespace Tegenaria */

#endif /* WIN32 */

#endif /* Tegenaria_Core_Service_H */
