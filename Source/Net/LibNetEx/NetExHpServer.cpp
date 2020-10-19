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
// Purpose: Callback based server based on libevent toolkit.
//

#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <signal.h>
#include <set>
#include <algorithm>

using std::max;

#ifdef WIN32
# include <windows.h>
# include <ws2tcpip.h>
#else
# include <netinet/in.h>
# include <unistd.h>
# ifdef _XOPEN_SOURCE_EXTENDED
#  include <arpa/inet.h>
# endif
# include <sys/socket.h>
#endif

#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/listener.h>
#include <event2/util.h>
#include <event2/event.h>
#include <event2/thread.h>

#include <Tegenaria/Debug.h>
#include <Tegenaria/Thread.h>
#include <Tegenaria/Mutex.h>
#include "NetEx.h"
#include "Utils.h"

namespace Tegenaria
{
  using std::set;

  const int NET_EX_MAX_THREADS = 64;

  //
  // Internal use only callbacks passed to libevent.
  //

  static void NetExHpOpenCallback(evutil_socket_t, short, void *);

  static void NetExHpExitCallback(evutil_socket_t, short, void *);

  static void NetExHpEventCallback(struct bufferevent *, short, void *);

  static void NetExHpReadCallback(struct bufferevent *, void *);

  //
  // Global variables.
  //

  static struct event_base *EventBase[NET_EX_MAX_THREADS] = {0};

  static int CpuCount = NetExGetCpuNumber();

  static int ServerRunning = 0;

  //
  // Map to check is given context correct.
  // Debug purpose only.
  //

  #ifdef NET_EX_CHECK_CTX

  static set<NetExHpContext *> CtxSet;

  static Mutex CtxSetMutex("NetExHpServer::CtxSetMutex");

  #endif

  //
  // Internal use only. Thread function falling into main libevent loop.
  // We create one libevent loop for every CPU core inside NetExHpServerLoop().
  //
  // ctx - HP context created inside NetExHpServerLoop() (IN).
  //

  int NetExHpServerWorkerLoop(NetExHpContext *ctx)
  {
    int workerNo = ctx -> workerNo_;

    struct event_base *eventBase = (struct event_base *) ctx -> eventBase_;

    DBG_INFO("Created HP worker #%d.\n", workerNo);

    event_base_dispatch(eventBase);

    DBG_INFO("HP worker #%d finished.\n", workerNo);
  }

  //
  // Create TCP server based on libevent library. Traffic is encrypted
  // basing on TLS protocol.
  //
  // port              - listening port (IN).
  // openHandler       - handler called when new connection arrived (IN/OPT).
  // closeHandler      - handler called when existing connection closed (IN/OPT).
  //
  // dataHandler       - handler called when something to read on one of existing
  //                     connection (IN).
  //
  // secureCert        - filename, where server certificate is stored (IN).
  //
  // securePrivKey     - filename, where server private key is stored (server side
  //                     only) (IN/OPT).
  //
  // securePrivKeyPass - passphrase to decode private key. Readed from keyboard
  //                     if skipped (IN/OPT).
  //
  //
  // TIP #1: Use NetExHpWrite() to write data inside data handler. Don't
  //         use write() or send() directly.
  //
  // RETURNS: never reached in correct work,
  //         -1 if error.
  //

  int NetExHpSecureServerLoop(int port, NetExHpOpenProto openHandler,
                                  NetExHpCloseProto closeHandler,
                                      NetExHpDataProto dataHandler,
                                          const char *secureCert,
                                              const char *securePrivKey,
                                                  const char *securePrivKeyPass)
  {
    DBG_ENTER("NetExHpServerLoop");

    int exitCode = -1;

    struct event *exitEvent = NULL;

    struct event *acceptEvent[NET_EX_MAX_THREADS] = {0};

    ThreadHandle_t *workerThread[NET_EX_MAX_THREADS] = {0};

    struct sockaddr_in sin = {0};

    int listenfd = -1;

    int eventFlags = 0;

    int reuseaddr_on = 1;

    struct linger so_linger;

    NetExHpContext *ctx[NET_EX_MAX_THREADS] = {0};

    //
    // Set up pthread locking for multithreading on Linux
    //

    #ifdef __linux__
    {
      struct sigaction act = {0};

      //
      // Enable multithread support in livevent.
      //

      evthread_use_pthreads();

      //
      // Ignore SIGPIPE on Linux.
      //

      act.sa_handler = SIG_IGN;
      act.sa_flags   = SA_RESTART;

      sigaction(SIGPIPE, &act, NULL);
    }
    #endif

    //
    // Fail if TLS requested without NET_EX_USE_LIBSECURE.
    //

    #ifndef NET_EX_USE_LIBSECURE
    {
      if (secureCert || securePrivKey)
      {
        Error("ERROR: NetExHp server is compiled without LibSecure.\n"
              "TLS server is not available.\n");

        goto fail;
      }
    }
    #endif

    //
    // HP Server is not thread safe. It means one process can have only one
    // (multithreaded) HP server running.
    //

    FAILEX(ServerRunning, "ERROR: Only one HP server per process allowed.\n");

    ServerRunning = 1;

    //
    // Init WINSOCK2 on windows.
    //

    #ifdef WIN32
    WSADATA wsadata;
    WSAStartup(0x0201, &wsadata);
    #endif

    //
    // Create listening socket.
    //

    listenfd = socket(AF_INET, SOCK_STREAM, 0);

    FAILEX(listenfd < 0, "ERROR: Cannot create listening socket.\n");

    //
    // Set SO_LINGER flag.
    //

    so_linger.l_onoff  = 1;
    so_linger.l_linger = 0;

    setsockopt(listenfd, SOL_SOCKET, SO_LINGER,
                   (const char *) &so_linger, sizeof(so_linger));

    //
    // Set address reuse on listening socket.
    //

    reuseaddr_on = 1;

    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR,
                   (const char *) &reuseaddr_on, sizeof(reuseaddr_on));

    //
    // Bind socket to given port.
    //

    sin.sin_family = AF_INET;
    sin.sin_port   = htons(port);

    FAILEX(bind(listenfd, (struct sockaddr *) &sin, sizeof(sin)),
               "ERROR: Cannot bind lsitening socket to port %d.\n", port);

    //
    // Start listening.
    //

    FAILEX(listen(listenfd, SOMAXCONN) < 0, "ERROR: Listen() failed.\n");

    DBG_INFO("NetExHpLoop : Listening on TCP port %d...\n", port);

    //
    // Set nonblock mode on listening socket.
    //

    FAILEX(evutil_make_socket_nonblocking(listenfd) < 0,
               "ERROR: Cannot set non-blocking mode on listening socket.\n");

    //
    // Initialize event worker per every CPU core.
    //

    for (int i = 0; i < CpuCount; i++)
    {
      //
      // Create new event base object for worker.
      //

      EventBase[i] = event_base_new();

      FAILEX(EventBase[i] == NULL, "ERROR: Cannot initialize libevent.\n");

      //
      // Allocate new NetExHp context.
      //

      ctx[i] = (NetExHpContext *) calloc(1, sizeof(NetExHpContext));

      FAILEX(ctx[i] == NULL, "ERROR: Out of memory.\n");

      ctx[i] -> eventBase_    = EventBase[i];
      ctx[i] -> openHandler_  = openHandler;
      ctx[i] -> closeHandler_ = closeHandler;
      ctx[i] -> dataHandler_  = dataHandler;
      ctx[i] -> workerNo_     = i;

      //
      // Save data to establish secure session on incoming connection.
      //

      #ifdef NET_EX_USE_LIBSECURE
      {
        if (secureCert)
        {
          ctx[i] -> secureCert_ = strdup(secureCert);
        }

        if (securePrivKey)
        {
          ctx[i] -> securePrivKey_ = strdup(securePrivKey);
        }

        if (securePrivKeyPass)
        {
          ctx[i] -> securePrivKeyPass_ = strdup(securePrivKeyPass);
        }
      }
      #endif

      //
      // Create accept event for given event base.
      //

      acceptEvent[i] = event_new(EventBase[i], listenfd, EV_READ | EV_PERSIST,
                                    NetExHpOpenCallback, (void *) ctx[i]);

      FAILEX(acceptEvent[i] == NULL, "ERROR: Cannot initialize accept event.\n");

      FAILEX(event_add(acceptEvent[i], NULL) < 0,
                 "ERROR: Cannot register accept event.\n");
    }

    //
    // Initialize exit event to catch SIGINT (linux) or CTRL_BREAK (windows).
    //

  /*
    FIXME: Only one thread finished.

    exitEvent = evsignal_new(EventBase[0], SIGINT,
                                 NetExHpExitCallback, NULL);

    FAILEX(exitEvent == NULL, "ERROR: Cannot initialize exit event.\n");

    //
    // Register exit event.
    //

    FAILEX(event_add(exitEvent, NULL) < 0,
               "ERROR: Cannot register exit event.\n");

  */

    //
    // Create libevent loop in another thread for every CPU core.
    //

    for (int i = 0; i < CpuCount; i++)
    {
      workerThread[i] = ThreadCreate(NetExHpServerWorkerLoop, ctx[i]);
    }

    //
    // Wait until every workers finished.
    //

    for (int i = 0; i < CpuCount; i++)
    {
      ThreadWait(workerThread[i]);
      ThreadClose(workerThread[i]);
      workerThread[i] = NULL;
    }

    //
    // Error handler.
    //

    exitCode = 0;

    fail:

    if (exitCode)
    {
      Error("ERROR: Cannot initialize HP server loop on port %d.\n"
                "Error code is : %d.\n", port, GetLastError());
    }

    if (exitEvent)
    {
      event_free(exitEvent);
    }

    for (int i = 0; i < CpuCount; i++)
    {
      if (acceptEvent[i])
      {
        event_free(acceptEvent[i]);
      }

      if (EventBase[i])
      {
        event_base_free(EventBase[i]);
      }

      if (ctx[i])
      {
        #ifdef NET_EX_USE_LIBSECURE
        {
          if (ctx[i] -> secureCert_)
          {
            free(ctx[i] -> secureCert_);
          }

          if (ctx[i] -> securePrivKey_)
          {
            free(ctx[i] -> securePrivKey_);
          }

          if (ctx[i] -> securePrivKeyPass_)
          {
            free(ctx[i] -> securePrivKeyPass_);
          }
        }
        #endif

        free(ctx[i]);
      }
    }

    ServerRunning = 0;

    DBG_LEAVE("NetExHpServerLoop");

    return exitCode;
  }

  //
  // Create TCP server based on libevent library.
  //
  // port         - listening port (IN).
  // openHandler  - handler called when new connection arrived (IN/OPT).
  // closeHandler - handler called when existing connection closed (IN/OPT).
  //
  // dataHandler  - handler called when something to read on one of existing
  //                connection (IN).
  //
  // TIP #1: Use NetExHpWrite() to write data inside data handler. Don't
  //         use write() or send() directly.
  //
  // TIP #2: Use NetExHpSecureServerLoop() to create TLS encrypted server.
  //
  // RETURNS: never reached in correct work,
  //         -1 if error.
  //

  int NetExHpServerLoop(int port, NetExHpOpenProto openHandler,
                            NetExHpCloseProto closeHandler,
                                NetExHpDataProto dataHandler)
  {
    return NetExHpSecureServerLoop(port, openHandler, closeHandler,
                                       dataHandler, NULL, NULL, NULL);
  }

  //
  // Write <len> bytes remote client related with given NetExHpContext.
  //
  // TIP #1: Caller should use this function to write data
  //         to remote client inside data handler specified to
  //         NetExServerLoop(). Don't use write() or send() directly.
  //
  // ctx - context received in data handler parameters (IN).
  // buf - buffer with data to write (IN).
  // len - how many bytes to write (IN).
  //
  // RETURNS: Number of bytes written or
  //          -1 if error.
  //

  int NetExHpWrite(NetExHpContext *ctx, void *buf, int len)
  {
    DBG_ENTER3("NetExHpWrite");

    int ret = -1;

    //
    // Debug purpose only.
    // Track created contexts.
    //

    #ifdef NET_EX_CHECK_CTX
    {
      CtxSetMutex.lock();

      if (CtxSet.count(ctx) == 0)
      {
        Fatal("FATAL: Context PTR#%p does not exist in NetExHpContext().\n", ctx);
      }

      CtxSetMutex.unlock();
    }
    #endif

    //
    // Use LibSecure.
    //

    #ifdef NET_EX_USE_LIBSECURE
    {
      if (ctx)
      {
        bufferevent *bev = (bufferevent *) ctx -> eventBuffer_;

        //
        // TLS encrypted session.
        //

        if (ctx -> sc_)
        {
          int encryptBufferSize = max(8096, len * 2);

          char *encryptBuffer = (char *) malloc(encryptBufferSize);

          FAILEX(encryptBuffer == NULL, "ERROR: Out of memory.\n");

          //
          // Encrypt outcoming data before send.
          //

          encryptBufferSize = ctx -> sc_ -> encrypt(encryptBuffer,
                                                        encryptBufferSize, buf, len);

          //
          // Send encrypted data to client.
          //

          if (bufferevent_write(bev, encryptBuffer, encryptBufferSize) == 0)
          {
            ret = len;
          }

          free(encryptBuffer);
        }

        //
        // Non-TLS unecrypted session.
        //

        else
        {
          if (bufferevent_write(bev, buf, len) == 0)
          {
            ret = len;
          }
        }
      }
    }

    //
    // LibSecure disabled.
    //

    #else
    {
      //
      // Write data into libevent buffer.
      //

      if (ctx)
      {
        bufferevent *bev = (bufferevent *) ctx -> eventBuffer_;

        if (bufferevent_write(bev, buf, len) == 0)
        {
          ret = len;
        }
      }
    }
    #endif

    fail:

    DBG_ENTER3("NetExHpWrite");

    return ret;
  }

  //
  // Callback called when new connection arrived.
  //

  static void NetExHpOpenCallback(evutil_socket_t fd, short ev, void *data)
  {
    DBG_ENTER3("NetExHpOpenCallback");

    int exitCode = -1;

    struct event_base *eventBase = NULL;

    struct bufferevent *eventBuffer = NULL;

    NetExHpContext *serverCtx = NULL;

    NetExHpContext *ctx = NULL;

    int clientfd = -1;

    struct sockaddr_in client_addr = {0};

    socklen_t client_len = sizeof(client_addr);

    //
    // Get back main HP server loop context from data pointer.
    //

    serverCtx = (NetExHpContext *) data;

    //
    // Allocate HP context for new connection.
    //

    ctx = (NetExHpContext *) calloc(1, sizeof(NetExHpContext));

    FAILEX(ctx == NULL, "ERROR: Out of memory.\n");

    //
    // Accept conncetion.
    //

    clientfd = accept(fd, (struct sockaddr *) &client_addr, &client_len);

    FAIL(clientfd < 0);

    //
    // Save client IP in connection context.
    //

    strncpy(ctx -> clientIp_,
                inet_ntoa(client_addr.sin_addr),
                    sizeof(ctx -> clientIp_) - 1);

    //
    // Initialize new event buffer for new connection.
    //

    eventBase = (event_base *) serverCtx -> eventBase_;

    eventBuffer = bufferevent_socket_new(eventBase, clientfd, BEV_OPT_CLOSE_ON_FREE);

    FAILEX(eventBuffer == NULL, "ERROR: Cannot create event buffer.\n");

    //
    // Debug purpose only.
    // Track created contexts.
    //

    #ifdef NET_EX_CHECK_CTX
    {
      CtxSetMutex.lock();
      CtxSet.insert(ctx);
      CtxSetMutex.unlock();
    }
    #endif

    //
    // Init secure context for secure connection.
    //

    #ifdef NET_EX_USE_LIBSECURE
    {
      //
      // TLS encrypted session. Init secure context.
      //

      if (serverCtx -> secureCert_ && serverCtx -> securePrivKey_)
      {
        ctx -> sc_ = SecureConnectionCreate(SECURE_INTENT_SERVER,
                                                serverCtx -> secureCert_,
                                                    serverCtx -> securePrivKey_,
                                                        serverCtx -> securePrivKeyPass_);
        FAILEX(ctx -> sc_ == NULL,
                   "ERROR: Cannot init secure context for new connection.\n");
      }
    }
    #endif

    //
    // Call underlying open handler if specified.
    //

    ctx -> openHandler_  = serverCtx -> openHandler_;
    ctx -> closeHandler_ = serverCtx -> closeHandler_;
    ctx -> dataHandler_  = serverCtx -> dataHandler_;
    ctx -> workerNo_     = serverCtx -> workerNo_;
    ctx -> eventBuffer_  = eventBuffer;

    if (ctx -> openHandler_)
    {
      ctx -> openHandler_(ctx);
    }

    //
    // Set event and read callbacks.
    //

    bufferevent_setcb(eventBuffer, NetExHpReadCallback,
                          NULL, NetExHpEventCallback, ctx);

    bufferevent_enable(eventBuffer, EV_READ);

    //
    // Error handler.
    // Accept failed or already accepted from another libevent loop.
    //

    exitCode = 0;

    fail:

    if (exitCode)
    {
      //
      // Clean up on error.
      //

      if (clientfd > 0)
      {
        #ifdef WIN32
        closesocket(clientfd);
        #else
        close(clientfd);
        #endif
      }

      if (ctx)
      {
        #ifdef NET_EX_USE_LIBSECURE
        {
          if (ctx -> sc_)
          {
            ctx -> sc_ -> release();

            ctx -> sc_ = NULL;
          }
        }
        #endif

        free(ctx);
      }
    }

    DBG_LEAVE3("NetExHpOpenCallback");
  }

  //
  // Callback called when new data arrived.
  //

  static void NetExHpReadCallback(struct bufferevent *bev, void *data)
  {
    DBG_ENTER3("NetExHpReadCallback");

    NetExHpContext *ctx = (NetExHpContext *) data;

    //
    // Get event input buffer.
    //

    struct evbuffer *input = bufferevent_get_input(bev);

    size_t len = evbuffer_get_length(input);

    unsigned char *buf = evbuffer_pullup(input, len);

    //
    // Use TLS secure connection.
    //

    #ifdef NET_EX_USE_LIBSECURE
    {
      //
      // TLS encrypted session enabled.
      //

      if (ctx && ctx -> sc_)
      {
        switch(ctx -> sc_ -> getState())
        {
          case SECURE_STATE_ESTABLISHED:
          {
            char *decryptBuffer = (char *) malloc(len);

            int decryptBufferSize = len;

            FAILEX(decryptBuffer == NULL, "ERROR: Out of memory.\n");

            //
            // Decrypt incoming data into buffer[].
            //

            decryptBufferSize = ctx -> sc_ -> decrypt(decryptBuffer,
                                                          decryptBufferSize,
                                                              buf, len);

            //
            // Call underlying data handler with decrypted buffer.
            //

            if (ctx -> dataHandler_)
            {
              ctx -> dataHandler_(ctx, decryptBuffer, decryptBufferSize);
            }

            free(decryptBuffer);

            break;
          }

          //
          // SSL Handshake is pending.
          // We must handle it by own.

          case SECURE_STATE_HANDSHAKE_READ:
          case SECURE_STATE_HANDSHAKE_WRITE:
          {
            char toWrite[1024];

            int toWriteSize = 1024;

            if (ctx -> sc_ -> handshakeStep(toWrite, &toWriteSize, buf, len) == 0)
            {
              //
              // Write handshake data to client.
              //

              if (toWriteSize > 0)
              {
                bufferevent *bev = (bufferevent *) ctx -> eventBuffer_;

                bufferevent_write(bev, toWrite, toWriteSize);
              }
            }

            //
            // Handshake error. Close connection.
            //

            else
            {
              NetExHpEventCallback(bev, 0, ctx);

              bev = NULL;
            }

            break;
          }
        }
      }

      //
      // Non-TLS pure unencrypted session.
      //

      else
      {
        //
        // Call underlying data handler if exist.
        //

        if (ctx && ctx -> dataHandler_)
        {
          ctx -> dataHandler_(ctx, buf, len);
        }
      }
    }

    //
    // Don't use LibSecure. Only pure TCP server.
    //

    #else
    {
      //
      // Call underlying data handler if exist.
      //

      if (ctx && ctx -> dataHandler_)
      {
        ctx -> dataHandler_(ctx, buf, len);
      }
    }
    #endif

    //
    // Clean up.
    //

    fail:

    //
    // Pop processed data from event buffer.
    //

    if (bev)
    {
      evbuffer_drain(input, len);
    }

    DBG_LEAVE3("NetExHpReadCallback");
  }

  //
  // Callback called when connection closed.
  //

  static void NetExHpEventCallback(struct bufferevent *bev,
                                       short events, void *data)
  {
    DBG_ENTER3("NetExHpEventCallback");

    NetExHpContext *ctx = (NetExHpContext *) data;

    if (ctx)
    {
      if (ctx -> closeHandler_)
      {
        ctx -> closeHandler_(ctx);
      }

      //
      // Debug purpose only.
      // Track created contexts.
      //

      #ifdef NET_EX_CHECK_CTX
      {
        CtxSetMutex.lock();

        if (CtxSet.count(ctx) == 0)
        {
          Fatal("FATAL: Context PTR #%p does not exists in NetExHpEventCallback().\n", ctx);
        }

        CtxSet.erase(ctx);

        CtxSetMutex.unlock();
      }
      #endif

      //
      // Free secure context if needed.
      //

      #ifdef NET_EX_USE_LIBSECURE
      {
        if (ctx -> sc_)
        {
          ctx -> sc_ -> release();

          ctx -> sc_ = NULL;
        }
      }
      #endif

      //
      // Free context.
      //

      free(ctx);
    }

    bufferevent_free(bev);

    DBG_LEAVE3("NetExHpEventCallback");
  }

  //
  // Callback called when SIGINT handled.
  //

  static void NetExHpExitCallback(evutil_socket_t sig, short events, void *data)
  {
    DBG_INFO("NetExHpExitCallback : CTRL_BREAK received. Going to shutdown...\n");

    for (int i = CpuCount - 1; i >= 0; i--)
    {
      event_base_loopbreak(EventBase[i]);
    }
  }
} /* namespace Tegenaria */
