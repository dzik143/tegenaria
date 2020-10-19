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

#ifndef Tegenaria_Core_Job_H
#define Tegenaria_Core_Job_H

#include <cstdlib>
#include <string>

#include <Tegenaria/Mutex.h>
#include <Tegenaria/Thread.h>

namespace Tegenaria
{
  using namespace std;

  //
  // Defines.
  //

  #define JOB_STATE_ERROR        1
  #define JOB_STATE_INITIALIZING 2
  #define JOB_STATE_PENDING      3
  #define JOB_STATE_FINISHED     4
  #define JOB_STATE_STOPPED      5

  #define JOB_NOTIFY_STATE_CHANGED 1
  #define JOB_NOTIFY_PROGRESS      2

  //
  // Forward declaration.
  //

  class Job;

  //
  // Typedef.
  //

  typedef void (*JobNotifyCallbackProto)(int code, Job *job, void *ctx);
  typedef void (*JobWorkerCallbackProto)(Job *job, void *ctx);

  //
  // Class.
  //

  class Job
  {
    protected:

    //
    // Current job state (see JOB_STATE_XXX defines).
    //

    int state_;

    double percentCompleted_;

    int errorCode_;

    //
    // Basic properties set in constructor.
    //

    string title_;

    JobNotifyCallbackProto notifyCallback_;

    void *notifyCallbackCtx_;

    //
    // Refference counter.
    //

    Mutex refCountMutex_;

    int refCount_;

    //
    // Worker thread.
    //

    ThreadHandle_t *workerThread_;

    //
    // Underlying worker function.
    //

    JobWorkerCallbackProto workerCallback_;

    void *workerCallbackCtx_;

    //
    // --------------------------------------------------------------------------
    //
    //                            Public interface.
    //
    // --------------------------------------------------------------------------

    public:

    Job(const char *title,
            JobNotifyCallbackProto notifyCallback,
                void *notifyCallbackCtx,
                    JobWorkerCallbackProto workerCallback,
                        void *workerCallbackCtx);

    void triggerNotifyCallback(int code);

    //
    // Refference counter.
    //

    void addRef();
    void release();

    //
    // Worker thread.
    //

    static int workerLoop(void *jobPtr);

    //
    // State.
    //

    void setState(int state);

    int getState();

    const char *getStateString();

    //
    // General.
    //

    const char *getTitle();

    double getPercentCompleted();

    void setPercentCompleted(double percentCompleted);

    void setErrorCode(int code);

    int getErrorCode();

    //
    // Synchronize functinos.
    //

    int wait(int timeout = -1);

    void cancel();

    //
    // Private functions.
    //

    protected:

    virtual ~Job();
  };

} /* namespace Tegenaria */

#endif /* Tegenaria_Core_Job_H */
