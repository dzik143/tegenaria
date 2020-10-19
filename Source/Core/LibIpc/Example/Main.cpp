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
#include <cstring>

#include <Tegenaria/Debug.h>
#include <Tegenaria/Ipc.h>

#ifndef WIN32
# include <unistd.h>
#endif

using namespace Tegenaria;

//
// Callback function to process incoming connetion.
//
// fd - CRT descriptor connected to remote server (IN).
//
// ctx - optional caller context passed to IpcServerLoop() or
//       IpcServerCreate() (IN).
//
// RETURNS: 0 if OK.
//

int ProcessConnection(int fd, void *ctx)
{
  DBG_ENTER("ProcessConnection");

  char msg[64] = {0};

  //
  // Read message from client.
  //

  FAILEX(read(fd, msg, sizeof(msg)) <= 0,
             "ERROR: Cannot read data from client.\n");

  DBG_MSG("Readed [%s] from client.\n", msg);

  //
  // Write answer to client.
  //

  strcpy(msg, "Response from server.");

  DBG_MSG("Writing answer [%s] to client...\n", msg);

  FAILEX(write(fd, msg, strlen(msg)) <= 0,
           "ERROR: Cannot write answer to client.\n");

  fail:

  DBG_LEAVE("ProcessConnection");

  return 0;
}

//
// Entry point.
//

int main(int argc, char **argv)
{
  DBG_INIT_EX(NULL, "debug3", -1);

  //
  // No args.
  // Run server listening on 'dupa-1234'.
  //

  if (argc < 2)
  {
    DBG_HEAD("LibIpc Example - server mode");

    DBG_MSG("Creating server on 'dupa-1234'...\n");

    FAIL(IpcServerLoop("dupa-1234", ProcessConnection));
  }

  //
  // Pipename specified in argv[1].
  // Client mode.
  //

  else
  {
    DBG_HEAD("LibIpc Example - client mode");

    char msg[64] = "Hello from client.";

    //
    // Open connection to server.
    //

    DBG_MSG("Connecting to '%s'...\n", argv[1]);

    int fd = IpcOpen(argv[1]);

    FAIL(fd < 0);

    //
    // Write message to server.
    //

    DBG_MSG("Writing [%s] to server...\n", msg);

    FAIL(write(fd, msg, strlen(msg)) < 0);

    //
    // Read response from server.
    //

    FAIL(read(fd, msg, sizeof(msg)) < 0);

    DBG_MSG("Readed [%s] from server.\n", msg);

    //
    // Close connetion.
    //

    close(fd);
  }

  //
  // Error handler.
  //

  fail:

  return 0;
}
