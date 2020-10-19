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
// Purpose: Callback based TCP server based on IO Completion ports on
//          Windows. It's equivalent of epoll server on Linux.
//

//
// Includes.
//
#pragma qcbuild_set_file_title("IOCP based server (Windows only)");
#pragma qcbuild_set_private(1)

#include "NetIOCPServer.h"
#include "NetInternal.h"
#include "Utils.h"
#include "Net.h"

namespace Tegenaria
{
  //
  // Main worker loop to handle IOCP events.
  // We create two workers per CPU core in NetIocpServerLoop().
  //
  // One worker do in loop:
  // - pop next IOCP ready event
  // - pass data to caller callback
  // - post another IO request to go on.
  //

  #ifdef WIN32

  DWORD NetIocpWorkerLoop(LPVOID param)
  {
    DBG_ENTER("NetIocpWorkerLoop");

    HANDLE iocp = HANDLE(param);

    int ret = 0;

    LPWSAOVERLAPPED ov = NULL;

    NetIocpContext *ctx = NULL;

    PER_IO_CONTEXT *ioCtx = NULL;

    WSABUF buffRecv;
    WSABUF buffSend;

    DWORD readed  = 0;
    DWORD written = 0;
    DWORD flags   = 0;
    DWORD ioSize  = 0;

    //
    // Continually loop to service IO completion packets.
    //

    while(1)
    {
      //
      // Pop next event from IOCP queue.
      //

      ret = GetQueuedCompletionStatus(iocp, &ioSize, (PDWORD_PTR) &ctx,
                                          (LPOVERLAPPED *) &ov, INFINITE);

      if (ret == 0 && GetLastError() != ERROR_NETNAME_DELETED)
      {
        Error("GetQueuedCompletionStatus() failed: %d\n", GetLastError());
      }

      //
      // CTRL-C handler used PostQueuedCompletionStatus to post an I/O packet with
      // a NULL CompletionKey (or if we get one for any reason).  It is time to exit.
      //

      if(ctx == NULL)
      {
        return 0;
      }

      //
      // Empty queue IO popped.
      // Connection dropped.
      // Treat as close event.
      //

      if(ret == 0 || ioSize == 0)
      {
        NetIocpContextDestroy(ctx, 0);

        continue;
      }

      //
      // Read or write completed event.
      //

      ioCtx = (PER_IO_CONTEXT *) ov;

      switch(ioCtx -> operation_)
      {
        //
        // Read completed event.
        //

        case NET_IOCP_READ:
        {
          //
          // Call caller specified callback.
          //

          if (ctx -> dataHandler_)
          {
            ctx -> dataHandler_(ctx, ctx -> socket_,
                                    ioCtx -> wsabuf_.buf, ioSize);
          }

          //
          // Check is ctx marked as dead while processing caller handler.
          //

          if (ctx -> dead_)
          {
            NetIocpContextDestroy(ctx, 0);
          }

          //
          // Post aother read request.
          //

          else
          {
            if (ctx -> socket_ != INVALID_SOCKET)
            {
              flags  = 0;

              ret = WSARecv(ctx -> socket_, &(ioCtx -> wsabuf_), 1,
                                &readed, &flags, &ioCtx -> ov_, NULL);

              if(ret == SOCKET_ERROR && (ERROR_IO_PENDING != WSAGetLastError()))
              {
                Error("WSARecv() failed: %d\n", WSAGetLastError());

                NetIocpContextDestroy(ctx, 0);
              }
            }
          }

          break;
        }

        //
        // Write completed event.
        //

        case NET_IOCP_WRITE:
        {
          //
          // A write operation has completed, determine if all the data
          // intended to be sent actually was sent.
          //

          ioCtx -> operation_ = NET_IOCP_WRITE;
          ioCtx -> written_  += ioSize;

          flags = 0;

          //
          // The previous write operation didn't send all the data,
          // post another send to complete the operation.
          //

          if(ioCtx -> written_ < ioCtx -> total_)
          {
            buffSend.buf = ioCtx -> buffer_ + ioCtx -> written_;
            buffSend.len = ioCtx -> total_ - ioCtx -> written_;

            ret = WSASend(ctx -> socket_, &buffSend, 1,
                              &written, flags, &(ioCtx -> ov_), NULL);

            if(ret == SOCKET_ERROR && (ERROR_IO_PENDING != WSAGetLastError()))
            {
              Error("WSASend() failed: %d\n", WSAGetLastError());

              NetIocpContextDestroy(ctx, 0);
            }
          }

          break;
        }
      }
    }

    DBG_LEAVE("NetIocpWorkerLoop");

    return 0;
  }
  #endif /* WIN32 */

  //
  // Create TCP server based on IO Completion Ports API.
  //
  // port         - listening port (IN).
  // openHandler  - handler called when new connection arrived (IN/OPT).
  // closeHandler - handler called when existing connection closed (IN/OPT).
  //
  // dataHandler  - handler called when something to read on one of existing
  //                connection (IN).
  //
  // TIP #1: Use NetIocpWrite() to write data inside data handler.
  //
  // RETURNS: 0 if terminated correctly by NetIocpServerKill(),
  //         -1 if error.
  //

  int NetIocpServerLoop(int port, NetIocpOpenProto openHandler,
                             NetIocpCloseProto closeHandler,
                                 NetIocpDataProto dataHandler)
  {
    DBG_ENTER("NetIocpServerLoop");

    int exitCode = -1;

    //
    // Windows.
    //

    #ifdef WIN32
    {
      HANDLE threads[NET_IOCP_MAX_THREADS] = {0};

      HANDLE iocp = NULL;

      NetIocpContext *ctx = NULL;

      SOCKET listenSocket = INVALID_SOCKET;
      SOCKET acceptSocket = INVALID_SOCKET;

      DWORD recvFlags = 0;
      DWORD readed    = 0;

      //
      // Init Winsock if needed.
      //

      FAIL(_NetInit());

      //
      // Init empty IO Completion Port context.
      //

      iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE,
                                             NULL, ULONG_PTR(iocp), 0);

      FAILEX(iocp == NULL, "ERROR: Cannot init IOCP context.\n");

      //
      // Create non-block listening socket.
      //

      listenSocket = NetCreateListenSocket(port, NET_IOCP_MAXCONN);

      FAIL(listenSocket == INVALID_SOCKET);

      //
      // Create 2 worker threads per every CPU core.
      //

      for (int i = 0; i < NetGetCpuNumber() * 2; i++)
      {
        threads[i] = CreateThread(NULL, 0,
                                      LPTHREAD_START_ROUTINE(NetIocpWorkerLoop),
                                          iocp, 0, NULL);

        FAILEX(threads[i] == NULL, "ERROR: Cannot create worker thread.\n");
      }

      //
      // Fall into main server loop.
      //

      while(1)
      {
        //
        // Accept new connection.
        //

        acceptSocket = WSAAccept(listenSocket, NULL, NULL, NULL, 0);

        if (acceptSocket == INVALID_SOCKET)
        {
          Error("ERROR: Cannot accept connection on socket #%d.\n"
                    "Error code is : %d.\n", listenSocket, GetLastError());

          continue;
        }

        //
        // Allocate new IOCP context for accepted connection.
        //

        ctx = NetIocpContextCreate(acceptSocket);

        if(ctx == NULL)
        {
          Error("ERROR: Cannot allocate context for new client.\n");

          continue;
        }

        //
        // Set user specified handlers.
        //

        if (openHandler)
        {
          openHandler(ctx, acceptSocket);
        }

        ctx -> openHandler_  = openHandler;
        ctx -> closeHandler_ = closeHandler;
        ctx -> dataHandler_  = dataHandler;

        //
        // Add new socket to IO Completion Port queue.
        //

        if (CreateIoCompletionPort((HANDLE) acceptSocket, iocp,
                                       DWORD_PTR(ctx), 0) == NULL)
        {
          Error("ERROR: Cannot add client socket to IOCP queue.\n");

          continue;
        }

        //
        // Post initial receive on this socket.
        //

        if (WSARecv(acceptSocket,
                        &(ctx -> inpIoCtx_.wsabuf_),
                            1, &readed, &recvFlags,
                                &(ctx -> inpIoCtx_.ov_),
                                    NULL) == SOCKET_ERROR)
        {
          if (WSAGetLastError() != ERROR_IO_PENDING)
          {
            Error("WSARecv() Failed: %d\n", WSAGetLastError());

            NetIocpContextDestroy(ctx, 0);

            continue;
          }
        }
      }

      //
      // Clean up.
      //

      exitCode = 0;

      fail:

      if (iocp)
      {
        CloseHandle(iocp);
      }
    }

    //
    // Linux, not implemented.
    //

    #else
    {
      fprintf(stderr, "NetIocpServerLoop() not implemented on this OS.\n");
    }
    #endif

    //
    // Common error handler.
    //

    if (exitCode)
    {
      Error("ERROR: Cannot set up IOCP blocking server loop.\n"
                "Error code is : %d.\n", GetLastError());
    }

    DBG_LEAVE("NetIocpServerLoop");

    return exitCode;
  }

  //
  // Allocate new IOCP context associated with given socket.
  //
  // sock - related socket, already connected to client (IN).
  //
  // WARNING! Returned struct MUST be freed by NetIocpContextFree()
  //          if not needed longer.
  //
  // RETURNS: Pointer to new allocated IOCP struct or
  //          NULL if error.
  //

  NetIocpContext *NetIocpContextCreate(SOCKET sock)
  {
    DBG_ENTER("NetIocpContextCreate");

    int exitCode = -1;

    NetIocpContext *ctx = NULL;

    //
    // Windows.
    //

    #ifdef WIN32

    {
      //
      // Allocate new context.
      //

      ctx = (NetIocpContext *) malloc(sizeof(NetIocpContext));

      FAILEX(ctx == NULL, "ERROR: Cannot allocate new IOCP context.\n");

      //
      // Associate context with given socket.
      //

      ctx -> dead_   = 0;
      ctx -> socket_ = sock;

      //
      // Allocate one input I/O context.
      // We can have only one pending reading at one time.
      //

      ctx -> inpIoCtx_.ov_.Internal       = 0;
      ctx -> inpIoCtx_.ov_.InternalHigh   = 0;
      ctx -> inpIoCtx_.ov_.Offset         = 0;
      ctx -> inpIoCtx_.ov_.OffsetHigh     = 0;
      ctx -> inpIoCtx_.ov_.hEvent         = NULL;
      ctx -> inpIoCtx_.operation_         = NET_IOCP_READ;
      ctx -> inpIoCtx_.total_             = 0;
      ctx -> inpIoCtx_.written_           = 0;
      ctx -> inpIoCtx_.bufferSize_        = NET_IOCP_INIT_INPUT_BUFFER;

      //
      // Allocate input buffer.
      //

      ctx -> inpIoCtx_.buffer_ = (char *) malloc(NET_IOCP_INIT_INPUT_BUFFER);

      FAILEX(ctx -> inpIoCtx_.buffer_ == NULL, "ERROR: Out of memory.\n");

      ctx -> inpIoCtx_.wsabuf_.buf = ctx -> inpIoCtx_.buffer_;
      ctx -> inpIoCtx_.wsabuf_.len = ctx -> inpIoCtx_.bufferSize_;

      //
      // Allocate one output I/O context.
      //

      ctx -> outIoCtx_.ov_.Internal       = 0;
      ctx -> outIoCtx_.ov_.InternalHigh   = 0;
      ctx -> outIoCtx_.ov_.Offset         = 0;
      ctx -> outIoCtx_.ov_.OffsetHigh     = 0;
      ctx -> outIoCtx_.ov_.hEvent         = NULL;
      ctx -> outIoCtx_.operation_         = NET_IOCP_WRITE;
      ctx -> outIoCtx_.total_             = 0;
      ctx -> outIoCtx_.written_           = 0;
      ctx -> outIoCtx_.bufferSize_        = NET_IOCP_INIT_OUTPUT_BUFFER;

      //
      // Allocate output buffer.
      //

      ctx -> outIoCtx_.buffer_ = (char *) malloc(NET_IOCP_INIT_OUTPUT_BUFFER);

      FAILEX(ctx -> outIoCtx_.buffer_ == NULL, "ERROR: Out of memory.\n");

      ctx -> outIoCtx_.wsabuf_.buf = ctx -> outIoCtx_.buffer_;
      ctx -> outIoCtx_.wsabuf_.len = ctx -> outIoCtx_.bufferSize_;

      //
      // Clean up.
      //

      exitCode = 0;

      fail:

      if (exitCode)
      {
        Error("ERROR: Cannot create new IOCP context for socket #%d.\n", sock);

        if (ctx)
        {
          if (ctx -> inpIoCtx_.buffer_)
          {
            free(ctx -> inpIoCtx_.buffer_);
          }

          if (ctx -> outIoCtx_.buffer_)
          {
            free(ctx -> outIoCtx_.buffer_);
          }

          free(ctx);
        }
      }
    }

    //
    // Linux, MacOS.
    // Not implemented.
    //

    #else
    {
      Error("ERROR: NetIocpContextCreate() not implemented on this OS.\n");
    }
    #endif

    DBG_LEAVE("NetIocpContextCreate");

    return ctx;
  }

  //
  // Free IOCP context created by NetIocpContextCreate before.
  //
  // ctx      - pointer to IOCP context returned by NetIocpContextCreate() (IN).
  // graceful - 1 if we want to force graceful socket shutdown (IN).
  //
  // RETURN: none.
  //

  void NetIocpContextDestroy(NetIocpContext *ctx, int graceful)
  {
    DBG_ENTER("NetIocpContextDestroy");

    //
    // Windows.
    //

    #ifdef WIN32
    {
      NetIocpContext *pBack    = NULL;
      NetIocpContext *pForward = NULL;

      PER_IO_CONTEXT * pNextIO = NULL;
      PER_IO_CONTEXT * pTempIO = NULL;

      FAILEX(ctx == NULL, "ERROR: ctx cannot be NULL in NetIocpContextDestroy().\n");

      if (ctx -> closeHandler_)
      {
        ctx -> closeHandler_(ctx, ctx -> socket_);
      }

      //
      // force the subsequent closesocket to be abortative.
      //

      if(graceful == 0)
      {
        LINGER linger;

        linger.l_onoff  = 1;
        linger.l_linger = 0;

        setsockopt(ctx -> socket_, SOL_SOCKET, SO_LINGER, (char *) &linger, sizeof(linger));
      }

      //
      // Close related socket.
      //

      closesocket(ctx -> socket_);

      ctx -> socket_ = INVALID_SOCKET;

      //
      // Wait until writing pending IO finished.
      //

      while(HasOverlappedIoCompleted(LPOVERLAPPED(&(ctx -> outIoCtx_.ov_))) == FALSE)
      {
        Sleep(0);
      }

      //
      // Wait until reading pending IO finished.
      //

      while(HasOverlappedIoCompleted(LPOVERLAPPED(&(ctx -> inpIoCtx_.ov_))) == FALSE)
      {
        Sleep(0);
      }

      //
      // Free asociated input and output buffers.
      //

      if (ctx -> inpIoCtx_.buffer_)
      {
        free(ctx -> inpIoCtx_.buffer_);
      }

      if (ctx -> outIoCtx_.buffer_)
      {
        free(ctx -> outIoCtx_.buffer_);
      }

      //
      // Free context.
      //

      free(ctx);

      fail:;
    }

    //
    // Linux, MacOS.
    // Not implemented.
    //

    #else
    {
      Error("ERROR: NetIocpContextDestroy() not implemented on this OS.\n");
    }
    #endif

    DBG_LEAVE("NetIocpContextDestroy");
  }

  //
  // Write <len> bytes to FD received inside IOCP data handler.
  //
  // TIP #1: Caller should use this function to write data
  //         to client inside data handler specified to NetIocpServerLoop().
  //
  // ctx - IOCP context created in NetIocpServerLoop (IN).
  // fd  - related FD where to write data (IN).
  // buf - buffer with data to write (IN).
  // len - how many bytes to write (IN).
  //
  // RETURNS: Number of bytes written or
  //          -1 if error.
  //

  int NetIocpWrite(NetIocpContext *ctx, int fd, void *buf, int len)
  {
    DBG_ENTER("NetIocpWrite");

    int ret = -1;

    //
    // Windows.
    //

    #ifdef WIN32
    {
      DWORD written = 0;

      //
      // Check args.
      //

      FAILEX(ctx -> socket_ != fd,
                 "ERROR: FD#%d doesn't match IOCP context PTR#%p.\n", fd, ctx);

      FAILEX(ctx -> socket_ == INVALID_SOCKET,
                 "ERROR: Writing to closed IOCP FD#%d/%p.\n", fd, ctx);

      FAILEX(len > NET_IOCP_MAX_OUTPUT_BUFFER,
                 "ERROR: Packet %d bytes is too big for FD#%d.\n", len, fd);

      //
      // Wait until pending write finished if another thread was
      // written something to the same socket.
      //

      while(HasOverlappedIoCompleted(LPOVERLAPPED(&(ctx -> outIoCtx_.ov_))) == FALSE)
      {
        Sleep(0);
      }

      //
      // Realloc output buffer if needed.
      //

      if (len > ctx -> outIoCtx_.bufferSize_)
      {
        ctx -> outIoCtx_.bufferSize_ = len;

        free(ctx -> outIoCtx_.buffer_);

        ctx -> outIoCtx_.buffer_ = (char *) malloc(len);

        ctx -> outIoCtx_.wsabuf_.buf = ctx -> outIoCtx_.buffer_;
      }

      //
      // Put data into output buffer.
      //

      ctx -> outIoCtx_.total_      = len;
      ctx -> outIoCtx_.written_    = 0;
      ctx -> outIoCtx_.wsabuf_.len = len;

      memcpy(ctx -> outIoCtx_.buffer_, buf, len);

      //
      // Post write request.
      //

      ret = WSASend(ctx -> socket_, &(ctx -> outIoCtx_.wsabuf_), 1,
                        &written, 0, &(ctx -> outIoCtx_.ov_), NULL);

      if (ret == SOCKET_ERROR && (ERROR_IO_PENDING != WSAGetLastError()))
      {
        ctx -> dead_ = 1;

        ret = -1;
      }
      else
      {
        ret = len;
      }
    }

    //
    // Linux, MacOS.
    // Not implemented.
    //

    #else
    {
      Error("ERROR: NetIocpWrite() not implemented on this OS.\n");
    }
    #endif

    //
    // Error handler.
    //

    fail:

    DBG_LEAVE("NetIocpWrite");

    return ret;
  }
} /* namespace Tegenaria */
