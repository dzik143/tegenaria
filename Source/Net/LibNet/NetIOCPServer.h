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

#ifndef Tegenaria_Core_IOCPServer_H
#define Tegenaria_Core_IOCPServer_H

#ifdef WIN32
# include <windows.h>
# include <winsock2.h>
# include <ws2tcpip.h>
#endif

#include <cstdio>
#include <cstdlib>
#include <Tegenaria/Debug.h>

namespace Tegenaria
{
  #define NET_IOCP_INIT_INPUT_BUFFER  1024
  #define NET_IOCP_INIT_OUTPUT_BUFFER 128

  #define NET_IOCP_MAX_INPUT_BUFFER   (1024 * 8)
  #define NET_IOCP_MAX_OUTPUT_BUFFER  (1024 * 32)

  #define NET_IOCP_ACCEPT 0
  #define NET_IOCP_READ   1
  #define NET_IOCP_WRITE  2

  #define NET_IOCP_MAXCONN     200000
  #define NET_IOCP_MAX_THREADS 16

  //
  // Forward declaration.
  //

  struct NetIocpContext;

  //
  // Typedef.
  //

  typedef void (*NetIocpOpenProto)(NetIocpContext *ctx, int fd);
  typedef void (*NetIocpCloseProto)(NetIocpContext *ctx, int fd);
  typedef void (*NetIocpDataProto)(NetIocpContext *ctx, int fd, void *buf, int len);

  #ifndef WIN32
  typedef int SOCKET;
  #endif

  //
  // Structs.
  //

  #ifdef WIN32

  typedef BOOL (PASCAL FAR * LPFN_ACCEPTEX)(SOCKET, SOCKET, PVOID, DWORD, DWORD,
                                                DWORD, LPDWORD, LPOVERLAPPED);

  //
  // Data to be associated for every I/O operation on a socket.
  //

  struct PER_IO_CONTEXT
  {
    WSAOVERLAPPED ov_;

    char *buffer_;

    WSABUF wsabuf_;

    int total_;
    int written_;
    int operation_;
    int bufferSize_;

    SOCKET acceptSocket_;
  };

  //
  // Data to be associated with every socket added to the IOCP.
  //

  struct NetIocpContext
  {
    void *custom_;

    SOCKET socket_;

    LPFN_ACCEPTEX fnAcceptEx;

    //
    // linked list for all outstanding I/O on the socket.
    //

    PER_IO_CONTEXT outIoCtx_;
    PER_IO_CONTEXT inpIoCtx_;

    //
    // Caller specified handlers to change general server template
    // into real app doing real work.
    //

    NetIocpOpenProto openHandler_;
    NetIocpCloseProto closeHandler_;
    NetIocpDataProto dataHandler_;

    int dead_;
  };

  #endif /* WIN32 */

  //
  // Exported functions.
  //

  int NetIocpServerLoop(int port, NetIocpOpenProto openHandler,
                             NetIocpCloseProto closeHandler,
                                 NetIocpDataProto dataHandler);

  void NetIocpContextDestroy(NetIocpContext * lpPerSocketContext, int bGraceful);

  NetIocpContext *NetIocpContextCreate(SOCKET sock);

  int NetCreateListenSocket(int port, int maxConnections, int nonBlock = 0);

  int NetIocpWrite(NetIocpContext *ctx, int fd, void *buf, int len);

} /* namespace Tegenaria */

#endif /* Tegenaria_Core_IOCPServer_H */
