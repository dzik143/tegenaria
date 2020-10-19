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

#ifndef WIN32

#include <execinfo.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/syscall.h>
#include <ctime>
#include <signal.h>
#include <sys/utsname.h>

#include "md5.h"

extern char *__progname;

static int SignalCode = -1;

//
// Print backtrace to file.
//
// fd - CRT file descriptor, where to write backtrace (IN).
//

static void PrintBacktraceToFd(int fd)
{
  const int asize = 10;

  void *array[asize];

  size_t size;

  size = backtrace(array, asize);

  if (size > 0)
  {
    backtrace_symbols_fd(array, size, fd);
  }
}

//
// Create <pid>.<timestamp>.dump file.
//

static void CreateDumpFile()
{
  FILE *f = NULL;

  char fname[1024] = {0};

  //
  // Print log to <pid>.<timestamp>.dump
  //

  if (getenv("CRASHREPORT_DIR"))
  {
    sprintf(fname, "%s/%d.%d.dump", getenv("CRASHREPORT_DIR"), (int) getpid(), (int) time(NULL));
  }
  else
  {
    sprintf(fname, "%d.%d.dump", (int) getpid(), (int) time(NULL));
  }

  f = fopen(fname, "wt+");

  if (f)
  {
    MD5 md5;

    const char *signalName = "Unknown";

    struct timeval nowMs;

    int ms = 0;

    char timestamp[128] = {0};

    time_t now = time(0);

    struct tm tstruct = *localtime(&now);

    struct utsname utsnamebuf = {0};

    //
    // Timestamp.
    //

    gettimeofday(&nowMs, NULL);

    ms = nowMs.tv_usec / 1000;

    strftime(timestamp, sizeof(timestamp), "%F %X", &tstruct);

    fprintf(f, "Unhandled exception at %s %d.\n\n", timestamp, ms);

    fflush(f);

    //
    // Process info.
    //

    fprintf(f, "Process name : [%s].\n", __progname);
    fprintf(f, "Process ID   : [%d].\n", getpid());
    fprintf(f, "Thread ID    : [%d].\n", (int) syscall (SYS_gettid));
    fprintf(f, "Image md5    : [%s].\n\n", md5.digestFile(__progname));

    fflush(f);

    //
    // Print signal info if any.
    //

    switch(SignalCode)
    {
      case SIGABRT: signalName = "SIGABRT"; break;
      case SIGFPE:  signalName = "SIGFPE"; break;
      case SIGILL:  signalName = "SIGILL"; break;
      case SIGINT:  signalName = "SIGINT"; break;
      case SIGSEGV: signalName = "SIGSEGV"; break;
      case SIGTERM: signalName = "SIGTERM"; break;
    }

    fprintf(f, "Signal       : [%s]\n\n", signalName);

    fflush(f);

    //
    // OS version info.
    //

    if (uname(&utsnamebuf) == 0)
    {
      fprintf(f, "OS name      : [%s].\n", utsnamebuf.sysname);
      fprintf(f, "OS node name : [%s].\n", utsnamebuf.nodename);
      fprintf(f, "OS vesion    : [%s].\n", utsnamebuf.version);
      fprintf(f, "OS machine   : [%s].\n\n", utsnamebuf.machine);
    }
    else
    {
      fprintf(f, "OS info not available.\n\n");
    }

    fflush(f);

    //
    // Architecture info.
    //

    fprintf(f, "CPU cores    : [%d].\n\n", (int) sysconf(_SC_NPROCESSORS_ONLN));

    fflush(f);

    //
    // Backtrace.
    //

    PrintBacktraceToFd(fileno(f));

    fflush(f);

    fclose(f);
  }
}

//
// Signal handler. Caller by OS when one of SIGXXX signal raised.
//

static void SignalHandler(int signalCode)
{
  static int signalHandled = 0;

  if (signalHandled == 0)
  {
    signalHandled = 1;

    SignalCode = signalCode;

    CreateDumpFile();

    exit(-1);
  }
}

//
// Install our handlers when shared library loaded.
//

__attribute__((constructor)) void InitHandlers()
{
  signal(SIGABRT, SignalHandler);
  signal(SIGSEGV, SignalHandler);
}

#endif
