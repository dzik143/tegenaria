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

#include <cstdio>
#include <Tegenaria/Process.h>
#include <Tegenaria/Mutex.h>
#include <Tegenaria/Debug.h>

#include <cstdlib>

#define N_SLAVES 16

using namespace Tegenaria;

int Count = 0;

Mutex CountMutex;

//
// Callback handler to catch process dead or timeout.
//

int CatchProcessCb(int pid, void *ctx)
{
  printf("Process PID #%d signaled.\n", pid);

  CountMutex.lock();
  Count --;
  CountMutex.unlock();

  printf("%d processes left.\n", Count);
}

//
// Entry point.
//

int main(int argc, char **argv)
{
  DBG_INIT("libprocess-example03-monitor.log");

  //
  // No argument specified.
  // Master process.
  // Create N slave processes with random time life
  // and monitor how they become dead or timeout.
  //

  if (argc == 1)
  {
    const char *param[4] =
    {
      argv[0],
      argv[0],
      "--slave",
      NULL
    };

    //
    // Create N childs.
    //

    for (int i = 0; i < N_SLAVES; i++)
    {
      //
      // Create process.
      //

      printf("Creating slave #%d...\n", i);

      ProcessHandle_t *proc = ProcessCreate(param);

      //
      // Start monitoring it with random 0-10s timeout.
      //

      ProcessWatch(proc, CatchProcessCb, rand() % 10000);

      //
      // Count number of processes.
      //

      Count ++;
    }

    //
    // Wait until at least one child left.
    //

    printf("Waiting until processes dead...\n");

    while(Count > 0)
    {
      ProcessSleepSec(1);
    }
  }

  //
  // Argument specified.
  // Create slave worker with random life time.
  //

  else
  {
    printf("Hello from slave!\n");

    ProcessSleepMs(rand() % 10000);

    printf("Slave going to dead...\n");
  }

  return 0;
}
