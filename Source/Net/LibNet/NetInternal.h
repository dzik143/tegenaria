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
// Purpose: Group internal structure, which should NOT
//          be visible in public. This structures can be
//          modified from version to version wthout risk.
//

#ifndef Tegenaria_Core_NetInternal_H
#define Tegenaria_Core_NetInternal_H


#ifdef WIN32

  //
  // Windows.
  //

  #include <WinSock2.h>
  #include <stdint.h>
  #include <io.h>

  //
  // Hide differences beetwen WinSock and BSD defines.
  //

  #define SHUT_RDWR SD_BOTH
  #define SHUT_RD   SD_RECEIVE
  #define SHUT_WR   SD_SEND

  typedef int socklen_t;

#else

  //
  // Linux, MacOS includes.
  //

  typedef int SOCKET;

  //
  // Hide differences beetwen WinSock and BSD defines.
  //

  #define SD_BOTH        SHUT_RDWR
  #define SD_RECEIVE     SHUT_RD
  #define SD_SEND        SHUT_WR
  #define closesocket(X) close(X)

  #include <unistd.h>
  #include <sys/types.h>
  #include <sys/socket.h>
  #include <netinet/in.h>
  #include <arpa/inet.h>

#endif

namespace Tegenaria
{
  //
  // Structs.
  //

  struct NetRequestData
  {
    int fd_[2];

    int *serverCode_;

    char *serverMsg_;

    int serverMsgSize_;

    void *data_;

    int dataSize_;
  };

  //
  // Functions.
  //

  int _NetRequestWorker(NetRequestData *req);

  int _NetInit();

} /* namespace Tegenaria */

#endif /* Tegenaria_Core_NetInternal_H */
