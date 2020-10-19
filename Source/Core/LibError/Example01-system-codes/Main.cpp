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
#include <Tegenaria/Error.h>

using namespace Tegenaria;

int main(int argc, char **argv)
{
  FILE *f = NULL;

  const char *fname = NULL;

  if (argc < 2)
  {
    fprintf(stderr, "Usage is: %s <path>\n", argv[0]);

    return -1;
  }

  fname = argv[1];

  //
  // Try open file.
  //

  f = fopen(fname, "r");

  //
  // Error, put system code to user.
  //

  if (f == NULL)
  {
    printf("Can't open '%s' file. System error is: '%s' (%d).\n",
               fname, ErrGetSystemErrorString().c_str(), ErrGetSystemError());
  }

  //
  // Success, file opened.
  //

  else
  {
    printf("File '%s' opened.\n", fname);

    fclose(f);
  }

  return 0;
}
