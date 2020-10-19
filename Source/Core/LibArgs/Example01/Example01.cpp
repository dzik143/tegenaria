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

using namespace Tegenaria;

int main(int argc, char **argv)
{
  #define MODE_ERROR -1
  #define MODE_1      1
  #define MODE_2      2
  #define MODE_3      3

  int mode = MODE_ERROR;

  double thre = 0.0;

  int n_vars = 0;

  char *name = NULL;

  int quiet = 0;

  //
  // Declare known options.
  //

  ArgsObj ao[] =
  {
    {ARGS_MODE,    "--mode1", &mode,   MODE_1},
    {ARGS_MODE,    "--mode2", &mode,   MODE_2},
    {ARGS_MODE,    "--mode3", &mode,   MODE_3},

    {ARGS_INT,     "--nvars", &n_vars},
    {ARGS_DOUBLE,  "--thre",  &thre},
    {ARGS_STRING,  "--name",  &name},
    {ARGS_FLAG,    "--quiet", &quiet},

    //
    // End of table terminator.
    //

    {ARGS_NULL,    NULL,      NULL}
  };

  //
  // Parse options.
  //

  ArgsParse(ao, argc, argv);

  //
  // Show parsed result.
  //

  printf("mode      : [%d]\n", mode);
  printf("nvars     : [%d]\n", n_vars);
  printf("threshold : [%lf]\n", thre);
  printf("name      : [%s]\n", name);
  printf("quiet     : [%d]\n", quiet);

  return 0;
}
