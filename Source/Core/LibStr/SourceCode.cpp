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
  // Minimalize source code (e.g. JS/CSS) by removing whitespaces and comments.
  //
  // code              - source code to minify (IN).
  // singleComment     - string used as comment line e.g. '//' in C/C++ (IN/OPT).
  // multiCommentBegin - string used to begin comment e.g. '/*' in C/C++ (IN/OPT).
  // multiCommentEnd   - string used to close comment e.g. '*/' in C/C++ (IN/OPT).
  // removeWhites      - remove redundant whites if set to 1 (IN/OPT).
  //
  // RETURNS: New string containing minimalized code.
  //

  string StrMinifyCode(const string &code, const char *singleComment,
                           const char *multiCommentBegin, const char *multiCommentEnd,
                               int removeWhites)
  {
    enum MinifierState
    {
      CODE,
      SINGLE_COMMENT,
      MULTI_COMMENT,
      SINGLE_STRING,
      DOUBLE_STRING
    };

    MinifierState state = CODE;

    string rv;

    int currentChar = -1;
    int lastChar    = -1;
    int nextChar    = -1;

    int singleCommentSize     = singleComment ? strlen(singleComment) : -1;
    int multiCommentBeginSize = multiCommentBegin ? strlen(multiCommentBegin) : -1;
    int multiCommentEndSize   = multiCommentEnd ? strlen(multiCommentEnd) : -1;

    int appendToResult = 1;

    const char *src = code.c_str();

    for (int i = 0; i < code.size(); i++)
    {
      appendToResult = 1;

      currentChar = code[i];
      src         = code.c_str() + i;

      if (i < code.size() - 1)
      {
        nextChar = code[i + 1];
      }
      else
      {
        nextChar = -1;
      }

      switch(state)
      {
        //
        // We're inside code.
        //

        case CODE:
        {
          //
          // Begin of single line comment.
          //

          if (singleComment && (strncmp(src, singleComment, singleCommentSize) == 0))
          {
            state = SINGLE_COMMENT;

            i += singleCommentSize - 1;

            appendToResult = 0;
          }

          //
          // Begin of multi line comment.
          //

          else if (multiCommentBegin && strncmp(src, multiCommentBegin, multiCommentBeginSize) == 0)
          {
            state = MULTI_COMMENT;

            appendToResult = 0;
          }

          //
          // Begin of "" string (double).
          //

          else if (currentChar == '"')
          {
            state = DOUBLE_STRING;
          }

          //
          // Begin of '' string (single).
          //

          else if (currentChar == '\'')
          {
            state = SINGLE_STRING;
          }

          //
          // Ordinal code character.
          //

          else
          {
          }

          break;
        }

        //
        // We're inside single line comment.
        // Wait for end of line.
        //

        case SINGLE_COMMENT:
        {
          if (currentChar == 10 || currentChar == 13)
          {
            state = CODE;

            i --;
          }

          appendToResult = 0;

          break;
        }

        //
        // We're inside multi line comment.
        // Wait for end comment tag.
        //

        case MULTI_COMMENT:
        {
          if (multiCommentEnd && strncmp(src, multiCommentEnd, multiCommentEndSize) == 0)
          {
            i += multiCommentEndSize - 1;

            state = CODE;
          }

          appendToResult = 0;

          break;
        }

        //
        // We're inside "" string (double).
        // Wait for ending " char.
        //

        case DOUBLE_STRING:
        {
          if (currentChar == '"' && lastChar != '\\')
          {
            state = CODE;
          }

          break;
        }

        //
        // We're inside '' string (single).
        // Wait for ending ' char.
        //

        case SINGLE_STRING:
        {
          if (currentChar == '\'' && lastChar != '\\')
          {
            state = CODE;
          }

          break;
        }
      }

      //
      // Don't put redundant whites if needed.
      //

      if (removeWhites && state == CODE)
      {
        if (currentChar == 10 || currentChar == 13)
        {
          currentChar = ' ';
        }

        if (isspace(currentChar))
        {
          switch(lastChar)
          {
            case ' ':
            case '\t':
            case ';':
            case '=':
            case ':':
            case '}':
            case '{':
            case '(':
            case ')':
            case '+':
            case '-':
            case '*':
            case '/':
            case '&':
            case '|':
            case '?':
            case ',':
            case '<':
            case '>':
            {
              appendToResult = 0;

              break;
            }
          }

          switch(nextChar)
          {
            case ' ':
            case '\t':
            case ';':
            case '=':
            case ':':
            case '}':
            case '{':
            case '(':
            case ')':
            case '+':
            case '-':
            case '*':
            case '/':
            case '&':
            case '|':
            case '?':
            case ',':
            case '<':
            case '>':
            {
              appendToResult = 0;

              break;
            }
          }
        }
      }

      //
      // Append char tu result if needed.
      //

      if (appendToResult)
      {
        rv += currentChar;
        lastChar = currentChar;
      }
    }

    return rv;
  };

  //
  // Remove single line comments from string.
  //
  // code          - string containg source code (IN).
  // commentString - string used as comment line e.g. '//' in C/C++ (IN).
  //
  // RETURNS: New string with removed comments.
  //

  string StrRemoveSingleLineComments(string code, const char *commentString)
  {
    return StrMinifyCode(code, commentString, NULL, NULL, 0);
  }

  //
  // Remove multiline comments from string.
  //
  // code         - string containg source code (IN).
  // commentBegin - string used to begin comment e.g. '/*' in C/C++ (IN).
  // commentEnd   - string used to close comment e.g. '*/' in C/C++ (IN).
  //
  // RETURNS: New string with removed comments.
  //

  string StrRemoveMultiLineComments(const string &code,
                                        const char *commentBegin,
                                            const char *commentEnd)
  {
    return StrMinifyCode(code, NULL, commentBegin, commentEnd, 0);
  }
} /* namespace Tegenaria */
