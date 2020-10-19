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

#include <cstdio>
#include <Tegenaria/Job.h>

using namespace Tegenaria;

//
// Callback function called when job changed state or progress meter.
//

void JobNotify(int code, Job *job, void *ctx)
{
  switch(code)
  {
    //
    // Job changed it's state.
    //

    case JOB_NOTIFY_STATE_CHANGED:
    {
      printf("%s : changed state to %s...\n", job -> getTitle(), job -> getStateString());

      break;
    }

    //
    // Job changed it's progress meter.
    //

    case JOB_NOTIFY_PROGRESS:
    {
      printf("%s : %3.1lf completed...\n", job -> getTitle(), job -> getPercentCompleted());

      break;
    }
  }
}

//
// Function performing our time consuming job.
//

void DoMyJob(Job *job, void *ctx)
{
  for (int i = 0; i <= 100; i++)
  {
    job -> setPercentCompleted(i);

    ThreadSleepMs(100);

    //
    // Check for cancel signal.
    //

    if (job -> getState() == JOB_STATE_STOPPED)
    {
      return;
    }
  }

  //
  // Mark job as finished.
  //
  // TIP: Use JOB_STATE_ERROR to mark job finished with error.
  //

  job -> setState(JOB_STATE_FINISHED);
}

//
// Entry point.
//

int main()
{
  //
  // Create new job.
  //

  Job *job = new Job("MyJob", JobNotify, NULL, DoMyJob, NULL);

  //
  // Wait until job finished.
  //
  // TIP#1: Use timeout parameter if you have time limit.
  // TIP#2: Use job -> cancel() to cancel pending job.
  //

  job -> wait();

  //
  // Check is job finished with success or error.
  //

  if (job -> getState() == JOB_STATE_FINISHED)
  {
    printf("OK.\n");
  }
  else
  {
    printf("Job finished with error no. %d.\n", job -> getErrorCode());
  }

  //
  // Release job object.
  //

  job -> release();

  return 0;
}
