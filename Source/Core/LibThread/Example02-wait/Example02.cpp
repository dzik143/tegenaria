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
#include <Tegenaria/Debug.h>

using namespace Tegenaria;

int WorkerMain(void *unused)
{
  int exitCode = 0;

  printf("-> worker\n");

  printf("worker: Enter exit code and press enter to finish thread.\n");

  scanf("%d", &exitCode);

  printf("<- worker\n");

  return exitCode;
}

//
// Entry point.
//

int main(int argc, char **argv)
{
  DBG_INIT("libthread-example02-wait.log");

  ThreadHandle_t *thread = NULL;

  int threadResult = -1;

  //
  // Create thread.
  //

  printf("Creating worker thread...\n");

  thread = ThreadCreate(WorkerMain);

  //
  // Wait up to 5s to finish.
  //

  if (ThreadWait(thread, &threadResult, 5000) == 0)
  {
    printf("Thread finished with status [%d].\n", threadResult);
  }

  //
  // If not finished witin 5s, kill it.
  //

  else
  {
    printf("Thread timeout. Killing him!\n");

    ThreadKill(thread);
  }

  //
  // Clean up.
  //

  ThreadClose(thread);

  return 0;
}
