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
// Purpose: Generate pseudo-random strings.
//
// WARNING! These functions are NOT criptografically strong.
//

#include "Str.h"

namespace Tegenaria
{
  //
  // Generate random 64-byte decimal only string.
  //
  // RETURNS: Random decimal-only string.
  //

  string RandomIntString()
  {
    char tmp[64];

    snprintf(tmp, 64, "%d%d", rand(), int(time(0)));

    return string(tmp);
  }

  //
  // Generate general random string.
  //
  // len     - length of string to generate (IN).
  // minChar - minimum character to use e.g. 'a' (IN).
  // maxChar - maximum character to use e.g. 'z' (IN).
  //
  // RETURNS: Random string containing characters from <minChar, maxChar>
  //          range.
  //

  string RandomString(int len, int minChar, int maxChar)
  {
    string ret;

    ret.resize(len);

    for (int i = 0; i < len; i++)
    {
      ret[i] = minChar + rand() % (maxChar - minChar);
    }

    return ret;
  }
} /* namespace Tegenaria */
