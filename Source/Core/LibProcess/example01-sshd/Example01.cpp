/******************************************************************************/
/*                                                                            */
/* Copyright (c) 2010, 2014 Lukasz Bienczyk <lukasz.bienczyk@gmail.com>,      */
/* Radoslaw Kolodziejczyk <radek.kolodziejczyk@gmail.com>,                    */
/* Sylwester Wysocki <sw143@wp.pl>                                            */
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

#ifndef _WIN32
# include <errno.h>
# include <unistd.h>
# include <signal.h>
# include <wait.h>
#endif

#include <Tegenaria/Process.h>
#include <Tegenaria/Debug.h>
#include <cstdio>

using namespace Tegenaria;

/*
TODO: Obsolete due to removed ProcessCreateRead().

int runSshd(int port)
{
  //
  // Start sshd and return it's pid
  // and change port to pid.
  //

  int killAfterWhile = 1;

  char buffer[33];

  snprintf(buffer, sizeof(buffer), "%d", port);

  const char *parameters[6];

  parameters[0] = "/usr/sbin/sshd";
  parameters[1] = "-De";
  parameters[2] = "-p";
  parameters[3] = buffer;
  parameters[4] = NULL;

  ProcessHandle_t *sshd = ProcessCreate(parameters);

  printf("sshd pid: [%d]\n", ProcessGetPidByHandle(sshd));

  int timer = 0;

  for (;;)
  {
    ProcessSleepSec(1);

    printf("sshd running: [%d].\n", ProcessIsRunning(sshd));

    if (ProcessIsRunning(sshd) == 0)
    {
      break;
    }

    timer++;

    if (timer == 5)
    {
      printf("Process wait started.\n");

      int resultCode = 1;

      int result = ProcessWait(sshd, 10000, &resultCode);

      printf("Process wait finished with the result [%d] code [%d].\n", result, resultCode);
    }

    if (timer == 10 && killAfterWhile == 1)
    {
      printf("going to kill sshd.\n");

      ProcessKill(sshd);
    }
  }

  printf("sshd running:  %d\n", ProcessIsRunning(sshd));

  return 0;
}


int main(int argc, char **argv)
{
  Fatal("Not implemented.\n");
/*
  runSshd(1337);

  char *result = NULL;

  const char *parameters[6];

  parameters[0] = "/usr/bin/ssh";
  parameters[1] = "/usr/bin/ssh";
  parameters[2] = "-v";
  parameters[3] = NULL;

  ProcessCreateRead(parameters, result);

  printf("ProcessExample: read: %s.\n", result);

  printf("ProcessExample: FINISHED\n");

  return 0;
}
*/

int main()
{
  fprintf(stderr, "This example is obselete due to removed ProcessCreateRead() function.\n");
}
