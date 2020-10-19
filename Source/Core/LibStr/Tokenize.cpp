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
// Purpose: Tokenize string into tokens.
//


#include "Str.h"

namespace Tegenaria
{
  //
  // Peek first character from input <p> string and check is it one of
  // allowed delimer character from <delims> set.
  //
  // p      - input string (IN).
  // delims - list of allowed delims e.g. "\t\n " (IN)
  //
  // RETURNS: Delim character if detected or
  //          0 if delim character NOT found.
  //

  inline char StrIsDelim(char *p, const char *delims)
  {
    for (int i = 0; delims[i]; i++)
    {
      if (*p == delims[i]) return delims[i];
    }

    return 0;
  }

  //
  // Tokenize string.
  //
  // tokens   - std::vector containing generated tokens (OUT).
  // inputStr - string to tokenize (IN).
  // delims   - list of delim characters e.g. "\t " (IN).
  //

  void StrTokenize(vector<string> &tokens, const string &inputStr, const char *delims)
  {
    string str = string(inputStr.c_str());

    char *pch = strtok((char *) str.c_str(), delims);

    tokens.clear();

    while (pch)
    {
      tokens.push_back(pch);

      pch = strtok (NULL, delims);
    }
  }

  //
  // Tokenize string.
  //
  // tokens   - std::set containing generated tokens (OUT).
  // inputStr - string to tokenize (IN).
  // delims   - list of delim characters e.g. "\t " (IN).
  //

  void StrTokenize(set<string> &tokens, const string &inputStr, const char *delims)
  {
    string str = string(inputStr.c_str());

    char *pch = strtok((char *) str.c_str(), delims);

    tokens.clear();

    while (pch)
    {
      tokens.insert(pch);

      pch = strtok (NULL, delims);
    }
  }

  //
  // Tokenize string.
  //
  // TIP #1: Set <delimsExtra> to '"' if your string containing quoted "words",
  //         which should NOT be tokenized.
  //
  // tokens      - vector containing generated tokens (OUT).
  // inputStr    - string to tokenize (IN).
  // delims      - list of delim characters e.g. "\t " (IN).
  // delimsExtra - quatation characters to mark words should NOT be processed
  //               by tokenizer (IN).
  //

  void StrTokenize(vector<char *> &tokens, char *inputStr,
                       const char *delims, const char *delimsExtra)
  {
    char *p = inputStr;

    char *begin = p;
    char *end   = p;

    char activeDelim = 1;
    char activeDelimExtra = 0;

    char foundDelim = 0;

    tokens.clear();

    while(end && *end)
    {
      if (foundDelim = StrIsDelim(end, delimsExtra))
      {
        if (activeDelimExtra)
        {
          activeDelimExtra = 0;

          *end = 0;
        }
        else
        {
          while(StrIsDelim(begin, delimsExtra)) begin ++;

          activeDelimExtra = foundDelim;
        }
      }

      if (activeDelimExtra == 0)
      {
        if (end && (foundDelim = StrIsDelim(end, delims)))
        {
          *end = 0;

          tokens.push_back(begin);

          end ++;

          while(StrIsDelim(end, delims)) end ++;

          begin = end;

          continue;
        }
      }

      end ++;
    }

    if (end > begin)
    {
      tokens.push_back(begin);
    }
  }

  //
  // Tokenize string.
  //
  // TIP #1: Set <delimsExtra> to '"' if your string containing quoted "words",
  //         which should NOT be tokenized.
  //
  // tokens      - vector containing generated tokens (OUT).
  // inputStr    - string to tokenize (IN).
  // delims      - list of delim characters e.g. "\t " (IN).
  // delimsExtra - quatation characters to mark words should NOT be processed
  //               by tokenizer (IN).
  //

  void StrTokenize(vector<string> &tokens, const string &inputStr,
                       const char *delims, const char *delimsExtra)
  {
    string str = inputStr;

    char *p = (char *) str.c_str();

    char *begin = p;
    char *end   = p;

    char activeDelim = 1;
    char activeDelimExtra = 0;

    char foundDelim = 0;

    tokens.clear();

    while(*end)
    {
      if (foundDelim = StrIsDelim(end, delimsExtra))
      {
        if (activeDelimExtra)
        {
          activeDelimExtra = 0;

          *end = 0;
        }
        else
        {
          while(StrIsDelim(begin, delimsExtra)) begin ++;

          activeDelimExtra = foundDelim;
        }
      }

      if (activeDelimExtra == 0)
      {
        if (foundDelim = StrIsDelim(end, delims))
        {
          *end = 0;

          tokens.push_back(begin);

          end ++;

          while(StrIsDelim(end, delims)) end ++;

          begin = end;

          continue;
        }
      }

      end ++;
    }

    if (end > begin)
    {
      tokens.push_back(begin);
    }
  }

  //
  // Tokenize string.
  //
  // TIP #1: Set <delimsExtra> to '"' if your string containing quoted "words",
  //         which should NOT be tokenized.
  //
  // tokens      - std:set containing generated tokens (OUT).
  // inputStr    - string to tokenize (IN).
  // delims      - list of delim characters e.g. "\t " (IN).
  // delimsExtra - quatation characters to mark words should NOT be processed
  //               by tokenizer (IN).
  //

  void StrTokenize(set<string> &tokens, const string &inputStr,
                       const char *delims, const char *delimsExtra)
  {
    string str = string(inputStr.c_str());

    char *p = (char *) str.c_str();

    char *begin = p;
    char *end   = p;

    char activeDelim      = 1;
    char activeDelimExtra = 0;
    char foundDelim       = 0;

    tokens.clear();

    while(*end)
    {
      if (foundDelim = StrIsDelim(end, delimsExtra))
      {
        if (activeDelimExtra)
        {
          activeDelimExtra = 0;

          *end = 0;
        }
        else
        {
          while(begin && StrIsDelim(begin, delimsExtra)) begin ++;

          activeDelimExtra = foundDelim;
        }
      }

      if (activeDelimExtra == 0)
      {
        if (foundDelim = StrIsDelim(end, delims))
        {
          *end = 0;

          tokens.insert(string(begin));

          end ++;

          while(end && StrIsDelim(end, delims)) end ++;

          begin = end;

          continue;
        }
      }

      end ++;
    }

    //
    // Last insert.
    //

    if (end > begin)
    {
      tokens.insert(string(begin));
    }
  }

  //
  // Tokenize string with multiple characters delimiter.
  //
  // TIP #1: Set <delimsExtra> to '"' if your string containing quoted "words",
  //         which should NOT be tokenized.
  //
  // tokens      - vector containing generated tokens (OUT).
  // inputStr    - string to tokenize (IN).
  // delims      - multiple character delimiter e.g. "<end>" (IN).
  // delimsExtra - quatation characters to mark words should NOT be processed
  //               by tokenizer (IN).
  //

  void StrStrTokenize(vector<char *> &tokens, char *str, const char *delim)
  {
    char *p = str;

    char *next = NULL;

    int delimLen = strlen(delim);

    tokens.clear();

    while (next = strstr(p, delim))
    {
      *next = 0;

      tokens.push_back(p);

      p = next + delimLen;
    }

    if (p && *p)
    {
      tokens.push_back(p);
    }
  }

  //
  // Tokenize string with multiple characters delimiter.
  //
  // TIP #1: Set <delimsExtra> to '"' if your string containing quoted "words",
  //         which should NOT be tokenized.
  //
  // tokens      - vector containing generated tokens (OUT).
  // inputStr    - string to tokenize (IN).
  // delims      - multiple character delimiter e.g. "<end>" (IN).
  // delimsExtra - quatation characters to mark words should NOT be processed
  //               by tokenizer (IN).
  //

  void StrStrTokenize(vector<string> &tokens, const string &inputStr, const char *delim)
  {
    string str = string(inputStr.c_str());

    char *p = (char *) str.c_str();
    char *next = NULL;

    int delimLen = strlen(delim);

    tokens.clear();

    while (next = strstr(p, delim))
    {
      *next = 0;

      tokens.push_back(p);

      p = next + delimLen;
    }

    if (p && *p)
    {
      tokens.push_back(p);
    }
  }

  //
  // Split input string into <left> and <right> parts separated by <spliChar>.
  // Eg. it can splits "variable=value" string into "variable" and "value" tokens.
  //
  // token - input string to split. Warning input token will be destroyed (IN/OUT).
  // left  - pointer to left token (OUT).
  // right - pointer to right token (first character after splitChar) (OUT).
  //
  // RETURNS: 0 if <splitChar> found and string are splitteng sucessfuly,
  //          -1 otherwise.
  //

  int StrSplit(char *token, char **left, char **right, char splitChar)
  {
    int exitCode = -1;

    char *split = NULL;

    //
    // Check args.
    //

    FAILEX(token == NULL, "ERROR: 'token' cannot be NULL in StrSplit().\n");
    FAILEX(left == NULL, "ERROR: 'left' cannot be NULL in StrSplit().\n");
    FAILEX(right == NULL, "ERROR: 'right' cannot be NULL in StrSplit().\n");

    //
    // Search for splitChar.
    //

    split = strchr(token, splitChar);

    FAILEX(split == NULL,
               "ERROR: Split char '%c' not found in token '%s'.\n",
                   splitChar, token);

    //
    // Split token into lvalue and rvalue.
    //

    *split = 0;

    *left  = token;
    *right = split + 1;

    //
    // Error handler.
    //

    exitCode = 0;

    fail:

    return exitCode;
  }
} /* namespace Tegenaria */
