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

#ifdef WIN32
# include <windows.h>
# include <io.h>
# include <fcntl.h>
#else
# include <errno.h>
# include <stdio.h>
# include <unistd.h>
# include <signal.h>
# include <wait.h>
# include <sys/stat.h>
#endif

#include <Tegenaria/Debug.h>
#include <Tegenaria/Str.h>
#include "Process.h"
#include "Internal.h"

namespace Tegenaria
{
  // --------------------------------------------------------------------------
  //
  //                  Helper functions, internall only.
  //
  // --------------------------------------------------------------------------

  //
  // Allocate new ProcessHandle_t struct basing on system handle/pid data.
  //
  // handle - underlying system handle (IN).
  // pid    - underlying process id (IN).
  //
  // RETURNS: Pointer to new allocated handle.
  //

  static ProcessHandle_t *_ProcessHandleAlloc(void *handle, int pid)
  {
    #pragma qcbuild_set_private(1)

    ProcessHandle_t *rv = (ProcessHandle_t *) calloc(sizeof(ProcessHandle_t), 1);

    if (rv)
    {
      rv -> handle_ = handle;
      rv -> pid_    = pid;
    }
    else
    {
      Error("ERROR: Can't allocate ProcessHandle_t. Out of memory?\n");
    }

    return rv;
  }

  // --------------------------------------------------------------------------
  //
  //                         Create process functions
  //
  // --------------------------------------------------------------------------

  //
  // USE:
  //   const char *argv[5]
  //   argv[0] = "path_to_process"
  //   argv[1] = "parameter#1"
  //   argv[2] = "parameter#2"
  //   argv[3] = "parameter#3"
  //   argv[4] = NULL
  //
  // argv[]  - NULL terminated parameter list (IN).
  // fdin    - FD to set for std input (IN/OPT).
  // fdout   - FD to set for std output (IN/OPT).
  // fderr   - FD to set for std error (IN/OPT).
  //
  // fdType  - type of specified fdin/fdout/fderr values. One of
  //           PROCESS_TYPE_XXX values defined in LibProcess.h. Defaulted
  //           to PROCESS_FDTYPE_CRT if skipped. (IN/OPT).
  //
  // options - combination of PROCESS_OPTIONS_XXX flags (IN/OPT).
  //
  // TIP #1: Set fdin/fdout/fderr to PROCESS_FD_PARENT to inherit handle from parent.
  // TIP #2: Set fdin/fdout/fderr to PROCESS_FD_NULL to redirect handle to nul.
  // TIP #3: Set fdType to PROCESS_FD_TYPE_WINAPI_HANDLE if you have raw windows handle.
  //
  // RETURNS: Pointer to ProcessHandle_t structure,
  //          or NULL if error.
  //

  ProcessHandle_t *ProcessCreate(const char *const argv[],
                                     int fdin, int fdout, int fderr,
                                         int fdType, int options)
  {
    ProcessHandle_t *rv = NULL;

    int exitCode = -1;

    //
    // Check args.
    //

    FAILEX(argv == NULL,    "ERROR: 'argv' cannot be NULL in ProcessCreate().\n");
    FAILEX(argv[0] == NULL, "ERROR: 'argv[0]' cannot be NULL in ProcessCreate().\n");

    DBG_MSG("Creating process [%s]\n", argv[0]);

    for (int i = 1; argv[i]; i++)
    {
      DBG_MSG("With parameter [%d][%s]\n", i, argv[i]);
    }

    //
    // Allocate new process handle.
    //

    rv = _ProcessHandleAlloc(NULL, -1);

    FAIL(rv == NULL);

    //
    // Windows.
    //

    #ifdef WIN32
    {
      PROCESS_INFORMATION pi = {0};

      STARTUPINFO si = {0};

      DWORD creationFlags = 0;

      char cmd[1024 * 4] = {0};

      //
      // Set std handles for child process.
      //

      si.hStdInput  = ProcessParseFdParameter(fdin,  fdType, STD_INPUT_HANDLE);
      si.hStdOutput = ProcessParseFdParameter(fdout, fdType, STD_OUTPUT_HANDLE);
      si.hStdError  = ProcessParseFdParameter(fderr, fdType, STD_ERROR_HANDLE);

      //
      // Set up STARTUPINFO.
      //

      si.cb      = sizeof(STARTUPINFO);
      si.dwFlags = STARTF_USESTDHANDLES;

      //
      // Set up command to execute in one continous string.
      //

      for (int i = 1; argv[i]; i++)
      {
        strncat(cmd, argv[i], sizeof(cmd));
        strncat(cmd, " ", sizeof(cmd));
      }

      //
      // Set up creation flags
      //

      if (options & PROCESS_OPTIONS_NO_WINDOW)
      {
        creationFlags = CREATE_NO_WINDOW;
      }

      //
      // Create new process.
      //

      DBG_MSG
      (
        "Creating process...\n"
        "command : [%s]\n"
        "fdin    : [%d/%d]\n"
        "fdout   : [%d/%d]\n"
        "fderr   : [%d/%d]\n",
        cmd,
        fdin,  si.hStdInput,
        fdout, si.hStdOutput,
        fderr, si.hStdError
      );

      FAIL(CreateProcess(NULL, cmd, NULL, NULL, TRUE,
                             creationFlags, NULL, NULL, &si, &pi) == FALSE);

      DBG_MSG("Created process pid '%d'.\n", int(pi.dwProcessId));

      //
      // Wrap system handle/pid into ProcessHandle_t struct.
      //

      rv -> handle_ = pi.hProcess;
      rv -> pid_    = pi.dwProcessId;
    }

    //
    // Linux, MacOS.
    //

    #else
    {
      Fatal("ProcessCreate() not implemented on this platform.\n");
    }
    #endif

    //
    // Error handler.
    //

    exitCode = 0;

    fail:

    if (exitCode)
    {
      const char *binaryName = "(null)";

      if (argv && argv[0])
      {
        binaryName = argv[0];
      }

      Error("ERROR: Cannot create process '%s'.\n"
                "Error code is : %d.\n", binaryName, GetLastError());

      ProcessClose(rv);
    }

    return rv;
  }
} /* namespace Tegenaria */
