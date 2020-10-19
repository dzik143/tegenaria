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
// Purpose: abstract network connection.
//
//          Server does : NetworkConnection *nc = NetAccept(...)
//                        nc -> write(...)
//                        nc -> read(...)
//                        nc -> release()
//
//          Client does : NetworkConnection *nc = NetConnect(...)
//                        nc -> write(...)
//                        nc -> read(...)
//                        nc -> release()
//
//          To add new connection type (e.g. tcp/ip4):
//
//          1. Create class derived from NetConnection (e.g. NetTcpConnection).
//             Derived class MUST implement at least read() and write() methods.
//
//          2. Implement NetAccept() or NetServerCreate() like function
//             for server side.
//
//          3. Implement NetConnect() like function for client side.
//

#pragma qcbuild_set_file_title("NetConnection class");

#include "NetConnection.h"

namespace Tegenaria
{
  //
  // Static variables.
  //

  set<NetConnection *> NetConnection::instances_;

  Mutex NetConnection::instancesMutex_("NetConnection::instances_");

  //
  // Constructor.
  //
  // ctx      - caller specified context (IN).
  // protocol - artbitrary protocol name e.g. "TCP/IP" (IN).
  // socket   - asociatet socket number, can be -1 if not needed (IN/OPT).
  // handler  - callback to handle incoming connections (IN/OPT).
  // thread   - asociated thread handle (IN/OPT).
  //

  NetConnection::NetConnection(const void *ctx, const char *protocol,
                                   int socket, NetHandleConnProto handler,
                                       ThreadHandle_t *thread)
  {
    DBG_ENTER3("NetConnection");

    DBG_SET_ADD("NetConnection", this);

    DBG_MSG3("NetConnection : Creating net connection...");
    DBG_MSG3("  this      : [%p]\n", this);
    DBG_MSG3("  context   : [%p]\n", ctx);
    DBG_MSG3("  protocol  : [%s]\n", protocol);
    DBG_MSG3("  socket    : [%d]\n", int(socket));
    DBG_MSG3("  handler   : [%p]\n", handler);
    DBG_MSG3("  ref count : [%d]\n", 1);
    DBG_MSG3("  thread    : [%p]\n", thread);

    ctx_       = ctx;
    socket_    = socket;
    handler_   = handler;
    refCount_  = 1;
    thread_    = thread;
    quietMode_ = 0;

    //
    // Track created instances.
    //

    instancesMutex_.lock();
    instances_.insert(this);
    instancesMutex_.unlock();

    if (protocol)
    {
      protocol_ = protocol;
    }

    state_ = NET_STATE_PENDING;

    clientInfo_ = "N/A";

    DBG_LEAVE3("NetConnection");
  }

  //
  // Desctructor. Shutdown connection to force flush.
  //

  NetConnection::~NetConnection()
  {
    DBG_ENTER("NetConnection::~NetConnection");

    //
    // Check is this pointer correct.
    //

    if (isPointerCorrect(this) == 0)
    {
      Error("ERROR: Attemp to destroy non existing NetConnection PTR#%p.\n", this);

      return;
    }

    //
    // Track created instances.
    //

    instancesMutex_.lock();
    instances_.erase(this);
    instancesMutex_.unlock();

    DBG_MSG3("Destroying NetConnection PTR=[%p] CTX=[%p]...\n", this, ctx_);

    shutdown();

    DBG_SET_DEL("NetConnection", this);

    DBG_LEAVE("NetConnection::~NetConnection");
  }

  //
  // Low level I/O.
  //

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

  int NetConnection::write(const void *buf, int count, int timeout)
  {
    Error("ERROR: write() is NOT implemented.\n");
  }
  //
  // Reveive up to <count> bytes and store them in <buf>.
  //
  // buf     - destination buffer where to store readed data (OUT).
  // count   - expecting number of bytes to be readed (IN).
  // timeout - timeout in miliseconds (IN).
  //
  // RETURNS: Number of bytes readed or
  //          -1 if error.
  //

  int NetConnection::read(void *buf, int count, int timeout)
  {
    Error("ERROR: read() is NOT implemented.\n");
  }

  //
  // Cancel all pending I/O associated with connection (if any).
  //

  void NetConnection::cancel()
  {
    Error("ERROR: cancel() is NOT implemented.\n");
  }

  //
  // Shutdown connection.
  //

  int NetConnection::shutdown(int how)
  {
  }

  //
  // Wait until connection finished work.
  //

  int NetConnection::join()
  {
    while(1)
    {
      #ifdef WIN32
      Sleep(1000.0);
      #else
      sleep(1.0);
      #endif
    }
  }

  //
  // Static wrappers for read/write methods to be compatible with pure C code.
  // Ctx means this pointer (NetConnection object).
  // Used to pass read/write callback to IOMixer without adding dependency
  // on LibNet.
  //
  // To pass read/write method to another C code we need to pass 2 values:
  //
  // - this pointer ('this')
  // - pointer to NetConnection::readCallback ('callback')
  //
  // After that third C code can use it by calling:
  //
  // callback(buf, count, timeout, this)
  //

  int NetConnection::readCallback(void *buf, int count, int timeout, void *ctx)
  {
    NetConnection *this_ = (NetConnection *) ctx;

    return this_ -> read(buf, count, timeout);
  }

  int NetConnection::writeCallback(void *buf, int count, int timeout, void *ctx)
  {
    NetConnection *this_ = (NetConnection *) ctx;

    return this_ -> write(buf, count, timeout);
  }

  void NetConnection::cancelCallback(void *ctx)
  {
    NetConnection *this_ = (NetConnection *) ctx;

    this_ -> cancel();
  }

  //
  // High level I/O.
  //

  //
  // - Send single, printf like formatted request to server
  // - read answer in format 'XYZ > message'
  // - split answer to <XYZ> code and <message> parts.
  //
  // Example usage:
  //
  // request(&serverCode, serverMsg, sizeof(serverMsg),
  //             "share --alias %s --path %s", alias, path);
  //
  // TIP: If only exit code is needed <answer> can be set to NULL.
  //
  // serverCode    - exit code returned by server (OUT).
  // serverMsg     - ASCIZ message returned by server (OUT/OPT).
  // serverMsgSize - size of answer buffer in bytes (IN).
  // timeout       - timeout in ms, defaulted to infinite if -1 (IN/OPT).
  // fmt           - printf like parameters to format command to send (IN).
  //
  // RETURNS: 0 if request sucessfuly sent and asnwer from server received.
  //         -1 otherwise.
  //
  // WARNING!: Request could still failed on server side.
  //           To get server's side exit code use 'answerCode' parameter.
  //

  int NetConnection::request(int *serverCode, char *serverMsg,
                                 int serverMsgSize, int timeout,
                                     const char *fmt, ...)
  {
    DBG_ENTER("NetConnection::request");

    int exitCode = -1;

    char buf[1024];

    int cmdLen = 0;

    char *dst = NULL;

    int readed = 0;
    int total  = 0;

    int eofReceived = 0;

    int len = 0;

    va_list ap;

    //
    // Check args.
    //

    FAILEX(serverCode == NULL, "ERROR: 'serverCode' cannot be NULL in NetConnection::request.");
    FAILEX(fmt == NULL, "ERROR: 'fmt' cannot be NULL in NetConnection::request.");

    //
    // Format printf like message.
    //

    va_start(ap, fmt);

    len = vsnprintf(buf, sizeof(buf) - 1, fmt, ap);

    va_end(ap);

    //
    // Send command to server INCLUDING zero terminator byte.
    //

    FAILEX(write(buf, len + 1, timeout) < 0,
               "ERROR: Cannot send request.\n");

    //
    // Read answer from server in below format:
    // 'XYZ> <message>'
    //
    // Where <XYZ> is 3 decimal server side code e.g. "871".
    //

    //
    // Read 'XYZ> ' prefix first.
    // where XYZ is 3 decimal exit code returned by server.
    //

    FAILEX(read(buf, 5, timeout) != 5,
               "ERROR: Cannot read 'XYZ> ' prefix.");

    buf[4] = 0;

    *serverCode = atoi(buf);

    //
    // Read ASCIZ message part if needed.
    //

    if (serverMsg && serverMsgSize > 0)
    {
      dst = serverMsg;

      total = 0;

      //
      // FIXME: Avoid reading byte by byte.
      //

      while(read(dst, 1, timeout) == 1)
      {
        //
        // Caller buffer too short.
        //

        if (total == serverMsgSize)
        {
          break;
        }

        //
        // End of message, it's ordinal end.
        //

        if (dst[0] == 0)
        {
          eofReceived = 1;

          break;
        }

        total ++;

        dst ++;
      }

      serverMsg[total] = 0;
    }

    //
    // Flush remaining message from server if any.
    // This is scenario when caller message[] buffer is shorter
    // than message sent by server.
    //

    while(eofReceived == 0)
    {
      if (read(buf, 1, timeout) <= 0 || buf[0] == 0)
      {
        eofReceived = 1;
      }
    }

    exitCode = 0;

    //
    // Clean up.
    //

    fail:

    if (exitCode)
    {
      Error("Cannot send NET request.\n"
                "Error code is : %d.\n", GetLastError());
    }

    DBG_LEAVE("NetConnection::request");

    return exitCode;
  }

  //
  // Get back caller context passed to constructor if any.
  //

  const void *NetConnection::getContext()
  {
    return ctx_;
  }

  //
  // Get back protocol passed to constructor if any.
  //

  const char *NetConnection::getProtocol()
  {
    return protocol_.c_str();
  }

  //
  // Get back socket passed to constructor if any.
  //

  int NetConnection::getSocket()
  {
    return socket_;
  }

  //
  // Get back handler function passed to constructor if any.
  //

  NetHandleConnProto NetConnection::getHandler()
  {
    return handler_;
  }

  //
  // Get string with information about remote client.
  //

  const char *NetConnection::getClientInfo()
  {
    return clientInfo_.c_str();
  }

  //
  // Increase refference counter.
  //
  // WARNING! Every call to addRef() MUSTS be followed by one release() call.
  //
  // TIP #1: Object will not be destroyed until refference counter is greater
  //         than 0.
  //
  // TIP #2: Don't call destructor directly, use release() instead. If
  //         refference counter achieve 0, object will be destroyed
  //         automatically.
  //

  void NetConnection::addRef()
  {
    refCountMutex_.lock();

    refCount_ ++;

    DEBUG2("Increased refference counter to %d for NetConnection PTR#%p.\n",
               refCount_, this);

    refCountMutex_.unlock();
  }

  //
  // Decrease refference counter increased by addRef() before.
  //

  void NetConnection::release()
  {
    int deleteNeeded = 0;

    //
    // Check is this pointer correct.
    //

    if (isPointerCorrect(this) == 0)
    {
      Error("ERROR: Attemp to destroy non existing NetConnection PTR#%p.\n", this);

      return;
    }

    //
    // Decrease refference counter by 1.
    //

    refCountMutex_.lock();

    refCount_ --;

    DEBUG2("Decreased refference counter to %d for NetConnection PTR#%p.\n",
               refCount_, this);

    if (refCount_ == 0)
    {
      deleteNeeded = 1;
    }

    refCountMutex_.unlock();

    //
    // Delete object if refference counter goes down to 0.
    //

    if (deleteNeeded)
    {
      delete this;
    }
  }

  //
  // Connection's state management.
  //

  //
  // Set connection state. See NET_STATE_XXX defines in LibNet.h for
  // possible states.
  //
  // state - one of NET_STATE_XXX state defined in LibNet.h (IN).
  //
  // RETURNS: 0 if OK.
  //

  int NetConnection::setState(int state)
  {
    DBG_MSG("NetConnection PTR 0x%p changed state to [%d].\n", this, state);

    state_ = state;
  }

  //
  // Get back current connection state set by setState() before.
  //

  int NetConnection::getState()
  {
    return state_;
  }

  //
  // Wait until connection reach given state or timeout.
  //
  // state   - target state, which should be set (IN).
  // timeout - timeout limit in ms. Defaulted to infinite if -1. (IN/OPT).
  //
  // RETURNS: 0 if target status reached,
  //         -1 otherwise.
  //

  int NetConnection::waitForState(int state, int timeout)
  {
    DBG_ENTER("NetConnection::waitForState");

    int exitCode = -1;

    int timeLeft = timeout;

    DBG_MSG3("NetConnection::waitForState : Waiting for state"
                " [%d] for PTR 0x%p...\n", state, this);

    while(state != state_)
    {
      #ifdef WIN32
      Sleep(100);
      #else
      usleep(100000);
      #endif

      if (timeout > 0)
      {
        timeLeft -= 100;

        FAILEX(timeLeft <= 0, "ERROR: Timeout reached.\n")
      }

      FAIL(state_ == NET_STATE_DEAD);
    }

    //
    // Error handler.
    //

    exitCode = 0;

    fail:

    if (exitCode)
    {
      Error("ERROR: State [%d] not reached on connection PTR 0x%x.\n", state, this);
    }

    DBG_LEAVE("NetConnection::waitForState");

    return exitCode;
  }

  //
  // Disable nagle algorithm on given connection if possible.
  //
  // value - 1 to disable algo, 0 to enable it back (IN).
  //

  void NetConnection::setNoDelay(int value)
  {
  }

  //
  // Enable SO_KEEPALIVE flag, it keeps connections active by
  // enabling the periodic transmission of messages.
  //
  // interval - -1 to disable SO_KEEPALIVE, above 0 will set time
  //            in seconds between individual keepalive probes.
  //

  void NetConnection::setKeepAlive(int interval)
  {
  }

  //
  // Get back thread handle set in costructor or by setThread() later.
  //

  ThreadHandle_t *NetConnection::getThread()
  {
    return thread_;
  }

  //
  // Set thread handle associated with object.
  //
  // thread - thread handle created by threadCreate() before (IN).
  //

  void NetConnection::setThread(ThreadHandle_t *thread)
  {
    thread_ = thread;
  }

  //
  // Authroize connection if needed.
  //
  // RETURNS: 0 if OK.
  //

  int NetConnection::authorize(const void *authData, int authDataSize)
  {
    return 0;
  }

  //
  // Get authorization data needed to be passed to another side if needed.
  //
  // RETURNS: 0 if OK.
  //

  int NetConnection::getAuthData(void *authData, int authDataSize)
  {
    return 0;
  }

  //
  // Disable inherit to child process (exec).
  //
  // RETURNS: 0 if OK.
  //

  int NetConnection::disableInherit()
  {
    return -1;
  }

  //
  // Check is given this pointer points to correct NetConnection *object.
  //
  // ptr - this pointer to check (IN).
  //
  // RETURNS: 1 if given pointer points to correct net connection object,
  //          0 otherwise.
  //

  int NetConnection::isPointerCorrect(NetConnection *ptr)
  {
    int found = 0;

    instancesMutex_.lock();
    found = instances_.count(ptr);
    instancesMutex_.unlock();

    return found;
  }

  //
  // Set quiet mode to avoid printing error message longer.
  //

  void NetConnection::setQuietMode(int value)
  {
    quietMode_ = value;
  }
} /* namespace Tegenaria */
