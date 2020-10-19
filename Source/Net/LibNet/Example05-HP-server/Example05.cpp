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
// Example shows how to set up handler based server.
// Code should works on Windows and Linux without modifications.
//

#include <cstdio>
#include <string>
#include <Tegenaria/Net.h>
#include <Tegenaria/Debug.h>

using namespace std;
using namespace Tegenaria;

//
// Custom, optional object related with incoming connections.
//

class ConnectionContext
{
  string name_;

  public:

  ConnectionContext(const char *name)
  {
    name_ = name;
  }

  const char *getName()
  {
    return name_.c_str();
  }

  //
  // ...
  //
};

//
// Handler called when new connection arrived.
//

void OpenHandler(NetHpContext *ctx, int fd)
{
  printf("Opened connection on FD #%d.\n", fd);

  //
  // Initialize some connection related object e.g. user context.
  //

  ConnectionContext *connection = new ConnectionContext("Marian");

  //
  // And associate it with opened connection.
  // We will get it back in further close and data handlers.
  //

  ctx -> custom_ = (void *) connection;
}

//
// Handler called when existing connection closed.
//

void CloseHandler(NetHpContext *ctx, int fd)
{
  ConnectionContext *this_ = (ConnectionContext *) ctx -> custom_;

  printf("Closed connection on FD #%d for user %s.\n", fd, this_ -> getName());

  //
  // Free related connection object.
  //

  delete this_;
}

//
// Handler called when something to read on given fd.
//

void DataHandler(NetHpContext *ctx, int fd, void *buf, int len)
{
  ConnectionContext *this_ = (ConnectionContext *) ctx -> custom_;

  printf("%s: Readed %d bytes from FD #%d.\n", this_ -> getName(), len, fd);

  //
  // Write back readed data to client.
  //
  // !WARNING! Don't use raw write here.
  //

  int written = NetHpWrite(ctx, fd, buf, len);

  printf("%s: Written %d bytes to FD #%d.\n", this_ -> getName(), written, fd);
}

//
// Entry point.
//

int main(int argc, char **argv)
{
  DBG_HEAD("DIRLIGO-HP-SERVER\nBuild [%s, %s]\n", __DATE__, __TIME__);

  //
  // Start HP server waiting on TCP port 6666.
  //

  NetHpServerLoop(6666, OpenHandler, CloseHandler, DataHandler);

  return 0;
}
