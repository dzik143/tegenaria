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
// Purpose: TCP server based on Linux epoll API.
//

#pragma qcbuild_set_file_title("epoll based server (Linux only)");
#pragma qcbuild_set_private(1)

#include <Tegenaria/Debug.h>
#include "NetEpollServer.h"
#include "Utils.h"

namespace Tegenaria
{
  //
  // Allocate new epoll context and associate it with given fd.
  //
  // epollFd      - epoll fd returned from epoll_create() before (IN).
  // fd           - related fd to associate (IN).
  // openHandler  - callback to inform about new connections (IN/OPT).
  // closeHandler - callback to inform about closed connections (IN/OPT).
  // dataHandler  - callback to process incoming data (IN/OPT).
  //
  // WARNING! Returned pointer MUST be freed by NetEpollContextDestroy()
  //          if no needed longer.
  //
  // RETURNS: Pointer to new allocated epoll context or,
  //          NULL if error.
  //

  NetEpollContext *NetEpollContextCreate(int epollFd, int fd,
                                             NetEpollOpenProto openHandler,
                                                 NetEpollCloseProto closeHandler,
                                                     NetEpollDataProto dataHandler)
  {
    DBG_ENTER("NetEpollContextCreate");

    NetEpollContext *ctx = NULL;

    //
    // Allocate new epoll context.
    //

    ctx = (NetEpollContext *) calloc(1, sizeof(NetEpollContext));

    FAILEX(ctx == NULL, "ERROR: Cannot allicate epoll context.\n");

    //
    // Associate context with given epoll and fd.
    //

    ctx -> custom_       = NULL;
    ctx -> epollFd_      = epollFd;
    ctx -> lastError_    = NET_EPOLL_SUCCESS;
    ctx -> fd_           = fd;
    ctx -> openHandler_  = openHandler;
    ctx -> closeHandler_ = closeHandler;
    ctx -> dataHandler_  = dataHandler;

    //
    // Error handler.
    //

    fail:

    DBG_LEAVE("NetEpollContextCreate");

    return ctx;
  }

  //
  // Free epoll context created by NetEpollContextCreate() before.
  //
  // ctx - pointer returned by NetEpollContextCreate() before (IN).
  //

  void NetEpollContextDestroy(NetEpollContext *ctx)
  {
    DBG_ENTER("NetEpollContextDestroy");

    if (ctx)
    {
      if (ctx -> closeHandler_)
      {
        ctx -> closeHandler_(ctx, ctx -> fd_);
      }

      if (ctx -> data_)
      {
        free(ctx -> data_);
      }

      free(ctx);
    }

    DBG_LEAVE("NetEpollContextDestroy");
  }

  //
  // Slave loop to handle one worker thread.
  // Used internally only.
  //
  // data - pointer to listenCtx struct created in NetEpollServerLoop() (IN/OUT).
  //
  // RETURNS: 0 if OK.
  //

  int NetEpollServerSlaveLoop(void *data)
  {
    DBG_ENTER("NetEpollServerSlaveLoop");

    int exitCode = -1;

    //
    // Windows.
    //

    #ifdef WIN32
    {
      Error("NetEpollServerSlaveLoop not implemented on Windows.\n");

      return -1;
    }

    //
    // Linux.
    //

    #else

    int eventstriggered = 0;
    int newconns        = 0;
    int fd              = -1;

    struct epoll_event *events = NULL;

    int activeconns = 0;

    NetEpollContext *listenCtx = (NetEpollContext *) data;

    NetEpollContext *ctx = NULL;

    //
    // Allocate event table.
    //

    events = (struct epoll_event *) malloc(NET_EPOLL_MAXEVENTS * sizeof(struct epoll_event));

    FAILEX(events == NULL, "ERROR: out of memory.\n");

    //
    // Fall into main interrupt-triggerd polling queue loop.
    //

    while(1)
    {
      //
      // Wait for events.
      //

      eventstriggered = epoll_wait(listenCtx -> epollFd_, events, NET_EPOLL_MAXEVENTS, -1);

      if (eventstriggered == -1)
      {
        Fatal("FATAL: epoll wait returned -1.\n");
      }

      DEBUG2("Event triggered: %d\n", eventstriggered);

      //
      // Serve all events.
      //

      for (int i = 0; i < eventstriggered; i++)
      {
        ctx = (NetEpollContext *) events[i].data.ptr;

        fd = ctx -> fd_;

        //
        // Event on listening socket.
        // We're expecting new incoming connetions here.
        //

        if (fd == listenCtx -> fd_)
        {
          DEBUG3("NetEpollServerLoop : Accept event on FD #%d/%p.\n", fd, ctx);

          newconns = NetEpollAccept(ctx, fd, activeconns);

          if (newconns > 0)
          {
            activeconns += newconns;

            DBG_MSG("Added %d new fd to list (currently %d connections in event loop)\n", newconns, activeconns);
          }
        }

        //
        // Event on data socket.
        //

        else
        {
          //
          // Check if the socket went in err/hangup (triggered by wait by default)
          //

          if((events[i].events & EPOLLERR) || (events[i].events & EPOLLHUP))
          {
            //
            // Socket will automatically be removed from epoll list.
            //

            close(fd);

            activeconns--;

            DBG_MSG("Closed connection (currently %d connections in event loop)\n", activeconns);

            //
            // Free related epoll context.
            //

            NetEpollContextDestroy(ctx);

            continue;
          }

          //
          // Write ready event.
          //

          if(events[i].events & EPOLLOUT)
          {
            DEBUG3("NetEpollServerLoop : Write event on FD #%d/%p.\n", fd, ctx);

            if(NetEpollWriteEvent(ctx, fd))
            {
              activeconns--;

              DBG_MSG("Closed connection (currently %d connections in event loop)\n",activeconns);

              //
              // Free related epoll context.
              //

              NetEpollContextDestroy(ctx);
            }
          }

          //
          // Read ready event.
          //

          else if (events[i].events & EPOLLIN)
          {
            DEBUG3("NetEpollServerLoop : Write event on FD #%d/%p.\n", fd, ctx);

            NetEpollReadEvent(ctx, fd);

            if (ctx -> lastError_ == NET_EPOLL_EOF || ctx -> lastError_ == NET_EPOLL_ERROR)
            {
              activeconns--;

              DBG_MSG("Closed connection (currently %d connections in event loop)\n",activeconns);

              //
              // Free related epoll context.
              //

              NetEpollContextDestroy(ctx);
            }
          }
        }
      }

      DEBUG2("Looping event loop.\n", activeconns);
    }

    //
    // Error handler.
    //

    exitCode = 0;

    fail:

    if (exitCode)
    {
      Error("ERROR: Cannot create epoll server slave loop.\n");
    }

    if (events)
    {
      free(events);
    }

    DBG_LEAVE("NetEpollServerSlaveLoop");

    return 0;

    #endif
  }

  //
  // Main server loop.
  // Create non-blocking epoll based TCP server.
  //
  // port         - listening port (IN).
  // openHandler  - handler called when new connection arrived (IN/OPT).
  // closeHandler - handler called when existing connection closed (IN/OPT).
  //
  // dataHandler  - handler called when something to read on one of existing
  //                connection (IN).
  //
  // TIP #1: Use NetEpollWrite() to write data inside data handler.
  // TIP #2: Use NetEpollRead() to read data inside data handler.
  //
  // RETURNS: 0 if terminated correctly by NetEpollServerKill(),
  //         -1 if error.
  //

  int NetEpollServerLoop(int port, NetEpollOpenProto openHandler,
                             NetEpollCloseProto closeHandler,
                                 NetEpollDataProto dataHandler)
  {
    DBG_ENTER("NetEpollServerLoop");

    int exitCode = -1;

    //
    // Windows.
    //

    #ifdef WIN32
    {
      Error("NetEpollServerLoop not implemented on Windows.\n");

      return -1;
    }

    //
    // Linux.
    //

    #else

    int listensocket;
    int epollfd;

    struct epoll_event ev = {0};

    NetEpollContext listenCtx[64] = {0};

    ThreadHandle_t *workerThread[64];

    //
    // Check and change if needed the per-user limit of open files.
    //

    FAIL(NetSetFDsLimit(NET_EPOLL_MAXCONNS + 1));

    //
    // Start listening socket on given port.
    //

    listensocket = NetEpollListen(port);

    FAIL(listensocket < 0);

    //
    // Create worker threads.
    //

    for (int i = 0; i < NetGetCpuNumber(); i++)
    {
      //
      // Set up epoll queue for worker.
      //

      epollfd = epoll_create(NET_EPOLL_MAXEVENTS);

      if (epollfd == -1)
      {
        Fatal("FATAL: Could not create epoll FD.");
      }

      listenCtx[i].lastError_    = NET_EPOLL_SUCCESS;
      listenCtx[i].epollFd_      = epollfd;
      listenCtx[i].openHandler_  = openHandler;
      listenCtx[i].closeHandler_ = closeHandler;
      listenCtx[i].dataHandler_  = dataHandler;
      listenCtx[i].fd_           = listensocket;

      //
      // Add listening socket to epoll.
      //

      ev.events   = EPOLLIN;
      ev.data.ptr = &listenCtx[i];

      FAILEX(epoll_ctl(epollfd, EPOLL_CTL_ADD, listensocket, &ev) == -1,
                 "ERROR: epoll_ctl failure on listening socket #%d.\n", listensocket);

      workerThread[i] = ThreadCreate(NetEpollServerSlaveLoop, &listenCtx[i]);

      DBG_INFO("NetEpollServerLoop: Craeted worker ID#%d.\n", i);
    }

    //
    // Wait until workers finished.
    //

    for (int i = 0; i < NetGetCpuNumber(); i++)
    {
      ThreadWait(workerThread[i]);
      ThreadClose(workerThread[i]);

      workerThread[i] = NULL;

      DBG_INFO("NetEpollServerLoop: Worker #%d finished.\n", i);
    }

    //
    // Error handler.
    //

    exitCode = 0;

    fail:

    if (exitCode)
    {
      Error("ERROR: Cannot create epoll server loop.\n");
    }

    if (listensocket != -1)
    {
      close(listensocket);
    }

    DBG_LEAVE("NetEpollServerLoop");

    return 0;

    #endif
  }

  //
  // This functions creates non-block listening socket.
  //
  // port - listening port (IN).
  //
  // RETURNS: Listening socket
  //          or -1 if error.
  //

  int NetEpollListen(int port)
  {
    //
    // Windows.
    //

    #ifdef WIN32
    {
      Error("NetEpollListen() not implemented on Windows.\n");

      return -1;
    }

    //
    // Linux.
    //

    #else

    DBG_ENTER("NetEpollListen");

    int sock = -1;

    int exitCode = -1;

    int reuseAddress = 1;

    struct sockaddr_in6 addr = {0};

    //
    // Set up IP6 address to ::port.
    //

    addr.sin6_port   = htons(port);
    addr.sin6_addr   = in6addr_any;
    addr.sin6_family = AF_INET6;

    //
    // Create listening socket.
    //

    sock = socket(AF_INET6,SOCK_STREAM | SOCK_NONBLOCK, 0);

    FAILEX(sock == -1,
               "ERROR: Cannot create listening socket.\n");

    //
    // Set address reuse.
    //

    FAILEX(setsockopt(sock, SOL_SOCKET, SO_REUSEADDR,
                         &reuseAddress, sizeof(reuseAddress)) == -1,
                             "ERROR: Cannot set reuse flag on socket #%d.\n", sock);
    //
    // Bind socket to IP address.
    //

    FAILEX(bind(sock, (struct sockaddr *) &addr, sizeof(addr)) == -1,
               "ERROR: Cannot bind address to socket #%d.\n", sock);

    //
    // Listen for incoming connection.
    //

    FAILEX(listen(sock, NET_EPOLL_MAXCONNS) == -1,
               "ERROR: Cannot listen on socket #%d.\n", sock);

    DBG_INFO("Listening on port %d...\n", port);

    //
    // Clean up.
    //

    exitCode = 0;

    fail:

    if (exitCode)
    {
      Error("Cannot create listening epoll socket.\n"
            "Error code is %d.\n", errno);

      if (sock != -1)
      {
        close(sock);

        sock = -1;
      }
    }

    DBG_LEAVE("NetEpollListen");

    return sock;

    #endif
  }

  //
  // This function accepts incoming connections, and adds read-ready events for
  // them to the event queue. The function returns the number of newly opened
  // connections.
  //
  //
  // Handle epoll event on listening socket.
  // Called internall from NetEpollServerLoop() when new connection arrived.
  //
  // listenCtx    - epoll context asociated with listening socket created in
  //                NetEpollServerLoop() function (IN).
  //
  // listensocket - listening socket related with event (IN).
  //
  // activeconns  - number of already active connections on listensocket (IN).
  //
  // RETURNS: Number of new accepted connections,
  //          or -1 if error.
  //

  int NetEpollAccept(NetEpollContext *listenCtx, int listensocket, int activeconns)
  {
    //
    // Windows.
    //

    #ifdef WIN32
    {
      Error("NetEpollAccept() not implemented on Windows.\n");

      return -1;
    }

    //
    // Linux.
    //

    #else

    DBG_ENTER("NetEpollAccept");

    int addrlen      = 0;
    int newfd        = 0;
    int newconncount = 0;

    struct sockaddr_in caddr;

    struct epoll_event ev = {0};

    NetEpollContext *ctx = NULL;

    //
    // Accept connections until something to accept.
    //

    newconncount = 0;

    while(1)
    {
      addrlen = sizeof(caddr);

      newfd = accept4(listensocket, (struct sockaddr *) &caddr,
                          (socklen_t *) &addrlen, SOCK_NONBLOCK);

      //
      // Error or socket busy.
      //

      if (newfd == -1)
      {
        //
        // No more new connections ?
        //

        if ((errno == EAGAIN) || (errno == EWOULDBLOCK))
        {
          break;
        }

        //
        // We did not break, error occurred
        //

        Error("ERROR: Cannot accept new connection.\n");

        continue;
      }

      DBG_MSG("Accepted new connection on FD#%d.\n", newfd);

      //
      // Check limit of connection.
      //

      if ((activeconns + 1) >= NET_EPOLL_MAXEVENTS)
      {
        Fatal("FATAL: Too many open connections at FD#%d.\n", newfd);
      }

      //
      // Allocate epoll context for new connection.
      //

      ctx = NetEpollContextCreate(listenCtx -> epollFd_, newfd,
                                      listenCtx -> openHandler_,
                                          listenCtx -> closeHandler_,
                                              listenCtx -> dataHandler_);

      if (ctx == NULL)
      {
        Error("Cannot allocate new epoll context.\n");

        continue;
      }

      //
      // Available for input and non edge_triggered.
      //

      ev.events   = EPOLLIN;
      ev.data.ptr = ctx;

      if (epoll_ctl(ctx -> epollFd_, EPOLL_CTL_ADD, newfd, &ev) == -1)
      {
        Fatal("FATAL: epoll_ctl failure on FD#%d", newfd);
      }

      //
      // Successfully accepted and added newfd to the list.
      // Loop for new connections.
      //

      if (ctx -> openHandler_)
      {
        ctx -> openHandler_(ctx, newfd);
      }

      newconncount ++;
    }

    fail:

    DBG_LEAVE("NetEpollAccept");

    return newconncount;

    #endif
  }

  //
  // Push data to write queue for delay write, when socket will became ready
  // to write.
  // Used internally only if blocked write failed or only part of data
  // written in NetEpollWrite().
  //
  // ctx     - epoll queue context created in NetEpollServerLoop function (IN).
  // fd      - CRT FD configured to work with epoll queue (IN).
  // buf     - source buffer with data to write (IN).
  // len     - number of bytes in buf[] (IN).
  //
  // written - how many bytes already sent, only remaining part will be
  //           processed (IN).
  //

  void NetEpollDelayWrite(NetEpollContext *ctx, int fd,
                              char *buf, int len, int written)
  {
    DBG_ENTER("NetEpollDelayWrite");

    //
    // Windows.
    //

    #ifdef WIN32
    {
      Error("NetEpollDelayWrite() not implemented on Windows.\n");
    }

    //
    // Linux.
    //

    #else

    //
    // Will simply not mark the socket, discard the data,
    // and keep the socket in the loop.
    //

    if(NET_EPOLL_TCP_SEND_BUFFER_CORRECT == 0)
    {
      return;
    }

    struct epoll_event ev = {0};

    DBG_MSG("WARNING: Socket became write blocked"
                " (still %d/%d bytes to send).", written, len);

    //
    // Remove the socket from the queue
    //

    if (epoll_ctl(ctx -> epollFd_, EPOLL_CTL_DEL, fd, NULL) == -1)
    {
      Fatal("FATAL: epoll_ctl removal failure on fd %d", fd);
    }

    //
    // Check if we already have an entry (we shouldn't).
    // If we have data in send buffer append data to it.
    //

    if (ctx -> len_ > 0)
    {
      int oldLen = ctx -> len_;

      ctx -> len_  = oldLen + len - written;
      ctx -> data_ = realloc(ctx -> data_, ctx -> len_);

      memcpy((char *) ctx -> data_ + oldLen, buf + written, len - written);
    }

    //
    // Buffer empty.
    // Create new one.
    //

    else
    {
      ctx -> len_  = len - written;
      ctx -> data_ = realloc(ctx -> data_, ctx -> len_);

      memmove(ctx -> data_, buf + written, ctx -> len_);
    }

    //
    // Readd the socket to the queue.
    //

    ev.events   = EPOLLOUT;
    ev.data.ptr = ctx;

    if(epoll_ctl(ctx -> epollFd_, EPOLL_CTL_ADD, fd, &ev) == -1)
    {
      Fatal("FATAL: epoll_ctl failure on readdition of fd %d", fd);
    }

    #endif

    DBG_LEAVE("NetEpollDelayWrite");
  }

  //
  // Read <len> bytes from FD created by epoll server.
  //
  // ctx - epoll queue context (IN).
  // fd  - CRT FD configured to work with epoll queue (IN).
  // buf - destination buffer (OUT).
  // len - number of bytes to read (IN).
  //
  // RETURNS: Number of bytes readed - success (always greater than 0),
  //          NET_EPOLL_WOULD_BLOCK  - FD not ready right now,
  //          NET_EPOLL_EOF          - connection shutted down by remote side,
  //          NET_EPOLL_ERROR        - connection shutted down due to unexpected error.
  //

  int NetEpollRead(NetEpollContext *ctx, int fd, void *buf, int len)
  {
    DBG_ENTER("NetEpollRead");

    int ret = NET_EPOLL_ERROR;

    //
    // Windows.
    //

    #ifdef WIN32
    {
      Error("NetEpollRead() not implemented on Windows.\n");
    }

    //
    // Linux.
    //

    #else

    int readed = read(fd, buf, len);

    switch(readed)
    {
      //
      // Socket not ready right now or error.
      //

      case -1:
      {
        //
        // FD busy right now, go away and try once again later.
        //

        if((errno == EAGAIN) && (errno == EWOULDBLOCK))
        {
          ret = NET_EPOLL_WOULD_BLOCK;
        }

        //
        // Unexpected error.
        //

        else
        {
          close(fd);

          Error("Could not read from FD #%d due to unexpected reason"
                    " (hence closing it). Reason: %s\n", fd, strerror(errno));

          ret = NET_EPOLL_ERROR;
        }

        break;
      }

      //
      // EOF.
      //

      case 0:
      {
        close(fd);

        ret = NET_EPOLL_EOF;

        break;
      }

      //
      // Data readed.
      //

      default:
      {
        ret = readed;

        break;
      }
    }

    //
    // Save last error code for given epoll queue.
    //

    if (ret > 0)
    {
      ctx -> lastError_ = NET_EPOLL_SUCCESS;
    }
    else
    {
      ctx -> lastError_ = ret;
    }

    #endif

    DBG_LEAVE("NetEpollRead");

    return ret;
  }

  //
  // Write <len> bytes to FD created by epoll server.
  //
  // ctx - epoll queue context (IN).
  // fd  - CRT FD configured to work with epoll queue (IN).
  // buf - destination buffer (OUT).
  // len - number of bytes to read (IN).
  //
  // RETURNS: Number of bytes written or
  //          -1 if error.
  //

  int NetEpollWrite(NetEpollContext *ctx, int fd, void *buf, int len)
  {
    DBG_ENTER("NetEpollWrite");

    int ret = NET_EPOLL_ERROR;

    int written = -1;

    //
    // Windows.
    //

    #ifdef WIN32
    {
      Error("NetEpollWrite() not implemented on Windows.\n");
    }

    //
    // Linux.
    //

    #else
    {
      //
      // Check args.
      //

      FAILEX(ctx -> fd_ != fd,
                 "ERROR: FD #%d doesn't match epoll context PTR #%p.\n",
                     fd, ctx);

      //
      // Note that this is a bit tricky. It is possible that the write does not return the number of bytes
      // that we tried to send to it. (This could be the case because TCP's send buffer is full, and the receiver's
      // flow control is blocking it.) Although unlikely, we do try to not loose data here, and keep a userspace
      // buffer active to send it the next time the socket becomes writable. In addition, we then have
      // remove the "read" event, and schedule the write event.
      //

      if (ctx -> len_ == 0)
      {
        written = write(fd, buf, len);
      }

      //
      // Write error or socket not ready.
      //

      if (written == -1)
      {
        //
        // Socket not ready.
        // Delay writing to next write ready event.
        //

        if((errno == EAGAIN) || (errno == EWOULDBLOCK))
        {
          DBG_MSG("WARNING: Send-buffer full for connection %d\n", fd);

          NetEpollDelayWrite(ctx, fd, (char *) buf, len, 0);

          ret = len;

          ctx -> lastError_ = NET_EPOLL_WOULD_BLOCK;
        }

        //
        // Write error. Close socket.
        //

        else
        {
          Error("ERROR: Cannot write to FD #%d due to unexpected reason (hence closing it)."
                    " Reason: %s\n", fd, strerror(errno));

          close(fd);

          ctx -> lastError_ = NET_EPOLL_ERROR;

          ret = NET_EPOLL_ERROR;
        }
      }

      //
      // Data partially written.
      // Delay writing remaining part to next write ready event.
      //

      else if (written < len)
      {
        NetEpollDelayWrite(ctx, fd, (char *) buf, len, written);

        ctx -> lastError_ = NET_EPOLL_SUCCESS;

        ret = len;
      }

      //
      // Whole data written.
      //

      else
      {
        ret = len;

        ctx -> lastError_ = NET_EPOLL_SUCCESS;
      }
    }

    fail:

    #endif

    DBG_LEAVE("NetEpollWrite");

    return ret;
  }

  //
  // Handle epoll read ready event on given FD.
  // Called internally only from NetEpollServerLoop().
  //
  // ctx - epoll queue context created in NetEpollServerLoop (IN).
  // fd  - fd, where to read data from (IN).
  //
  // RETURNS: 1 if socket closed inside function,
  //          0 otherwise.
  //

  int NetEpollReadEvent(NetEpollContext *ctx, int fd)
  {
    DBG_ENTER("NetEpollReadEvent");

    int ret = -1;

    //
    // Windows.
    //

    #ifdef WIN32
    {
      Error("NetEpollReadEvent() not implemented on Windows.\n");
    }

    //
    // Linux.
    //

    #else

    int readed = 0;

    int socketClosed = 0;

    char buf[NET_EPOLL_READBUFF];

    //
    // Note that because we potentially have a lot of data,
    // we might need to do this in a few loops.
    //

    ctx -> lastError_ = NET_EPOLL_SUCCESS;

    while(ctx -> lastError_ == NET_EPOLL_SUCCESS)
    {
      readed = NetEpollRead(ctx, fd, buf, sizeof(buf));

      switch(readed)
      {
        //
        // Read error.
        //

        case NET_EPOLL_ERROR:
        {
          Error("ERROR: Cannot read data from FD #%d.\n", fd);

          break;
        }

        //
        // EOF readed.
        //

        case NET_EPOLL_EOF:
        {
          break;
        }

        //
        // FD not ready right now, go away and try again.
        //

        case NET_EPOLL_WOULD_BLOCK:
        {
          break;
        }

        //
        // Data readed.
        // Call user handler.
        //

        default:
        {
          if (ctx -> dataHandler_)
          {
            ctx -> dataHandler_(ctx, fd, buf, readed);
          }

          break;
        }
      }
    }

    ret = 0;

    #endif

    DBG_LEAVE("NetEpollReadEvent");

    return ret;
  }

  //
  // Handle epoll write ready event on given FD.
  // Called internally only from NetEpollServerLoop().
  //
  // ctx - epoll queue context created in NetEpollServerLoop (IN).
  // fd  - fd, where to write data to (IN).
  //
  // RETURNS: 1 if socket closed inside function,
  //          0 otherwise.
  //

  int NetEpollWriteEvent(NetEpollContext *ctx, int fd)
  {
    DBG_ENTER("NetEpollWriteEvent");

    int ret = -1;

    //
    // Windows.
    //

    #ifdef WIN32
    {
      Error("NetEpollWriteEvent() not implemented on Windows.\n");
    }

    //
    // Linux.
    //

    #else

    char *storedbuffer;

    int storedsize  = 0;
    int closedconns = 0;
    int written   = 0;

    struct epoll_event ev = {0};

    storedbuffer = (char *) ctx -> data_;

    if (storedbuffer == NULL)
    {
      Fatal("FATAL: socket was write blocked, but out of memory.\n");
    }

    storedsize = ctx -> len_;

    if (storedsize == -1)
    {
      Fatal("FATAL: socket was write blocked, but out of memory.\n");
    }

    //
    // Set delay buffer as empty.
    //

    ctx -> len_ = 0;

    //
    // Now we can again start to attempt to write this data to the socket
    //

    written = write(fd, storedbuffer, storedsize);

    //
    // Write failed.
    //

    if(written == -1)
    {
      //
      // Something went wrong. Either we are blocking (buffer full), or there was a real error
      //

      if((errno == EAGAIN) || (errno == EWOULDBLOCK))
      {
        DBG_MSG("WARNING: write-blocked socket became ready for"
                  " writing, but blocked again immediately! (fd %d)\n", fd);

        //
        // We again did not succeed in sending everything we wanted to send.
        // Stop echoing, and queue the data until the socket becomes ready.
        //

        NetEpollDelayWrite(ctx, fd, storedbuffer, storedsize, 0);

        return 0;
      }
      else
      {
        Error("Could not read from socket (fd %d) due to "
                  "unexpected reason (hence closing it). Reason: %s\n",
                      fd, strerror(errno));

        close(fd);

        return 1;
      }
    }

    //
    // Written only part of data.
    //

    else if (written < storedsize)
    {
      //
      // We did not sent all the bytes we wanted to send.
      // Stop echoing, and queue the data until the socket becomes ready.
      //

      DBG_MSG("WARNING: write-blocked socket became ready for writing, but blocked again immediately! (fd %d)\n", fd);

      NetEpollDelayWrite(ctx, fd, storedbuffer, storedsize, written);

      return 0;
    }

    //
    // Whole buffer written.
    //

    else if (written == storedsize)
    {
      free(ctx -> data_);

      ctx -> data_ = NULL;
    }

    //
    // At this point, we have successfully written our data. We can now
    // finally reenable read events. Remove the socket from the queue
    //

    if (epoll_ctl(ctx -> epollFd_, EPOLL_CTL_DEL, fd, NULL) == -1)
    {
      Fatal("FATAL: epoll_ctl removal failure on fd %d", fd);
    }

    //
    // Readd the socket to the queue.
    //

    //
    // Available for read and non edge_triggered.
    //

    ev.events   = EPOLLIN;
    ev.data.ptr = ctx;

    if (epoll_ctl(ctx -> epollFd_, EPOLL_CTL_ADD, fd, &ev) == -1)
    {
      Fatal("FATAL: epoll_ctl failure on readdition of fd %d", fd);
    }

    DBG_MSG("INFO: write-blocked socket became ready"
                " for writing, and userspace buffer was cleared! (fd %d)\n", fd);

    ret = 0;

    #endif

    DBG_LEAVE("NetEpollWriteEvent");

    return ret;
  }
} /* namespace Tegenaria */
