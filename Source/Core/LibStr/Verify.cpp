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
// Functions to verify strings (e.g. email, logins).
//

#include <Tegenaria/Error.h>
#include "Str.h"

namespace Tegenaria
{
  //
  // Check is given email is in correct X@Y format.
  //
  // email - email address to verify (IN).
  //
  // RETURNS: 0 if email seems correct,
  //          one of ERR_VERIFY_XXX code otherwise.
  //

  int StrEmailVerify(const char *email)
  {
    DBG_ENTER3("StrEmailVerify");

    int exitCode = ERR_VERIFY_WRONG_FORMAT;

    const char *atPtr  = NULL;
    const char *dotPtr = NULL;

    FAIL(email == NULL);

    //
    // Check for X@Y format.
    //

    exitCode = ERR_VERIFY_WRONG_FORMAT;

    atPtr = strchr(email, '@');

    FAIL(atPtr == NULL);
    FAIL(atPtr == email);

    FAIL(*(atPtr + 1) == 0);

    //
    // Check for SQL injection.
    //

    exitCode = ERR_VERIFY_WRONG_CHAR;

    FAIL(strchr(email, '\'') != NULL);

    //
    // () comments are forbidden eg. john(comment)@gmail.com.
    //

    if (strchr(email, '(') || strchr(email, ')'))
    {
      exitCode = ERR_VERIFY_COMMENT_FORBIDDEN;

      goto fail;
    }

    //
    // + aliases are forbidden e.g. john+alias@gmail.com
    //

    if (strchr(email, '+'))
    {
      exitCode = ERR_VERIFY_ALIAS_FORBIDDEN;

      goto fail;
    }

  /*
    dotPtr = strchr(atPtr + 1, '.');

    FAIL(dotPtr == NULL);
    FAIL(dotPtr == atPtr + 1);
    FAIL(*(dotPtr + 1) == 0);
  */

    //
    // Error handler.
    //

    exitCode = 0;

    fail:

    DBG_LEAVE3("StrEmailVerify");

    return exitCode;
  }

  //
  // Check is given login correct.
  //
  // Allowed chars are: A-Z a-z 0-9 -_@.
  // (No space allowed, dot allowed).
  //
  // Minimum login length is STR_MIN_LOGIN_LEN.
  // Maximum login length is STR_MAX_LOGIN_LEN.
  //
  // login - login to verify (IN).
  //
  // RETURNS: 0 if login seems correct,
  //          one of ERR_VERIFY_XXX code otherwise.
  //

  int StrLoginVerify(const char *login)
  {
    DBG_ENTER3("StrLoginVerify");

    int exitCode = ERR_VERIFY_WRONG_FORMAT;

    //
    // Check args.
    //

    FAIL(login == NULL);

    //
    // Check length.
    //

    if (strlen(login) < STR_MIN_LOGIN_LEN)
    {
      exitCode = ERR_VERIFY_TOO_SHORT;

      goto fail;
    }

    if (strlen(login) > STR_MAX_LOGIN_LEN)
    {
      exitCode = ERR_VERIFY_TOO_LONG;

      goto fail;
    }

    //
    // Check allowed alphabet.
    //

    for (int i = 0; login[i]; i++)
    {
      switch(login[i])
      {
        //
        // Allowed chars.
        //

        case 'A'...'Z':
        case 'a'...'z':
        case '0'...'9':
        case '-':
        case '_':
        case '@':
        case '.':
        {
          break;
        }

        //
        // Not allowed chars. Fail with ERR_VERIFY_WRONG_CHAR code.
        //

        default:
        {
          exitCode = ERR_VERIFY_WRONG_CHAR;

          goto fail;
        }
      }
    }

    //
    // Error handler.
    //

    exitCode = 0;

    fail:

    DBG_LEAVE3("StrLoginVerify");

    return exitCode;
  }

  //
  // Check is given password correct.
  //
  // Allowed chars are <32;125> ascii.

  // Minimum login length is STR_MIN_PASS_LEN.
  // Maximum login length is STR_MAX_PASS_LEN.
  //
  // pass - password to verify (IN).
  //
  // RETURNS: 0 if password seems correct,
  //          one of ERR_VERIFY_XXX code otherwise.
  //

  int StrPassVerify(const char *pass)
  {
    DBG_ENTER3("StrPassVerify");

    int exitCode = ERR_VERIFY_WRONG_FORMAT;

    //
    // Check args.
    //

    FAIL(pass == NULL);

    //
    // Check length.
    //

    if (strlen(pass) < STR_MIN_PASS_LEN)
    {
      exitCode = ERR_VERIFY_TOO_SHORT;

      goto fail;
    }

    if (strlen(pass) > STR_MAX_PASS_LEN)
    {
      exitCode = ERR_VERIFY_TOO_LONG;

      goto fail;
    }

    //
    // Check allowed alphabet.
    //

    for (int i = 0; pass[i]; i++)
    {
      if (pass[i] < 32 || pass[i] > 125)
      {
        exitCode = ERR_VERIFY_WRONG_CHAR;

        goto fail;
      }
    }

    //
    // Error handler.
    //

    exitCode = 0;

    fail:

    DBG_LEAVE3("StrPassVerify");

    return exitCode;
  }

  //
  // Compute password strength in <0;6> scale.
  //
  // pass - password to check (IN).
  //
  // RETURNS: Password strength in <0;6> scale.
  //

  int StrPassStrength(const char *pass)
  {
    DBG_ENTER3("StrPassStrength");

    int hasUpper   = 0;
    int hasLower   = 0;
    int hasSymbol  = 0;
    int hasSpecial = 0;
    int hasNumber  = 0;
    int strength   = 0;

    //
    // Check args.
    //

    FAIL(pass == NULL);

    //
    // Hardcode strength = 0 if password too short.
    //

    FAIL(strlen(pass) < 6);

    //
    // Scan chars in password.
    //

    for (int i = 0; pass[i]; i++)
    {
      switch(pass[i])
      {
        case 'A'...'Z':
        {
          hasUpper = 1;

          break;
        }

        case 'a'...'z':
        {
          hasLower = 1;

          break;
        }

        case 33 ... 47:
        case 58 ... 64:
        case 91 ... 96:
        {
          hasSymbol = 1;

          break;
        }

        case 123 ... 126:
        {
          hasSpecial = 1;

          break;
        }

        case '0'...'9':
        {
          hasNumber = 1;

          break;
        }
      }
    }

    //
    // Compute strenghth.
    //

    if (hasUpper) strength ++;
    if (hasLower) strength ++;
    if (hasNumber) strength ++;
    if (hasSymbol) strength ++;
    if (hasSpecial) strength ++;

    //
    // Extra point for pass longer than 12 characters.
    //

    if (strlen(pass) > 12)
    {
      strength ++;
    }

    //
    // Error handler.
    //

    fail:

    DBG_LEAVE3("StrPassStrength");

    return strength;
  }
} /* namespace Tegenaria */
