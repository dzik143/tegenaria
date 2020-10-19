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

//
// Purpose: Functions to disable echo and read password from console.
//

#include "Secure.h"

namespace Tegenaria
{
  //
  // Disable echo on stdin.
  //
  // RETURNS: 0 if OK.
  //

  int SecureDisableEcho()
  {
    int exitCode = -1;

    //
    // Windows.
    //

    #ifdef WIN32
    {
      DWORD mode;

      FAIL(GetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), &mode) == FALSE);

      mode = mode & (~ENABLE_ECHO_INPUT);

      FAIL(SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), mode) == FALSE);
    }

    //
    // Linux, MacOS.
    //

    #else
    {
      struct termios mode;

      FAIL(tcgetattr(1, &mode));

      mode.c_lflag &= ~ECHO;

      FAIL(tcsetattr(1, TCSAFLUSH, &mode));
    }
    #endif

    //
    // Error handler.
    //

    exitCode = 0;

    fail:

    if (exitCode)
    {
      Error("ERROR: Cannot disable echo on stdin.\n"
                "Error code is : %d.\n", GetLastError());
    }

    return exitCode;
  }

  //
  // Enable echo on stdin.
  //
  // RETURNS: 0 if OK.
  //

  int SecureEnableEcho()
  {
    int exitCode = -1;

    //
    // Windows.
    //

    #ifdef WIN32
    {
      DWORD mode;

      FAIL(GetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), &mode) == FALSE);

      mode = mode | ENABLE_ECHO_INPUT;

      FAIL(SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), mode) == FALSE);
    }

    //
    // Linux, MacOS.
    //

    #else
    {
      struct termios mode;

      FAIL(tcgetattr(1, &mode));

      mode.c_lflag |= ECHO;

      FAIL(tcsetattr(1, TCSAFLUSH, &mode));
    }
    #endif

    //
    // Error handler.
    //

    exitCode = 0;

    fail:

    if (exitCode)
    {
      Error("ERROR: Cannot enable echo on stdin.\n"
                "Error code is : %d.\n", GetLastError());
    }

    return exitCode;
  }

  //
  // Disable echo and read password from stdin.
  //
  // pass     - buffer, where to store readed password (OUT).
  // passSize - size of pass[] buffer in bytes (IN).
  // prompt   - optional prompt to show before ask for pass (IN/OPT).
  //
  // RETURNS: 0 if OK.
  //

  int SecureReadPassword(char *pass, int passSize, const char *prompt)
  {
    DBG_ENTER("SecureReadPassword");

    int exitCode = -1;

    char *ret = NULL;

    //
    // Check args.
    //

    FAILEX(pass == NULL, "ERROR: 'pass' cannot be NULL in SecureReadPassword().\n");
    FAILEX(passSize < 1, "ERROR: 'passSize' cannot be < 1 in SecureReadPassword().\n");

    //
    // Show prompt if specified.
    //

    if (prompt)
    {
      printf("%s", prompt);
    }

    fflush(stdout);

    //
    // Disable echo.
    //

    FAIL(SecureDisableEcho());

    //
    // Read password from stdin.
    //

    ret = fgets(pass, passSize, stdin);

    FAIL(ret == NULL);

    //
    // Enable back echo.
    //

    printf("\n");

    fflush(stdout);

    //
    // Remove end of line chars from readed pass.
    //

    for (int i = 0; pass[i]; i++)
    {
      if (pass[i] == 10 || pass[i] == 13)
      {
        pass[i] = 0;
      }
    }

    //
    // Error handler.
    //

    exitCode = 0;

    fail:

    if (exitCode)
    {
      Error("ERROR: Cannot read password from stdin.\n");
    }

    SecureEnableEcho();

    DBG_LEAVE("SecureReadPassword");

    return exitCode;
  }

  //
  // Verify password stored as SHA256(pass + salt).
  // Function compute SHA256(pass + salt) by own and compare result with
  // expectedHash parameter.
  //
  // expectedHash - expected SHA256(pass + salt) hash computed in register time (IN).
  // password     - plain text password (IN).
  // salt         - salt generated in register time (IN).
  //
  // RETURNS: 0 if SHA256(pass + salt) matches expectedHash (authorization ok),
  //          1 if hashes does not matches (authorization failed)
  //         -1 if unexpected error occures.
  //

  int SecurePassAuthorize(const char *expectedHash,
                              const char *password, const char *salt)
  {
    DBG_ENTER3("SecurePassAuthorize");

    int exitCode = -1;

    char hash[64 + 1];

    //
    // Check args.
    //

    FAILEX(expectedHash == NULL, "ERROR: 'expectedHash' cannot be NULL in SecurePassAuthorize().\n");
    FAILEX(password == NULL, "ERROR: 'password' cannot be NULL in SecurePassAuthorize().\n");
    FAILEX(salt == NULL, "ERROR: 'salt' cannot be NULL in SecurePassAuthorize().\n");

    //
    // Compute SHA256(password + salt).
    //

    FAIL(SecureHashSha256(hash, sizeof(hash), password,
                              strlen(password), salt, strlen(salt)));

    DEBUG3("SecurePassAuthorize: Computed hash is [%s].\n", hash);

    //
    // Compare computed hash with expected one.
    //

    if (strcmp(hash, expectedHash) == 0)
    {
      exitCode = 0;

      DEBUG2("Authorization OK.\n");
    }
    else
    {
      exitCode = 1;

      DEBUG2("Authorization failed.\n");
    }

    //
    // Error handler.
    //

    fail:

    if (exitCode == -1)
    {
      Error("ERROR: Cannot perform password authorization.\n");
    }

    DBG_LEAVE3("SecurePassAuthorize");

    return exitCode;
  }
} /* namespace Tegenaria */
