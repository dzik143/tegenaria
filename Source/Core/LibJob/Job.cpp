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

#include <cmath>
#include <Tegenaria/Debug.h>
#include "Job.h"

namespace Tegenaria
{
  //
  // Create job object.
  //
  // title             - job's title (IN/OPT).
  // notifyCallback    - callback called when job changed state or progress meter (IN/OPT).
  // notifyCallbackCtx - caller context passed to notifyCallback() directly (IN/OPT).
  // workerCallback    - callback function performing real job work (IN).
  // workerCallbackCtx - caller context passed to workerCallback() directly (IN/OPT).
  //

  Job::Job(const char *title,
               JobNotifyCallbackProto notifyCallback,
                   void *notifyCallbackCtx,
                       JobWorkerCallbackProto workerCallback,
                           void *workerCallbackCtx)
  {
    DBG_ENTER3("Job::Job");

    //
    // Set job title.
    //

    if (title)
    {
      title_ = title;
    }
    else
    {
      char title[64];

      snprintf(title, sizeof(title) - 1, "Anonymous job %p.\n", this);

      title_ = title;
    }

    //
    // Add object to debug set.
    //

    DBG_SET_ADD("Job", this, "%s", getTitle());

    //
    // Set notify callback.
    //

    notifyCallback_    = notifyCallback;
    notifyCallbackCtx_ = notifyCallbackCtx;

    //
    // Set worker function.
    //

    workerCallback_    = workerCallback;
    workerCallbackCtx_ = workerCallbackCtx;

    //
    // Zero refference counter.
    //

    refCount_ = 1;

    //
    // Set state to initializing.
    //

    setState(JOB_STATE_INITIALIZING);

    //
    // Create worker thread performing real job.
    //

    workerThread_ = ThreadCreate(workerLoop, this);

    //
    // Zero statistics.
    //

    percentCompleted_ = 0.0;
    errorCode_        = 0;

    DBG_SET_RENAME("thread", workerThread_, getTitle());

    DBG_LEAVE3("Job::Job");
  }

  //
  // Call underlying notify callback set in constructor.
  //
  // code - one of JOB_NOTIFY_XXX codes (IN).
  //

  void Job::triggerNotifyCallback(int code)
  {
    if (notifyCallback_)
    {
      notifyCallback_(code, this, notifyCallbackCtx_);
    }
  }

  //
  // Increase refference counter.
  //
  // WARNING! Every call to addRef() MUSTS be followed by one release() call.
  //
  // TIP #1: Object will not be destroyed until refference counter is greater
  //         than 0.
  //
  // TIP #2: Don't call destructor directly, use release() instead. If
  //         refference counter achieve 0, object will be destroyed
  //         automatically.
  //

  void Job::addRef()
  {
    refCountMutex_.lock();

    refCount_ ++;

    DEBUG2("Increased refference counter to %d for job '%s'.\n", refCount_, getTitle());

    refCountMutex_.unlock();
  }

  //
  // Decrease refference counter increased by addRef() before.
  //

  void Job::release()
  {
    int deleteNeeded = 0;

    //
    // Decrease refference counter by 1.
    //

    refCountMutex_.lock();

    refCount_ --;

    DEBUG2("Decreased refference counter to %d for job '%s'.\n", refCount_, getTitle());

    if (refCount_ == 0)
    {
      deleteNeeded = 1;
    }

    refCountMutex_.unlock();

    //
    // Delete object if refference counter goes down to 0.
    //

    if (deleteNeeded)
    {
      delete this;
    }
  }

  //
  // Worker loop performing real job in background thread.
  // This function calls underlying doTheJob() function implemented in child class.
  //
  // jobPtr - pointer to related Job object (this pointer) (IN/OUT).
  //

  int Job::workerLoop(void *jobPtr)
  {
    Job *this_ = (Job *) jobPtr;

    this_ -> addRef();
    this_ -> setState(JOB_STATE_PENDING);

    if (this_ -> workerCallback_)
    {
      this_ -> workerCallback_(this_, this_ -> workerCallbackCtx_);
    }
    else
    {
      Error("ERROR: Worker callback is NULL for '%s'.\n", this_ -> getTitle());
    }

    this_ -> release();

    return 0;
  }

  //
  // Change current state. See JOB_STATE_XXX defines in Job.h.
  //
  // state - new state to set (IN).
  //

  void Job::setState(int state)
  {
    state_ = state;

    DEBUG2("%s: changed state to [%s].\n", getTitle(), getStateString());

    switch(state)
    {
      case JOB_STATE_ERROR:        DBG_INFO("%s : finished with error.\n", getTitle()); break;
      case JOB_STATE_INITIALIZING: DEBUG2("%s : initializing.\n", getTitle()); break;
      case JOB_STATE_PENDING:      DEBUG2("%s : pending.\n", getTitle()); break;
      case JOB_STATE_FINISHED:     DBG_INFO("%s : finished with success.\n", getTitle()); break;
      case JOB_STATE_STOPPED:      DBG_INFO("%s : stopped.\n", getTitle()); break;
    }

    //
    // Call notify callback if set.
    //

    triggerNotifyCallback(JOB_NOTIFY_STATE_CHANGED);
  }

  //
  // Get current state code. See JOB_STATE_XXX defines in Job.h.
  //
  // RETURNS: Current state code.
  //

  int Job::getState()
  {
    return state_;
  }

  //
  // Get current state as human readable string.
  //
  // RETURNS: Name of current job's state.
  //

  const char *Job::getStateString()
  {
    switch(state_)
    {
      case JOB_STATE_ERROR:        return "Error";
      case JOB_STATE_INITIALIZING: return "Initializing";
      case JOB_STATE_PENDING:      return "Pending";
      case JOB_STATE_FINISHED:     return "Finished";
      case JOB_STATE_STOPPED:      return "Stopped";
    };

    return "Unknown";
  }

  //
  // Wait until job finished or stopped with error.
  //
  // timeout - maximum time to wait in ms. Set to -1 for infinite (IN).
  //
  // RETURNS: 0 if OK (job finished/stopped on exit),
  //         -1 otherwise (job still active on exit).

  int Job::wait(int timeout)
  {
    DBG_ENTER2("Job::wait");

    int exitCode = -1;

    int timeLeft = timeout;

    while(state_ == JOB_STATE_INITIALIZING || state_ == JOB_STATE_PENDING)
    {
      ThreadSleepMs(50);

      if (timeout > 0)
      {
        timeLeft -= 50;

        if (timeLeft <= 0)
        {
          Error("ERROR: Timeout while waiting for job '%s'.\n", this -> getTitle());

          goto fail;
        }
      }
    }

    //
    // Error handler.
    //

    exitCode = 0;

    fail:

    DBG_LEAVE2("SftpJob::wait");

    return exitCode;
  }

  //
  // Send stop signal for pending job object.
  // After that related thread should stop working and state
  // should change to JOB_STATE_STOPPED.
  //
  // WARNING#1: Job object MUSTS be still released with release() method.
  //
  // TIP#1: To stop and release resources related with job use below code:
  //
  //        job -> cancel();
  //        job -> release();
  //

  void Job::cancel()
  {
    this -> setState(JOB_STATE_STOPPED);
  }

  //
  // Retrieve job's title set in constructor before.
  //

  const char *Job::getTitle()
  {
    return title_.c_str();
  }

  Job::~Job()
  {
    ThreadWait(workerThread_);
    ThreadClose(workerThread_);

    DBG_SET_DEL("Job", this);
  }

  //
  // Get current job's progress in percentages (0-100%).
  //

  double Job::getPercentCompleted()
  {
    return percentCompleted_;
  }

  //
  // Get error code related with object.
  // This function should be used when job finished with error state.
  //

  int Job::getErrorCode()
  {
    return errorCode_;
  }

  //
  // Set current error code related with job.
  // This function should be used to inform caller why job object
  // finished with error state.
  //

  void Job::setErrorCode(int code)
  {
    errorCode_ = code;
  }

  //
  // Set current job's progress in percentages (0-100%).
  //

  void Job::setPercentCompleted(double percentCompleted)
  {
    int notifyNeeded = 0;

    if (fabs(percentCompleted_ - percentCompleted) > 0.01)
    {
      notifyNeeded = 1;
    }

    percentCompleted_ = percentCompleted;

    if (notifyNeeded)
    {
      triggerNotifyCallback(JOB_NOTIFY_PROGRESS);
    }
  }
} /* namespace Tegenaria */
