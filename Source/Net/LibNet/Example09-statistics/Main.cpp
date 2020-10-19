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

#include <Tegenaria/Net.h>

using namespace Tegenaria;

//
// Entry point.
//

int main(int argc, char *argv[])
{
  NetStatistics ns;

  //
  // Some packets sent.
  //

  ns.insertOutcomingPacket(1024);
  ns.insertOutcomingPacket(1024);
  ns.insertOutcomingPacket(1024);

  //
  // Some packets arrived.
  //

  ns.insertIncomingPacket(1024);
  ns.insertIncomingPacket(1024);
  ns.insertIncomingPacket(1024);
  ns.insertIncomingPacket(1024);

  //
  // Some network requests executed.
  //

  ns.insertRequest(1024, 10);
  ns.insertRequest(1024, 20);
  ns.insertRequest(1024, 30);
  ns.insertRequest(1024, 40);
  ns.insertRequest(1024, 50);

  //
  // Perform some download and upload tasks.
  //

  ns.insertUploadEvent(1024 * 64, 100);
  ns.insertUploadEvent(1024 * 64, 200);
  ns.insertUploadEvent(1024 * 64, 300);

  ns.insertDownloadEvent(1024 * 32, 100);
  ns.insertDownloadEvent(1024 * 32, 200);
  ns.insertDownloadEvent(1024 * 32, 300);

  //
  // Print statistics.
  //

  printf("%s", ns.toString().c_str());

  return 0;
}
