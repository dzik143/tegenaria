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

#include <Tegenaria/Debug.h>

#ifndef WIN32
# include <sys/ioctl.h>
# include <net/if.h>
# include <netinet/in.h>
# include <arpa/inet.h>
# include <unistd.h>
#endif

#include <ctime>
#include <cstdlib>

namespace Tegenaria
{
  //
  // Return number of CPU cores installed on system.
  //

  int NetExGetCpuNumber()
  {
    DBG_ENTER("GetCpuNumber");

    static int cpuCount = -1;

    //
    // Windows.
    //

    #ifdef WIN32
    {
      if (cpuCount == -1)
      {
        SYSTEM_INFO si = {0};

        GetSystemInfo(&si);

        cpuCount = si.dwNumberOfProcessors;
      }
    }

    //
    // Linux, MacOS.
    //

    #else
    {
      cpuCount = sysconf(_SC_NPROCESSORS_ONLN);
    }
    #endif

    DBG_LEAVE("GetCpuNumber");

    return cpuCount;
  }
} /* namespace Tegenaria */
