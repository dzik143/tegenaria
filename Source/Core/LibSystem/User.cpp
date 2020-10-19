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

#include "System.h"

namespace Tegenaria
{
  //
  // Check does current process has admin privilege.
  //
  // RETURNS: 0 if current process has admin privilege,
  //          1 if current process has NO admin privilege,
  //         -1 if error.
  //

  int SystemCheckForAdmin()
  {
    DBG_ENTER("SystemCheckForAdmin");

    const int EXIT_ERROR    = -1;
    const int EXIT_NO_ADMIN =  1;
    const int EXIT_OK       =  0;

    int exitCode = EXIT_ERROR;

    //
    // Windows.
    //

    #ifdef WIN32
    {

      HANDLE token = NULL;

      PSID adminSid = NULL;

      TOKEN_GROUPS *tokenGroups = NULL;;

      DWORD cbSize = 0;

      SID_IDENTIFIER_AUTHORITY sidIdAuth = {SECURITY_NT_AUTHORITY};

      int adminFound = 0;

      //
      // Get current process token.
      //

      DEBUG3("CheckForAdmin : retrieving current process token...\n");

      FAIL(OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token) == FALSE);

      //
      // Retrieve groups from token.
      //

      DBG_MSG("CheckForAdmin : retrieving token's groups...\n");

      GetTokenInformation(token, TokenGroups, NULL, 0, &cbSize);

      tokenGroups = (PTOKEN_GROUPS) LocalAlloc(LPTR, cbSize);

      FAIL(GetTokenInformation(token, TokenGroups, tokenGroups,
                                   cbSize, &cbSize) == FALSE);

      //
      // Create admin group SID for reference.
      //

      DBG_MSG("CheckForAdmin : creating admin group SID...\n");

      FAIL(AllocateAndInitializeSid(&sidIdAuth, 2, SECURITY_BUILTIN_DOMAIN_RID,
                                        DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0,
                                            0, &adminSid) == FALSE);

      //
      // Search admin group SID in current process token.
      //

      DBG_MSG("CheckForAdmin : searching for admin group...\n");

      for (unsigned int i = 0; i < tokenGroups -> GroupCount && adminFound == 0; i++)
      {
        if (EqualSid(adminSid, tokenGroups -> Groups[i].Sid))
        {
          adminFound = 1;
        }
      }

      //
      // If admin account found, try some admin privilege function
      // to exclude we are blocked by UAC.
      //

      exitCode = EXIT_NO_ADMIN;

      if (adminFound)
      {
        DBG_MSG("CheckForAdmin : trying privilege function...\n");

        SC_HANDLE scm = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);

        if (scm)
        {
          CloseServiceHandle(scm);

          exitCode = EXIT_OK;
        }
      }

      //
      // Clean up.
      //

      fail:

      if (exitCode == EXIT_ERROR)
      {
        int err = GetLastError();

        Error("ERROR: Cannot to check for admin privilege.\n"
                  "Error code is : %d.\n", err);

        if (err != 0)
        {
          exitCode = err;
        }
      }

      CloseHandle(token);

      if (adminSid)
      {
        FreeSid(adminSid);
      }

      if (tokenGroups)
      {
        LocalFree(tokenGroups);
      }

      DBG_MSG("CheckForAdmin : Admin check status : [%d]\n", exitCode);
    }

    //
    // Linux, MacOS.
    //

    #else
    {
      DEBUG1("WARNING: CheckForAdmin() not implemented on Linux. Assumed success.\n");

      exitCode = EXIT_OK;
    }
    #endif

    DBG_LEAVE("SystemCheckForAdmin");

    return exitCode;
  }

  //
  // Retrieve name of current running user.
  //
  // user     - buffer, where to store retrieved username (OUT).
  // userSize - size of user[] buffer in bytes (IN).
  //
  // RETURNS: 0 if OK.
  //

  int SystemGetCurrentUser(char *user, int userSize)
  {
    int exitCode = -1;

    //
    // Check args.
    //

    FAILEX(user == NULL, "ERROR: 'user' cannot be NULL in SystemGetCurrentUser().\n");
    FAILEX(userSize <= 0, "ERROR: 'userSize' cannot be <= 0 in SystemGetCurrentUser().\n");

    //
    // Windows.
    //

    #ifdef WIN32
    {
      FAIL(GetUserName(user, PDWORD(&userSize)) == FALSE);
    }

    //
    // Linux.
    //

    #else
    {
      FAIL(getlogin_r(user, userSize));
    }
    #endif

    //
    // Error handler.
    //

    exitCode = 0;

    fail:

    if (exitCode)
    {
      Error("ERROR: Cannot retrieve username. System error is %d.\n", GetLastError());
    }

    return exitCode;
  }
} /* namespace Tegenaria */
