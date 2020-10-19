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
#include <sys/time.h>

using namespace Tegenaria;

//
// Get current time in ms.
//

inline double GetTimeMs()
{
  struct timeval tv;

  gettimeofday(&tv, NULL);

  return tv.tv_sec * 1000.0 + tv.tv_usec / 1000.0;
}

//
// Entry point.
//

int main(int argc, char *argv[])
{
  const char *ip = NULL;

  int port = 0;

  char buf1[1024 * 32] = {0};
  char buf2[1024 * 32] = {0};

  int readed  = 0;
  int written = 0;

  int readedSum  = 0;
  int writtenSum = 0;

  double totalReaded  = 0.0;
  double totalWritten = 0.0;
  double delayTotal   = 0.0;
  double rate         = 0.0;

  int goOn = 1;

  double t0 = GetTimeMs();
  double dt = 0.0;

  double lastStatTime = 0;

  NetConnection *nc = NULL;

  //
  // Init log.
  //

  DBG_INIT_EX(NULL, "info", -1);

  //
  // Parse arguments
  //

  if (argc < 3)
  {
    Fatal("Usage: %s <ip> <port>\n", argv[0]);
  }

  ip = argv[1];

  port = atoi(argv[2]);

  //
  // Connect to echo server.
  //

  nc = NetConnect(ip, port);

  FAILEX(nc == NULL, "ERROR: Cannot connect to server.\n");

  //
  // Set no delay.
  //

  nc -> setNoDelay(1);

  printf("Connected!\n");

  //
  // Fall into main echo loop.
  //

  printf("Falling into client loop...\n");

  lastStatTime = GetTimeMs();

  while(goOn)
  {
    //
    // Generate random buffer.
    //

    for (int i = 0; i < sizeof(buf1); i++)
    {
      buf1[i] = rand() % 256;
    }

    //
    // Send buffer.
    //

    writtenSum = 0;
    written    = 1;

    while(writtenSum < sizeof(buf1) && written > 0)
    {
      written = nc -> write(buf1 + writtenSum, sizeof(buf1) - writtenSum);

      if (written > 0)
      {
        writtenSum += written;
      }
    }

    written = writtenSum;

    //
    // Read back buffer.
    //

    readedSum = 0;
    readed    = 1;

    while(readed > 0 && readedSum < written)
    {
      readed = nc -> read(buf2 + readedSum, sizeof(buf2) - readedSum);

      if (readed <= 0)
      {
        fprintf(stderr, "ERROR: Readed [%d] bytes.\n", readed);

        goOn = 0;
      }
      else
      {
        readedSum += readed;
      }
    }

    readed = readedSum;

    //
    // Compare readed and written buffers.
    //

    if (readed != written)
    {
      fprintf(stderr, "ERROR: Written [%d] bytes, but only [%d] received.\n", written, readed);

      goOn = 0;
    }

    if (memcmp(buf1, buf2, sizeof(buf1)) != 0)
    {
      fprintf(stderr, "ERROR: Written and received buffers mismatch.\n");

      goOn = 0;
    }

    totalReaded  += readed / 1024.0 / 1024.0;
    totalWritten += written / 1024.0 / 1024.0;

    //
    // Statistics every 1s.
    //

    dt = (GetTimeMs() - lastStatTime) / 1000.0;

    if (dt > 1.0)
    {
      lastStatTime = GetTimeMs();

      dt = (GetTimeMs() - t0) / 1000.0;

      rate = totalReaded / dt;

      printf("total sent = %lf MB, avg rate = %lf MB/s\n", totalReaded, rate);
    }
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
