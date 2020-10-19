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
// Example shows how to wrap socket conected to remote machine into secure
// DTLS connection.
//

#include <Tegenaria/Net.h>
#include <Tegenaria/Secure.h>

using namespace Tegenaria;

//
// Entry point.
//

int main(int argc, char **argv)
{
  DBG_SET_LEVEL("debug3");

  SecureConnection *sc = NULL;

  char msg[1024] = "Hello world!.";

  int readed  = 0;
  int written = 0;

  //
  // Server.
  //

  if (argc == 1)
  {
    DBG_INFO("Server mode.\n");

    //
    // Wait for incoming connection on TCP 6666 port.
    //

    NetConnection *nc = NetAccept(6666);

    FAIL(nc == NULL);

    //
    // Wrap socket from NetworkConnection class into SecureConnection object.
    //

    sc = SecureConnectionCreate(SECURE_INTENT_SERVER, nc -> getSocket(),
                                    "server.crt", "server.key", NULL);

    FAIL(sc == NULL);
  }

  //
  // Client.
  //

  else
  {
    DBG_INFO("Client mode.\n");

    //
    // Connect with 127.0.0.1:6666.
    //

    NetConnection *nc = NetConnect("127.0.0.1", 6666);

    FAIL(nc == NULL);

    //
    // Wrap socket from from NetConnection into SecureConnection object.
    //

    sc = SecureConnectionCreate(SECURE_INTENT_CLIENT,
                                    nc -> getSocket(), NULL, NULL, NULL);

    FAIL(sc == NULL);
  }

  //
  // Secure connection established.
  //

  //
  // Write "hello world" message to another side via
  // encrypted connection.
  //

  written = sc -> write(msg, strlen(msg), -1);

  FAIL(write <= 0);

  //
  // Read message from another side via encrypted connection.
  //

  readed = sc -> read(msg, sizeof(msg), -1);

  FAIL(readed <= 0);

  //
  // Print out what is readed.
  //

  msg[readed] = 0;

  DBG_INFO("Readed message [%s].\n", msg);

  //
  // Clean up.
  //

  fail:

  if (sc)
  {
    sc -> release();
  }

  return 0;
}
