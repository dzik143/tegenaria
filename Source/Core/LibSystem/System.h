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

#ifndef Tegenaria_Core_System_H
#define Tegenaria_Core_System_H

//
// Includes.
//

#ifdef WIN32
# ifndef WINVER
#   define WINVER 0x501
# endif

# include <windows.h>
# include <psapi.h>
#else
# include <sys/resource.h>
# include <sys/ioctl.h>
# include <unistd.h>
# include <sys/utsname.h>
#endif

#include <Tegenaria/Debug.h>
#include <Tegenaria/Str.h>
#include <cstring>
#include <stdint.h>
#include <sys/time.h>
#include <string>
#include <map>

namespace Tegenaria
{
  using std::string;
  using std::map;


  //
  // Defines.
  //

  #define SYSTEM_MMX_AVAILABLE  0x0800000  // check edx
  #define SYSTEM_SSE_AVAILABLE  0x1FFFFFF  // check edx
  #define SYSTEM_SSE2_AVAILABLE 0x4000000  // check edx
  #define SYSTEM_SSE3_AVAILABLE 0x0000001  // check ecx

  #define SYSTEM_CLIENTID_MAX_SIZE 128

  //
  // Exported functions.
  //

  //
  // Memory info.
  //

  int64_t SystemGetFreeMemory();
  int64_t SystemGetUsedMemory();

  //
  // CPU info.
  //


  int SystemGetCpuCores();

  void SystemCpuId(unsigned int func, unsigned int *eax,
                       unsigned int *ebx, unsigned int *ecx, unsigned int *edx);

  int SystemCpuHasSSE3();
  int SystemCpuHasSSE2();
  int SystemCpuHasMMX();

  //
  // OS info.
  //

  const char *SystemGetOsName();
  const char *SystemGetOsVersionString();

  int SystemGetMachineName(char *machineName, int machineNameSize);

  int SystemGenerateClientId(char *clientId, int clientIdSize);

  int SystemParseClientId(string &os, string &machineName,
                              string &id, const char *clientId);

  //
  // Info about current user.
  //

  int SystemGetCurrentUser(char *user, int userSize);

  int SystemCheckForAdmin();

  //
  // Manage environment variables.
  //

  int SystemGetEnvironmentBlock(map<string, string> &env);

} /* namespace Tegenaria */

#endif /* Tegenaria_Core_System_H */
