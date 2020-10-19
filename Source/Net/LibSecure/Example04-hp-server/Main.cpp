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
// WARNING!
// To create fully secure connection we need two steps:
//
// 1. Create secure TLS session - after that transfered data are encrypted
//   i.e. not readable by third parties,
//
// 2. validate *WHO* is on the other side basing on data from certificate,
//   e.g. by check domain name.
//
// Below code performs (1) but does *NOT* perform (2).
// Caller *SHOULD* deliver own mechanism to check does data delivered
// within certificate are match to what he's expecting.
//

//
// Example shows how to integrate callback based server with SSL.
//

#include <cstdio>
#include <string>

#include <Tegenaria/Net.h>
#include <Tegenaria/Secure.h>
#include <Tegenaria/Debug.h>

using namespace std;
using namespace Tegenaria;

//
// Custom, optional object related with incoming connections.
//

struct UserContext
{
  string name_;

  SecureConnection *sc_;
};

//
// Handler called when new connection arrived.
//

void OpenHandler(NetHpContext *ctx, int fd)
{
  printf("Opened connection on FD #%d.\n", fd);

  //
  // Initialize custom SecureConnection object without underlying
  // IO defined.
  //

  SecureConnection *sc = SecureConnectionCreate(SECURE_INTENT_SERVER,
                                                    "server.crt", "server.key",
                                                        NULL);

  //
  // Initialize some connection related object e.g. user context
  // and associate SecureConnection object with our context.
  //

  UserContext *user = new UserContext;

  user -> name_ = "Marian";
  user -> sc_   = sc;

  //
  // Associate user context with opened connection.
  // We will get it back in further close and data handlers.
  //

  ctx -> custom_ = (void *) user;
}

//
// Handler called when existing connection closed.
//

void CloseHandler(NetHpContext *ctx, int fd)
{
  UserContext *user = (UserContext *) ctx -> custom_;

  printf("Closed connection on FD #%d for user %s.\n", fd, user -> name_.c_str());

  //
  // Free related connection object.
  //

  delete user;
}

//
// Handler called when something to read on given fd.
//

void DataHandler(NetHpContext *ctx, int fd, void *buf, int len)
{
  UserContext *user = (UserContext *) ctx -> custom_;

  //
  // SSL connection already established.
  // Normal session flow.
  //

  switch(user -> sc_ -> getState())
  {
    case SECURE_STATE_ESTABLISHED:
    {
      char buffer[1024];

      int bufferSize = 1024;

      char msg[] = "Hello world.";

      //
      // Decrypt incoming data into buffer[].
      //

      len = user -> sc_ -> decrypt(buffer, bufferSize, buf, len);

      printf("%s: Readed %d bytes from FD #%d.\n", user -> name_.c_str(), len, fd);

      //
      // ...
      // do some real work here
      // ...
      //

      //
      // Encrypt outcoming data before send.
      //
      //

      len = user -> sc_ -> encrypt(buffer, bufferSize, msg, sizeof(msg));

      //
      // Write encrypted message to client.
      //

      int written = NetHpWrite(ctx, fd, buffer, len);

      printf("%s: Written %d bytes to FD #%d.\n", user -> name_.c_str(), written, fd);

      break;
    }

    //
    // SSL Handshake is pending.
    // We must handle it by own.

    case SECURE_STATE_HANDSHAKE_READ:
    case SECURE_STATE_HANDSHAKE_WRITE:
    {
      char toWrite[1024];

      int toWriteSize = 1024;

      FAIL(user -> sc_ -> handshakeStep(toWrite, &toWriteSize, buf, len));

      if (toWriteSize > 0)
      {
        NetHpWrite(ctx, fd, toWrite, toWriteSize);
      }

      break;
    }
  }

  fail:

  return;
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
