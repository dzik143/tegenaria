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

#include <cstdio>
#include <Tegenaria/Args.h>
#include <Tegenaria/Debug.h>

using namespace Tegenaria;

#define MODE_ERROR  -1

#define MODE_SHARE   1
#define MODE_UNSHARE 2
#define MODE_VERSION 4
#define MODE_HELP    5
#define MODE_EXIT    6


int InterpreterLoop(FILE *fout, FILE *fin, FILE *ferr)
{
  int exitCode = -1;

  int mode = MODE_ERROR;

  char *alias = NULL;
  char *path  = NULL;

  int goOn = 1;

  char line[1024] = {0};

  //
  // Declare known commands.
  //

  ArgsObj ao[] =
  {
    //
    // share --alias <alias> --path <path>
    //

    {ARGS_MODE,   "share",   &mode,   MODE_SHARE},
    {ARGS_STRING, "--alias", &alias             },
    {ARGS_STRING, "--path",  &path              },

    //
    // unshare --alias <alias>
    //

    {ARGS_MODE, "unshare", &mode, MODE_UNSHARE},

    //
    // version
    //

    {ARGS_MODE, "version", &mode, MODE_VERSION},

    //
    // help
    //

    {ARGS_MODE, "help", &mode, MODE_HELP},

    //
    // exit
    //

    {ARGS_MODE, "exit", &mode, MODE_EXIT},

    //
    // End of table terminator.
    //

    {ARGS_NULL}
  };

  //
  // Fall into main interpreter loop.
  //

  while(goOn)
  {
    mode = MODE_ERROR;

    //
    // Parse next command line.
    //

    printf(">");

    FAILEX(fgets(line, sizeof(line), fin) == NULL,
               "ERROR: Cannot read command from stdin.\n");

    if (ArgsParse(ao, line) == 0)
    {
      //
      // If command sucessfully parsed dispatch control
      // depending on received 'work mode'.
      //

      switch(mode)
      {
        case MODE_VERSION:
        {
          fprintf(fout, "ARGS-EXAMPLE-SHELL 0.1\n");

          break;
        }

        case MODE_SHARE:
        {
          fprintf(fout, "Shared path [%s] as alias [%s].\n", path, alias);

          break;
        }

        case MODE_UNSHARE:
        {
          fprintf(fout, "Unshared alias [%s].\n", alias);

          break;
        }

        case MODE_HELP:
        {
          fprintf
          (
            fout,
            "\n"
            "help                                : show this help message\n"
            "version                             : show intepreter version\n"
            "share --path <path> --alias <alias> : Share <path> as alias <alias>\n"
            "unshare --alias <alias>             : Unshare alias <alias>\n"
            "exit                                : Finish session\n"
            "\n"
          );

          break;
        }

        case MODE_EXIT:
        {
          fprintf(fout, "Logout.\n");

          goOn = 0;

          break;
        }
      }
    }
  }

  //
  // Clean up.
  //

  fail:

  return exitCode;
}

int main(int argc, char **argv)
{
  InterpreterLoop(stdout, stdin, stderr);

  return 0;
}
