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

#ifndef Tegenaria_Core_RequestPool_H
#define Tegenaria_Core_RequestPool_H

//
// Includes.
//

#include <string>
#include <Tegenaria/Debug.h>
#include <Tegenaria/Error.h>

#include "Mutex.h"
#include "Semaphore.h"

namespace Tegenaria
{
  //
  // Defines.
  //

  #define REQUEST_POOL_DEFAULT_SIZE 16

  //
  // Structure to store one generic request.
  //

  struct Request
  {
    int id_;

    void *inputData_;
    void *outputData_;

    Semaphore *served_;

    Mutex *dataMutex_;

    void lockData();
    void unlockData();
    void serve();
  };

  //
  // Class to store request pool.
  //

  class RequestPool
  {
    //
    // Private fields.
    //

    Request *table_;

    Mutex *mutex_;

    int size_;

    string name_;

    //
    // Internal use functions.
    //

    Request *findFree();

    //
    // Public interface.
    //

    public:

    //
    // Init.
    //

    RequestPool(int size = REQUEST_POOL_DEFAULT_SIZE, const char *name = NULL);

    ~RequestPool();

    //
    // Request management.
    //

    int push(int id, void *inputData, void *outputData);

    int wait(int id, int timeout = -1);

    int cancel(int id);

    int serve(int id);

    int serve(Request *r);

    Request *find(int id);

    //
    // Thread synchronization.
    //

    void lock();
    void unlock();

    //
    // General.
    //

    const char *getName();
  };

} /* namespace Tegenaria */

#endif /* Tegenaria_Core_RequestPool_H */
