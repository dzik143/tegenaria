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

#include <Tegenaria/Debug.h>
#include <Tegenaria/SSMap.h>
#include "CGI.h"

namespace Tegenaria
{
  //
  // Global variables.
  //

  SSMap _GET;
  SSMap _COOKIE;

  static int InitOK = 0;

  //
  // Converts a hex character to its integer value.
  //

  static char _from_hex(char ch)
  {
    #pragma qcbuild_set_private(1)

    return isdigit(ch) ? ch - '0' : tolower(ch) - 'a' + 10;
  }

  //
  // Converts an integer value to its hex character.
  //

  static char _to_hex(char code)
  {
    #pragma qcbuild_set_private(1)

    static char hex[] = "0123456789abcdef";

    return hex[code & 15];
  }

  //
  // Decode URL string.
  //
  // SRC - URL encoded string (IN).
  //
  // RETURNS: Decoded string.
  //

  string CgiUrlDecode(string SRC)
  {
    DBG_ENTER3("CgiUrlDecode");

    string ret;

    char ch;

    int i, ii;

    for (i = 0; i < SRC.length(); i++)
    {
      if (int(SRC[i]) == 37)
      {
        sscanf(SRC.substr(i + 1, 2).c_str(), "%x", &ii);

        ch = static_cast<char>(ii);

        ret += ch;

        i = i + 2;
      }
      else if (SRC[i] == '+')
      {
        ret += ' ';
      }
      else
      {
        ret += SRC[i];
      }
    }

    DBG_LEAVE3("CgiUrlDecode");

    return (ret);
  }

  //
  // Encode arbitral string to URL.
  //
  // str - string to encode (IN).
  //
  // RETURNS: URL encoded string.
  //

  string CgiUrlEncode(string str)
  {
    DBG_ENTER3("CgiUrlEncode");

    string ret;

    char *pstr = (char *) str.c_str();

    ret.resize(str.size() * 3 + 1);

    char *buf = (char *) ret.c_str();

    char *pbuf = buf;

    while (*pstr)
    {
      if (isalnum(*pstr) || *pstr == '-' || *pstr == '_' || *pstr == '.' || *pstr == '~')
      {
        *pbuf++ = *pstr;
      }
      else if (*pstr == ' ')
      {
        *pbuf++ = '+';
      }
      else
      {
        *pbuf++ = '%', *pbuf++ = _to_hex(*pstr >> 4);

        *pbuf++ = _to_hex(*pstr & 15);
      }

      pstr++;
    }

    *pbuf = '\0';

    ret.resize(strlen(buf));

    DBG_LEAVE3("CgiUrlEncode");

    return ret;
  }

  //
  // Decode QUERY string to {lvalue |-> rvalue} map.
  //
  // ssmap    - {lvalue |-> rvalue} map with decoded variables (OUT)
  // userData - raw url encoded query get string (IN).
  // delim    - what is delim char to use (default '&') (IN/OPT).
  //
  // RETURNS: 0 if OK.
  //

  int CgiDecodeUserVars(SSMap &ssmap, char *userData, const char delim)
  {
    DBG_ENTER3("CgiDecodeUserVars");

    int exitCode = -1;

    char *field    = NULL;
    char *fieldEnd = NULL;
    char *value    = NULL;
    char *valueEnd = NULL;

    char fieldEndZero = 0;
    char valueEndZero = 0;

    string lvalue;
    string rvalue;

    DBG_MSG("[%s]\n", userData);

    FAIL(userData == NULL);

    field = userData;

    while(fieldEnd = strchr(field, '='))
    {
      swap(*fieldEnd, fieldEndZero);

      value = fieldEnd + 1;

      if (valueEnd = strchr(value, delim))
      {
        swap(*valueEnd, valueEndZero);
      }

      if (value && value[0] && strcmp(value, "0") != 0)
      {
        lvalue = CgiUrlDecode(field);
        rvalue = CgiUrlDecode(value);

        ssmap[lvalue] = rvalue;

        DBG_MSG("[%s] : [%s]\n", lvalue.c_str(), rvalue.c_str());
      }

      swap(*fieldEnd, fieldEndZero);

      if (valueEndZero)
      {
        swap(*valueEnd, valueEndZero);

        field = valueEnd + 1;
      }
      else
      {
        break;
      }
    }

    //
    // Error handler.
    //

    exitCode = 0;

    fail:

    if (exitCode)
    {
      Error("ERROR: Cannot parse user data.\n");
    }

    DBG_LEAVE3("CgiDecodeUserVars");

    return exitCode;
  }

  //
  // Create QUERY string from string |-> string map.
  //

  string CgiEncodeUserVars(SSMap &ssmap)
  {
    DBG_ENTER3("CgiEncodeUserVars");

    string ret;

    map<string, string>::iterator it;

    for (it = ssmap.begin(); it != ssmap.end(); it ++)
    {
      if (it -> second != "0")
      {
        if (!ret.empty())
        {
          ret += "&amp;";
        }

        ret += CgiUrlEncode(it -> first);
        ret += "=";
        ret += CgiUrlEncode(it -> second);
      }
    }

    DBG_MSG3("EncodeUserVars : [%s]\n", ret.c_str());

    DBG_LEAVE3("CgiEncodeUserVars");

    return ret;
  }

  //
  // Init CGI library.
  // MUST be called one time before first use.
  //
  // RETURNS: 0 if OK.
  //

  int CgiInit()
  {
    DBG_ENTER("CgiInit");

    int exitCode = -1;

    char *getData    = NULL;
    char *cookieData = NULL;

    if (InitOK == 0)
    {
      //
      // Decode Get variables.
      //

      getData = getenv("QUERY_STRING");

      if (getData && getData[0])
      {
        DBG_MSG("CgiInit : Parsing QUERY_STRING [%s]...\n", getData);

        FAIL(CgiDecodeUserVars(_GET, getData));
      }

      //
      // Decode Cookie variables.
      //

      cookieData = getenv("HTTP_COOKIE");

      if (cookieData && cookieData[0])
      {
        DBG_MSG("CgiInit : Parsing HTTP_COOKIE [%s]...\n", cookieData);

        FAIL(CgiDecodeUserVars(_COOKIE, cookieData, ';'));
      }

      InitOK = 1;
    }
    else
    {
      DBG_MSG("CgiInit : CGI is already initiated.\n");
    }

    //
    // Error handler.
    //

    exitCode = 0;

    fail:

    if (exitCode)
    {
      Error("ERROR: Cannot init CGI interface.\n");
    }

    DBG_LEAVE("CgiInit");

    return exitCode;
  };

  //
  // Retrieve value of given GET variable.
  //
  // lvalue - name of variable to found (IN).
  //
  // RETURNS: Pointer to variable value or NULL if not found.
  //

  const char *CgiGet(const char *lvalue)
  {
    if (lvalue == NULL)
    {
      return NULL;
    }

    SSMap::iterator it = _GET.find(lvalue);

    if (it == _GET.end())
    {
      return NULL;
    }

    return it -> second.c_str();
  }

  //
  //
  //

  void CgiHashAdd(string &hash, const string &str)
  {
    DBG_ENTER3("CgiHashAdd");

    char tmp[16];

    DBG_MSG3("Adding string [%s] to hash...\n", str.c_str());

    //
    // Add string to hash letter by letter, but
    // convert special characters into hex.
    //

    for (int i = 0; i < str.size(); i++)
    {
      unsigned char uc = (unsigned char) str[i];

      //
      // Allowed char. Add to hash directly.
      //

      switch(uc)
      {
        //
        // Concert PL chars into pure ASCII.
        //

        case 0xc4:
        {
          i ++;

          uc = (unsigned char) str[i];

          switch(uc)
          {
            case 0x84:
            case 0x85: hash += 'a'; break;

            case 0x86:
            case 0x87: hash += 'c'; break;

            case 0x98:
            case 0x99: hash += 'e'; break;
          }

          break;
        }

        case 0xc5:
        {
          i ++;

          uc = (unsigned char) str[i];

          switch(uc)
          {
            case 0x81:
            case 0x82: hash += 'l'; break;

            case 0x83:
            case 0x84: hash += 'n'; break;

            case 0x9a:
            case 0x9b: hash += 's'; break;

            case 0xb9:
            case 0xba: hash += 'z'; break;

            case 0xbb:
            case 0xbc: hash += 'z'; break;
          }

          break;
        }

        case 0xc3:
        {
          i ++;

          uc = (unsigned char) (str[i]);

          switch(uc)
          {
            case 0x93:
            case 0xb3: hash += 'o'; break;
          }

          break;
        }

        //
        // Lowerize big letters.
        //

        case 'A'...'Z':
        {
          hash += str[i] + 32;

          break;
        }

        //
        // Low letters and digits go without change.
        //

        case 'a'...'z':
        case '0'...'9':
        {
          hash += str[i];

          break;
        }

        //
        // Unallowed. Add hex dump.
        //

        default:
        {
          char tmp[8];

          snprintf(tmp, 3, "%02x", (unsigned char) str[i]);

          hash += tmp;
        }
      }
    }

    hash += '#';

    DBG_LEAVE3("CgiHashAdd");
  }

  void CgiHashAdd(string &hash, int value)
  {
    DBG_ENTER3("CgiHashAdd");

    DBG_MSG("Adding integer [%d] to hash...\n", value);

    char tmp[16 + 1];

    snprintf(tmp, 16, "%x", value);

    hash += tmp;

    hash += '#';

    DBG_LEAVE3("CgiHashAdd");
  }

  //
  // Default hash function.
  // Works always, but may be ineficient in general case.
  // Should be reimplemented in derived to fit case sensitive,
  // local characters, arguments order etc.
  //
  // RETURNS: none.
  //

  void CgiHashArgs(string &hash)
  {
    DBG_ENTER("CgiHashArgs");

    SSMap::iterator it;

    hash.clear();

    for (it = _GET.begin(); it != _GET.end(); it++)
    {
      //
      // If argument's value is empty, tt's unuset from CGI point of view, so
      // don't add it to hash.
      //

      if (!it -> second.empty())
      {
        CgiHashAdd(hash, it -> first);
        CgiHashAdd(hash, it -> second);
      }
      else
      {
        DBG_MSG("CgiHashArgs : Skipped empty variable [%s].\n", it -> first.c_str());
      }
    }

    if (hash.empty())
    {
      hash = "#";
    }

    DBG_MSG("Hashed args to [%s].\n", hash.c_str());

    DBG_LEAVE("CgiHashArgs");
  }
} /* namespace Tegenaria */
