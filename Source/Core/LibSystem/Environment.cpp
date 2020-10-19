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
  // Retrieve all environment variables as string |-> string map.
  //
  // env - std string to string map, where to store retrieved
  //       environment block (OUT).
  //
  // RETURNS: 0 if OK.
  //

  int SystemGetEnvironmentBlock(map<string, string> &env)
  {
    DBG_ENTER("SystemGetEnvironment");

    int exitCode = -1;

    env.clear();

    char *delim  = NULL;
    char *tmp    = NULL;
    char *lvalue = NULL;
    char *rvalue = NULL;
    char *ptr    = NULL;

    //
    // Windows.
    //

    #ifdef WIN32
    {
      char *block = GetEnvironmentStrings();

      ptr = block;

      while(*ptr)
      {
        tmp = strdup(ptr);

        delim = strchr(tmp + 1, '=');

        if (delim)
        {
          *delim = 0;

          lvalue = tmp;
          rvalue = delim + 1;

          env[lvalue] = rvalue;
        }

        free(tmp);

        ptr = strchr(ptr, 0) + 1;
      }

      FreeEnvironmentStrings(block);
    }

    //
    // Linux.
    //

    #else
    {
      extern char **environ;

      for (int i = 0; environ[i]; i++)
      {
        tmp = strdup(environ[i]);

        delim = strchr(tmp, '=');

        if (delim)
        {
          *delim = 0;

          lvalue = tmp;
          rvalue = delim + 1;

          env[lvalue] = rvalue;
        }

        free(tmp);
      }
    }
    #endif

    //
    // Error handler.
    //

    exitCode = 0;

    DBG_LEAVE("SystemGetEnvironment");

    return exitCode;
  }
} /* namespace Tegenaria */
