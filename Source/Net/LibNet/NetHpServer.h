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

#ifndef Tegenaria_Core_HPServer_H
#define Tegenaria_Core_HPServer_H

namespace Tegenaria
{
  //
  // Structs.
  //

  //
  // Wrap first, common field for NetEpollContext and NetIocpContext.
  // This gives possibility to set custom per FD specified context.

  struct NetHpContext
  {
    void *custom_;
  };

  //
  // Typedef.
  //

  typedef void (*NetHpOpenProto)(NetHpContext *ctx, int fd);
  typedef void (*NetHpCloseProto)(NetHpContext *ctx, int fd);
  typedef void (*NetHpDataProto)(NetHpContext *ctx, int fd, void *buf, int len);

  //
  // Exported functions.
  //

  int NetHpServerLoop(int port, NetHpOpenProto openHandler,
                          NetHpCloseProto closeHandler,
                              NetHpDataProto dataHandler);

  int NetHpWrite(NetHpContext *ctx, int fd, void *buf, int len);

} /* namespace Tegenaria */

#endif /* Tegenaria_Core_HPServer_H */
