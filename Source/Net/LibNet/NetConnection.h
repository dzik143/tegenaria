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

#ifndef Tegenaria_Core_NetConnection_H
#define Tegenaria_Core_NetConnection_H

#include <cstdlib>
#include <stdarg.h>
#include <string>
#include <set>

#include <Tegenaria/Thread.h>
#include <Tegenaria/Mutex.h>
#include <Tegenaria/Debug.h>

#include "Net.h"

namespace Tegenaria
{
  using std::string;
  using std::set;

  class NetConnection
  {
    protected:

    const void *ctx_;

    string protocol_;

    string clientInfo_;

    int socket_;

    NetHandleConnProto handler_;

    int refCount_;

    int state_;
    int quietMode_;

    ThreadHandle_t *thread_;

    //
    // Protected desctructor.
    // Use release() instead to handle refference counter.
    //

    virtual ~NetConnection();

    public:

    //
    // Constructors.
    //

    NetConnection(const void *ctx, const char *protocol,
                      int socket = -1, NetHandleConnProto handler = NULL,
                          ThreadHandle_t *thread = NULL);

    //
    // Refference counter.
    //

    void addRef();
    void release();

    Mutex refCountMutex_;

    //
    // Track created instances to check is given this pointer correct or not.
    //

    static set<NetConnection *> instances_;

    static Mutex instancesMutex_;

    static int isPointerCorrect(NetConnection *ptr);

    //
    // Get property.
    //

    const void *getContext();

    virtual const char *getProtocol();

    virtual const char *getClientInfo();

    virtual int getSocket();

    NetHandleConnProto getHandler();

    ThreadHandle_t *getThread();

    //
    // Set property.
    //

    void setThread(ThreadHandle_t *thread);

    void setQuietMode(int value);

    //
    // Authorization/hash exchange function.
    //

    virtual int authorize(const void *authData, int authDataSize);

    virtual int getAuthData(void *localHash, int localHashSize);

    //
    // Low level I/O.
    //

    virtual int write(const void *buf, int count, int timeout = -1);
    virtual int read(void *buf, int count, int timeout = -1);
    virtual void cancel();

    virtual int shutdown(int how = SD_BOTH);

    virtual int join();

    virtual void setNoDelay(int value);
    virtual void setKeepAlive(int interval);

    static int readCallback(void *buf, int count, int timeout, void *ctx);
    static int writeCallback(void *buf, int count, int timeout, void *ctx);
    static void cancelCallback(void *ctx);

    //
    // High level I/O.
    //

    int request(int *serverCode, char *serverMsg, int serverMsgSize,
                    int timeout, const char *fmt, ...);

    int setState(int state);
    int getState();
    int waitForState(int state, int timeout = -1);

    //
    // Disable inherit in child process (exec).
    //

    virtual int disableInherit();
  };

} /* namespace Tegenaria */

#endif /* Tegenaria_Core_NetConnection */
