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

#pragma qcbuild_set_file_title("NetTcpConnection class")
#pragma qcbuild_set_private(1)

#include "NetTcpConnection.h"

namespace Tegenaria
{
  //
  // Constructor.
  //
  // ctx      - caller specified context (IN/OPT).
  // socket   - asociatet socket number (IN.
  // handler  - callback to handle incoming connections (IN).
  // addr     - associated remote IP address (IN).
  // thread   - handle to related worker thread (IN).
  //

  NetTcpConnection::NetTcpConnection(const void *ctx, int socket,
                                         NetHandleConnProto handler,
                                             struct sockaddr_in addr,
                                                 ThreadHandle_t *thread)

  : NetConnection(ctx, "TCP/IP4", socket, handler)
  {
    char tmp[64] = {0};

    DBG_SET_ADD("NetTcpConnection", this);

    addr_ = addr;

    snprintf(tmp, sizeof(tmp), "%s:%d",
                 inet_ntoa(addr.sin_addr), htons(addr.sin_port));

    clientInfo_ = tmp;

    thread_ = thread;

    #ifdef WIN32
    {
      ZeroMemory(&ov_, sizeof(ov_));

      ov_.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

      cancelEvent_ = CreateEvent(NULL, FALSE, FALSE, NULL);
    }
    #else
    {
      cancelPipe_[0] = -1;
      cancelPipe_[1] = -1;

      if (pipe(cancelPipe_) < 0)
      {
        Error("ERROR: Cannot create cancel pipe for"
                  " TCP/IP4 connection on '%s'.\n", clientInfo_.c_str());
      }
    }
    #endif

    DBG_MSG("Created TCP/IP4 connection on '%s'.\n", clientInfo_.c_str());
  }

  //
  // Desctructor.
  // - shutdownc onnetcion.
  // - wait until associated thread finished work.
  //

  NetTcpConnection::~NetTcpConnection()
  {
    DBG_ENTER("NetTcpConnection::~NetTcpConnection");

    shutdown(SD_SEND);

    //
    // Join connection thread if any.
    //

    if (thread_)
    {
      DBG_MSG3("NetTcpConnection::shutdown : Joining thread for connection PTR 0x%x...\n", this);

      ThreadWait(thread_);
      ThreadClose(thread_);

      thread_ = NULL;
    }

    #ifdef WIN32
    {
      CloseHandle(ov_.hEvent);
      CloseHandle(cancelEvent_);
    }
    #else
    {
      if (cancelPipe_[0] != -1)
      {
        close(cancelPipe_[0]);
      }

      if (cancelPipe_[1] != -1)
      {
        close(cancelPipe_[1]);
      }
    }
    #endif

    DBG_SET_DEL("NetTcpConnection", this);

    DBG_LEAVE("NetTcpConnection::~NetTcpConnection");
  }

  //
  // Get back IP address set in constructor time before.
  //

  const struct sockaddr_in NetTcpConnection::getAddr()
  {
    return addr_;
  }

  //
  // Send <count> bytes stored in <buf>.
  //
  // buf     - source buffer where data to write stored (IN).
  // count   - number of bytes to send (IN).
  // timeout - timeout in miliseconds (IN).
  //
  // RETURNS: Number of bytes written or
  //          -1 if error.
  //

  int NetTcpConnection::write(const void *buf, int count, int timeout)
  {
    int written      = 0;
    int goOn         = 1;
    int totalWritten = 0;

    DEBUG3("NetTcpConnection::write : Writing [%d]"
               " bytes inside PTR=[%p] CTX=[%p]...\n", count, this, ctx_);

    if (int(socket_) != -1)
    {
      DBG_IO_WRITE_BEGIN("socket", socket_, buf, count);

      while(goOn)
      {
        //
        // Try write another piece.
        //

        written = send(socket_, ((const char *) buf) + totalWritten,
                           count - totalWritten, 0);

        //
        // Piece written.
        //

        if (written > 0)
        {
          totalWritten += written;

          if (totalWritten == count)
          {
            goOn = 0;
          }
        }

        //
        // Piece not written, but would block error found.
        // Wait until socket available for write.
        //

        #ifdef WIN32
        else if (GetLastError() == WSAEWOULDBLOCK)
        #else
        else if (errno == EWOULDBLOCK)
        #endif
        {
          fd_set fdset;

          struct timeval tv;

          int ret = -1;

          tv.tv_sec  = timeout / 1000;
          tv.tv_usec = timeout % 1000 * 1000;

          FD_ZERO(&fdset);

          FD_SET(socket_, &fdset);

          if (timeout > 0)
          {
            ret = select(socket_ + 1, NULL, &fdset, NULL, &tv);
          }
          else
          {
            ret = select(socket_ + 1, NULL, &fdset, NULL, NULL);
          }

          if (ret <= 0)
          {
            Error("ERROR: Timeout while writing to TCP connection PTR#%p.\n", this);

            totalWritten = -1;

            goOn = 0;
          }
        }

        //
        // Unexpected error. Fail with -1.
        //

        else
        {
          totalWritten = -1;

          goOn = 0;
        }
      }

      DBG_IO_WRITE_END("socket", socket_, buf, written);
    }

    return totalWritten;
  }

  //
  // Read up <count> bytes and save it to buf[].
  //
  // buf     - destination buffer, where to put readed data (OUT).
  // count   - number of bytes to read (IN).
  // timeout - timeout in miliseconds, defaulted to infinite if skipped (IN/OPT).
  //
  // RETURNS: Number of bytes readed or
  //          -1 if error.
  //

  int NetTcpConnection::read(void *buf, int count, int timeout)
  {
    int readed = -1;

    DBG_MSG5("NetTcpConnection::read : Reading [%d] bytes"
                " inside PTR=[%p] CTX=[%p]...\n", count, this, ctx_);

    if (int(socket_) != -1)
    {
      DBG_IO_READ_BEGIN("socket", socket_, buf, count);
      {
        #ifdef WIN32
        {
          DWORD to = timeout;

          HANDLE events[2] =
          {
            ov_.hEvent,
            cancelEvent_
          };

          ReadFile(HANDLE(socket_), buf, count, NULL, &ov_);

          if (timeout == -1)
          {
            to = INFINITE;
          }

          switch(WaitForMultipleObjects(2, events, FALSE, to))
          {
            case WAIT_OBJECT_0 + 0:
            {
              GetOverlappedResult(HANDLE(socket_), &ov_, (PDWORD) &readed, FALSE);

              break;
            }

            case WAIT_OBJECT_0 + 1:
            {
              DBG_MSG("NetTcpConnection : Read canceled on SOCKET#%d.\n", socket_);

              readed = 0;

              break;
            }
          }
        }
        #else
        {
          struct timeval tv;

          tv.tv_sec  = timeout / 1000;
          tv.tv_usec = timeout * 1000;

          fd_set rfd;

          int fdmax = std::max(socket_, cancelPipe_[0]);

          FD_ZERO(&rfd);

          FD_SET(socket_, &rfd);
          FD_SET(cancelPipe_[0], &rfd);

          if (timeout == -1)
          {
            select(fdmax + 1, &rfd, NULL, NULL, NULL);
          }
          else
          {
            select(fdmax + 1, &rfd, NULL, NULL, &tv);
          }

          if (FD_ISSET(cancelPipe_[0], &rfd))
          {
            DBG_INFO("NetTcpConnection : Read canceled on socket #%d.\n", socket_);

            readed = 0;
          }
          else if (FD_ISSET(socket_, &rfd))
          {
            readed = recv(socket_, (char *) buf, count, 0);
          }
        }
        #endif
      }
      DBG_IO_READ_END("socket", socket_, buf, readed);
    }

    return readed;
  }

  //
  // Shutdown connection.
  //
  // how - SD_RECV to close reading, SD_SEND to close sending, SD_BOTH to close
  //       for reading and writing (IN).
  //
  // RETURNS: 0 if OK.
  //

  int NetTcpConnection::shutdown(int how)
  {
    DBG_ENTER("NetTcpConnection::shutdown");

    int ret = -1;

    int state = state_;

    if (state != NET_STATE_DEAD)
    {
      setState(NET_STATE_DEAD);

      cancel();

      if (int(socket_) != -1)
      {
        DBG_MSG3("NetTcpConnection : shutting down SOCKET #%d with method [%d].\n", socket_, how);

        DBG_IO_CLOSE_BEGIN("socket", socket_);
        {
          ret = ::shutdown(socket_, how);

          if (how == SD_SEND)
          {
            fd_set rfd;

            struct timeval tv;

            FD_ZERO(&rfd);

            FD_SET(socket_, &rfd);

            tv.tv_sec  = 0;
            tv.tv_usec = 100000;

            if (select(socket_ + 1, &rfd, NULL, NULL, &tv) > 0
                    && FD_ISSET(socket_, &rfd))
            {
              char buf[64];

              recv(socket_, buf, sizeof(buf), 0);
            }
          }

          closesocket(socket_);
        }
        DBG_IO_CLOSE_END("socket", socket_);

        DBG_SET_DEL("socket", socket_);

        socket_ = -1;
      }
    }

    DBG_LEAVE("NetTcpConnection::shutdown");

    return ret;
  }

  //
  // Disable/enable nagle algorithm.
  //
  // value - 1 to disable algo, 0 to enable back (IN).
  //

  void NetTcpConnection::setNoDelay(int value)
  {
    DEBUG1("NetTcpConnection::setNoDelay : set nodelay for"
               " SOCKET #%d connection PTR [%p] to value [%d]",
                   socket_, this, value);

    setsockopt(socket_, IPPROTO_TCP, TCP_NODELAY, (char *) &value, sizeof(value));
  }

  //
  // Enable SO_KEEPALIVE flag, it keeps connections active by
  // enabling the periodic transmission of messages.
  //
  // interval - -1 to disable SO_KEEPALIVE, above 0 will set time
  //            in seconds between individual keepalive probes.
  //

  void NetTcpConnection::setKeepAlive(int interval)
  {
    DEBUG1("NetTcpConnection::setKeepAlive : set KeepAlive for "
               "SOCKET #%d connection PTR [%p] to value [%d]",
                   socket_, this, interval);

    int value;

    //
    // Disable keepalive.
    //

    if (interval == -1)
    {
      value = 0;

      setsockopt(socket_, SOL_SOCKET, SO_KEEPALIVE, (char *)&value, sizeof(value));
    }

    //
    // Enable keepalive.
    //

    else if (interval > 0)
    {
      //
      // Windows.
      //

      #ifdef WIN32
      {
        struct tcp_keepalive alive;

        DWORD dwBytesRet = 0;

        alive.onoff = TRUE;
        alive.keepalivetime     = interval * 1000;
        alive.keepaliveinterval = interval * 1000;

        WSAIoctl(socket_, SIO_KEEPALIVE_VALS, &alive, sizeof(alive), NULL, 0, &dwBytesRet, NULL, NULL);
      }

      //
      // Linux, MacOS.
      //

      #else
      {
        value = 1;
        setsockopt(socket_, SOL_SOCKET, SO_KEEPALIVE, (char *)&value, sizeof(value));

        value = 2;
        setsockopt (socket_, SOL_TCP, TCP_KEEPCNT, (char *)&value, sizeof(value));

        value = interval;
        setsockopt (socket_, SOL_TCP, TCP_KEEPIDLE, (char *)&value, sizeof(value));

        value = interval;
        setsockopt (socket_, SOL_TCP, TCP_KEEPINTVL, (char *)&value, sizeof(value));
      }
      #endif
    }
  }

  //
  // Cancel pending I/O operations asociated with connection if any.
  //

  void NetTcpConnection::cancel()
  {
    DBG_IO_CANCEL("socket", socket_);

    DEBUG3("Sending cancel event for SOCKET#%d.\n", socket_);

    #ifdef WIN32
    {
      SetEvent(cancelEvent_);
    }
    #else
    {
      if (::write(cancelPipe_[1], "x", 1) <= 0)
      {
        Error("NetTcpConnection::cancel: ERROR: Cannot cancel pending I/O.\n");
      }
    }
    #endif
  }

  //
  // Disable inherit to child process (exec).
  //
  // RETURNS: 0 if OK.
  //

  int NetTcpConnection::disableInherit()
  {
    int exitCode = -1;

    //
    // Windows.
    //

    #ifdef WIN32
    {
      FAIL(SetHandleInformation(HANDLE(socket_),
                                    HANDLE_FLAG_INHERIT, 0) == FALSE);
    }

    //
    // Linux.
    //

    #else
    {
      FAIL(fcntl(socket_, F_SETFD, FD_CLOEXEC) != 0);
    }
    #endif

    DEBUG1("Disabled inherit for socket #%d.\n", socket_);

    //
    // Error handler.
    //

    exitCode = 0;

    fail:

    if (exitCode)
    {
      Error("ERROR: Cannot disable inherit for socket #%d.\n", socket_);
    }

    return exitCode;
  }
} /* namespace Tegenaria */
