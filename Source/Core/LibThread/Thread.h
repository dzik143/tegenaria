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

#ifndef Tegenaria_Core_Thread_h
#define Tegenaria_Core_Thread_h

#include <cstdlib>

namespace Tegenaria
{
  //
  // Typedef.
  //

  typedef int (*ThreadEntryProto)(void *);

  //
  // Structs.
  //

  struct ThreadHandle_t;

  //
  // Functions.
  //

  ThreadHandle_t *ThreadCreate(ThreadEntryProto entry, void *ctx = NULL);

  int ThreadIsRunning(ThreadHandle_t *th);

  int ThreadWait(ThreadHandle_t *th, int *result = NULL, int timeoutMs = -1);

  int ThreadClose(ThreadHandle_t *th);

  int ThreadKill(ThreadHandle_t *th);

  //
  // Template ThreadCrate() wrapper to hide type conversion for different
  // pointers in entry function. Examples of correct entry point:
  //
  // int ThreadEntry(SomeMyClass *ctx);
  // int ThreadEntry(void *ctx);
  // int ThreadEntry(double *ctx);
  // int ThreadEntry(string *ctx);
  // etc...
  //
  // Generally: int ThreadEntry(T *ctx),
  // where T is any data type (simple or complex).
  //

  template<class T> ThreadHandle_t *ThreadCreate(int (*entry)(T *), void *ctx)
  {
    ThreadCreate((ThreadEntryProto) entry, ctx);
  }

  //
  // Sleep functions.
  //

  void ThreadSleepUs(int ms);
  void ThreadSleepMs(int ms);
  void ThreadSleepSec(double seconds);

  //
  // General utils.
  //

  int ThreadGetCurrentId();

} /* namespace Tegenaria */

#endif /* Tegenaria_Core_Thread_H */
