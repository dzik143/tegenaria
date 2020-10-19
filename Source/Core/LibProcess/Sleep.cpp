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

#ifdef WIN32
# include <windows.h>
#else
# include <errno.h>
# include <stdio.h>
# include <unistd.h>
# include <signal.h>
# include <wait.h>
# include <sys/stat.h>
#endif

#include <Tegenaria/Debug.h>
#include <Tegenaria/Str.h>
#include "Process.h"

namespace Tegenaria
{
  // --------------------------------------------------------------------------
  //
  //                             Sleep functions
  //
  // --------------------------------------------------------------------------

  //
  // Sleep current thread for given number of microseconds.
  //

  void ProcessSleepUs(int us)
  {
    #ifdef WIN32
    {
      //
      // TODO: Handle us on Windows.
      //

      if (us < 1000)
      {
        Sleep(1);
      }
      else
      {
        Sleep(us / 1000);
      }
    }
    #else
    usleep(us);
    #endif
  }

  //
  // Sleep current thread for given number of miliseconds.
  //

  void ProcessSleepMs(int ms)
  {
    #ifdef WIN32
    Sleep(ms);
    #else
    usleep(ms * 1000);
    #endif
  }

  //
  // Sleep current thread for given number of seconds.
  //

  void ProcessSleepSec(double seconds)
  {
    #ifdef WIN32
    Sleep(seconds * 1000);
    #else
    usleep(seconds * 1000000);
    #endif
  }
} /* namespace Tegenaria */
