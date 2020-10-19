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

#include <Tegenaria/Debug.h>
#include <Tegenaria/Thread.h>
#include <Tegenaria/Mutex.h>
#include <map>
#include <string>
#include <fcntl.h>

#include "Ipc.h"
#include "Utils.h"

using std::map;
using std::string;

//
// Define this to avoid TIME_WAIT warning
// on Linux.
//

#ifndef WIN32
# undef AVOID_TIMEWAIT
#endif

#ifdef WIN32
# include <io.h>
#endif

namespace Tegenaria
{
  //
  // Server state on given pipe.
  //

  static map<string, int> IpcServerState;

  static Mutex IpcServerStateMutex("IpcServerStateMutex");

  //
  // Main loop for named pipe server.
  // Can be used to set up blocking server loop in user app.
  //
  // TIP #1: To create server loop in another thread ('non-blocking') use
  //         IpcServerCreate() instead.
  //
  // TIP #2: Use IpcServerKill to close server.
  //
  // name     - pipe name to create (IN).
  // callback - handler function to process incoming connections (IN).
  // timeout  - maximum allow time to process one connection in ms (IN).
  // ctx      - caller context passed to callback function (IN/OPT).
  // ready    - set to 1 when server loop initialized (OUT/OPT).
  //
  // RETURNS: 0 if OK.
  //

  int IpcServerLoop(const char *nameIn, IpcWorkerProto callback, void *ctx, int *ready)
  {
    DBG_ENTER("IpcServerLoop");

    int exitCode = -1;

    int goOn = 1;

    int alreadyExist = 0;

    #ifdef AVOID_TIMEWAIT
    int iSetOption = 1;
    struct linger so_linger;
    so_linger.l_onoff  = 1;
    so_linger.l_linger = 0;
    #endif

    string name;

    //
    // Check args.
    //

    FAILEX(nameIn == NULL, "ERROR: 'nameIn' cannot be NULL in IpcServerLoop().\n");
    FAILEX(callback == NULL, "ERROR: 'callback' cannot be NULL in IpcServerLoop().\n");

    #ifndef WIN32
    FAILEX((strlen(nameIn) >= 107), "ERROR: 'nameIn' too long.\n");
    #endif

    name = nameIn;

    //
    // Check is server already running on given pipe.
    //

    IpcServerStateMutex.lock();
    alreadyExist = IpcServerState[name];
    IpcServerStateMutex.unlock();

    FAILEX(alreadyExist == 1,
               "ERROR: Server already running on pipe '%s'.\n", name.c_str());

    IpcServerStateMutex.lock();
    IpcServerState[name] = 1;
    IpcServerStateMutex.unlock();

    //
    // Windows.
    //

    #ifdef WIN32
    {
      char pipeName[MAX_PATH] = {0};

      HANDLE pipe    = NULL;
      HANDLE pipeAlt = NULL;

      DWORD pipeFlags = PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT;
      DWORD openMode  = PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED;

      SECURITY_ATTRIBUTES sa = {0};

      //
      // Add '\\.\pipe\' frefix to name.
      //

      strncpy(pipeName, "\\\\.\\pipe\\", sizeof(pipeName));
      strncat(pipeName, name.c_str(), sizeof(pipeName));

      //
      // Set up security attributes with read and write access for everybody.
      //

      FAIL(SetUpSecurityAttributesEverybody(&sa));

      //
      // Create named pipe.
      //

      DBG_MSG("IpcServerLoop : Creating named pipe '%s'...\n", pipeName);

      pipe = CreateNamedPipe(pipeName, openMode, pipeFlags, 1, 1024, 1024, 0, &sa);

      FAIL(pipe == INVALID_HANDLE_VALUE);

      //
      // Avoid closing underlying handle, when callback closes
      // CRT FD wrapper.
      //

      //
      // SetHandleInformation(pipe, HANDLE_FLAG_PROTECT_FROM_CLOSE,
      //                         HANDLE_FLAG_PROTECT_FROM_CLOSE);

      //
      // Enter main server loop.
      //

      DBG_INFO("Started IPC server loop on '%s'.\n", name.c_str());

      if (ready)
      {
        *ready = 1;
      }

      while(goOn == 1)
      {
        //
        // Wait for next connection.
        //

        DBG_MSG("IpcServerLoop : Waiting for connection on '%s'...\n", name.c_str());

        if (ConnectNamedPipe(pipe, NULL))
        {
          char clientMsg[8] = {0};

          char endTag[] = "<<end>>";

          HANDLE pipeDup = NULL;

          DBG_MSG("IpcServerLoop : Received connection on '%s'.", pipeName);

          //
          // Duplicate pipe handle to avoid closing original pipe,
          // when FD wrapper is closed.
          //

          DuplicateHandle(GetCurrentProcess(), pipe,
                              GetCurrentProcess(), &pipeDup,
                                  0, FALSE, DUPLICATE_SAME_ACCESS);

          //
          // Wrap native HANDLE into CRT FD to be consistent
          // with Linux/MacOS.
          //

          //
          // FIXME: Move worker to another thread.
          // FIXME: Handle worker timeout.
          //

          int fd = _open_osfhandle(intptr_t(pipeDup), 0);

          callback(fd, ctx);

          FlushFileBuffers(pipe);

          //
          // Close connection.
          //

          DBG_IO_CLOSE_BEGIN("IPC/FD", fd);
          close(fd);
          DBG_IO_CLOSE_END("IPC/FD", fd);

          DisconnectNamedPipe(pipe);
        }

        //
        // If client failed to connect close current pipe instance
        // before creating another one.
        //

        else
        {
          DBG_MSG("IpcServerLoop : Failed to connect client on '%s'."
                  "Error code is : %d.\n", pipeName, GetLastError());
        }

        //
        // Check do we need wait for another connection.
        //

        IpcServerStateMutex.lock();
        goOn = IpcServerState[name];
        IpcServerStateMutex.unlock();
      }

      CloseHandle(pipe);

      FreeSecurityAttributes(&sa);
    }

    //
    // Linux, MacOS.
    //

    #else
    {
      int fdListen = -1;
      int fdAccept = -1;

      struct sockaddr_un local, remote;

      memset(local.sun_path, 0, sizeof(local.sun_path));

      local.sun_family = AF_UNIX;

      //
      // Set first byte of sun_path to \0 to create socket in abstract namespace.
      //

      strcpy(local.sun_path + 1, name.c_str());

      DBG_MSG("IpcServerLoop : Obtaining fdListen descriptor.\n");

      if ((fdListen = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
      {
        int err = errno;

        Error("Error while obtaining fdListen. Error is: %d - %s", err, strerror(err));

        goto fail;
      }

      #ifdef AVOID_TIMEWAIT
      setsockopt(fdListen, SOL_SOCKET, SO_LINGER, &so_linger, sizeof(so_linger));
      setsockopt(fdListen, SOL_SOCKET, SO_REUSEADDR, (char*)&iSetOption, sizeof(iSetOption));
      #endif

      DBG_MSG("IpcServerLoop : Binding named socket '%s' in abstract namespace.\n", name.c_str());

      if (bind(fdListen, (struct sockaddr *) &local, sizeof(struct sockaddr_un)) == -1)
      {
        int err = errno;

        Error("Could not bind socket. Error is: %d - %s", err, strerror(err));

        goto fail;
      }

      //
      // Don't inherit listening socket.
      //

      fcntl(fdListen, F_SETFD, FD_CLOEXEC);

      //
      // Enter main server loop.
      //

      DBG_INFO("Started IPC server loop on '%s'.\n", name.c_str());

      if (ready)
      {
        *ready = 1;
      }

      while (goOn == 1)
      {
        fdAccept = -1;

        //
        // Waiting for connection for client. Second parameter means that ten conenctions
        // can be queued before we accept it.
        //

        DBG_MSG("IpcServerLoop : Listening for connections on %s...\n", name.c_str());

        if (listen(fdListen, 10) == -1)
        {
          int err = errno;

          Error("Error while listening for connection. Error is: %d - %s", err, strerror(err));

          goto fail;
        }

        unsigned int remoteSize = sizeof(remote);

        if ((fdAccept = accept(fdListen, (struct sockaddr *)&remote, &remoteSize)) == -1)
        {
          int err = errno;

          Error("Error while accepting connection. Error is: %d - %s", err, strerror(err));

          goto fail;
        }

        //
        // Passing fd to handler.
        //

        //
        // FIXME: Move worker to another thread.
        // FIXME: Handle worker timeout.
        //

        #ifdef AVOID_TIMEWAIT
        setsockopt(fdListen, SOL_SOCKET, SO_LINGER, &so_linger, sizeof(so_linger));
        setsockopt(fdListen, SOL_SOCKET, SO_REUSEADDR, (char*)&iSetOption, sizeof(iSetOption));
        #endif

        callback(fdAccept, ctx);

        shutdown(fdAccept, SHUT_RDWR);

        DBG_IO_CLOSE_BEGIN("IPC/FD", fdAccept);
        close(fdAccept);
        DBG_IO_CLOSE_END("IPC/FD", fdAccept);

        //
        // Check do we need wait for another connection.
        //

        IpcServerStateMutex.lock();
        goOn = IpcServerState[name];
        IpcServerStateMutex.unlock();
      }

      close(fdListen);
    }
    #endif

    DBG_INFO("IPC Server loop on '%s' finished.\n", name.c_str());

    //
    // Clean up.
    //

    exitCode = 0;

    fail:

    if (exitCode)
    {
      Error("ERROR: Cannot perform pipe server loop.\n"
                "Error code is : %d.\n", GetLastError());
    }

    IpcServerStateMutex.lock();
    IpcServerState[name] = 0;
    IpcServerStateMutex.unlock();

    DBG_LEAVE("IpcServerLoop");

    return exitCode;
  }

  //
  // Wrapper to pass IpcServerLoop() to threadCreate.
  // Used internally only by IpcServerCreate.
  //

  int IpcServerThreadLoop(IpcJob *job)
  {
    IpcServerLoop(job -> pipeName_, job -> callback_, job -> ctx_, &job -> ready_);
  }

  //
  // Create IPC server in another thread. Works as non-blocking IpcServerLoop().
  //
  // name     - pipe name to create (IN).
  // callback - handler function to process incoming connections (IN).
  // ctx      - caller context passed to callback function directly (IN/OPT).
  //
  // TIP #1: Use IpcServerKill to close server.
  //
  // RETURNS: 0 if OK.
  //

  int IpcServerCreate(const char *name, IpcWorkerProto callback, void *ctx)
  {
    int exitCode = -1;

    ThreadHandle_t *thread = NULL;

    int timeLeft = 5000;

    IpcJob job = {name, callback, 0, ctx};

    int alreadyExist = 0;

    //
    // Check is server already running on given pipe?
    //

    IpcServerStateMutex.lock();
    alreadyExist = IpcServerState[name];
    IpcServerStateMutex.unlock();

    FAILEX(alreadyExist,
               "ERROR: Server already listening on pipe '%s'.\n", name);

    //
    // Create server listening on given pipe in another thread.
    //

    thread = ThreadCreate(IpcServerThreadLoop, &job);

    //
    // Wait until server loop initialized.
    //

    while(job.ready_ == 0 && timeLeft > 0)
    {
      ThreadSleepMs(10);

      timeLeft -= 10;
    }

    FAILEX(timeLeft <= 0,
               "ERROR: Timeout while initializing IPC server loop on pipe '%s'.\n", name);

    //
    // Error handler.
    //

    exitCode = 0;

    if (exitCode)
    {
      Error("ERROR: Cannot create IPC server on pipe '%s'.\n", name);
    }

    fail:

    return exitCode;
  }

  //
  // Close server loop started by IpcServerLoop() or IpcServerCreate() before.
  //
  // WARNING! Do not use this function inside IPC request handler (passed
  //          to IpcServerCreate as callback). It can causes deadlock.
  //
  // TIP#1: To finish server inside IPC request handler use IpcServerFinish()
  //        instead.
  //
  // name - name passed to IpcServerXXX function before (IN).
  //
  // RETURNS: 0 if OK.
  //

  int IpcServerKill(const char *name)
  {
    //
    // Set server state to disabled.
    //

    IpcServerStateMutex.lock();
    IpcServerState[name] = -1;
    IpcServerStateMutex.unlock();

    int fd = IpcOpen(name);

    close(fd);

    DEBUG1("Pipe loop '%s' canceled.\n", name);

    return 0;
  }

  //
  // Mark pipe loop started by IpcServerLoop() or IpcServerCreate() as
  // finished. After exit from last pending IPC request server should exit
  // from server loop.
  //
  // WARNING! This function should be called from IPC Request handler (passed
  //          to IpcServerCreate as callback). It means that current IPC
  //          request is last request in loop.
  //
  // TIP#1: If you want to finish IPC loop outside IPC request handler use
  //        IpcServerKill() instead.
  //
  // name - name passed to IpcServerXXX function before (IN).
  //
  // RETURNS: 0 if OK.
  //

  int IpcServerMarkLastRequest(const char *name)
  {
    //
    // Set server state to disabled.
    //

    IpcServerStateMutex.lock();
    IpcServerState[name] = -1;
    IpcServerStateMutex.unlock();

    DEBUG1("Pipe loop '%s' marked as finished.\n", name);

    return 0;
  }

  #ifdef WIN32

  //
  // Open connection to existing named pipe server.
  //
  // name    - pipe name passed to IpcServer() in server process (IN).
  // timeout - maximum allow wait time in ms (IN/OPT).
  //
  // RETURNS: Pipe HANDLE use with WriteFile/ReadFile functions,
  //          or INVALID_HANDLE_VALUE if error.
  //

  HANDLE IpcOpenHandle(const char *name, int timeout = IPC_DEFAULT_TIMEOUT)
  {
    HANDLE pipe = INVALID_HANDLE_VALUE;

    DBG_ENTER("IpcOpenHandle");

    char pipeName[MAX_PATH];

    int connected = 0;

    //
    // Check args.
    //

    FAILEX(name == NULL, "ERROR: 'name' cannot be NULL in IpcOpen().\n");

    FAILEX((strlen(name) >= 107), "ERROR: 'name' too long.\n");

    //
    // Add '\\.\pipe\' frefix to name.
    //

    strncpy(pipeName, "\\\\.\\pipe\\", sizeof(pipeName));
    strncat(pipeName, name, sizeof(pipeName));

    //
    // Connect to named pipe.
    //

    while(connected == 0)
    {
      //
      // Try connect.
      //

      DBG_MSG("IpcOpenHandle : Connecting to pipe '%s'...\n", pipeName);

      pipe = CreateFile(pipeName, GENERIC_READ | GENERIC_WRITE,
                          0, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);

      //
      // Success - go out.
      //

      if (pipe != INVALID_HANDLE_VALUE)
      {
        DBG_MSG("IpcOpenHandle : Connected to '%s'.\n", pipeName);

        connected = 1;
      }
      else
      {
        FAIL(GetLastError() != ERROR_PIPE_BUSY);

        //
        // Server is busy. Wait up to specified timeout.
        //

        DBG_MSG("IpcOpenHandle : Server is busy. Waiting up to %d ms...\n", timeout);

        WaitNamedPipe(pipeName, timeout);
      }
    }

    fail:

    return pipe;
  }

  #endif

  //
  // Open connection to existing pipe server.
  //
  // name    - pipe name passed to IpcServer() in server process (IN).
  // timeout - maximum allow wait time in ms (IN/OPT).
  //
  // RETURNS: CRT descriptor (fd) ready to use with write/read funcions,
  //          or -1 if error.
  //

  int IpcOpen(const char *name, int timeout)
  {
    DBG_ENTER("IpcOpen");

    int fd = -1;

    #ifdef AVOID_TIMEWAIT
    int iSetOption = 1;
    struct linger so_linger;
    so_linger.l_onoff  = 1;
    so_linger.l_linger = 0;
    #endif

    //
    // Check args.
    //

    FAILEX(name == NULL, "ERROR: 'name' cannot be NULL in IpcOpen().\n");

    //
    // Windows.
    //

    #ifdef WIN32
    {
      HANDLE pipe = IpcOpenHandle(name, timeout);

      FAIL(pipe == INVALID_HANDLE_VALUE);

      //
      // Connected. Wrap native HANDLE into CRT fd to be
      // consistent with Linux/MacOS.
      //

      fd = _open_osfhandle(intptr_t(pipe), 0);

      DBG_MSG("IpcOpen : Connection on '%s' wrapped to fd '%d'.\n", name, fd);
    }

    //
    // Linux, MacOS.
    //

    #else
    {
      struct sockaddr_un sock;

      memset(sock.sun_path, 0, sizeof(sock.sun_path));

      sock.sun_family = AF_UNIX;

      //
      // Set first byte of sun_path to \0 to create socket in abstract namespace.
      //

      strcpy(sock.sun_path + 1, name);

      DBG_MSG("IpcServerLoop : Obtaining fd for socket '%s' in abstract namespace.\n", name);

      if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
      {
        int err = errno;

        Error("Error while obtaining fd. Error is: %d - %s", err, strerror(err));

        goto fail;
      }

      #ifdef AVOID_TIMEWAIT
      setsockopt(fd, SOL_SOCKET, SO_LINGER, &so_linger, sizeof(so_linger));
      setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char*)&iSetOption, sizeof(iSetOption));
      #endif

      DBG_MSG("IpcServerLoop : Connecting to socket.\n", name);

      //
      // FIXME: Handle timeout.
      //

      if (connect(fd, (struct sockaddr *)&sock, sizeof(sockaddr_un)) == -1)
      {
        int err = errno;

        Error("Error while connecting to server. Error is: %d - %s", err, strerror(err));

        goto fail;
      }
    }
    #endif

    //
    // Error handler.
    //

    fail:

    if (fd == -1)
    {
      Error("ERROR: Cannot open connection to pipe server '%s'.\n"
                "Error code is : %d.\n", name, GetLastError());
    }

    DBG_LEAVE("IpcOpen");

    return fd;
  }

  //
  // - Send single request to pipe server
  // - read answer in format 'XYZ > message'
  // - split answer to <XYZ> code and <message> parts.
  //
  // Example usage:
  //
  // IpcRequest(PIPENAME, &code, answer, sizeof(answer),
  //                "share --alias %s --path %s", alias, path);
  //
  // TIP: If only exit code is needed <answer> can be set to NULL.
  //
  // pipeName      - pipe name, where to send command (IN).
  // serverCode    - exit code returned by server (OUT/OPT).
  // serverMsg     - ASCIZ message returned by server (OUT/OPT).
  // serverMsgSize - size of answer buffer in bytes (IN/OPT).
  // fmt           - printf like parameters to format command to send (IN).
  //
  // RETURNS: 0 if request sucessfuly sent and asnwer from server received.
  //         -1 otherwise.
  //
  // WARNING!: Request could still failed on server side.
  //           To get server's side exit code use 'answerCode' parameter.
  //

  int IpcRequest(const char *pipeName, int *serverCode,
                     char *serverMsg, int serverMsgSize, const char *fmt, ...)
  {
    DBG_ENTER("IpcRequest");

    int exitCode = -1;

    char buf[IPC_LEN_MAX];

    int cmdLen = 0;

    char *dst = NULL;

    int readed = 0;
    int total  = 0;

    int len = 0;

    va_list ap;

    int fd = -1;

    //
    // Windows variables.
    //

    #ifdef WIN32

    HANDLE pipe = INVALID_HANDLE_VALUE;

    OVERLAPPED ov = {0};

    DWORD written = 0;

    //
    // Linux variables.
    //

    #else

    char buffer[1024];

    fd_set rfd;

    struct timeval tv;

    #endif


    //
    // Check args.
    //

    FAILEX(pipeName == NULL, "ERROR: 'pipeName' cannot be NULL in IpcRequest.");
    FAILEX(fmt == NULL, "ERROR: 'fmt' cannot be NULL in IpcRequest.");

    //
    // Clear output buffers to avoid returning random data
    // on error.
    //

    if (serverCode)
    {
      *serverCode = -1;
    }

    if (serverMsg && serverMsgSize)
    {
      serverMsg[0] = 0;
    }

    //
    // Windows.
    //

    #ifdef WIN32
    {
      //
      // Open connection to pipe server living on the same machine.
      //

      pipe = IpcOpenHandle(pipeName);

      FAILEX(pipe == INVALID_HANDLE_VALUE, "ERROR: Cannot connect to [%s].\n", pipeName);

      //
      // Format printf like message.
      //

      va_start(ap, fmt);

      len = vsnprintf(buf, IPC_LEN_MAX - 1, fmt, ap);

      va_end(ap);

      //
      // Send command to server.
      //

      FAILEX(WriteFile(pipe, buf, len, &written, NULL) == FALSE,
               "ERROR: Cannot send command to [%s].\n", pipeName);

      //
      // Read 'XYZ> ' prefix first.
      // where XYZ is 3 decimal exit code returned by server.
      //

      TimeoutReadFileOv(pipe, buf, 5, &readed, 60000);

      FAILEX(readed != 5, "ERROR: Cannot read 'XYZ> ' prefix from [%s].", pipeName);

      buf[4] = 0;

      if (serverCode)
      {
        *serverCode = atoi(buf);
      }

      //
      // Read message part if needed.
      //

      if (serverMsg && serverMsgSize > 0)
      {
        int goOn = 1;

        dst = serverMsg;

        total = 0;

        //
        // Read data until something to read.
        //

        while(goOn)
        {
          goOn = TimeoutReadFileOv(pipe, dst,
                                       serverMsgSize - 1 - total,
                                           &readed, 30000);

          DBG_MSG("IpcRequest : Readed [%d] bytes from pipe [%s].\n", readed, pipeName);

          total += readed;

          dst += readed;
        }

        serverMsg[total] = 0;
      }
    }

    //
    // Linux.
    //

    #else
    {
      //
      // Open connection to pipe server living on the same machine.
      //

      fd = IpcOpen(pipeName);

      FAILEX(fd < 0, "ERROR: Cannot connect to [%s].\n", pipeName);

      //
      // Format printf like message.
      //

      va_start(ap, fmt);

      len = vsnprintf(buf, IPC_LEN_MAX - 1, fmt, ap);

      va_end(ap);

      //
      // Send command to server.
      //

      FAILEX(write(fd, buf, len) < 0,
               "ERROR: Cannot send command to [%s].\n", pipeName);

      //
      // Read 'XYZ >' prefix first.
      // where XYZ is 3 decimal exit code returned by server.
      //

      FAILEX(TimeoutReadSelect(fd, buf, 5, 60) != 5,
                 "ERROR: Cannot read 'XYZ> prefix ' from [%s].\n", pipeName);

      buf[4] = 0;

      if (serverCode)
      {
        *serverCode = atoi(buf);
      }

      //
      // Read message part if needed.
      //

      if (serverMsg && serverMsgSize > 0)
      {
        dst = serverMsg;

        total = 0;

        readed = 1;

        while(readed > 0)
        {
          readed = TimeoutReadSelect(fd, dst, serverMsgSize - 1 - total, 60);

          DBG_MSG("IpcRequest : Readed [%d] bytes from pipe [%s].\n", readed, pipeName);

          if (readed > 0)
          {
            total += readed;

            dst += readed;
          }
        }

        serverMsg[total] = 0;
      }
    }
    #endif

    exitCode = 0;

    //
    // Clean up.
    //

    fail:

    if (exitCode)
    {
      Error("Cannot send IPC request to [%s].\n"
                "Error code is : %d.\n", pipeName, GetLastError());

      if (serverCode)
      {
        *serverCode = -1;
      }
    }

    #ifdef WIN32
    CloseHandle(pipe);
    #else
    close(fd);
    #endif

    DBG_LEAVE("IpcRequest");

    return exitCode;
  }

  //
  // Format 'XYZ> message' string and send it to given CRT FD.
  //
  // fd   - CRT FD, where to send answer (IN).
  // code - 3 digit answer code (e.g. 100) (IN).
  // msg  - Optional, ASCIZ message to send, can be NULL (IN/OPT).
  //
  // RETURNS: 0 if OK.
  //

  int IpcSendAnswer(int fd, int code, const char *msg)
  {
    DBG_ENTER3("IpcSendRequestAnswer");

    int exitCode = -1;

    char buf[8] = {0};

    int written = -1;

    int msgSize = -1;

    //
    // Send 5-bytes "XYZ> " prefix.
    //

    snprintf(buf, sizeof(buf) - 1, "%03d> ", code);

    DBG_IO_WRITE_BEGIN("IPC/FD", fd, buf, 5);
    {
      written = write(fd, buf, 5);
    }
    DBG_IO_WRITE_END("IPC/FD", fd, buf, written);

    FAILEX(written != 5, "ERROR: Cannot write to IPC/FD #%d.\n", fd);

    //
    // Send message including zero terminator.
    //

    if (msg)
    {
      msgSize = strlen(msg) + 1;

      DBG_IO_WRITE_BEGIN("IPC/FD", fd, msg, msgSize);
      {
        written = write(fd, msg, msgSize);
      }
      DBG_IO_WRITE_END("IPC/FD", fd, msg, written);
    }

    FAILEX(written != msgSize, "ERROR: Cannot write to IPC/FD #%d.\n", fd);

    //
    // Error handler.
    //

    exitCode = 0;

    fail:

    DBG_LEAVE3("IpcSendRequestAnswer");

    return exitCode;
  }
} /* namespace Tegenaria */
