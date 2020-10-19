/**************************************************************************/
/*                                                                        */
/* This file is part of Tegenaria project.                                */
/* Copyright (c) 2010, 2014                                               */
/*   Radoslaw Kolodziejczyk (radek.kolodziejczyk@gmail.com),              */
/*   Lukasz Bienczyk (lukasz.bienczyk@gmail.com).                         */
/*                                                                        */
/* The Tegenaria library and any derived work however based on this       */
/* software are copyright of Sylwester Wysocki. Redistribution and use of */
/* the present software is allowed according to terms specified in the    */
/* file LICENSE which comes in the source distribution.                   */
/*                                                                        */
/* All rights reserved.                                                   */
/*                                                                        */
/**************************************************************************/

#ifndef WIN32
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <wait.h>
#endif

#include <stdio.h>
#include <Tegenaria/Thread.h>

using namespace Tegenaria;

int loop1(int *result)
{
  printf("ThreadExample: loop1 started with a parameter :  %d\n", *result);

  for (int x = 0; x < 5; x++)
  {
    printf("ThreadExample: LOOP nr %d\n",int(x));

    ThreadSleepSec(2);
  }

  *result = 10;

  printf("ThreadExample: loop1 finished with a result :  %d\n", *result);

  return *result;
}

int main(int argc, char **argv)
{
  int port   = 1337;
  int timer  = 0;
  int finish = 15;

  ThreadHandle_t *thread = NULL;

  //
  // Example of starting thread
  //

  printf("\nThreadExample: Starting method loop.\n");

  thread = ThreadCreate((ThreadEntryProto) loop1, &port);

  for (;;)
  {
    ThreadSleepSec(1);

    printf("ThreadExample: thread running:  %d\n", ThreadIsRunning(thread));

    if (ThreadIsRunning(thread) == 0)
    {
      printf("ThreadExample: thread is not running  \n");

      break;
    }

    if (timer == finish)
    {
      printf("ThreadExample: going to kill thread \n");

      ThreadKill(thread);
    }

    timer++;
  }

  printf("ThreadExample: FINISH THREAD EXAMPLE\n");

  return 0;
}
