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
// Example shows how to set up echo epoll server.
// Code works on Linux only.
//

#include <cstdio>
#include <string.h>
#include <Tegenaria/Net.h>
#include <Tegenaria/Debug.h>

using namespace Tegenaria;

//
// Handler called when new connection arrived.
//

void OpenHandler(NetEpollContext *ctx, int fd)
{
  printf("Opened connection on FD #%d.\n", fd);
}

//
// Handler called when existing connection closed.
//

void CloseHandler(NetEpollContext *ctx, int fd)
{
  printf("Closed connection on FD #%d.\n", fd);
}

//
// Handler called when something to read on given fd.
//

void DataHandler(NetEpollContext *ctx, int fd, void *buf, int len)
{
  printf("Readed %d bytes from FD #%d.\n", len, fd);

  //
  // Write back readed data to client.
  //
  // !WARNING! Don't use raw write here.
  //

  int written = NetEpollWrite(ctx, fd, buf, len);

  printf("Written %d bytes to FD #%d.\n", written, fd);
}

//
// Entry point.
//

int main(int argc, char **argv)
{
  DBG_HEAD("DIRLIGO-EPOLL-SERVER\nBuild [%s, %s]\n", __DATE__, __TIME__);

  //
  // Start epoll server waiting on TCP port 6666.
  //

  NetEpollServerLoop(6666, OpenHandler, CloseHandler, DataHandler);

  return 0;
}
