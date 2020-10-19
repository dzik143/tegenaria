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
//
//

#define CONTROL_PIPE "dlserver-6311294E-FE1D-11E2-BE3E-2AD76188709B"

//
// Entry point.
//

int main(int argc, char **argv)
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

  //
  // Error handler.
  //

  fail:

  return 0;
}
