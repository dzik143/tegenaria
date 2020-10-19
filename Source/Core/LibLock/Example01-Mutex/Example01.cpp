/******************************************************************************/
/*                                                                            */
/* Copyright (c) 2010, 2014 Lukasz Bienczyk <lukasz.bienczyk@gmail.com>,      */
/* Radoslaw Kolodziejczyk <radek.kolodziejczyk@gmail.com>.                    */
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

#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>

#ifndef WIN32
#include <wait.h>
#endif

#include <Tegenaria/Thread.h>
#include <Tegenaria/Process.h>
#include <Tegenaria/Mutex.h>

using namespace Tegenaria;

int loop1(void *parameter)
{
  printf("lockExample: LOOP-1 started\n");

  Mutex *mutex = (Mutex *) parameter;

  printf("lockExample: LOOP-1 lock state: %d\n", mutex -> getState());

  mutex -> lock();

  for (int x = 0; x < 5; x++)
  {
    printf("lockExample: LOOP-1 nr %d\n", int(x));

    ProcessSleepSec(2);
  }

  mutex -> unlock();

  printf("lockExample: LOOP-1 finished\n");
}

int loop2(void *parameter)
{
  printf("lockExample: LOOP-2 started\n");

  Mutex *mutex = (Mutex *) parameter;

  printf("lockExample: LOOP-2 lock state: %d\n", mutex -> getState());

  mutex -> lock();

  for (int x = 0; x < 5; x++)
  {
    printf("lockExample: LOOP-2 nr %d\n", int(x));

    ProcessSleepSec(2);
  }

  mutex -> unlock();

  printf("lockExample: LOOP-2 finished\n");
}

int main(int argc, char **argv)
{
  //
  // Example of lock
  //

  printf("\nlockExample: Starting.\n");

  Mutex mutex;

  ThreadHandle_t *thread1 = ThreadCreate(loop1, &mutex);

  ProcessSleepSec(1);

  ThreadHandle_t *thread2 = ThreadCreate(loop2, &mutex);

  for (;;)
  {
    ProcessSleepSec(1);

    if (ThreadIsRunning(thread1) == 0 && ThreadIsRunning(thread2) == 0)
    {
      printf("Mutex example: threads are not running\n");

      break;
    }
  }

  printf("lockExample: FINISH LOCK EXAMPLE\n");

  ThreadClose(thread1);
  ThreadClose(thread2);

  return 0;
}
