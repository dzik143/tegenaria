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

#pragma qcbuild_set_file_title("General utils")

#include "Net.h"
#include "NetInternal.h"
#include "Utils.h"

#ifndef WIN32
# include <sys/ioctl.h>
# include <net/if.h>
# include <netinet/in.h>
# include <arpa/inet.h>
# include <unistd.h>
#endif

#include <ctime>
#include <cstdlib>

namespace Tegenaria
{
  //
  // Initialize WinSock2 on Windows.
  // Called internally.
  //
  // RETURNS: 0 if OK.
  //

  int _NetInit()
  {
    #pragma qcbuild_set_private(1)

    DBG_ENTER3("NetInit");

    int exitCode = -1;

    static int initOk = 0;

    //
    // Call WsaStartup only at first call.
    //

    if (initOk == 0)
    {
      //
      // Initialize WinSock 2.2 on Windows.
      //

      #ifdef WIN32

      WSADATA wsaData = {0};

      WORD version = MAKEWORD(2, 2);

      FAILEX(WSAStartup(version, &wsaData),
                 "ERROR: Cannot initialize WinSock DLL.");
      #endif

      //
      // Init randomize seed.
      //

      srand(time(0));

      initOk = 1;
    }

    exitCode = 0;

    fail:

    DBG_LEAVE3("NetInit");

    return exitCode;
  }

  //
  // Split network address string into ip and port parts.
  //
  // WARNING: ip1, ip2 buffers MUST have at least 16 bytes length
  //          if specified.
  //
  // ip1     - buffer, where to store recognized ip (eg. "1.2.3.4").
  //           Can be NULL. (OUT/OPT).
  //
  // ip2     - buffer where to store second ip if UPNP scheme detected.
  //           Set to empty string if UPNP not detected. Can be NULL. (OUT/OPT).
  //
  // port    - recognized port number. Can be NULL. (OUT/OPT).
  //
  // address - input address to parse e.g. "127.0.0.1:80" (IN).
  //
  // RETURNS: 0 if OK,
  //         -1 if string not recognized as net address.
  //

  int NetParseAddress(char *ip1, char *ip2, int *port, const char *address)
  {
    DBG_ENTER("NetParseAddress");

    int exitCode = -1;

    string addressString;

    char *p     = NULL;
    char *delim = NULL;
    char *colon = NULL;

    //
    // Check args.
    //

    FAILEX(address == NULL, "ERROR: 'addr' cannot be NULL in NetParseAddress.\n");

    //
    // Parse address.
    //

    addressString = address;

    p = &addressString[0];

    delim = strchr(p, ':');
    colon = strchr(p, ',');

    //
    // UPNP "ip1,ip2:port" scheme.
    //

    if (delim && colon)
    {
      *delim = 0;
      *colon = 0;

      DBG_MSG("NetParseAddress : Recognized [%s] as UPNP pair "
                  "[%s][%s] port [%s].\n", address, p, colon + 1, delim + 1);

      if (ip1)
      {
        strcpy(ip1, p);
      }

      if (ip2)
      {
        strcpy(ip2, colon + 1);
      }

      if (port)
      {
        *port = atoi(delim + 1);
      }
    }

    //
    // "ip:port" scheme.
    //

    else if (delim)
    {
      *delim = 0;

      DBG_MSG("NetParseAddress : Recognized [%s]"
                  " as [%s] port [%s].", address, p, delim + 1);

      if (ip1)
      {
        strcpy(ip1, p);
      }

      if (ip2)
      {
        ip2[0] = 0;
      }

      if (port)
      {
        *port = atoi(delim + 1);
      }
    }
    else
    {
      goto fail;
    }

    //
    // Error handler.
    //

    exitCode = 0;

    fail:

    if (exitCode)
    {
      Error("ERROR: Cannot parse network address [%s].\n", address);
    }

    DBG_LEAVE("NetParseAddress");

    return exitCode;
  }

  //
  // Run underlying netstat command and return state string of
  // given network port.
  //
  // port     - port number check e.g. 80 (IN).
  // protocol - protocol string, defaulted to TCP (IN/OPT).
  //
  // RETURNS: Status string e.g. TIME_WAIT or
  //          "-" if port not opened or error.
  //

  const string NetGetPortState(int port, const char *protocol)
  {
    DBG_ENTER("NetGetPortState");

    char line[1024];
    char pattern[64];
    char command[128];

    char state[128] = "-";

    FILE *f = NULL;

    _NetInit();

    //
    // Run:
    //
    // netstat -abn -p <protocol> on Widows
    // netstat -apn -p <protocol> on Linux
    //

    #ifdef WIN32
    snprintf(command, sizeof(command) - 1, "netstat -an -p %s", protocol);
    #else
    snprintf(command, sizeof(command) - 1, "netstat -an -p %s 2>/dev/null", protocol);
    #endif

    f = popen(command, "r");

    FAILEX(f == NULL, "ERROR: Can't execute netstat command.\n");
    FAILEX(port == 0, "ERROR: Port can't be 0 in NetGetPortState.\n");

    //
    // Read netstat result line by line and search for given port.
    //

    snprintf(pattern, sizeof(pattern) - 1, ":%d ", port);

    while(fgets(line, sizeof(line), f))
    {
      //
      // Port found, get status string.
      //

      if (strstr(line, pattern))
      {
        #ifdef WIN32
        sscanf(line, "%s%s%s%s", state, state, state, state);
        #else
        sscanf(line, "%s%s%s%s%s%s", state, state, state, state, state, state);
        #endif
      }
    }

    fail:

    DBG_LEAVE("NetGetPortState");

    return state;
  }

  //
  // Check is given port opened in any state.
  //
  // port     - port number to check e.g. 80 (IN).
  // protocol - protocol string, defaulted to TCP (IN/OPT).
  //
  // RETURNS: 1 if port opened,
  //          0 if port NOT opened or error.
  //

  int NetIsPortOpen(int port, const char *protocol)
  {
    _NetInit();

    string state = NetGetPortState(port, protocol);

    if (state != "-")
    {
      return 1;
    }
    else
    {
      return 0;
    }
  }

  //
  // Find random unused port.
  //
  // protocol - protocol string, defaulted to "TCP" if skipped (IN/OPT).
  //
  // RETURNS: First found unused port or
  //          -1 if error.
  //

  int NetGetFreePort(const char *protocol)
  {
    _NetInit();

    for (int i = 0; i < 128; i++)
    {
      int port = 1024 + rand() % (65536 - 1024);

      if (NetTryBind(port) == 0)
      {
        DEBUG2("NetGetFreePort: Free %s port found at [%d].\n", protocol, port);

        return port;
      }
    }

    Error("ERROR: Cannot found free %s port.\n", protocol);

    return -1;
  }

  //
  // Check is given port closed in any state.
  //
  // port     - port number to check e.g. 80 (IN).
  // protocol - protocol string, defaulted to TCP (IN/OPT).
  //
  // RETURNS: 1 if port opened,
  //          0 if port NOT opened or error.
  //

  int NetIsPortClosed(int port, const char *protocol)
  {
    string state = NetGetPortState(port, protocol);

    if (state == "-" ||
            strstr(state.c_str(), "TIME_WAIT") ||
                strstr(state.c_str(), "CZAS_OCZEK"))
    {
      return 1;
    }
    else
    {
      return 0;
    }
  }

  //
  // Set limit of maximum opened FD for current user.
  //
  // limit - new limit to set (IN).
  //
  // RETURNS: 0 if OK.
  //

  int NetSetFDsLimit(int limit)
  {
    DBG_ENTER("NetSetFDsLimit");

    int exitCode = -1;

    //
    // Windows.
    //

    #ifdef WIN32
    {
      Error("NetSetFDsLimit not implemented.\n");
    }

    //
    // Linux.
    //

    #else

    //
    // Check and change if needed the per-user limit of open files
    //

    struct rlimit limitstruct;

    if(getrlimit(RLIMIT_NOFILE,&limitstruct) == -1)
    {
      fprintf(stderr, "Could not establish user limits of open files.");

      goto fail;
    }

    DBG_MSG("Polled user limits for maximum number of open files:"
                "  soft: %d; hard: %d\n", (int) limitstruct.rlim_cur, (int) limitstruct.rlim_max);

    if(limitstruct.rlim_max < limit)
    {
      // The maximum value of the maximum number of open files is currently to low.
      // We can try to increase this, but this probably will only work as root.
      // A better durable solution is to use the /etc/security/limits.conf
      //
      // Attempt to increase the limits

      limitstruct.rlim_cur = limit;
      limitstruct.rlim_max = limit;

      if(setrlimit(RLIMIT_NOFILE,&limitstruct) == -1)
      {
        fprintf(stderr, "Could not increase hard user limit of open files to %d.\n"
                        "You can either try to run this program as root, or more recommended,\n"
                        "change the user limits on the system (e.g. /etc/security.limits.conf)\n", limit);

        goto fail;
      }

      DBG_MSG("Changed hard & soft limit to %d.\n", limit);
    }
    else if (limitstruct.rlim_cur < limit)
    {
      //
      // The maximum limit is high enough, but the current limit might not be.
      // We should be able to increase this.
      //

      limitstruct.rlim_cur = limit;

      if(setrlimit(RLIMIT_NOFILE,&limitstruct) == -1)
      {
        fprintf(stderr, "Could not increase soft user limit of open files to %d.\n"
                        "You can either try to run this program as root, or more recommended,\n"
                        "change the user limits on the system (e.g. /etc/security.limits.conf)\n",limit);

        goto fail;
      }

      DBG_MSG("Changed soft limit to %d\n", limit);
    }
    else
    {
      DBG_MSG("Limit was high enough\n");
    }

    exitCode = 0;

    #endif

    //
    // Error handler.
    //

    fail:

    if (exitCode)
    {
      fprintf(stderr, "ERROR: Cannot set up open FDs limit to %d.\n", limit);
    }

    DBG_LEAVE("NetSetFDsLimit");

    return exitCode;
  }

  //
  // Return number of CPU cores installed on system.
  //

  int NetGetCpuNumber()
  {
    DBG_ENTER("NetGetCpuNumber");

    static int cpuCount = -1;

    //
    // Windows.
    //

    #ifdef WIN32
    {
      if (cpuCount == -1)
      {
        SYSTEM_INFO si = {0};

        GetSystemInfo(&si);

        cpuCount = si.dwNumberOfProcessors;
      }
    }

    //
    // Linux, MacOS.
    //

    #else
    {
      cpuCount = 1;//sysconf(_SC_NPROCESSORS_ONLN);
    }
    #endif

    DBG_LEAVE("NetGetCpuNumber");

    return cpuCount;
  }

  //
  // Retrieve IP of current running machine e.g. 10.0.0.14.
  //
  // ip     - buffer, where to store retrieved ip of local machine (OUT).
  // ipSize - size of ip[] buffer in bytes (IN).
  //
  // RETURNS: 0 if OK.
  //

  int NetGetLocalIp(char *ip, int ipSize)
  {
    DBG_ENTER("NetGetLocalIp");

    int exitCode = -1;

    int sd = -1;

    _NetInit();

    //
    // Check args.
    //

    FAILEX(ip == NULL,  "ERROR: ip parameter cannot be NULL in NetGetLocalIp().\n");
    FAILEX(ipSize <= 0, "ERROR: ipSize parameter cannot be <= 0 in NetGetLocalIp().\n");

    ip[0] = 0;

    //
    // Init LibNet if needed.
    //

    _NetInit();

    //
    // Windows.
    //

    #ifdef WIN32
    {
      char hostName[1024] = {0};

      struct hostent *host = NULL;

      //
      // Get name of current running machine.
      //

      FAILEX(gethostname(hostName, sizeof(hostName)) != 0,
                 "ERROR: Cannot get name of current running machine.\n");

      //
      // Convert host name to host struct.
      //

      host = gethostbyname(hostName);

      FAILEX(host == NULL, "ERROR: Cannot import host '%s'.\n", hostName);

      //
      // Put retrieved ip into caller buffer.
      //

      strncpy(ip, inet_ntoa(*(struct in_addr *) *host -> h_addr_list), ipSize - 1);
    }

    //
    // Linux, MacOS.
    //

    #else
    {
      struct ifreq ifreqs[32];

      struct ifconf ifconf = {0};

      ifconf.ifc_req = ifreqs;
      ifconf.ifc_len = sizeof(ifreqs);

      //
      // Create PF_INET socket.
      //

      sd = socket(PF_INET, SOCK_STREAM, 0);

      FAILEX(sd < 0, "ERROR: Cannot create PF_INET socket.\n");

      //
      // Get ifconfig from sd socket.
      //

      FAILEX(ioctl(sd, SIOCGIFCONF, (char *) &ifconf) != 0,
                 "ERROR: Cannot send SIOCGIFCONF ioctl to %d socket.\n", sd);

      //
      // Iterate over interfaces.
      //

      for(int i = 0; i < ifconf.ifc_len / sizeof(struct ifreq); i++)
      {
        struct ifreq ifreq;

        strcpy(ifreq.ifr_name, ifreqs[i].ifr_name);

        //
        // Get interface flags.
        //

        FAILEX(ioctl(sd, SIOCGIFFLAGS, (char *) &ifreq) != 0,
                   "ERROR: Cannot send SIOCGIFFLAGS to %d socket.\n", sd);

        //
        // Skip loopback devices like 127.0.0.1.
        //

        if ((ifreq.ifr_flags & IFF_LOOPBACK) == 0)
        {
          //
          // Put interface IP to caller buffer.
          //

          strncpy(ip, inet_ntoa(((struct sockaddr_in *) &ifreqs[i].ifr_addr) -> sin_addr), ipSize - 1);

          break;
        }
      }
    }
    #endif

    DEBUG3("NetGetLocalIp : Local IP is [%s].\n", ip);

    //
    // Clean up.
    //

    exitCode = 0;

    fail:

    if (exitCode)
    {
      Error("ERROR: Cannot retrieve IP of current running machine."
                " Error is %d.\n", GetLastError());
    }

    if (sd > 0)
    {
      close(sd);
    }

    return exitCode;
  }

  //
  // Switch socket to non-block mode.
  //
  // RETURNS: 0 if OK.

  int NetSetNonBlockMode(int sock)
  {
    DBG_ENTER("NetSetNonBlockMode");

    int exitCode = -1;

    #ifdef WIN32
    {
      u_long nonblock = 1;

      FAILEX(ioctlsocket(sock, FIONBIO, &nonblock),
                 "ERROR: Cannot set non-block mode on SOCKET #%d.\n", sock);
    }
    #else
    {
      long arg = 0;

      arg = fcntl(sock, F_GETFL, NULL) | O_NONBLOCK;

      FAILEX(fcntl(sock, F_SETFL, arg),
                 "ERROR: Cannot set non-block mode on SOCKET #%d.\n", sock);
    }
    #endif

    //
    // Error handler.
    //

    exitCode = 0;

    fail:

    DBG_LEAVE("NetSetNonBlockMode");

    return exitCode;
  }

  //
  // Switch socket to block mode.
  //
  // RETURNS: 0 if OK.

  int NetSetBlockMode(int sock)
  {
    DBG_ENTER("NetSetBlockMode");

    int exitCode = -1;

    #ifdef WIN32
    {
      u_long nonblock = 0;

      FAILEX(ioctlsocket(sock, FIONBIO, &nonblock),
                 "ERROR: Cannot set block mode on SOCKET #%d.\n", sock);
    }
    #else
    {
      long arg = 0;

      arg = fcntl(sock, F_GETFL, NULL) & ~O_NONBLOCK;

      FAILEX(fcntl(sock, F_SETFL, arg),
                 "ERROR: Cannot set block mode on SOCKET #%d.\n", sock);
    }
    #endif

    //
    // Error handler.
    //

    exitCode = 0;

    fail:

    DBG_LEAVE("NetSetBlockMode");

    return exitCode;
  }

  //
  // Resolve ip adresses for given host name.
  //
  // ips  - list of found IP addresses (OUT).
  // host - host name to resolve e.g. "google.pl" (IN).
  //
  // RETURNS: 0 if OK,
  //         -1 otherwise.
  //

  int NetResolveIp(vector<string> &ips, const char *host)
  {
    int exitCode = -1;

    hostent *remoteHost = NULL;

    in_addr addr;

    //
    // Check args.
    //

    ips.clear();

    FAILEX(host == NULL, "ERROR: 'host' cannot be NULL in NetResolveIp().\n");

    //
    // Initialize WinSock 2.2 on windows.
    //

    FAIL(_NetInit());

    //
    // Get host info.
    //

    remoteHost = gethostbyname(host);

    FAIL(remoteHost == NULL);

    //
    // Retrieve IP addresses.
    //

    FAIL(remoteHost -> h_addrtype != AF_INET);

    for (int i = 0; remoteHost -> h_addr_list[i]; i++)
    {
      addr.s_addr = *(u_long *) remoteHost -> h_addr_list[i];

      ips.push_back(inet_ntoa(addr));
    }

    FAIL(ips.empty());

    DEBUG1("Host '%s' resolved to '%s'.\n", host, ips[0].c_str());

    //
    // Error handler.
    //

    exitCode = 0;

    fail:

    if (exitCode)
    {
      Error("ERROR: Cannot resolve host '%s'.\n"
                "Error code is: %d.\n", host, GetLastError());
    }

    return exitCode;
  };

  //
  // Encode data to base64 string.
  //
  // data - binary data to encode (IN).
  // size - size of data[] buffer in bytes (IN).
  //
  // RETURNS: base64 string.
  //

  string NetBase64(const char *data, int size)
  {
    static const char b64_charset[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    string ret;

    unsigned char block_3[3];
    unsigned char block_4[4];
    unsigned char *str = (unsigned char *) data;

    int i = 0;
    int j = 0;

    while(size--)
    {
      block_3[i++] = *(str++);

      if( i == 3 )
      {
        block_4[0] = (block_3[0] & 0xfc) >> 2;
        block_4[1] = ((block_3[0] & 0x03) << 4) + ((block_3[1] & 0xf0) >> 4);
        block_4[2] = ((block_3[1] & 0x0f) << 2) + ((block_3[2] & 0xc0) >> 6);
        block_4[3] = block_3[2] & 0x3f;

        for(i = 0; (i <4) ; i++)
        {
          ret += b64_charset[block_4[i]];
        }

        i = 0;
      }
    }

    if(i)
    {
      for(j = i; j < 3; j++)
      {
        block_3[j] = '\0';
      }

      block_4[0] = (block_3[0] & 0xfc) >> 2;
      block_4[1] = ((block_3[0] & 0x03) << 4) + ((block_3[1] & 0xf0) >> 4);
      block_4[2] = ((block_3[1] & 0x0f) << 2) + ((block_3[2] & 0xc0) >> 6);
      block_4[3] = block_3[2] & 0x3f;

      for(j = 0; (j < i + 1); j++)
      {
        ret += b64_charset[block_4[j]];
      }

      while((i++ < 3))
      {
        ret += '=';
      }
    }

    return ret;
  }
} /* namespace Tegenaria */
