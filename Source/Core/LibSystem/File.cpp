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

#include "System.h"

namespace Tegenaria
{
  //
  // Retrieve current limit of allowed opened FD for current user.
  //
  // RETURNS: Current set FD limit
  //          or -1 if error.
  //

  int SystemGetFdLimit()
  {
    Error("SystemGetFdLimit() not implemented.\n");

    return -1;
  }

  // Set limit of maximum opened FD for current user.
  //
  // limit - new limit to set (IN).
  //
  // RETURNS: 0 if OK.
  //

  int SystemSetFdLimit(int limit)
  {
    DBG_ENTER3("SystemSetFDLimit");

    int exitCode = -1;

    //
    // Windows.
    //

    #ifdef WIN32
    {
      Error("SystemSetFDLimit not implemented on this platform.\n");
    }

    //
    // Linux.
    //

    #else

    //
    // Check and change if needed the per-user limit of open files
    //

    struct rlimit limitstruct;

    if(getrlimit(RLIMIT_NOFILE,&limitstruct) == -1)
    {
      Error("Could not establish user limits of open files.");

      goto fail;
    }

    DEBUG1("Polled user limits for maximum number of open files:"
               "  soft: %d; hard: %d\n", (int) limitstruct.rlim_cur, (int) limitstruct.rlim_max);

    if(limitstruct.rlim_max < limit)
    {
      // The maximum value of the maximum number of open files is currently to low.
      // We can try to increase this, but this probably will only work as root.
      // A better durable solution is to use the /etc/security/limits.conf
      //
      // Attempt to increase the limits

      limitstruct.rlim_cur = limit;
      limitstruct.rlim_max = limit;

      if(setrlimit(RLIMIT_NOFILE,&limitstruct) == -1)
      {
        Error("Could not increase hard user limit of open files to %d.\n"
              "You can either try to run this program as root, or more recommended,\n"
              "change the user limits on the system (e.g. /etc/security.limits.conf)\n", limit);

        goto fail;
      }

      DEBUG1("Changed hard & soft limit to %d.\n", limit);
    }
    else if (limitstruct.rlim_cur < limit)
    {
      //
      // The maximum limit is high enough, but the current limit might not be.
      // We should be able to increase this.
      //

      limitstruct.rlim_cur = limit;

      if(setrlimit(RLIMIT_NOFILE,&limitstruct) == -1)
      {
        Error("Could not increase soft user limit of open files to %d.\n"
              "You can either try to run this program as root, or more recommended,\n"
              "change the user limits on the system (e.g. /etc/security.limits.conf)\n", limit);

        goto fail;
      }

      DEBUG1("Changed soft limit to %d\n", limit);
    }
    else
    {
      DEBUG1("Limit was high enough\n");
    }

    exitCode = 0;

    #endif

    //
    // Error handler.
    //

    fail:

    if (exitCode)
    {
      Error("ERROR: Cannot set up open FDs limit to %d.\n", limit);
    }

    DBG_LEAVE3("SystemSetFdLimit");

    return exitCode;
  }
} /* namespace Tegenaria */
