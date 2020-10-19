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

#include "Str.h"

namespace Tegenaria
{
  //
  // Capitalize string.
  //
  // str - string to captalize (IN/OUT).
  //

  void StrCapitalize(string &str)
  {
    for (int i = 0; i < str.size(); i++)
    {
      if (str[i] >= 'a' && str[i] <= 'z')
      {
        str[i] -= 'a' - 'A';
      }
    }
  }

  //
  // Capitalize string.
  //
  // str - string to captalize (IN/OUT).
  //

  void StrCapitalize(char *str)
  {
    for (int i = 0; str[i]; i++)
    {
      if (str[i] >= 'a' && str[i] <= 'z')
      {
        str[i] -= 'a' - 'A';
      }
    }
  }

  //
  // Lowerize string.
  //
  // s - string to captalize (IN/OUT).
  //

  string &StrLowerize(string &s)
  {
    for (int i = 0; i < s.size(); i++)
    {
      if (s[i] >= 'A' && s[i] <= 'Z')
      {
        s[i] += 'a' - 'A';
      }
    }
  }

  //
  // Lowerize string.
  //
  // s - string to captalize (IN/OUT).
  //

  void StrLowerize(char *s)
  {
    for (int i = 0; s[i]; i++)
    {
      if (s[i] >= 'A' && s[i] <= 'Z')
      {
        s[i] += 'a' - 'A';
      }
    }
  }
} /* namespace Tegenaria */
