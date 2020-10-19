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

#ifndef Tegenaria_Core_Net_H
#define Tegenaria_Core_Net_H

//
// Includes.
//

#include <cstdlib>
#include <stdarg.h>
#include <cstring>
#include <string>
#include <vector>

#include <Tegenaria/Thread.h>
#include <Tegenaria/Debug.h>

#include "NetHpServer.h"
#include "NetStatistics.h"

#ifndef WIN32
# include <unistd.h>
# include <fcntl.h>
# include <netinet/tcp.h>
#endif

using std::string;
using std::vector;

//
// Defines.
//

#define NET_STATE_DEAD       -1
#define NET_STATE_PENDING     1
#define NET_STATE_LISTENING   2
#define NET_STATE_ESTABLISHED 3

//
// Typedef.
//

namespace Tegenaria
{
  class NetConnection;

  typedef int (*NetHandleConnProto)(NetConnection *nc);

  //
  // Structs.
  //

  struct NetUpnpInfo
  {
    char localIp_[32];
    char publicIp_[32];
    char gatewayIp_[32];

    int port_;
  };

} /* namespace Tegenaria */

//
// Windows includes.
//

#ifdef WIN32

  #include <WinSock2.h>
  #include <stdint.h>
  #include <io.h>

  //
  // Hide differences beetwen WinSock and BSD defines.
  //

  #define SHUT_RDWR SD_BOTH
  #define SHUT_RD   SD_RECEIVE
  #define SHUT_WR   SD_SEND

  typedef int socklen_t;

//
// Linux, MacOS includes.
//

#else

  typedef int SOCKET;

  //
  // Hide differences beetwen WinSock and BSD defines.
  //

  #define SD_BOTH        SHUT_RDWR
  #define SD_RECEIVE     SHUT_RD
  #define SD_SEND        SHUT_WR
  #define closesocket(X) close(X)

  #include <unistd.h>
  #include <sys/types.h>
  #include <sys/socket.h>
  #include <netinet/in.h>
  #include <arpa/inet.h>

#endif

#include "NetConnection.h"

namespace Tegenaria
{
  //
  // Server side functions.
  //

  NetConnection *NetServerCreate(int port, NetHandleConnProto handler,
                                     void *custom = NULL);

  int NetServerTerminate(NetConnection *serverConn);

  int NetServerLoop(NetConnection *serverConn);

  NetConnection *NetAccept(int port, int timeout = -1);

  //
  // Client side functions.
  //

  NetConnection *NetConnect(const char *ip, int port, int timeout = 10000);

  int NetRequest(int fd[2], int *serverCode, char *serverMsg,
                     int serverMsgSize, const char *fmt, ...);

  //
  // Universal Plug and play (upnp).
  //

  int NetUpnpGetInfo(NetUpnpInfo *info);

  int NetUpnpOpenPort(NetUpnpInfo *info, int port);

  int NetUpnpClosePort(int port);

  int NetUpnpGetFreePort();

  //
  // General.
  //

  int NetParseAddress(char *ip1, char *ip2, int *port, const char *address);

  const string NetGetPortState(int port, const char *protocol);

  int NetIsPortOpen(int port, const char *protocol);

  int NetIsPortClosed(int port, const char *protocol);

  int NetGetLocalIp(char *ip, int ipSize);

  int NetGetFreePort(const char *protocol);

  int NetResolveIp(vector<string> &ips, const char *host);

  //
  // SMTP client.
  //

  int NetSmtpSendMail(const char *smtpHost, int smtpPort, const char *from,
                              const char *fromFull, const char *subject,
                                  vector<string> receivers, const char *message,
                                      const char *login, const char *password);

  //
  // Firewall rules.
  //

  int NetFirewallRuleAdd(const char *name, const char *group,
                             const char *desc, const char *appPath, int protocol);

  int NetFirewallRuleDel(const char *name);

  //
  // General utils functions.
  //

  int NetTryBind(int port);

  int NetSetBlockMode(int sock);
  int NetSetNonBlockMode(int sock);

  string NetBase64(const char *data, int size);

} /* namespace Tegenaria */

#endif /* Tegenaria_Core_LibNet_H */
