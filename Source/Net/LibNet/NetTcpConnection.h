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

#ifndef Tegenaria_Core_NetTcpConnection_H
#define Tegenaria_Core_NetTcpConnection_H

#include "NetConnection.h"

//
// Windows.
//

#ifdef WIN32
# include <windows.h>

# ifndef SIO_KEEPALIVE_VALS
# define SIO_KEEPALIVE_VALS  0x98000004
# endif

struct tcp_keepalive
{
  u_long onoff;
  u_long keepalivetime;
  u_long keepaliveinterval;
};

//
// Linux.
//

#else
# include <unistd.h>
#endif

namespace Tegenaria
{
  //
  // Class to wrap TCP/IP socket.
  //

  class NetTcpConnection : public NetConnection
  {
    private:

    struct sockaddr_in addr_;

    #ifdef WIN32
    HANDLE cancelEvent_;
    OVERLAPPED ov_;
    #else
    int cancelPipe_[2];
    #endif

    virtual ~NetTcpConnection();

    public:

    NetTcpConnection(const void *ctx, int socket,
                         NetHandleConnProto handler,
                             struct sockaddr_in addr,
                                 ThreadHandle_t *thread = NULL);

    const struct sockaddr_in getAddr();

    virtual int write(const void *buf, int count, int timeout = -1);
    virtual int read(void *buf, int count, int timeout = -1);
    virtual void cancel();

    virtual int shutdown(int how = SD_BOTH);

    virtual void setNoDelay(int value);
    virtual void setKeepAlive(int interval);

    virtual int disableInherit();
  };

} /* namespace Tegenaria */

#endif /* Tegenaria_Core_NetTcpConnection */
