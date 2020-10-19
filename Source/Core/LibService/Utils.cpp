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

#define WINVER 0x501

#include "Utils.h"

namespace Tegenaria
{
  // Transform relative (to where dlsetup stored) path into full path.
  // E.g. it transforms "/somedir" into "<where is dlsetup.exe>/somedir".
  //
  // WARNING: If passed path is already absolute it returns the same string.
  //
  // normPath - full, generated path (OUT).
  // pathSize - size of path buffer in bytes (IN).
  // relative - relative path postfix to add to base path (IN).
  // quiet    - do not write logs if set to 1 (IN).
  //
  // RETURNS: 0 if OK.
  //

  int ExpandRelativePath(char *normPath, int pathSize, const char *relative, int quiet)
  {
    DBG_ENTER2("ExpandRelativePath");

    int exitCode = -1;

    int lastSlash = 0;

    char *path = (char *) malloc(pathSize);

    FAILEX(path == NULL, "ERROR: Out of memory.\n");

    //
    // Check is it already absolute?
    //

    if (relative && relative[0] && relative[1] == ':')
    {
      DEBUG1("Path [%s] is already absolute.\n", relative);

      strncpy(path, relative, pathSize);
    }
    else
    {
      DEBUG3("Expanding path [%s]...\n", relative);

      //
      // Get full path to nxdevice.exe binary.
      //

      FAIL(GetModuleFileName(NULL, path, pathSize) == FALSE);

      //
      // Remove filename from path.
      //

      for (int i = 0; path[i] && i < pathSize; i++)
      {
        if (path[i] == '\\' || path[i] == '/')
        {
          lastSlash = i;
        }
      }

      path[lastSlash] = 0;

      //
      // Add relative postfix if any.
      //

      if (relative)
      {
        strncat(path, "\\", pathSize);
        strncat(path, relative, pathSize);
      }
    }

    //
    // Normalize path for output (e.g. remove '..' entries).
    //

    for (int i = 0; path[i]; i++)
    {
      if (path[i] == '/')
      {
        path[i] = '\\';
      }
    }

    PathCanonicalize(normPath, path);

    exitCode = 0;

    DEBUG1("Path [%s] expanded to [%s].\n", relative, normPath);

    //
    // Error handler.
    //

    fail:

    if (exitCode)
    {
      int err = GetLastError();

      Error("ERROR: Cannot retrieve path of binary.\n"
                "Error code is : %d.\n", err);

      if (err != 0)
      {
        exitCode = err;
      }
    }

    if (path != NULL)
    {
      free(path);
    }

    DBG_LEAVE2("ExpandRelativePath");

    return exitCode;
  }

  //
  // Enable or disable privilege for current running process
  //
  // privName - privilege name (IN)
  // enabled  - 1 for enabling, 0 for disabling (IN)
  //
  // RETURNS: 0 if OK.
  //

  int SetPrivilege(const char *privName, int enabled)
  {
    TOKEN_PRIVILEGES tp;

    HANDLE hProcToken = NULL;

    LUID luid;

    int exitCode = 1;

    //
    // Retrievie LUID from privilege name
    //

    FAIL(LookupPrivilegeValue(NULL, privName, &luid) == FALSE);

    //
    // Retrievie token for current running process
    //

    FAIL(OpenProcessToken(GetCurrentProcess(),
                              TOKEN_ADJUST_PRIVILEGES, &hProcToken) == FALSE);

    //
    // Adjust privilege to current running process
    //

    tp.PrivilegeCount           = 1;
    tp.Privileges[0].Luid       = luid;
    tp.Privileges[0].Attributes = enabled ? SE_PRIVILEGE_ENABLED : 0;

    FAIL(AdjustTokenPrivileges(hProcToken, FALSE, &tp,
                                   sizeof(TOKEN_PRIVILEGES), NULL, NULL) == FALSE);

    exitCode = 0;

    fail:

    //
    // Free allocated memory if needed.
    //

    if (hProcToken)
    {
      CloseHandle(hProcToken);
    }

    if (exitCode)
    {
      DWORD err = GetLastError();

      Error("ERROR. Cannot enable privilege"
                " to current process (%d).", (int) err);
    }

    return exitCode;
  }
} /* namespace Tegenaria */
