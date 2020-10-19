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
// Purpose: TCP/IP4 client.
//
// Usage:
//
//   NetConection *nc = NetConnect(ip, port)
//   nc -> write(...)
//   nc -> read(...)
//   nc -> request(...)
//   delete nc
//

#pragma qcbuild_set_file_title("Client side API");

#include "Net.h"
#include "NetInternal.h"
#include "NetTcpConnection.h"

namespace Tegenaria
{
  //
  // Open connection to server.
  //
  // host    - server's ip address or hostname eg. "google.pl" (IN).
  // port    - port, on which server is listening (IN).
  // timeout - timeout in miliseconds (IN/OPT).
  //
  // RETURNS: Pointer to new allocated NetConnection or
  //          NULL if error.
  //
  // WARNING: Caller MUST delete returned NetConnection object.
  //
  // TIP: Use read/write method fro NetConnection to communicate with
  //      remote host.
  //

  NetConnection *NetConnect(const char *host, int port, int timeout)
  {
    DBG_ENTER("NetConnect");

    int exitCode = -1;

    SOCKET sock = -1;

    struct sockaddr_in sa = {0};

    NetConnection *nc = NULL;

    int connected = 0;

    vector<string> ip;

    //
    // Check args.
    //

    FAILEX(host == NULL, "ERROR: Host cannot be NULL in NetConnect().\n");
    FAILEX(port == 0,    "ERROR: Port cannot be 0 in NetConnect().\n");

    //
    // Initialize WinSock 2.2 on windows.
    //

    FAIL(_NetInit());

    //
    // Resolve hostname.
    //

    FAIL(NetResolveIp(ip, host));

    //
    // Create socket.
    //

    sock = socket(AF_INET, SOCK_STREAM, 0);

    DBG_SET_ADD("socket", sock, "OUT");

    //
    // Set socket to non-block mode.
    //

    FAIL(NetSetNonBlockMode(sock));

    //
    // Async connect.
    //

    sa.sin_family      = AF_INET;
    sa.sin_addr.s_addr = inet_addr(ip[0].c_str());
    sa.sin_port        = htons(port);

    DBG_MSG3("NetConnect : Connecting to '%s:%d' on SOCKET #%d...\n", host, port, sock);

    if (connect(sock, (struct sockaddr *) &sa, sizeof(sa)) < 0)
    {
      //
      // Windows.
      //

      #ifdef WIN32
      {
        int lastError = WSAGetLastError();

        switch(lastError)
        {
          //
          // Already connected.
          //

          case WSAEISCONN:
          {
            connected = 1;

            break;
          }

          //
          // Work in progress.
          // Handle in select below.
          //

          case WSAEWOULDBLOCK:
          case WSAEINPROGRESS:
          {
            break;
          }

          //
          // Unexpected error.
          //

          default:
          {
            Error("ERROR: Unexpected connect() error code '%d'.\n", lastError);

            goto fail;
          }
        }
      }

      //
      // Linux, MacOS.
      //

      #else
      {
        switch(errno)
        {
          //
          // Already connected.
          //

          case EISCONN:
          {
            connected = 1;

            break;
          }

          //
          // Work in progress.
          // Handle in select below.
          //

          case EINPROGRESS:
          case EAGAIN:
          {
            break;
          }

          //
          // Unexpected error.
          //

          default:
          {
            Error("ERROR: Unexpected connect() error code '%d'.\n", errno);

            goto fail;
          }
        }
      }
      #endif

      //
      // If still not connected.
      //

      if (connected == 0)
      {
        fd_set fdset;

        struct timeval tv;

        int valopt = 0;

        socklen_t len = sizeof(valopt);

        tv.tv_sec  = timeout / 1000;
        tv.tv_usec = timeout % 1000 * 1000;

        FD_ZERO(&fdset);

        FD_SET(sock, &fdset);

        FAILEX(select(sock + 1, NULL, &fdset, NULL, &tv) <= 0,
                 "ERROR: Timeout while connecting to '%s':'%d'.\n", host, port);

        len = sizeof(int);

        getsockopt(sock, SOL_SOCKET, SO_ERROR, (char *) (&valopt), &len);

        FAILEX(valopt, "ERROR: connect() failed with error %d.\n", valopt);

        connected = 1;
      }
    }
    else
    {
      connected = 1;
    }

    FAIL(connected == 0);

    //
    // Set back to block mode.
    //

    FAIL(NetSetBlockMode(sock));

    //
    // Wrap connected socket into connection object.
    //

    nc = new NetTcpConnection(NULL, sock, NULL, sa);

    nc -> setState(NET_STATE_ESTABLISHED);

    DBG_SET_RENAME("NetTcpConnection", nc, "OUT");

    DEBUG1("NetConnect : Connected to '%s:%d' on SOCKET #%d...\n", host, port, sock);

    //
    // Clean up.
    //

    exitCode = 0;

    fail:

    if (exitCode)
    {
      Error("ERROR: Cannot connect to '%s:%d'.\n"
                "Error code is : %d.\n", host, port, GetLastError());

      if (int(sock) != -1)
      {
        if (nc)
        {
          nc -> release();

          nc = NULL;
        }
        else
        {
          shutdown(sock, SD_BOTH);

          closesocket(sock);

          DBG_SET_DEL("socket", sock);
        }

        sock = -1;
      }
    }

    DBG_LEAVE("NetConnect");

    return nc;
  }

  //
  // - Send single, printf like formatted request to server
  // - read answer in format 'XYZ > message'
  // - split answer to <XYZ> code and <message> parts.
  //
  // Example usage:
  //
  // NetRequest(fd, &serverCode, serverMsg, sizeof(serverMsg),
  //                "share --alias %s --path %s", alias, path);
  //
  // TIP: If only exit code is needed <answer> can be set to NULL.
  //
  // fd            - CRT FDs IN/OUT pair connected to the server (IN).
  // serverCode    - exit code returned by server (OUT).
  // serverMsg     - ASCIZ message returned by server (OUT/OPT).
  // serverMsgSize - size of answer buffer in bytes (IN).
  // fmt           - printf like parameters to format command to send (IN).
  //
  // RETURNS: 0 if request sucessfuly sent and asnwer from server received.
  //         -1 otherwise.
  //
  // WARNING!: Request could still failed on server side.
  //           To get server's side exit code use 'answerCode' parameter.
  //

  int _NetRequestWorker(NetRequestData *req)
  {
    #pragma qcbuild_set_private(1)

    DBG_ENTER("_NetRequestWorker");

    int exitCode = -1;

    char buf[1024];

    int cmdLen = 0;

    char *dst = NULL;

    int readed = 0;
    int total  = 0;

    int len = 0;

    int endReached = 0;

    va_list ap;

    //
    // Check args.
    //

    FAILEX(req -> fd_[0] < 0, "ERROR: Wrong FD #%d in NetRequestData.", req -> fd_[0]);
    FAILEX(req -> fd_[1] < 0, "ERROR: Wrong FD #%d in NetRequestData.", req -> fd_[1]);
    FAILEX(req -> serverCode_ == NULL, "ERROR: 'serverCode' cannot be NULL in NetRequestData.");
    FAILEX(req -> data_ == NULL, "ERROR: 'data' cannot be NULL in NetRequestData.");
    FAILEX(req -> dataSize_ <= 0, "ERROR: 'dataSize' cannot be NULL in NetRequestData.");

    //
    // Send command to server INCLUDING zero terminator byte.
    //

    FAILEX(write(req -> fd_[1], req -> data_, req -> dataSize_ + 1) < 0,
               "ERROR: Cannot send NetRequest to FD [%d].\n", req -> fd_[1]);

    //
    // Read answer from server in below format:
    // 'XYZ> <message>'
    //
    // Where <XYZ> is 3 decimal server side code e.g. "871".
    //

    //
    // Read 'XYZ >' prefix first.
    // where XYZ is 3 decimal exit code returned by server.
    //

    FAILEX(read(req -> fd_[0], buf, 5) != 5,
               "ERROR: Cannot read 'XYZ> ' prefix from FD [%d].", req -> fd_[0]);

    buf[4] = 0;

    *(req -> serverCode_) = atoi(buf);

    //
    // Read ASCIZ message part if needed.
    //

    if (req -> serverMsg_ && req -> serverMsgSize_ > 0)
    {
      dst = req -> serverMsg_;

      total = 0;

      //
      // FIXME: Avoid reading byte by byte.
      //

      while(read(req -> fd_[0], dst, 1) == 1)
      {
        //
        // Zero terminator reached.
        // All message sent.
        //

        if (dst[0] == 0)
        {
          endReached = 1;

          break;
        }

        //
        // No space in caller buffer.
        //

        if (total == req -> serverMsgSize_)
        {
          Error("ERROR: Caller buffer too small.\n");

          //
          // Eat remaining packet data to make transfer consistent.
          //

          char byte = 0;

          int bytesEated = 0;

          while(read(req -> fd_[0], &byte, 1) == 1 && byte != 0)
          {
            bytesEated ++;
          }

          Error("ERROR: Eated %d not catched bytes.\n", bytesEated);

          goto fail;
        }

        total ++;

        dst ++;
      }

      req -> serverMsg_[total] = 0;
    }

    exitCode = 0;

    //
    // Clean up.
    //

    fail:

    if (exitCode)
    {
      Error("Cannot send NET request to FDs [%d/%d].\n"
                "Error code is : %d.\n", req -> fd_[0], req -> fd_[1], GetLastError());
    }

    DBG_LEAVE("_NetRequestWorker");

    return exitCode;

  };

  int NetRequest(int fd[2], int *serverCode, char *serverMsg,
                     int serverMsgSize, const char *fmt, ...)
  {
    DBG_ENTER("NetRequest");

    int exitCode = -1;

    char buf[1024];

    int len = 0;

    va_list ap;

    NetRequestData req = {0};

    ThreadHandle_t *thread = NULL;

    int result = -1;

    //
    // Check args.
    //

    FAILEX(fd[0] < 0, "ERROR: Wrong FD #%d in NetRequest.", fd[0]);
    FAILEX(fd[1] < 0, "ERROR: Wrong FD #%d in NetRequest.", fd[1]);
    FAILEX(serverCode == NULL, "ERROR: 'serverCode' cannot be NULL in NetRequest.");
    FAILEX(fmt == NULL, "ERROR: 'fmt' cannot be NULL in NetRequest.");

    //
    // Format printf like message.
    //

    va_start(ap, fmt);

    len = vsnprintf(buf, sizeof(buf) - 1, fmt, ap);

    va_end(ap);

    //
    // Run request in background thread.
    //

    req.fd_[0]         = fd[0];
    req.fd_[1]         = fd[1];
    req.serverCode_    = serverCode;
    req.serverMsg_     = serverMsg;
    req.serverMsgSize_ = serverMsgSize;
    req.data_          = buf;
    req.dataSize_      = len;

    thread = ThreadCreate(_NetRequestWorker, &req);

    DBG_SET_RENAME("thread", thread, "NET Request FDs #%d/%d", fd[0], fd[1]);

    if (ThreadWait(thread, &result, 10000) == 0)
    {
      Error("Timeout reached while sending NET request to FDs [%d/%d].", fd[0], fd[1]);

      ThreadKill(thread);

      goto fail;
    }

    FAIL(result != 0);

    //
    // Clean up.
    //

    exitCode = 0;

    fail:

    if (exitCode)
    {
      Error("ERROR: Cannot send NET request to FDs [%d/%d].\n"
                "Error code is : %d.\n", fd[0], fd[1], GetLastError());

      #ifdef WIN32
      {
        if (CancelIo(HANDLE(_get_osfhandle(fd[0]))))
        {
          DBG_MSG("NetRequest : IO canceled on FD [%d].\n", fd[0]);
        }
        else
        {
          DBG_MSG("WARNING: Cannot cancel pending I/O on FD [%d].", fd[0]);
        }

        if (CancelIo(HANDLE(_get_osfhandle(fd[1]))))
        {
          DBG_MSG("NetRequest : IO canceled on FD [%d].\n", fd[1]);
        }
        else
        {
          DBG_MSG("WARNING: Cannot cancel pending I/O on FD [%d].", fd[1]);
        }
      }
      #endif
    }

    ThreadClose(thread);

    DBG_LEAVE("NetRequest");

    return exitCode;
  }
} /* namespace Tegenaria */
