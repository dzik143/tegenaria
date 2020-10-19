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
// Purpose: Callback based TCP server working on Linux and Windows.
// It used internally:
//  - epoll based server on linux
//  - IO Completion Port based server on Windows.
//

#pragma qcbuild_set_file_title("Callback TCP server");

#include "NetHpServer.h"
#include "NetEpollServer.h"
#include "NetIOCPServer.h"

namespace Tegenaria
{
  //
  // Create TCP server based on IO Completion Ports (WIndows)
  // or epoll (Linux).
  //
  // port         - listening port (IN).
  // openHandler  - handler called when new connection arrived (IN/OPT).
  // closeHandler - handler called when existing connection closed (IN/OPT).
  //
  // dataHandler  - handler called when something to read on one of existing
  //                connection (IN).
  //
  // TIP #1: Use NetHpWrite() to write data inside data handler. Don't
  //         use write() or send() directly.
  //
  // RETURNS: 0 if terminated correctly by NetIocpServerKill(),
  //         -1 if error.
  //

  int NetHpServerLoop(int port, NetHpOpenProto openHandler,
                          NetHpCloseProto closeHandler,
                              NetHpDataProto dataHandler)
  {
    //
    // Windows.
    // Create IO Completion Port server.
    //

    #ifdef WIN32
    {
      NetIocpOpenProto iocpOpen   = (NetIocpOpenProto) openHandler;
      NetIocpCloseProto iocpClose = (NetIocpCloseProto) closeHandler;
      NetIocpDataProto iocpData   = (NetIocpDataProto) dataHandler;

      return NetIocpServerLoop(port, iocpOpen, iocpClose, iocpData);
    }

    #else

    //
    // Linux.
    // Create epoll server.
    //

    {
      NetEpollOpenProto epollOpen   = (NetEpollOpenProto) openHandler;
      NetEpollCloseProto epollClose = (NetEpollCloseProto) closeHandler;
      NetEpollDataProto epollData   = (NetEpollDataProto) dataHandler;

      return NetEpollServerLoop(port, epollOpen, epollClose, epollData);
    }

    #endif
  }

  //
  // Write <len> bytes to FD received inside NetHpData handler called
  // from NetHpServerLoop().
  //
  // TIP #1: Caller should use this function to write data
  //         to client inside data handler specified to
  //         NetIocpServerLoop(). Don't use write() or send() directly.
  //
  // ctx - context received in data handler parameters (IN).
  // fd  - related FD where to write data (IN).
  // buf - buffer with data to write (IN).
  // len - how many bytes to write (IN).
  //
  // RETURNS: Number of bytes written or
  //          -1 if error.
  //

  int NetHpWrite(NetHpContext *ctx, int fd, void *buf, int len)
  {
    //
    // Windows.
    // IO Completion Port server.
    //

    #ifdef WIN32
    {
      NetIocpContext *iocpCtx = (NetIocpContext *) ctx;

      return NetIocpWrite(iocpCtx, fd, buf, len);
    }

    #else

    //
    // Linux.
    // Epoll server.
    //

    {
      NetEpollContext *epollCtx = (NetEpollContext *) ctx;

      return NetEpollWrite(epollCtx, fd, buf, len);
    }

    #endif
  }
} /* namespace Tegenaria */
