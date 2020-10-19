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

#ifndef Tegenaria_Core_LibNetEx_H
#define Tegenaria_Core_LibNetEx_H

#include <string>

using std::string;

//
// Defines.
//

//
// Define to enable NetExSecureServerLoop() to support TLS based server.
//

#define NET_EX_USE_LIBSECURE

//
// Define to enable extra debug code to track create/destroy of server
// contexts. Debug purpose only.
//

#undef  NET_EX_CHECK_CTX

//
// Include LibSecure to handle secure TLS connection.
//

#ifdef NET_EX_USE_LIBSECURE
# include <Tegenaria/Secure.h>
#endif

namespace Tegenaria
{
  //
  // Forward declaration.
  //

  struct NetExHpContext;

  //
  // Typedef.
  //

  typedef void (*NetExHpOpenProto)(NetExHpContext *ctx);
  typedef void (*NetExHpCloseProto)(NetExHpContext *ctx);
  typedef void (*NetExHpDataProto)(NetExHpContext *ctx, void *buf, int len);

  //
  // Structs.
  //

  struct NetExHpContext
  {
    void *custom_;
    void *eventBase_;
    void *eventBuffer_;

    int workerNo_;

    NetExHpOpenProto openHandler_;
    NetExHpCloseProto closeHandler_;
    NetExHpDataProto dataHandler_;

    #ifdef NET_EX_USE_LIBSECURE
    SecureConnection *sc_;

    char *secureCert_;
    char *securePrivKey_;
    char *securePrivKeyPass_;
    #endif

    char clientIp_[16];
  };

  //
  // Exported functions.
  //

  int NetExHpServerLoop(int port, NetExHpOpenProto openHandler,
                            NetExHpCloseProto closeHandler,
                                NetExHpDataProto dataHandler);

  int NetExHpSecureServerLoop(int port, NetExHpOpenProto openHandler,
                                  NetExHpCloseProto closeHandler,
                                      NetExHpDataProto dataHandler,
                                          const char *cert,
                                              const char *privKey,
                                                  const char *privKeyPass);

  int NetExHpWrite(NetExHpContext *ctx, void *buf, int len);

} /* namespace Tegenaria */

#endif /* Tegenaria_Core_LibNetEx_H */
