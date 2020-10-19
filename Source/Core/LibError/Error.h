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

#ifndef Tegenaria_Core_Error_H
#define Tegenaria_Core_Error_H

#include <string>

namespace Tegenaria
{
  using std::string;

  //
  // 0-99
  // Generic errors.
  //

  #define ERR_OK                   0
  #define ERR_GENERIC              999

  #define ERR_NOT_IMPLEMENTED       1
  #define ERR_NOT_FOUND             2
  #define ERR_ALREADY_EXISTS        3
  #define ERR_WRONG_PARAMETER       4
  #define ERR_ACCESS_DENIED         5
  #define ERR_TIMEOUT               6
  #define ERR_SYNTAX_ERROR          7
  #define ERR_WRONG_PATH            8
  #define ERR_WRONG_ALIAS           9
  #define ERR_WRONG_USER            10
  #define ERR_WRONG_MODE            11
  #define ERR_WRONG_PIPENAME        12
  #define ERR_WRONG_TEXT            13
  #define ERR_TEXT_TOO_LONG         14
  #define ERR_WRONG_TYPE            15
  #define ERR_WRONG_ACL             16
  #define ERR_WRONG_RESULT          17
  #define ERR_ACL_TOO_LONG          18
  #define ERR_COULD_NOT_CONNECT     19
  #define ERR_PATH_NOT_FOUND        20
  #define ERR_INVALID_SESSION_ID    21
  #define ERR_AUTH_ERROR            22
  #define ERR_NOT_CONNECTED         23
  #define ERR_FILE_TOO_BIG          24

  //
  // 250-299
  // Data validation.
  //

  #define ERR_VERIFY_TOO_LONG          250
  #define ERR_VERIFY_TOO_SHORT         251
  #define ERR_VERIFY_WRONG_CHAR        252
  #define ERR_VERIFY_WRONG_FORMAT      253
  #define ERR_VERIFY_ALIAS_FORBIDDEN   254
  #define ERR_VERIFY_COMMENT_FORBIDDEN 255

  //
  // Exported functions.
  //

  const char *ErrGetString(int code);

  int ErrGetSystemError();

  const string ErrGetSystemErrorString();

} /* namespace Tegenaria */

#endif /* Tegenaria_Core_Error_H */
