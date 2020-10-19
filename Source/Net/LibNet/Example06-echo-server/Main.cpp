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

#include <Tegenaria/Net.h>

using namespace Tegenaria;

//
// Entry point.
//

int main(int argc, char *argv[])
{
  char buf[1024 * 32] = {0};

  int readed  = 0;
  int written = 0;

  int totalReaded  = 0;
  int totalWritten = 0;

  int writtenSum = 0;

  int count = 0;
  int goOn  = 1;

  int port = -1;

  NetConnection *nc = NULL;

  //
  // Init log.
  //

  DBG_INIT_EX(NULL, "info", -1);

  //
  // Parse arguments
  //

  if (argc < 2)
  {
    Fatal("Usage: %s <port>\n", argv[0]);
  }

  port = atoi(argv[1]);

  //
  // Wait for connection on given port.
  //

  nc = NetAccept(port);

  FAILEX(nc == NULL, "ERROR: Cannot accept connection.\n");

  printf("Connected!\n");

  //
  // Fall into main echo loop.
  //

  printf("Falling into server loop...\n");

  while(goOn)
  {
    //
    // Read packet.
    //

    readed = nc -> read(buf, sizeof(buf));

    if (readed <= 0)
    {
      fprintf(stderr, "ERROR: Readed [%d] bytes.\n", readed);

      goOn = 0;
    }

    //
    // Write back packet.
    //

    writtenSum = 0;

    while(writtenSum < readed)
    {
      written = nc -> write(buf + writtenSum, readed - writtenSum);

      if (written > 0)
      {
        writtenSum += written;
      }
    }

    written = writtenSum;

    if (written != readed)
    {
      fprintf(stderr, "ERROR: Written [%d] bytes but readed [%d].\n", written, readed);

      goOn = 0;
    }

    totalReaded  += readed;
    totalWritten += written;
  }

  printf("Exited.\n");

  fail:

  getchar();

  if (nc)
  {
    nc -> release();
  }

  return 0;
}
