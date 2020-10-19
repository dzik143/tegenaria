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

#include <Tegenaria/Debug.h>
#include <Tegenaria/Thread.h>
#include <Tegenaria/RequestPool.h>

using namespace Tegenaria;

//
// Thread serving requests.
// It can be thread reading data from network and serving network requestes
// when answer packet received.
//

int ServeThread(void *data)
{
  RequestPool *rpool = (RequestPool *) data;

  Request *r = NULL;

  //
  // Serve request ID#1.
  //

  ThreadSleepSec(1);

  r = rpool -> find(1);                      // Find request ID#1

  if (r)
  {
    r -> lockData();                           // Lock data stored in request

    *((string *) r -> outputData_) = "data1";  // Set outputData (optional)

    r -> unlockData();                         // Unlock data stored in request

    r -> serve();                              // Mark request as served
                                               // (after that wait(1) will be
                                               // finished
  }

  //
  // Serve request ID#2.
  //

  ThreadSleepSec(1);

  r = rpool -> find(2);

  if (r)
  {
    r -> lockData();

    *((string *) r -> outputData_) = "data2";

    r -> unlockData();

    r -> serve();
  }

  //
  // Serve request ID#3.
  //

  ThreadSleepSec(1);

  r = rpool -> find(3);

  if (r)
  {
    r -> lockData();

    *((string *) r -> outputData_) = "data3";

    r -> unlockData();

    r -> serve();
  }
}

//
// Entry point.
//

int main()
{
  RequestPool rpool(8, "example-pool");

  string packet1;
  string packet2;
  string packet3;

  //
  // Push three requests with ID = 1,2,3.
  // They can be packets with given ID sent to network.
  //
  // We set input datas to NULL
  // and output datas to packet1, packet2, packet3.
  //
  //

  rpool.push(1, NULL, &packet1);
  rpool.push(2, NULL, &packet2);
  rpool.push(3, NULL, &packet3);

  //
  // Start thread serving requests.
  // It can be function reading packets from network.
  //

  ThreadCreate(ServeThread, &rpool);

  //
  // Wait until request ID#1 served.
  //

  rpool.wait(1);

  DBG_INFO("Request ID#1 result is '%s'.\n", packet1.c_str());

  //
  // Wait until request ID#2 served.
  //

  rpool.wait(2);

  DBG_INFO("Request ID#2 result is '%s'.\n", packet2.c_str());

  //
  // Wait until request ID#3 served.
  //

  rpool.wait(3);

  DBG_INFO("Request ID#3 result is '%s'.\n", packet3.c_str());

  return 0;
}
