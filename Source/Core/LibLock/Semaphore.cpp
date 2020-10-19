/******************************************************************************/
/*                                                                            */
/* Copyright (c) 2010, 2014 Lukasz Bienczyk <lukasz.bienczyk@gmail.com>,      */
/* Radoslaw Kolodziejczyk <radek.kolodziejczyk@gmail.com>,                    */
/* Sylwester Wysocki <sw143@wp.pl>                                            */
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

#include <stdio.h>

#include "Semaphore.h"
#include <Tegenaria/Debug.h>

namespace Tegenaria
{
  Semaphore::Semaphore(int initValue, const char *name)
  {
    //
    // Windows.
    //

    #ifdef WIN32
    {
      semaphore_ = CreateSemaphore(NULL, initValue, 1, NULL);
    }

    //
    // Linux, MacOS.
    //

    #else
    {
      sem_init(&semaphore_, 0, initValue);
    }
    #endif

    //
    // Common.
    //

    if (name)
    {
      DEBUG3("Created named Semaphore '%s'.\n", name);

      name_ = name;
    }
    else
    {
      char buf[16];

      snprintf(buf, sizeof(buf), "%p", this);

      name_ = buf;

      DEBUG3("Created anonymous Semaphore '%s'.\n", name_.c_str());
    }

    DBG_SET_ADD("Semaphore", this, name_.c_str());
  }

  Semaphore::~Semaphore()
  {
    DEBUG3("Destroying Semaphore [%s]...\n", name_.c_str());

    #ifdef WIN32
    CloseHandle(semaphore_);
    #else
    sem_destroy(&semaphore_);
    #endif

    DBG_SET_DEL("Semaphore", this);
  }

  //
  // Wait until semafore reach signaled state.
  //
  // TIP#1: Use signal() function to change object into signaled state.
  //
  // TIP#2: Use isSignaled() to check is semaphore signaled without falling into
  //        waiting loop.
  //
  // timeout - timeout in ms, -1 for infinite (IN/OPT).
  //
  // RETURNS:  0 on success,
  //           1 on timeout,
  //          -1 if error.
  //

  int Semaphore::wait(int timeout)
  {
    int result = 0;

    DBG_SET_ADD("Semaphore wait", this, name_.c_str());

    //
    // Windows.
    //

    #ifdef WIN32
    {
      if (timeout == -1)
      {
        DEBUG5("Waiting on Sempahore [%s] by thread #[%d]...\n", name_.c_str(), GetCurrentThreadId());

        result = (int) WaitForSingleObject(semaphore_, INFINITE);
      }
      else
      {
        DEBUG5("Waiting on Sempahore [%s] by thread #[%d] with timeout [%d]...\n", name_.c_str(), GetCurrentThreadId(), timeout);

        result = (int) WaitForSingleObject(semaphore_, timeout);
      }

      if (result == WAIT_OBJECT_0)
      {
        result = 0;
      }
      else if (result == WAIT_TIMEOUT)
      {
        result = -1;
      }
      else
      {
        result = (int) GetLastError();
      }

      DEBUG4("Finished waiting on Sempahore [%s] by thread #[%d] with timeout [%d] and result [%d]...\n", name_.c_str(), GetCurrentThreadId(), timeout, result);
    }

    //
    // Linux, MacOS.
    //

    #else
    {
      if (timeout == -1)
      {
        DEBUG5("Waiting on Semaphore [%s]...\n", name_.c_str());

        while ((result = sem_wait(&semaphore_)) != 0 && GetLastError() == EINTR)
        {
        }
      }
      else
      {
        DEBUG5("Waiting on Semaphore [%s] with timeout [%d]...\n", name_.c_str(), timeout);

        struct timespec ts = {0};

        clock_gettime(CLOCK_REALTIME, &ts);

        ts.tv_sec  += timeout / 1000;
        ts.tv_nsec += (timeout % 1000) * 1000000;

        while ((result = sem_timedwait(&semaphore_, &ts)) != 0 && GetLastError() == EINTR)
        {
        }
      }

      if (result == EAGAIN)
      {
        result = -1;
      }
      else if (result != 0)
      {
        result = errno;
      }
    }
    #endif

    DBG_SET_DEL("Semaphore wait", this);

    DEBUG4("Finished waiting on Sempahore [%s] with timeout [%d] and result [%d]...\n", name_.c_str(), timeout, result);

    return result;
  }

  void Semaphore::signal()
  {
    DEBUG5("Signalling Semaphore [%s]...\n", name_.c_str());

    #ifdef WIN32
    ReleaseSemaphore(semaphore_, 1, NULL);
    #else
    sem_post(&semaphore_);
    #endif

    DEBUG4("Semaphore [%s] signalled...\n", name_.c_str());
  }

  //
  // Decrement the counter of the semaphore. Do not block if traditional
  // wait() would block.
  //

  int Semaphore::tryWait()
  {
    int result = -1;

    DEBUG5("Trying a wait on Semaphore [%s].\n", name_.c_str());

    //
    // Windows.
    //

    #ifdef WIN32
    {
      result = WaitForSingleObject(semaphore_, 0);

      if (result != WAIT_OBJECT_0)
      {
        DEBUG4("Wait would block or an error occured. Error is [%d]\n", GetLastError());
      }
    }

    //
    // Linux, MacOS.
    //

    #else
    {
      while ((result = sem_trywait(&semaphore_)) != 0 && GetLastError() == EINTR)
      {
      }

      if (result != 0)
      {
        DEBUG4("Wait would block or an error occured. Error is [%d]\n", GetLastError());
      }
    }
    #endif

    return result;
  }

  //
  // Retrieve currect semaphore counter.
  //

  int Semaphore::getState()
  {
    int value = 0;

    //
    // Windows.
    //

    #ifdef WIN32
    {
      Error("ERROR: Semaphore::getState() not implemented on Windows.");

      value = -1;
    }

    //
    // Linux, MacOS.
    //

    #else
    {
      sem_getvalue(&semaphore_, &value);
    }
    #endif

    return value;
  }

  //
  // Decrement the counter untill the wait call would block.
  //

  int Semaphore::unwind()
  {
    int result = 0;

    DEBUG5("Unwinding semaphore [%s]...\n", name_.c_str());

    while(tryWait() == 0)
    {
      result++;
    }

    return result;
  }

  int Semaphore::unwoundWait(int timeout)
  {
    unwind();

    return this -> wait(timeout);
  }

  void Semaphore::setName(const char *name)
  {
    if (name)
    {
      DEBUG5("Renaming semaphore [%s] to [%s]...\n", name_.c_str(), name);

      name_ = name;
    }
  }
} /* namespace Tegenaria */
