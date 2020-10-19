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
  using namespace std;

  void StringSet(char *&destination, const char *source)
  {
    if (destination != source)
    {
      if (destination != NULL)
      {
        delete [] destination;
      }

      if (source == NULL)
      {
        destination = NULL;
      }
      else
      {
        int sourcelen = strlen(source) + 1;

        destination = new char[sourcelen];

        memcpy(destination, source, sourcelen);
      }
    }
  }

  void StringAdd(char *&destination, const char *source1, const char *source2,
                     const char *source3, const char *source4)
  {
    int length0 = (destination == NULL ? 0 : strlen(destination));

    int length1 = (source1 == NULL ? 0 : strlen(source1));
    int length2 = (source2 == NULL ? 0 : strlen(source2));
    int length3 = (source3 == NULL ? 0 : strlen(source3));
    int length4 = (source4 == NULL ? 0 : strlen(source4));

    int length = length0 + length1 + length2 + length3 + length4;

    if (length == 0)
    {
      return;
    }

    char *data = new char[length + 1];

    if (destination != NULL)
    {
      memcpy(data, destination, length0 + 1);
    }
    else
    {
      *data = '\0';
    }

    if (source1 != NULL)
    {
      memcpy(data + length0, source1, length1 + 1);
    }

    if (source2 != NULL)
    {
      memcpy(data + length0 + length1, source2, length2 + 1);
    }

    if (source3 != NULL)
    {
      memcpy(data + length0 + length1 + length2, source3, length3 + 1);
    }

    if (source4 != NULL)
    {
      memcpy(data + length0 + length1 + length2 + length3, source4, length4 + 1);
    }

    delete [] destination;

    destination = data;
  }

  void StringReset(char *&destination)
  {
    if (destination != NULL)
    {
      delete [] destination;

      destination = NULL;
    }
  }

  //
  // Replace all occurences of character <oldChar> by <newChar>.
  //
  // str     - string to modify (IN/OUT).
  // oldChar - character to be replaced (IN).
  // newChar - character to use for replacement (IN).
  //
  // RETURNS: Pointer to str[] parameter,
  //          or NULL if error.
  //

  void StrReplace(char *str, char oldChar, char newChar)
  {
    if (str == NULL)
    {
      return;
    }

    while (*str)
    {
      if ((*str) == oldChar)
      {
        *str = newChar;
      }

      str ++;
    }
  }

  //
  // Replace all occurences of character <oldChar> by <newChar>.
  //
  // str     - string to modify (IN/OUT).
  // oldChar - character to be replaced (IN).
  // newChar - character to use for replacement (IN).
  //
  // RETURNS: Pointer to str[] parameter,
  //          or NULL if error.
  //

  void StrReplace(string &str, char oldChar, char newChar)
  {
    for (int i = 0; i < str.size(); i++)
    {
      if (str[i] == oldChar)
      {
        str[i] = newChar;
      }
    }
  }

  //
  // Remove all occurences of character <c> from string <str>.
  //
  // str - string to modify (IN/OUT).
  // c   - character to be removed (IN).
  //
  // RETURNS: Pointer to str[] parameter,
  //          or NULL if error.
  //

  char *StrRemoveChar(char *str, char c)
  {
    if (str == NULL)
    {
      return NULL;
    }

    char *src = str;
    char *dst = str;

    while (*src)
    {
      if ((*src) != c)
      {
        *dst = *src;

        dst ++;
      }

      src ++;
    }

    *dst = 0;

    return str;
  }

  //
  // Remove all occurences of character <c> from string <str>.
  //
  // str - string to modify (IN).
  // c   - character to be removed (IN).
  //
  // RETURNS: new string with removed characters.
  //

  string StrRemoveChar(const string &str, char c)
  {
    string rv;

    for (int i = 0; i < str.size(); i++)
    {
      if (str[i] != c)
      {
        rv += str[i];
      }
    }

    return rv;
  }

  //
  // Remove all occurences of given <pattern> from string e.g.
  //
  //   Before call:
  //     buf[]     = "hello world hello!"
  //     pattern[] = "hello"
  //
  //   After call:
  //     buf[]     = "world!"
  //     pattern[] = "hello"
  //
  // buf     - buffer to change (IN/OUT).
  // pattern - pattern to remove (IN).
  //
  // RETURNS: Pointer to buf[] parameter,
  //          or NULL if error.
  //

  char *StrRemoveString(char *buf, const char *pattern)
  {
    if (buf == NULL || pattern == NULL)
    {
      return NULL;
    }

    char *p = NULL;

    int patternLen = strlen(pattern);

    while(p = strstr(buf, pattern))
    {
      strcpy(p, p + patternLen);
    }

    return buf;
  }

  //
  // Remove all occurences of given <pattern> without case sensity
  // from string e.g.
  //
  //   Before call:
  //     buf[]     = "HeLLo world hello!"
  //     pattern[] = "hello"
  //
  //   After call:
  //     buf[]     = "world!"
  //     pattern[] = "hello"
  //
  // buf     - buffer to change (IN/OUT).
  // pattern - pattern to remove (IN).
  //
  // RETURNS: Pointer to buf[] parameter,
  //          or NULL if error.
  //

  char *StrRemoveCaseString(char *buf, const char *pattern)
  {
    if (buf == NULL || pattern == NULL)
    {
      return NULL;
    }

    char *p = NULL;

    int patternLen = strlen(pattern);

    while(p = stristr(buf, pattern))
    {
      strcpy(p, p + patternLen);
    }

    return buf;
  }

  //
  // Remove declination postfix if any.
  //
  // WARNING: Function need pure ASCII text on input (i.e. with already
  //          tarnslated polish chars to coresponding ASCII eg. ¥ to a).
  //
  // Example:
  //   Input : domy
  //   Output: dom
  //
  //   Input : samochodow
  //   Output: samochod
  //

  string StrRemoveDeclensionPostfixPL(string word)
  {
    string rv;

    int len = word.size();

    //
    // Word too short. Don't touch.
    //

    if(len < 3)
    {
      rv = word;
    }

    //
    // Word has at least 3 chars. Go on.
    //

    else
    {
      int found = 0;

      const char *postfix1[] = {"y"   , "u"   , "i"  , "a"  , NULL};
      const char *postfix2[] = {"ow"  , "om"  , "em" , "ni" , "ka" , "ie" , NULL};
      const char *postfix3[] = {"ami" , "owi" , "iem", "ach", "owy", "owe", "owo", "owa", "ace", NULL};
      const char *postfix4[] = {"iowe", "czek", NULL};

      const char *p = word.c_str() + len;

      if (len >= 3 + 4)
      {
        for (int i = 0; found == 0 && postfix4[i]; i++)
        {
          if (stricmp(p - 4, postfix3[i]) == 0)
          {
            found = 4;
          }
        }
      }

      if (len >= 3 + 3)
      {
        for (int i = 0; found == 0 && postfix3[i]; i++)
        {
          if (stricmp(p - 3, postfix3[i]) == 0)
          {
            found = 3;
          }
        }
      }

      if (len >= 3 + 2)
      {
        for (int i = 0; found == 0 && postfix2[i]; i++)
        {
          if (stricmp(p - 2, postfix2[i]) == 0)
          {
            found = 2;
          }
        }
      }

      if (len >= 3 + 1)
      {
        for (int i = 0; found == 0 && postfix1[i]; i++)
        {
          if (stricmp(p - 1, postfix1[i]) == 0)
          {
            found = 1;
          }
        }
      }

      if (found != 0)
      {
        rv = word.substr(0, len - found);
      }
      else
      {
        rv = word;
      }
    }

    DEBUG3("StrRemoveDeclesionPostfixPL: Converted [%s] to [%s].\n", word.c_str(), rv.c_str());

    return rv;
  }

  //
  // Replace all <oldStr> occurences by <newStr> one.
  //
  // s      - string to modify (IN/OUT).
  // oldStr - pattern to be replaced (IN).
  // newStr - new pattern for replacement (IN).
  //
  // RETURNS: Refference to input/output <s> parmatere.
  //

  string &StrReplaceString(string &s, const char *oldStr, const char *newStr)
  {
    string ret;

    if (oldStr == NULL || newStr == NULL)
    {
      return s;
    }

    char *next = NULL;
    char *p    = (char *) s.c_str();

    int oldStrLen = strlen(oldStr);

    while(next = strstr(p, oldStr))
    {
      *next = 0;

      ret += p;
      ret += newStr;

      p = next + oldStrLen;
    }

    ret += p;

    s = ret;

    return s;
  }

  //
  // Mask last digits in phone number by 'x' characters e.g.
  //
  // Before call : 123-456-789
  // After call  : 123-456-xxx
  //
  // p - buffer where telefone number to change (IN/OUT).
  //
  // RETURNS: 0 if OK,
  //          -1 otherwise.
  //

  int StrMaskPhone(char *p)
  {
    if (p == NULL)
    {
      return -1;
    }

    int count = 0;

    while(*p)
    {
      switch(*p)
      {
        case '0'...'9':
        {
          count ++;

          if (count > 6)
          {
            *p = 'x';
          }

          break;
        }

        case 'a'...'z':
        case 'A'...'Z':
        case 13:
        case 10:
        case '.':
        case ',':
        {
          count = 0;

          break;
        }

        default:
        {
          //if (count > 0)
          //{
          //  count ++;
          //}
        }
      }

      p ++;
    }

    return 0;
  }

  //
  // Mask email address e.g.
  //
  // Before call : sucker@dirligo.com
  // After call  : xxxxxx@xxxxxxx.xxx
  //
  // p - buffer where mail to mask is stored (IN/OUT).
  //
  // RETURNS: 0 if OK,
  //         -1 otherwise.
  //

  int StrMaskEmail(char *p)
  {
    if (p == NULL)
    {
      return -1;
    }

    char *it   = p;
    char *next = p;

    while(next = strchr(next, '@'))
    {
      for (it = next - 1; it > p && isspace(*it) == 0; it --)
      {
        if (isalnum(*it))
        {
          *it = 'x';
        }
      }

      for (it = next + 1; *it && isspace(*it) == 0; it ++)
      {
        if (isalnum(*it))
        {
          *it = 'x';
        }
      }

      next ++;
    }

    return 0;
  }

  //
  // Encode special chars e.g. '<' into HTML data.
  //
  // str   - text to encode (IN).
  // flags - combination of STR_HTML_XXX flags defines in Str.h (IN/OPT).
  //
  // RETURNS: Text encoded to html.
  //

  string StrEncodeToHtml(const string &str, unsigned int flags)
  {
    DBG_ENTER("StrEncodeToHtml");

    string ret;

    int last = 0;

    for (int i = 0; i < str.size(); i++)
    {
      int c = str[i];

      switch(c)
      {
        //
        // Encode end of line to "<br/">.
        //

        case 13:
        case 10:
        {
          if (last != 13 && last != 10)
          {
            ret += "<br/>";
          }

          break;
        }

        //
        // Encode '<' to '&lt;'
        // Encode '>' to '&gt;'
        // Encode space to "&nbsp;" to force column align.
        //

        case '<': ret += "&lt;";   break;
        case '>': ret += "&gt;";   break;

        case ' ':
        {
          if (flags & STR_HTML_NON_BREAK_SPACES)
          {
            ret += "&nbsp;";
          }
          else
          {
            ret += ' ';
          }

          break;
        }

        //
        // Default. Append to output directly.
        //

        default:
        {
          ret += c;
        }
      }

      last = c;
    }

    DBG_LEAVE("StrEncodeToHtml");

    return ret;
  }

  string StrNormalizeWhiteSpaces(string s)
  {
    DBG_ENTER("StrNormalizeWhiteSpaces");

    int lastWhite = 1;

    string rv;

    for (int i = 0; i < s.size(); i++)
    {
      if (isspace(s[i]))
      {
        if (lastWhite == 0)
        {
          rv += s[i];
        }

        lastWhite = 1;
      }
      else
      {
        rv += s[i];

        lastWhite = 0;
      }
    }

/*
 TODO: Review this code.
    int len = rv.size();

    while(len > 0 && isspace(rv[len - 1]))
    {
      len --;
    }

    if (len > 0)
    {
      rv.resize(len);
    }
    else
    {
      rv.clear();
    }
*/

    DBG_LEAVE("StrNormalizeWhiteSpaces");

    return rv;
  }

  //
  // Remove all whitespaces from string.
  //

  string StrRemoveWhiteSpaces(const string &str)
  {
    string rv;

    for (int i = 0; i < str.size(); i++)
    {
      if (!isspace(str[i]))
      {
        rv += str[i];
      }
    }

    return rv;
  }

  //
  // Format money string by adding extra space per every 3 digits.
  // For example it converts 5000000 into 5 000 000.
  //
  // money     - output buffer, where to put formatted string (OUT).
  // moneySize - size of money[] buffer in bytes (IN).
  // value     - input value string to format (IN).
  //
  // RETURNS: 0 if OK.
  //

  int StrFormatMoney(char *money, int moneySize, const char *value)
  {
    int exitCode = -1;

    if (money && value)
    {
      int valueLen = strlen(value);
      int spaceCnt = (valueLen - 1) / 3;
      int moneyIdx = valueLen + spaceCnt - 1;
      int count    = 0;

      if (moneySize > moneyIdx + 1)
      {
        money[moneyIdx + 1] = 0;

        for (int valueIdx = valueLen - 1; valueIdx >= 0; valueIdx --)
        {
          money[moneyIdx--] = value[valueIdx];

          count ++;

          if (count == 3)
          {
            money[moneyIdx--] = ' ';

            count = 0;
          }
        }
      }

      exitCode = 0;
    }

    return exitCode;
  }
} /* namespace Tegenaria */
