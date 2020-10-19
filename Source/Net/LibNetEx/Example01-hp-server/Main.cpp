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
// Purpose: Show how to set up minimal callback echo server.
//

#include <Tegenaria/NetEx.h>
#include <Tegenaria/Debug.h>

using namespace Tegenaria;

//
// Open handler called when new connection created.
//

void OpenHandler(NetExHpContext *ctx)
{
  printf("Worker #%d : New connection PTR#%p from [%s].\n",
             ctx -> workerNo_, ctx, ctx -> clientIp_);

  //
  // TIP: Init custom_ field with user specified data.
  //      This field will be available back in close and data handlers.
  //

  // ctx -> custom_ = ...
}

//
// Close handler called when one of existing connection closed.
//

void CloseHandler(NetExHpContext *ctx)
{
  printf("Worker #%d : Closed connection PTR#%p.\n", ctx -> workerNo_, ctx);
}

//
// Data handler called when new data arrived inside one of existing
// connections.
//

void DataHandler(NetExHpContext *ctx, void *buf, int len)
{
  printf("Worker #%d : Received [%d] bytes inside connection PTR#%p.\n",
             ctx -> workerNo_, len, ctx);

  //
  // Echo server here.
  // Write back the same data to client.
  //

  NetExHpWrite(ctx, buf, len);
}

//
// Entry point.
//

int main(int argc, char **argv)
{
  DBG_HEAD("LibNetEx HP server");

  int port = -1;

  if (argc < 2)
  {
    Fatal("Usage is: %s <port>\n", argv[0]);
  }

  port = atoi(argv[1]);

  return NetExHpServerLoop(port, OpenHandler, CloseHandler, DataHandler);
}
