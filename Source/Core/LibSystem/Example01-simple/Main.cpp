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

#include <Tegenaria/System.h>
#include <Tegenaria/Debug.h>
#include <cstdio>

using namespace Tegenaria;

int main()
{
  int64_t bytesFree = SystemGetFreeMemory();
  int64_t bytesUsed = SystemGetUsedMemory();

  char machineName[128] = {0};
  char clientId[128]    = {0};
  char user[128]        = {0};

  SystemGetMachineName(machineName, sizeof(machineName));

  SystemGenerateClientId(clientId, sizeof(clientId));

  SystemGetCurrentUser(user, sizeof(user));

  printf("OS             : %s.\n", SystemGetOsName());
  printf("OS version     : %s.\n", SystemGetOsVersionString());
  printf("Machine name   : %s.\n", machineName);
  printf("Client ID      : %s.\n", clientId);
  printf("Memory free    : %lf MB.\n", bytesFree / 1024.0 / 1024.0);
  printf("Memory used    : %lf MB.\n", bytesUsed / 1024.0 / 1024.0);
  printf("CPU cores      : %d.\n", SystemGetCpuCores());
  printf("MMX available  : %s.\n", SystemCpuHasMMX() ? "yes" : "no");
  printf("SSE2 available : %s.\n", SystemCpuHasSSE2() ? "yes" : "no");
  printf("SSE3 available : %s.\n", SystemCpuHasSSE3() ? "yes" : "no");
  printf("Current user   : %s.\n", user);
  printf("Admin status   : %d.\n", SystemCheckForAdmin());
}
