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

#ifndef Tegenaria_Core_NetEpollServer_H
#define Tegenaria_Core_NetEpollServer_H

#ifndef WIN32
# include <stdio.h>
# include <sys/epoll.h>
# include <errno.h>
# include <unistd.h>
# include <cstdlib>
# include <cstring>
# include <netdb.h>
# include <sys/types.h>
# include <sys/socket.h>
# include <netinet/in.h>
# include <arpa/inet.h>
# include <fcntl.h>
#endif

#include <map>
#include <Tegenaria/Debug.h>
#include <Tegenaria/Thread.h>

namespace Tegenaria
{
  using std::map;

  //
  // Defines.
  //

  #define NET_EPOLL_SUCCESS      1
  #define NET_EPOLL_EOF          0
  #define NET_EPOLL_ERROR       -1
  #define NET_EPOLL_WOULD_BLOCK -2

  #define NET_EPOLL_MAXCONNS     255/*200000*/
  #define NET_EPOLL_MAXEVENTS    256/*200001*/
  #define NET_EPOLL_READBUFF     8096
  #define NET_EPOLL_MAXTHREADS   16

  #define NET_EPOLL_TCP_SEND_BUFFER_CORRECT 1

  //
  // Forward definitions.
  //

  struct NetEpollContext;

  //
  // Typedef.
  //

  typedef void (*NetEpollCloseProto)(NetEpollContext *ctx, int fd);
  typedef void (*NetEpollOpenProto)(NetEpollContext *ctx, int fd);
  typedef void (*NetEpollDataProto)(NetEpollContext *ctx, int fd, void *buf, int len);

  //
  // Structs.
  //

  struct NetEpollContext
  {
    void *custom_;

    int lastError_;
    int epollFd_;
    int fd_;

    //
    // Delayed write if blocking write couldn't be complited
    // immediatelly.
    //

    int len_;

    void *data_;

    //
    // User handlers.
    //

    NetEpollCloseProto closeHandler_;
    NetEpollOpenProto openHandler_;
    NetEpollDataProto dataHandler_;
  };

  //
  // Internal functions.
  //

  int NetEpollListen(int port);
  int NetEpollAccept(NetEpollContext *ctx, int listensocket, int activeconns);
  int NetEpollReadEvent(NetEpollContext *ctx, int fd);
  int NetEpollWriteEvent(NetEpollContext *ctx, int currfd);

  //
  // Exported functions.
  //

  int NetEpollServerLoop(int port, NetEpollOpenProto openHandler,
                             NetEpollCloseProto closeHandler,
                                 NetEpollDataProto dataHandler);

  int NetEpollRead(NetEpollContext *ctx, int fd, void *buf, int len);
  int NetEpollWrite(NetEpollContext *ctx, int fd, void *buf, int len);

} /* namespace Tegenaria */

#endif /* Tegenaria_Core_EpollServer_H */
