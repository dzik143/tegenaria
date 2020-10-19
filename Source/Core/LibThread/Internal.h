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

#include "Thread.h"

#ifndef Tegenaria_Core_Thread_Internal_H
#define Tegenaria_Core_Thread_Internal_H

namespace Tegenaria
{
  struct ThreadHandle_t
  {
    int id_;
    int isRunning_;
    int result_;
    int isResultSet_;

    #ifdef WIN32
      HANDLE handle_;
    #else
      pthread_t handle_;
    #endif
  };

  struct ThreadCtx_t
  {
    ThreadHandle_t *th_;

    ThreadEntryProto callerEntry_;

    void *callerCtx_;
  };
};

#endif /* Tegenaria_Core_Thread_Internal_H */
