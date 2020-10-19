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

#undef  DEBUG

#include <cstdio>
#include <cstring>
#include <cstdlib>

#include <Tegenaria/Str.h>
#include <Tegenaria/Debug.h>

#include "Args.h"

namespace Tegenaria
{
  //
  // Process arguments.
  //

  int ArgsParse(ArgsObj *ao, int argc, char **argv)
  {
    DBG_ENTER("ArgsParse");

    int exitCode = -1;

    for (int i = 1; i < argc; i++)
    {
      int argsToEnd = argc - i - 1;

      int found = 0;

      DBG_MSG("Checking [%s]...\n", argv[i]);

      for (int j = 0; ao[j].type_ != ARGS_NULL; j++)
      {
        //
        // Matching pattern found.
        //

        if (strcmp(ao[j].pattern_, argv[i]) == 0)
        {
          found = 1;

          //
          // --mode.
          //

          if (ao[j].type_ == ARGS_MODE)
          {
            DBG_MSG("Setting mode [%d]...\n", ao[j].intVal_);

            *((int *) ao[j].target_) = ao[j].intVal_;
          }

          //
          // --flag.
          //

          else if (ao[j].type_ == ARGS_FLAG)
          {
            *((int *) ao[j].target_) = 1;
          }

          //
          // --option arg
          //

          else
          {
            FAILEX(argsToEnd == 0, "ERROR: Missing argument for [%s].\n", argv[i]);

            i ++;

            DBG_MSG("Setting [%s] to [%s]...\n", ao[j].pattern_, argv[i]);

            switch(ao[j].type_)
            {
              case ARGS_INT:    *((int *)   (ao[j].target_)) = atoi(argv[i]); break;
              case ARGS_DOUBLE: *((double *)(ao[j].target_)) = atof(argv[i]); break;
              case ARGS_FLOAT:  *((float *) (ao[j].target_)) = atof(argv[i]); break;
              case ARGS_STRING: *((char **) (ao[j].target_)) = argv[i];       break;
              case ARGS_CHAR:   *((char *)  (ao[j].target_)) = argv[i][0];    break;
            }
          }
        }
      }

      //
      // Unknown option.
      //

      FAILEX(!found, "ERROR: Unknown option at [%s].\n", argv[i]);
    }

    //
    // Clean up.
    //

    exitCode = 0;

    fail:

    if (exitCode)
    {
      Error("ERROR: Cannot process input arguments.\n");
    }

    DBG_LEAVE("ArgsParse");

    return exitCode;
  }

  //
  // Wrapper for ArgsParse() working with one continous
  // string on input.
  //
  // RETURNS: 0 if OK.
  //

  int ArgsParse(ArgsObj *ao, const char *cmd)
  {
    DBG_ENTER("ArgsParse");

    int exitCode = -1;

    int argc = 0;

    char **argv = NULL;

    //
    // Convert one continous string argStr into {argc, argv[]}.
    //

    FAIL(ArgsAlloc(&argc, &argv, cmd));

    //
    // Pass control to oryginal function working on {argc, argv[]}.
    //

    FAIL(ArgsParse(ao, argc, argv));

    //
    // Clean up.
    //

    exitCode = 0;

    fail:

    if (argv)
    {
      ArgsFree(argc, argv);
    }

    DBG_LEAVE("ArgsParse");

    return exitCode;
  }

  //
  // Allocate argv[] table from continous string.
  //
  // WARNING: Output argv[] MUST be free by caller via ArgsFree().
  //
  // argc   - number of elements in argv[] (OUT).
  // argv   - table with tokenized arguments (OUT).
  // argStr - command line string to tokenize (IN).
  //
  // RETURNS: 0 if OK.
  //

  int ArgsAlloc(int *argc, char ***argv, const char *cmd)
  {
    DBG_ENTER("ArgsAlloc");

    int exitCode = -1;

    vector<char *> tokens;

    string cmdStr = cmd;

    //
    // Split command into tokens.
    //

    StrTokenize(tokens, (char *) cmd, " \t\n\r", "\"'");

    //
    // Allocate argv[].
    //

    *argc = tokens.size() + 1;

    *argv = (char **) calloc(*argc + 2, sizeof(char *));

    FAILEX(*argv == NULL, "ERROR: Out of memory.\n");

    //
    // Put tokens into argv[].
    //

    for (int i = 0; i < tokens.size(); i++)
    {
      DBG_MSG("ArgsAlloc : Putting [%s] at argv[%d]...\n", tokens[i], i);

      (*argv)[i + 1] = tokens[i];
    }

    //
    // Clean up.
    //

    exitCode = 0;

    fail:

    DBG_LEAVE("ArgsAlloc");

    return exitCode;
  }

  //
  // Free argv[] table allocated by ArgsAlloc().
  //

  void ArgsFree(int argc, char **argv)
  {
    if (argv)
    {
  /*
      for (int i = 0; i < argc; i++)
      {
        if (argv[i])
        {
          free(argv[i]);
        }
      }
  */

      free(argv);
    }
  }
} /* namespace Tegenaria */
