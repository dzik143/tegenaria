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

#include "RequestPool.h"

namespace Tegenaria
{
  //
  // Constructor.
  // Allocate and zero pending request table.
  //
  // size - maximum number of pending request in one time (IN/OPT).
  // name - arbitrary request pool name to debug code easier (IN/OPT).
  //

  RequestPool::RequestPool(int size, const char *name)
  {
    DBG_ENTER3("RequestPool::RequestPool");

    //
    // Use default pool size if given value incorrect.
    //

    if (size < 1)
    {
      Error("WARNING: 'size' parameter is less than 1 in"
                " RequestPool::RequestPool(). Defaulted to %d.\n",
                    REQUEST_POOL_DEFAULT_SIZE);

      size = REQUEST_POOL_DEFAULT_SIZE;
    }

    //
    // Save pool name if set.
    //

    if (name)
    {
      name_ = name;
    }
    else
    {
      char buf[16];

      snprintf(buf, sizeof(buf) - 1, "%p", this);

      name_ = name;
    }

    //
    // Create mutex to lock whole object.
    //

    mutex_ = new Mutex(getName());

    //
    // Init requests table.
    //

    size_  = size;

    table_ = new Request[size_];

    for (int i = 0; i < size; i++)
    {
      char buf[128];

      table_[i].id_ = -1;

      snprintf(buf, sizeof(buf) - 1, "%s#%d", getName(), i);

      table_[i].served_    = new Semaphore(0, buf);
      table_[i].dataMutex_ = new Mutex(buf);
    }

    DBG_SET_ADD("ThreadPool", this, name_.c_str());

    DBG_LEAVE3("RequestPool::RequestPool");
  }

  //
  // Destructor.
  // Free request table created in constructor.
  //

  RequestPool::~RequestPool()
  {
    DBG_ENTER3("RequestPool::~RequestPool");

    if (table_)
    {
      for (int i = 0; i < size_; i++)
      {
        delete table_[i].served_;
        delete table_[i].dataMutex_;
      }

      delete []table_;

      table_ = NULL;
    }

    delete mutex_;

    DBG_SET_DEL("ThreadPool", this);

    DBG_LEAVE3("RequestPool::~RequestPool");
  }

  //
  // Get object name.
  //

  const char *RequestPool::getName()
  {
    return name_.c_str();
  }

  //
  // Find request with given ID.
  //
  // WARNING: Function is not thread safe itselt. Caller MUSTS use lock()/unlock()
  //          by own.
  //
  // id - request's ID set int RequestPush() time before (IN).
  //
  // RETURNS: Pointer to found Request object,
  //          NULL if request with given ID does not exists.
  //

  Request *RequestPool::find(int id)
  {
    for (int i = 0; i < size_; i++)
    {
      if (table_[i].id_ == id)
      {
        return &table_[i];
      }
    }

    return NULL;
  }

  //
  // Find first not used request in requests table.
  //
  // WARNING: Function is not thread safe itselt. Caller MUSTS use lock()/unlock()
  //          by own.
  //
  // RETURNS: Pointer to first not used request,
  //          NULL if too many requests.
  //

  Request *RequestPool::findFree()
  {
    for (int i = 0; i < size_; i++)
    {
      if (table_[i].id_ == -1)
      {
        return &table_[i];
      }
    }

    Error("ERROR: Too many requests in request pool '%s'.\n", getName());

    return NULL;
  }

  //
  // Wait until given request served by serve() function.
  //
  // WARNING#1: This function pop request from table even if timeout reached.
  // WARNING#2: Only one thread can wait for request at one time.
  //
  // TIP#1: Use serve() method from another thread to serve pending request.
  //        After that wait() will finished with code 0.
  //
  // Parameters:
  //
  // id      - request ID passed to push() before (IN).
  // timeout - timeout in ms, -1 for infinity (IN).
  //
  // RETURNS: 0 if OK,
  //          ERR_WRONG_PARAMETER if there is no request with given ID,
  //          ERR_TIMEOUT if timeout reached.

  int RequestPool::wait(int id, int timeout)
  {
    DBG_ENTER5("RequestPool::wait");

    int exitCode = -1;

    Request *r = NULL;

    int waitRet = -1;

    //
    // Find request with given ID.
    //

    r = find(id);

    if (r == NULL)
    {
      Error("ERROR: Request ID#%d does not exist in request pool '%s'.\n", id, getName());

      exitCode = ERR_WRONG_PARAMETER;

      goto fail;
    }

    //
    // Wait until request served.
    //

    DEBUG5("RequestPool::wait : Waiting for request"
               " ID#%d pool '%s'...\n", id, getName());

    waitRet = r -> served_ -> wait(timeout);

    switch(waitRet)
    {
      //
      // OK.
      //

      case 0:
      {
        DEBUG5("RequestPool::wait : Waiting finished"
                   " request ID#%d pool '%s'...\n", id, getName());

        break;
      }

      //
      // Timeout.
      //

      case -1:
      {
        Error("ERROR: Timeout while waiting"
                  " for request ID#%d pool '%s'.\n", id, getName());

        exitCode = ERR_TIMEOUT;

        goto fail;
      }

      //
      // Unexpected error while waiting.
      //

      default:
      {
        Error("ERROR: Wait finished with system code '%d'"
                  " for request ID#%d pool '%s'.\n", waitRet, id, getName());

        exitCode = ERR_GENERIC;

        goto fail;
      }
    }

    //
    // Unmark request in table.
    // It's not used longer.
    //

    r -> lockData();

    r -> id_ = -1;
    r -> inputData_  = NULL;
    r -> outputData_ = NULL;

    r -> unlockData();

    //
    // Error handler.
    //

    exitCode = 0;

    fail:

    if (exitCode)
    {
      Error("ERROR: Cannot wait for request ID#%d"
                " in request pool '%s'.\n", id, getName());
    }

    DBG_LEAVE5("RequestPool::wait");

    return exitCode;
  }

  //
  // Mark request with given ID as served. After that thread wait() from
  // another thead will be finished.
  //
  // id - request ID to serve, set in push() time (IN).
  //
  // RETURNS: 0 if OK,
  //          ERR_WRONG_PARAMETER if request with given id does not exist.
  //

  int RequestPool::serve(int id)
  {
    DBG_ENTER5("RequestPool::serve");

    int exitCode = ERR_WRONG_PARAMETER;

    Request *r = NULL;

    //
    // Find request.
    //

    r = find(id);

    if (r == NULL)
    {
      Error("ERROR: Request ID#%d does not exist.\n", id);

      goto fail;
    }

    //
    // Serve request.
    //

    r -> served_ -> signal();

    DEBUG3("Request ID#%d served in request pool '%s'.\n", id, getName());

    //
    // Error handler.
    //

    exitCode = 0;

    fail:

    DBG_LEAVE5("RequestPool::serve");

    return exitCode;
  }

  //
  // Mark request as served. After that thread wait() from another
  // thread will be finished.
  //
  // r - pointer to request retrieved from find() function (IN).
  //
  // RETURNS: 0 if OK,
  //          ERR_WRONG_PARAMETER if request with given id does not exist.
  //

  int RequestPool::serve(Request *r)
  {
    DBG_ENTER5("RequestPool::serve");

    int exitCode = ERR_WRONG_PARAMETER;

    //
    // Check args.
    //

    if (r == NULL)
    {
      Error("ERROR: NULL request passed to RequestPool::serve().\n");

      goto fail;
    }

    //
    // Serve request.
    //

    r -> served_ -> signal();

    DEBUG3("Request ID#%d served in request pool '%s'.\n", r -> id_, getName());

    //
    // Error handler.
    //

    exitCode = 0;

    fail:

    DBG_LEAVE5("RequestPool::serve");

    return exitCode;
  }

  //
  // Push new request to pending table.
  //
  // TIP#1: Another thread should use serve() to finalize (serve) request.
  //
  // TIP#2: Use wait() to wait until request served.
  //
  // WARNING#1: Every call to push() MUSTS be followed by one call to serve()
  //            with the same ID.
  //
  // id         - request ID to assign, MUSTS be unique (IN).
  // inputData  - pointer to arbitrary input data, can be NULL (IN/OUT).
  // outputData - pointer to arbitrary output data, can be NULL (IN/OUT).
  //
  // RETURNS: 0 if OK,
  //         -1 otherwise.
  //

  int RequestPool::push(int id, void *inputData, void *outputData)
  {
    DBG_ENTER5("RequestPool::push");

    int exitCode = -1;

    Request *r = NULL;

    this -> lock();

    //
    // Check is given request ID unique.
    //

    if (this -> find(id) != NULL)
    {
      Error("ERROR: Request ID#%d already in use in request pool '%s'.\n", id, getName());

      goto fail;
    }

    //
    // Find unused request in table.
    //

    r = this -> findFree();

    FAIL(r == NULL);

    //
    // Assign buffer and id to request.
    // Mark request as pending.
    //

    DEBUG5("RequestPool::push : Pushing request ID#%d to pool '%s'...\n", id, getName());

    r -> lockData();

    r -> inputData_  = inputData;
    r -> outputData_ = outputData;
    r -> id_         = id;

    r -> unlockData();

    //
    // Error handler.
    //

    exitCode = 0;

    fail:

    if (exitCode)
    {
      Error("ERROR: Cannot push request ID#%d to request pool '%s'.\n", id, getName());
    }

    this -> unlock();

    DBG_LEAVE5("RequestPool::push");

    return exitCode;
  }

  //
  // Lock request pool object.
  //
  // WARNING: Every call to lock() MUSTS be followed by one unlock() call.
  //

  void RequestPool::lock()
  {
    mutex_ -> lock();
  }

  //
  // Unlock request pool object locked by lock() function before.
  //

  void RequestPool::unlock()
  {
    mutex_ -> unlock();
  }

  //
  // Lock data pointers stored inside Request struct.
  //
  // WARNING: Every calls to lockData() MUSTS be followed by one unlockData()
  //          call.
  //

  void Request::lockData()
  {
    dataMutex_ -> lock();
  }

  //
  // Unlock data pointers stored inside Request struct locked by lockData()
  // before.
  //

  void Request::unlockData()
  {
    dataMutex_ -> unlock();
  }

  //
  // Mark request as served.
  //

  void Request::serve()
  {
    served_ -> signal();
  }
} /* namespace Tegenaria */
