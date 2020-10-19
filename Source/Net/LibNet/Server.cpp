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
// Purpose: TCP/IP4 server.
//
// Usage I (wait for one client only):
// -----------------------------------
//
//   NetConnection *nc = NetAccept()
//   nc -> read(,..)
//   nc -> write(...)
//   nc -> release()
//
// Usage II (create main server loop in new thread,
//           serve connection in new threads):
// ------------------------------------------------
//
//  Create main server loop:
//    NetConnection *nc = NetServerCreate(..., handler, ...)
//    nc -> join()
//
//  Implement connection handler:
//    int handler(NetConnection *nc)
//    {
//      nc -> write()
//      nc -> read()
//      ...
//      nc -> release()
//    }
//

#pragma qcbuild_set_file_title("Server side API")

#include "Net.h"
#include "NetInternal.h"
#include "NetTcpConnection.h"

namespace Tegenaria
{
  //
  // Main server loop, which does:
  //
  // - Wait for incoming connection on given socket
  // - Accept connection
  // - Create new thread to serve connection by given handle
  // - wait again for next incoming connection
  // - ...
  //
  // conn - struct describing opened connection to use (IN).
  //
  // RETURNS: 0 if OK.
  //

  //
  // Define this to avoid TIME_WAIT warning
  // on Linux.
  //
  #ifndef WIN32

  #define AVOID_TIMEWAIT

  #endif

  int NetServerLoop(NetConnection *nc)
  {
    DBG_ENTER("NetServerLoop");

    int exitCode = -1;

    SOCKET client = -1;

    socklen_t len = 0;

    int port = -1;

    struct sockaddr_in clientAddress = {0};

    NetTcpConnection *clientConn = NULL;

    ThreadHandle_t *thread = NULL;

    NetTcpConnection *serv = (NetTcpConnection *) nc;

    DBG_SET_ADD("NetServerLoop", nc);

    //
    // Increase refference counter for listening connection.
    // Until refference counter is > 0 object cannot be destroyed
    // by third thread.
    //

    if (nc)
    {
      nc -> addRef();
    }

    //
    // Check args.
    //

    FAILEX(serv == NULL, "ERROR: 'Conn' cannot be NULL in NetServerLoop.\n");
    FAILEX(int(serv -> getSocket()) < 0, "ERROR: Wrong server's socket in NetServerLoop.\n");
    FAILEX(serv -> getHandler() == NULL, "ERROR: Wrong connection's handler in NetServerLoop.\n");

    //
    // Change connection state to LISTENING.
    //

    serv -> setState(NET_STATE_LISTENING);

    port = htons(serv -> getAddr().sin_port);

    DBG_SET_RENAME("NetTcpConnection", nc, "Listen/%d", port);

    //
    // Fall into main server loop.
    //

    while(serv -> getState() == NET_STATE_LISTENING)
    {
      //
      // Wait for connection.
      //

      DBG_MSG("NetServerLoop : Listening on port [%d] inside context [%p]...\n",
                  port, serv -> getContext());

      FAILEX(listen(serv -> getSocket(), 5),
                 "ERROR: Cannot listen on socket.\n", serv -> getSocket());

      //
      // Accept connection.
      //

      len = sizeof(clientAddress);

      client = accept(serv -> getSocket(), (struct sockaddr *) &clientAddress, &len);

      //
      // Avoid error message if server shutted down in usual way.
      //

      if (serv -> getState() == NET_STATE_DEAD)
      {
        DBG_INFO("NetServerLoop : Server on SOCKET #%d port #%d shutted down.\n",
                     serv -> getSocket(), port);

        break;
      }

      FAILEX(int(client) < 0,
                 "ERROR: Cannot accept connection on socket [%d].\n",
                     serv -> getSocket());

      DBG_MSG("NetServerLoop : Accepted connection from [%s] on socket [%d].\n",
                   inet_ntoa(clientAddress.sin_addr), client);

      DBG_SET_ADD("socket", client, "IN");

      //
      // Pass connection to handler.
      //

      clientConn = new NetTcpConnection(serv -> getContext(), client,
                                            serv -> getHandler(), clientAddress);

      FAILEX(clientConn == NULL, "ERROR: Out of memory.\n");

      DBG_SET_RENAME("NetTcpConnection", clientConn, "IN");

      //
      // Start up connection handler in another thread.
      //

      thread = ThreadCreate((ThreadEntryProto) serv -> getHandler(), clientConn);

      DBG_SET_RENAME("thread", thread, "NET/IN/Handler");

      clientConn -> setThread(thread);
    }

    //
    // Clean up.
    //

    exitCode = 0;

    fail:

    DBG_MSG("NetServerLoop : listening loop for connection [%p] finished.\n", nc);

    if (nc)
    {
      nc -> release();
    }

    DBG_SET_DEL("NetServerLoop", nc);

    DBG_LEAVE("NetServerLoop");

    return exitCode;
  }

  //
  // Start up TCP server in background thread.
  //
  // handler - callback routine to handle incoming connections (IN).
  //
  // custom  - custom, caller specified data passed to handler directly
  //           inside NetConnection struct as 'ctx' (IN/OPT).
  //
  // port    - listening port (IN).
  //
  // RETURNS: Pointer to server side connection,
  //          or NULL if error.
  //

  NetConnection *NetServerCreate(int port, NetHandleConnProto handler, void *custom)
  {
    DBG_ENTER("NetServerCreate");

    int exitCode = -1;

    SOCKET server = -1;

    struct sockaddr_in serverAddress = {0};

    NetTcpConnection *serverConn = NULL;

    ThreadHandle_t *serverLoopThread = NULL;

    #ifdef AVOID_TIMEWAIT
    int iSetOption = 1;
    struct linger so_linger;
    so_linger.l_onoff  = 1;
    so_linger.l_linger = 0;
    #endif

    //
    // Initialize WinSock 2.2 on windows.
    //

    FAIL(_NetInit());

    //
    // Create listening/server socket.
    //

    server = socket(AF_INET, SOCK_STREAM, 0);

    #ifdef AVOID_TIMEWAIT
    setsockopt(server, SOL_SOCKET, SO_LINGER, &so_linger, sizeof(so_linger));
    setsockopt(server, SOL_SOCKET, SO_REUSEADDR, (char*)&iSetOption, sizeof(iSetOption));
    #endif

    FAILEX(int(server) < 0, "ERROR: Cannot create listening socket.\n");

    DBG_SET_ADD("socket", server, "Listen");

    //
    // bind links the socket we just created with the sockaddr_in
    // structure. Basically it connects the socket with
    // the local address and a specified port.
    // If it returns non-zero quit, as this indicates error
    //

    serverAddress.sin_family      = AF_INET;               //Address family
    serverAddress.sin_addr.s_addr = INADDR_ANY;            //Wild card IP address
    serverAddress.sin_port        = htons((u_short) port); //port to use

    FAILEX(bind(server, (sockaddr *) &serverAddress, sizeof(serverAddress)),
               "ERROR: Cannot bind listening socket to port '%d'.\n", port);

    //
    // Allocate and fill output struct.
    //

    serverConn = new NetTcpConnection(custom, server, handler, serverAddress);

    FAILEX(serverConn == NULL, "ERROR: Out of memory.\n");

    //
    // Create server loop thread.
    //

    serverLoopThread = ThreadCreate((ThreadEntryProto) NetServerLoop, serverConn);

    DBG_SET_RENAME("thread", serverLoopThread, "NetServerLoop");

    FAILEX(serverLoopThread == NULL,
               "ERROR: Cannot create server loop thread.\n");

    DBG_MSG("Created server loop thread '%p' within context [%p].\n",
                serverLoopThread, serverConn -> getContext());

    serverConn -> setThread(serverLoopThread);

    //
    // Wait until server ready.
    //

    FAIL(serverConn -> waitForState(NET_STATE_LISTENING, 10000));

    //
    // Clean up.
    //

    exitCode = 0;

    fail:

    if (exitCode)
    {
      Error("Cannot create TCP server on port [%d].\n", port);

      if (int(server) != -1)
      {
        DBG_MSG("NetServerCreate : Shutting down SOCKET #%d...\n", server);

        shutdown(server, SD_BOTH);

        closesocket(server);

        DBG_SET_DEL("socket", server);
      }

      if (serverConn)
      {
        serverConn -> setState(NET_STATE_DEAD);

        serverConn -> release();

        serverConn = NULL;
      }
    }

    DBG_LEAVE("NetServerCreate");

    return serverConn;
  }

  //
  // Create listening TCP/IP4 socket and wait for one client.
  // After connection negociated, listening socket is shutted down.
  //
  // WARNING: Caller MUST release received net connection object by
  //          calling nc -> release() method on it.
  //
  // port    - listening port (IN).
  // timeout - maximum listening time in miliseconds. Defaulted to infinite if -1 (IN/OPT).
  //
  // RETURNS: Pointer to server side connection,
  //          or NULL if error.
  //

  NetConnection *NetAccept(int port, int timeout)
  {
    DBG_ENTER("NetAccept");

    int exitCode = -1;

    socklen_t len = 0;

    SOCKET server = -1;
    SOCKET client = -1;

    struct sockaddr_in serverAddress = {0};
    struct sockaddr_in clientAddress = {0};

    int ret = -1;

    fd_set fds;

    NetTcpConnection *nc = NULL;

    //
    // Initialize WinSock 2.2 on windows.
    //

    FAIL(_NetInit());

    //
    // Create listening/server socket.
    //

    server = socket(AF_INET, SOCK_STREAM, 0);

    FAILEX(int(server) < 0, "ERROR: Cannot create listening socket.\n");

    DBG_SET_ADD("socket", server, "Listen/%d", port);

    //
    // bind links the socket we just created with the sockaddr_in
    // structure. Basically it connects the socket with
    // the local address and a specified port.
    // If it returns non-zero quit, as this indicates error
    //

    serverAddress.sin_family      = AF_INET;               //Address family
    serverAddress.sin_addr.s_addr = INADDR_ANY;            //Wild card IP address
    serverAddress.sin_port        = htons((u_short) port); //port to use

    FAILEX(bind(server, (sockaddr *) &serverAddress, sizeof(serverAddress)),
               "ERROR: Cannot bind listening socket to port '%d'.\n", port);

    //
    // Set listening socket to non-block mode.
    //

    FAIL(NetSetNonBlockMode(server));

    //
    // Wait for connection.
    //

    DEBUG2("NetAccept : Listening on port [%d]...\n", port);

    FAILEX(listen(server, 5), "ERROR: Cannot listen on socket.\n", server);

    //
    // Wait for connection or timeout.
    //

    DEBUG2("NetAccept : Waiting for incoming connection on port [%d]...\n", port);

    FD_ZERO(&fds);
    FD_SET(server, &fds);

    if (timeout > 0)
    {
      struct timeval tv = {0};

      tv.tv_sec  = timeout / 1000;
      tv.tv_usec = timeout % 1000 * 1000;

      ret = select(server + 1, &fds, NULL, NULL, &tv);
    }
    else
    {
      ret = select(server + 1, &fds, NULL, NULL, NULL);
    }

    FAILEX(ret <= 0, "ERROR: Timeout while listening on port [%d].\n", port);

    //
    // Accept connection.
    //

    len = sizeof(clientAddress);

    client = accept(server, (struct sockaddr *) &clientAddress, &len);

    FAILEX(int(client) < 0, "ERROR: Cannot accept connection on socket [%d].\n", client);

    DBG_MSG("NetServerLoop : Accepted connection from [%s] on socket [%d].\n",
                inet_ntoa(clientAddress.sin_addr), client);

    DBG_SET_ADD("socket", client, "IN");

    //
    // Wrap socket into NetConnection class.
    //

    nc = new NetTcpConnection(NULL, client, NULL, clientAddress);

    nc -> setState(NET_STATE_ESTABLISHED);

    DBG_SET_RENAME("NetTcpConnection", nc, "IN");

    //
    // Clean up.
    //

    exitCode = 0;

    fail:

    if (exitCode)
    {
      Error("ERROR: Cannot create TCP server on port [%d]. Error is %d.\n",
                port, GetLastError());
    }

    //
    // Shutdown listening socket.
    //

    if (int(server) != -1)
    {
      DBG_MSG("NetAccept : Shutting down SOCKET #%d...\n", server);

      shutdown(server, SD_BOTH);

      closesocket(server);

      DBG_SET_DEL("socket", server);
    }

    DBG_LEAVE("NetAccept");

    return nc;
  }

  //
  // Try create listening socket on given port.
  //
  // port    - listening port (IN).
  //
  // RETURNS: 0 if socket can be created (port free)
  //          -1 otherwise.
  //

  int NetTryBind(int port)
  {
    DBG_ENTER("NetTryBind");

    int exitCode = -1;

    socklen_t len = 0;

    SOCKET server = -1;
    SOCKET client = -1;

    struct sockaddr_in serverAddress = {0};
    struct sockaddr_in clientAddress = {0};

    //
    // Initialize WinSock 2.2 on windows.
    //

    FAIL(_NetInit());

    //
    // Create listening/server socket.
    //

    server = socket(AF_INET, SOCK_STREAM, 0);

    FAIL(int(server) < 0);

    DBG_SET_ADD("socket", server, "Listen/%d", port);

    //
    // bind links the socket we just created with the sockaddr_in
    // structure. Basically it connects the socket with
    // the local address and a specified port.
    // If it returns non-zero quit, as this indicates error
    //

    serverAddress.sin_family      = AF_INET;               //Address family
    serverAddress.sin_addr.s_addr = INADDR_ANY;            //Wild card IP address
    serverAddress.sin_port        = htons((u_short) port); //port to use

    FAIL(bind(server, (sockaddr *) &serverAddress, sizeof(serverAddress)));

    //
    // Clean up.
    //

    exitCode = 0;

    fail:

    //
    // Shutdown listening socket.
    //

    if (int(server) != -1)
    {
      shutdown(server, SD_BOTH);

      closesocket(server);

      DBG_SET_DEL("socket", server);
    }

    DBG_LEAVE("NetTryBind");

    return exitCode;
  }

  //
  // Create listening socket on given port.
  // queue.
  //
  // port           - port number, where to start litsening (IN).
  // maxConnections - maximum allowed number of incoming connections (IN).
  // nonBlock       - create non-blocking socket if set to 1 (IN).
  //
  // RETURNS: Listening socket
  //          or -1 if error.
  //

  int NetCreateListenSocket(int port, int maxConnections, int nonBlock)
  {
    DBG_ENTER("NetCreateListenSocket");

    int exitCode = -1;

    int sock         = -1;
    int reuseAddress = 1;

    #ifdef WIN32
    struct sockaddr_in addr = {0};
    #else
    struct sockaddr_in6 addr = {0};
    #endif

    //
    // Create non-block listening socket.
    //

    #ifdef WIN32
    {
      sock = socket(AF_INET, SOCK_STREAM, 0);
    }
    #else
    {
      if (nonBlock)
      {
        sock = socket(AF_INET6, SOCK_STREAM | SOCK_NONBLOCK, 0);
      }
      else
      {
        sock = socket(AF_INET6, SOCK_STREAM, 0);
      }
    }
    #endif

    FAILEX(SOCKET(sock) == -1,
               "ERROR: Cannot create listening socket.\n");

    //
    // Windows doesn't support for SOCK_NONBLOCK option in socket().
    // We need to set up it manually.
    //

    #ifdef WIN32
    {
      if (nonBlock)
      {
        u_long nonblock = 1;

        FAILEX(ioctlsocket(sock, FIONBIO, &nonblock),
                   "ERROR: Cannot set non-block mode on SOCKET #%d.\n", sock);
      }
    }
    #endif

    //
    // Set address reuse.
    //

    FAILEX(setsockopt(sock, SOL_SOCKET, SO_REUSEADDR,
                         (char *) &reuseAddress, sizeof(reuseAddress)) == -1,
                             "ERROR: Cannot set reuse flag on socket #%d.\n", sock);
    //
    // Bind socket to any address and selected port.
    //

    #ifdef WIN32
    {
      addr.sin_family      = AF_INET;
      addr.sin_addr.s_addr = INADDR_ANY;
      addr.sin_port        = htons((u_short) port);
    }
    #else
    {
      addr.sin6_port   = htons(port);
      addr.sin6_addr   = in6addr_any;
      addr.sin6_family = AF_INET6;
    }
    #endif

    FAILEX(bind(sock, (struct sockaddr *) &addr, sizeof(addr)) == -1,
               "ERROR: Cannot bind address to socket #%d.\n", sock);

    //
    // Start listening for incoming connection.
    //

    FAILEX(listen(sock, maxConnections) == -1,
               "ERROR: Cannot listen on socket #%d.\n", sock);

    DBG_INFO("Listening on port %d...\n", port);

    //
    // Clean up.
    //

    exitCode = 0;

    fail:

    if (exitCode)
    {
      Error("Cannot create listening socket.\n"
            "Error code is %d.\n", GetLastError());

      if (SOCKET(sock) != -1)
      {
        #ifdef WIN32
        closesocket(sock);
        #else
        close(sock);
        #endif

        sock = -1;
      }
    }

    DBG_LEAVE("NetCreateListenSocket");

    return sock;
  }
} /* namespace Tegenaria */
