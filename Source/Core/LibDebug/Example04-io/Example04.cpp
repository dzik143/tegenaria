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
#include <cstdlib>

#ifdef WIN32
# include <io.h>
#else
# include <unistd.h>
#endif

using namespace Tegenaria;

//
// Entry point.
//

int main(int argc, char **argv)
{
  char buf1[]     = "Hello world.\nWrite something on stdin.\n";
  char buf2[1024] = {0};

  int written = -1;
  int readed  = -1;

  //
  // Init log with IO debug enabled.
  //

  DBG_INIT_EX(NULL, "debug3", DBG_IO_ENABLE);

  //
  // Write something.
  //

  DBG_IO_WRITE_BEGIN("fd", 1, buf1, sizeof(buf1));
  {
    written = write(1, buf1, sizeof(buf1));
  }
  DBG_IO_WRITE_END("fd", 1, buf1, sizeof(buf1));

  //
  // Write something.
  //

  DBG_IO_READ_BEGIN("fd", 0, buf2, sizeof(buf2));
  {
    readed = read(0, buf2, sizeof(buf2));
  }
  DBG_IO_READ_END("fd", 0, buf2, readed);

  //
  // Close stdout.
  //

  DBG_IO_CLOSE_BEGIN("fd", 1);
  {
    close(1);
  }
  DBG_IO_CLOSE_END("fd", 1);

  return 0;
}
