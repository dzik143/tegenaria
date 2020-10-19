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
// Purpose: Parse string part by part.
//

#include "Str.h"

namespace Tegenaria
{
  //
  // Count how many time given char <c> exist in string.
  //
  // s - input string to scan (IN).
  // c - character to be counted (IN).
  //
  // RETURNS: How many times char <c> exist in string <s>
  //

  int StrCountChar(const char *s, char c)
  {
    int count = 0;

    for (int i = 0; s[i]; i++)
    {
      if (s[i] == c)
      {
        count ++;
      }
    }

    return count;
  }

  //
  // Count how many digit character (0-9) exists in string.
  //
  // s - input string to scan (IN).
  //
  // RETURNS: How many digits exist in string <s>
  //

  int StrCountDigits(const char *s)
  {
    int count = 0;

    for (int i = 0; s[i]; i++)
    {
      if (isdigit(s[i]))
      {
        count ++;
      }
    }

    return count;
  }

  //
  // Count how many alpha (a-z and A-Z) characters exist in string.
  //
  // s - input string to scan (IN).
  //
  // RETURNS: How many alpha characters exist in string <s>
  //

  int StrCountAlpha(const char *s)
  {
    int count = 0;

    for (int i = 0; s[i]; i++)
    {
      if (isalpha(s[i]))
      {
        count ++;
      }
    }

    return count;
  }

  //
  // Pop integer (%d) value from string.
  //
  // it - pointer source string (IN/OUT).
  //
  // RETURNS: Popped integer.
  //

  int StrPopInt(const char **it)
  {
    int ret = 0;

    if (it && *it)
    {
      //
      // Skip whites first.
      //

      while(isspace(**it))
      {
        (*it) ++;
      }

      //
      // Get number.
      //

      ret = atoi(*it);

      while(isdigit(**it))
      {
        (*it) ++;
      }
    }

    return ret;
  }

  //
  // Pop double (%lf) value from string.
  //
  // it - pointer source string (IN/OUT).
  //
  // RETURNS: Popped integer.
  //

  double StrPopDouble(const char **it)
  {
    double ret = 0;

    if (it && *it)
    {
      //
      // Skip whites first.
      //

      while(isspace(**it))
      {
        (*it) ++;
      }

      //
      // Get number.
      //

      ret = atof(*it);

      while(isdigit(**it) || (**it == '.'))
      {
        (*it) ++;
      }
    }

    return ret;
  }

  //
  // Peek first character in string and pop it if it's match with
  // expected character.
  //
  // NOTE #1: Character is popped only if matches with <expected> parameter.
  //
  // NOTE #2: Input string <*it> is NOT changed if first character NOT matches
  //          to <expected> parameter.
  //
  // EXAMPLE 1:
  // ----------
  //
  //   Before call:
  //     *it      = "jozek"
  //     expected = 'j'
  //
  //   After call:
  //     *it      = "ozek"
  //     expected = 'j'
  //
  //   RETURNS: 0
  //
  // EXAMPLE2:
  // ---------
  //
  //   Before call:
  //     *it      = "jozek"
  //     expected = 'x'
  //
  //   After call:
  //     *it      = "jozek"
  //     expected = 'x'
  //
  //   RETURNS: -1
  //
  // it       - pointer source string (IN/OUT).
  // expected - character to match (IN).
  //
  // RETURNS: 0 if character matched and popped
  //         -1 otherwise.
  //

  int StrMatchChar(const char **it, char expected)
  {
    if (it && *it && **it == expected)
    {
      (*it) ++;

      return 0;
    }

    return -1;
  }

  //
  // Pop first character from string.
  //
  // it - pointer input string (IN/OUT).
  //
  // RETURNS: Popped character.
  //

  char StrPopChar(const char **it)
  {
    char ret = 0;

    if (it && *it && **it)
    {
      ret = (**it);

      (*it) ++;
    }

    return ret;
  }

  //
  // Pop alpha (a-z and A-Z) only word from string.
  // Functin stops reading on first non alpha character.
  //
  // it       - pointer to input string (IN/OUT).
  // word     - buffer where to store readed word (OUT).
  // wordSize - size of word[] buffer in bytes (IN).
  //

  void StrPopAlphaWord(const char **it, char *word, int wordSize)
  {
    if (it && *it && word)
    {
      //
      // Skip whites first.
      //

      while(**it && isspace(**it))
      {
        (*it) ++;
      }

      //
      // Get alpha.
      //

      while(isalpha(**it) && wordSize > 0)
      {
        *word = **it;

        (*it) ++;

        word ++;

        wordSize --;
      }

      if (wordSize > 0)
      {
        word[0] = 0;
      }
      else
      {
        word[-1] = 0;
      }
    }
  }

  //
  // Pop bound/alpha (a-z and A-Z and [] and $) only word from string.
  // Functin stops reading on first characer other than alpha, [] or $.
  //
  // it       - pointer to input string (IN/OUT).
  // word     - buffer where to store readed word (OUT).
  // wordSize - size of word[] buffer in bytes (IN).
  //

  void StrPopAlphaWordBound(const char **it, char *word, int wordSize)
  {
    if (it && *it && word)
    {
      //
      // Skip whites first.
      //

      while(**it && isspace(**it))
      {
        (*it) ++;
      }

      //
      // Get alpha.
      //

      while((isalpha(**it) || (**it) == ']' || (**it) == '[' ||
                        (**it) == '$') && wordSize > 0)
      {
        *word = **it;

        (*it) ++;

        word ++;

        wordSize --;
      }

      if (wordSize > 0)
      {
        word[0] = 0;
      }
      else
      {
        word[-1] = 0;
      }
    }
  }

  //
  // Pop integer (%d) value from string.
  //
  // it - pointer source string (IN/OUT).
  //
  // RETURNS: Popped integer.
  //

  int StrPopInt(char **it)
  {
    return StrPopInt((const char **) it);
  }

  //
  // Pop first character from string.
  //
  // it - pointer input string (IN/OUT).
  //
  // RETURNS: Popped character.
  //

  char StrPopChar(char **it)
  {
    return StrPopChar((const char **) it);
  }

  //
  // Pop double (%lf) value from string.
  //
  // it - pointer source string (IN/OUT).
  //
  // RETURNS: Popped integer.
  //

  double StrPopDouble(char **it)
  {
    return StrPopDouble((const char **) it);
  }

  //
  // Peek first character in string and pop it if it's match with
  // expected character.
  //
  // NOTE #1: Character is popped only if matches with <expected> parameter.
  //
  // NOTE #2: Input string <*it> is NOT changed if first character NOT matches
  //          to <expected> parameter.
  //
  // EXAMPLE 1:
  // ----------
  //
  //   Before call:            After call:
  //     *it      = "jozek"      *it      = "ozek"
  //     expected = 'j'          expected = 'j'
  //
  //   Return value : 0
  //
  // EXAMPLE2:
  // ---------
  //
  //   Before call:            After call:
  //     *it      = "jozek"      *it      = "jozek"
  //     expected = 'x'          expected = 'x'
  //
  //   Return value: -1
  //
  // it       - pointer source string (IN/OUT).
  // expected - character to match (IN).
  //
  // RETURNS: 0 if character matched and popped
  //         -1 otherwise.
  //

  int StrMatchChar(char **it, char expected)
  {
    return StrMatchChar((const char **) it, expected);
  }

  //
  // Pop alpha (a-z and A-Z) only word from string.
  // Functin stops reading on first non alpha character.
  //
  // it       - pointer to input string (IN/OUT).
  // word     - buffer where to store readed word (OUT).
  // wordSize - size of word[] buffer in bytes (IN).
  //

  void StrPopAlphaWord(char **it, char *word, int wordSize)
  {
    StrPopAlphaWord((const char **) it, word, wordSize);
  }

  //
  // Pop bound/alpha (a-z and A-Z and [] and $) only word from string.
  // Functin stops reading on first characer other than alpha, [] or $.
  //
  // it       - pointer to input string (IN/OUT).
  // word     - buffer where to store readed word (OUT).
  // wordSize - size of word[] buffer in bytes (IN).
  //

  void StrPopAlphaWordBound(char **it, char *word, int wordSize)
  {
    StrPopAlphaWordBound((const char **) it, word, wordSize);
  }

  //
  // Skip whites characters.
  //
  // s - input string (IN).
  //
  // RETURNS: Pointer to first non-white character in string.
  //

  const char *StrSkipWhites(const char *s)
  {
    while(isspace(s[0])) s++;

    return s;
  }

  //
  // Peek begin of string and pop it if matches with input <pattern>.
  //
  // Example 1:
  // ----------
  //
  //  Before call:             After call:
  //    *it     = "dirligo"      *it     = "ligo"
  //    pattern = "dir"          pattern = "dir"
  //
  //
  //  Return value: "ligo"
  //
  // Example 2:
  // ----------
  //
  //  Before call:             After call:
  //    *it     = "dirligo"      *it     = "dirligo"
  //    pattern = "xxx"          pattern = "xxx"
  //
  //  Return value: NULL
  //
  // it      - pointer to input string (IN/OUT).
  // pattern - pattern word to be matched (IN).
  //
  // RETURNS: Pointer to begin of patter found in string
  //          or NULL if pattern not found.
  //

  const char *StrMatch(const char **it, const char *pattern)
  {
    if (it == NULL || *it == NULL)
    {
      return NULL;
    }

    const char *next = strstr(*it, pattern);

    if (next)
    {
      *it = next + strlen(pattern);
    }

    return *it;
  }
} /* namespace Tegenaria */
