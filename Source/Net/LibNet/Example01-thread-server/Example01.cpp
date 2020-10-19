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

#include <cstdio>
#include <Tegenaria/Net.h>
#include <Tegenaria/Debug.h>
#include <Tegenaria/Process.h>

#include <unistd.h>

using namespace Tegenaria;

static int CountConnections = 0;

//
// Callback handler to process one incoming connection.
//
// This function is runned in another thred created
// by main server loop.
//

int ServeClient(NetConnection *nc)
{
  char msg[64] = {0};

  //nc -> read(msg, sizeof(msg));

  int readed = recv(nc -> getSocket(), msg, sizeof(msg), 0);

  #ifdef WIN32
  int lastError = GetLastError();
  #else
  int lastError = errno;
  #endif

  printf("--------------------\n");
  printf(" Connection no     : [%d]\n", CountConnections);
  printf(" Connection socket : [%d]\n", nc -> getSocket());
  printf(" Client info       : [%s]\n", nc -> getClientInfo());
  printf(" Bytes readed      : [%d]\n", readed);
  printf(" Clent message     : [%s]\n", msg);
  printf(" Errno             : [%d]\n", lastError);
  printf("--------------------\n");

  CountConnections ++;

  nc -> release();

  return 0;
};

//
// Entry point.
//

int main(int argc, char **argv)
{
  //
  // No arguments.
  // Create server in background thread, which:
  //
  // - uses our ServerClient() routine to process incoming connections.
  // - listens on port 6666
  //

  if (argc < 3)
  {
    DBG_HEAD("DIRLIGO-SERVER-0.1\nBuild [%s, %s]\n", __DATE__, __TIME__);

    NetConnection *nc = NetServerCreate(6666, ServeClient);

    FAIL(nc == NULL);

    printf("Server created with handle [%p].\n", nc);

    //
    // Fall into loop.
    //

    nc -> join();
  }

  //
  // 'libnet-example <ip> <port>'.
  // Client mode.
  //

  else
  {
    DBG_HEAD("DIRLIGO-CLIENT-0.1\nBuild [%s, %s]\n", __DATE__, __TIME__);

    const char *ip = argv[1];

    int port = atoi(argv[2]);

    int written = 0;

    NetConnection *nc = NetConnect(ip, port);

    FAIL(nc == NULL);

    written = nc -> write("HELLO FROM CLIENT!", 18);

    printf("Bytes written : [%d].\n", written);

    nc -> release();
  }

  //
  // Clean Up.
  //

  fail:

  return 0;
}
