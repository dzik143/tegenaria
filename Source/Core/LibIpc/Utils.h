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

#ifndef Tegenaria_Core_Ipc_Utils_H
#define Tegenaria_Core_Ipc_Utils_H

#ifdef WIN32
# include <windows.h>
# include <aclapi.h>
# include <io.h>
# include <fcntl.h>
#else
# include <sys/socket.h>
# include <unistd.h>
#endif

#include <Tegenaria/Debug.h>

namespace Tegenaria
{
  int TimeoutReadSelect(int fd, void *buf, int len, int timeout);

  #ifdef WIN32

  int TimeoutReadFileOv(HANDLE handle, void *buf, int len, int *readed, int timeout);

  int SetUpSecurityAttributesEverybody(SECURITY_ATTRIBUTES *sa);

  void FreeSecurityAttributes(SECURITY_ATTRIBUTES *sa);

  #endif

} /* namespace Tegenaria */

#endif /* Tegenaria_Core_Ipc_Utils_H */
