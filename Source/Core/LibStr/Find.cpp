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
// Purpose: Find substrings in string.
//

#include "Str.h"

namespace Tegenaria
{
  char *stristr(const char *arg1, const char *arg2)
  {
    const char *a, *b;

    for(; *arg1; *arg1++)
    {
      a = arg1;
      b = arg2;

      while((*a++ | 32) == (*b++ | 32))
      {
        if(!*b)
        {
          return (char *) arg1;
        }

      }
    }

    return NULL;
  }

  char *StrFindIWord(const char *arg1, const char *arg2)
  {
    const char *a, *b;

    const char *arg1Oryg = arg1;

    for(; *arg1; *arg1++)
    {
      a = arg1;
      b = arg2;

      while((*a++ | 32) == (*b++ | 32))
      {
        if(!*b
           && (arg1 == arg1Oryg || !isalpha(arg1[-1])
           && (*a == 0 || !isalpha(*a))))

        {
            return (char *) arg1;
        }

      }
    }

    return NULL;
  }

  char *StrFindWord(const char *arg1, const char *arg2)
  {
    const char *a, *b;

    const char *arg1Oryg = arg1;

    for(; *arg1; *arg1++)
    {
      a = arg1;
      b = arg2;

      while((*a++) == (*b++))
      {
        if(!*b
           && (arg1 == arg1Oryg || !isalpha(arg1[-1])
           && (*a == 0 || !isalpha(*a))))

        {
            return (char *) arg1;
        }

      }
    }

    return NULL;
  }

  string StrGetTextBeetwen(const char *buf, const char *startStr, const char *endStr)
  {
    string ret;

    char *start = NULL;
    char *end = NULL;

    char zero = 0;

    FAIL(buf == NULL);
    FAIL(startStr == NULL);
    FAIL(endStr == NULL);
    FAIL(startStr[0] == 0);
    FAIL(endStr[0] == 0);

    start = (char *) strstr(buf, startStr);

    FAIL(start == NULL);

    start += strlen(startStr);

    end = strstr(start, endStr);

    FAIL(end == NULL);

    swap(*end, zero);
    ret = start;
    swap(*end, zero);

    fail:

    return ret;
  }

  char *strstrex(const char *s, const char *pattern)
  {
    if (s == NULL)
    {
      return NULL;
    }

    char *ret = (char *) strstr(s, pattern);

    if (ret)
    {
      ret += strlen(pattern);
    }

    return ret;
  }

  char *strchrex(const char *s, char pattern)
  {
    if (s == NULL)
    {
      return NULL;
    }

    char *ret = (char *) strchr(s, pattern);

    if (ret)
    {
      ret ++;
    }

    return ret;
  }

  //
  // Multi character version of strchr.
  //
  // p             - pointer to begin of buffer string (IN).
  // chars         - string containing list of characters to find (IN).
  // zeroByteMatch - treat zero terminator as matching character if set to 1 (IN).
  //
  // RETURNS: Pointer to first character matching any of chars[] character
  //          or NULL if character not found.
  //

  const char *StrFindCharMulti(const char *p, const char *chars, int zeroByteMatch)
  {
    const char *ret = NULL;

    //
    // Check args.
    //

    FAIL(p == NULL);
    FAIL(chars == NULL);

    //
    // Find first occurence of any from chars[].
    //

    while(p[0] && ret == NULL)
    {
      for (int i = 0; chars[i]; i++)
      {
        if (*p == chars[i])
        {
          ret = p;
        }
      }
    }

    if (p[0] == 0 && zeroByteMatch)
    {
      ret = p;
    }

    fail:

    return ret;
  }
} /* namespace Tegenaria */
