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

#include "Mutex.h"
#include <Tegenaria/Debug.h>

namespace Tegenaria
{
  //
  // Constructor. Create named or anonymous mutex object.
  //
  // name - optional, human readable name (IN/OPT).
  //

  Mutex::Mutex(const char *name)
  {
    #ifdef WIN32
    mutex_ = CreateMutex(NULL, FALSE, NULL);
    #else
    pthread_mutex_init(&mutex_, NULL);
    #endif

    if (name)
    {
      DEBUG3("Created named mutex '%s'.\n", name);

      name_ = name;
    }
    else
    {
      char buf[16];

      snprintf(buf, sizeof(buf), "%p", this);

      name_ = buf;

      DEBUG3("Created anonymous mutex '%s'.\n", name_.c_str());
    }
  }

  //
  // Destructor.
  //

  Mutex::~Mutex()
  {
    DEBUG3("Destroying mutex [%s].\n", name_.c_str());

    #ifdef WIN32
    CloseHandle(mutex_);
    #else
    pthread_mutex_destroy(&mutex_);
    #endif
  }

  //
  // Get ownership of the mutex.
  //
  // TIP#1: Every call to lock() MUSTS be followed by one call to unlock().
  //

  void Mutex::lock()
  {
    //
    // Windows.
    //

    #ifdef WIN32
    {
      DEBUG5("Locking mutex [%s] by thread #[%d]...\n", name_.c_str(), GetCurrentThreadId());

      DBG_SET_ADD("Locking", this, name_.c_str());

      WaitForSingleObject(mutex_, INFINITE);

      DBG_SET_MOVE("Locked", "Locking", this);

      DEBUG4("Mutex [%s] locked by thread #[%d].\n",
                 name_.c_str(), GetCurrentThreadId());

    }

    //
    // Linux, MacOS.
    //

    #else
    {
      DEBUG5("Locking mutex [%s]...\n", name_.c_str());

      DBG_SET_ADD("Locking", this, name_.c_str());

      pthread_mutex_lock(&mutex_);

      DBG_SET_MOVE("Locked", "Locking", this);

      DEBUG4("Mutex [%s] locked.\n", name_.c_str());
    }
    #endif
  }

  //
  // Releae mutex locked by lock() method before.
  //
  // WARNING: Only thread, that called lock() can free mutex.
  //

  void Mutex::unlock()
  {
    //
    // Windows.
    //

    #ifdef WIN32
    {
      DEBUG5("Releasing mutex [%s] by thread #[%d]...\n",
                 name_.c_str(), GetCurrentThreadId());

      ReleaseMutex(mutex_);

      DBG_SET_DEL("Locked", this);

      DEBUG4("Mutex [%s] released from thread #[%d].\n",
                 name_.c_str(), GetCurrentThreadId());
    }

    //
    // Linux, MacOS.
    //

    #else
    {
      DEBUG5("Releasing mutex [%s]...\n", name_.c_str());

      pthread_mutex_unlock(&mutex_);

      DBG_SET_DEL("Locked", this);

      DEBUG4("Mutex [%s] released.\n", name_.c_str());
    }
    #endif
  }

  //
  // Get current mutex state.
  //
  // Unimplemented.
  //

  int Mutex::getState()
  {
    Error("ERROR: Mutex::getState() not implemented.\n");

    return -1;
  }

  //
  // Change mutex name.
  //
  // fmt - C printf like format (IN).
  //

  void Mutex::setName(const char *fmt, ...)
  {
    if (fmt)
    {
      char name[1024] = "";

      va_list ap;

      //
      // Format printf like message.
      //

      va_start(ap, fmt);

      vsnprintf(name, sizeof(name) - 1, fmt, ap);

      va_end(ap);

      //
      // Set name.
      //

      DEBUG4("Renaming mutex [%s] to [%s]...\n", name_.c_str(), name);

      name_ = name;

      DBG_SET_RENAME("Locked", this, name_.c_str());
      DBG_SET_RENAME("Locking", this, name_.c_str());
    }
  }
} /* namespace Tegenaria */
