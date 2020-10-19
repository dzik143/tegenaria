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
  /*
  ** This lookup table is used to help decode the first byte of
  ** a multi-byte UTF8 character.
  */

  static const unsigned char sqlite3Utf8Trans1[] =
  {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
    0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x00, 0x01, 0x02, 0x03, 0x00, 0x01, 0x00, 0x00,
  };

  //
  // Pop next UTF8 character from string.
  //

  inline unsigned int GetUtf8(unsigned const char *zIn, unsigned const char **pzNext)
  {
    unsigned int c;

    /* Same as READ_UTF8() above but without the zTerm parameter.
    ** For this routine, we assume the UTF8 string is always zero-terminated.
    */
    c = *(zIn++);
    if( c>=0xc0 ){
      c = sqlite3Utf8Trans1[c-0xc0];
      while( (*zIn & 0xc0)==0x80 ){
        c = (c<<6) + (0x3f & *(zIn++));
      }
      if( c<0x80
          || (c&0xFFFFF800)==0xD800
          || (c&0xFFFFFFFE)==0xFFFE ){  c = 0xFFFD; }
    }
    *pzNext = zIn;

    if (c == 0x105 || c == 0x104) c = 'a';
    else if (c == 0x119 || c == 0x118) c = 'e';
    else if (c == 0x107 || c == 0x106) c = 'c';
    else if (c == 0x142 || c == 0x141) c = 'l';
    else if (c == 0x144 || c == 0x143) c = 'n';
    else if (c == 0x0f3 || c == 0xd3) c = 'o';
    else if (c == 0x17a || c == 0x179) c = 'z';
    else if (c == 0x17c || c == 0x17b) c = 'z';
    else if (c == 0x15a || c == 0x15b) c = 's';
    else if (c == '-') c = ' ';
    else if (c == '\t') c = ' ';
    else if (c == ',') c = ' ';
    else if (c == '.') c = ' ';
    else if (c == '\'') c = ' ';

    return c;
  }

  //
  // Case insensitive UTF-8 version of C strstr().
  //

  char *stristr_utf8(const char *arg1, const char *arg2, int *byteLen)
  {
    unsigned const char *a, *b;

    for(; *arg1; *arg1++)
    {
      a = (unsigned const char *) arg1;
      b = (unsigned const char *) arg2;

      while((GetUtf8(a, &a) | 32) == (GetUtf8(b, &b) | 32))
      {
        if(!*b)
        {
          if (byteLen)
          {
            *byteLen = a - (unsigned const char *) arg1;
          }

          return (char *) arg1;
        }

      }
    }

    return NULL;
  }

  //
  //
  //

  char *StrFindIWord_utf8(const char *arg1, const char *arg2)
  {
    //printf("-> StrFindIWord_utf8()...\n");

    const unsigned char *a, *b;

    const char *arg1Oryg = arg1;

    for(; *arg1; *arg1++)
    {
      a = (const unsigned char *) arg1;
      b = (const unsigned char *) arg2;

  //    while((*a++ | 32) == (*b++ | 32))

      while((GetUtf8(a, &a) | 32) == (GetUtf8(b, &b) | 32))
      {
        if(!*b
           && (arg1 == arg1Oryg || !isalpha(arg1[-1])
           && (*a == 0 || !isalpha(*a))))

        {
    //      printf("<- StrFindIWord_utf8()...\n");

          return (char *) arg1;
        }

      }
    }

    //printf("<- StrFindIWord_utf8()...\n");

    return NULL;
  }

  //
  // Change polish local characters into pure ASCII equivalent.
  //
  // dst - buffer where to store pure ASCIIZ string (OUT).
  // src - source utf8 string potentially containing polish chars (IN).
  //
  // RETURNS: 0 if OK.
  //

  int StrRemovePlChars_utf8(unsigned char *dst, unsigned char *src)
  {
    if (src == NULL || dst == NULL)
    {
      return -1;
    }

    while(*src)
    {
      switch(*src)
      {
        case 0xc4:
        {
          src ++;

          switch(*src)
          {
            case 0x84: dst[0] = 'A'; break;
            case 0x85: dst[0] = 'a'; break;

            case 0x86: dst[0] = 'C'; break;
            case 0x87: dst[0] = 'c'; break;

            case 0x98: dst[0] = 'E'; break;
            case 0x99: dst[0] = 'e'; break;

            default  : dst[0] = ' ';
          }

          break;
        }

        case 0xc5:
        {
          src ++;

          switch(*src)
          {
            case 0x81: dst[0] = 'L'; break;
            case 0x82: dst[0] = 'l'; break;

            case 0x83: dst[0] = 'N'; break;
            case 0x84: dst[0] = 'n'; break;

            case 0x9a: dst[0] = 'S'; break;
            case 0x9b: dst[0] = 's'; break;

            case 0xb9: dst[0] = 'Z'; break;
            case 0xba: dst[0] = 'z'; break;

            case 0xbb: dst[0] = 'Z'; break;
            case 0xbc: dst[0] = 'z'; break;

            default  : dst[0] = ' ';
          }

          break;
        }

        case 0xc3:
        {
          src ++;

          switch(*src)
          {
            case 0x93: dst[0] = 'O'; break;
            case 0xb3: dst[0] = 'o'; break;

            default  : dst[0] = ' ';
          }

          break;
        }

        default:
        {
          dst[0] = src[0];
        }
      }

      src ++;
      dst ++;
    }

    *dst = 0;

    return 0;
  }

  static const char *STR_Cyr[] =
  {
    "а","б","в","г","д","е","ё","ж","з","и","й","к","л","м","н","о","п","р","с","т","у",
    "ф","х","ц","ч","ш","щ","ъ", "ы","ь", "э", "ю","я",
    "А","Б","В","Г","Д","Е","Ё","Ж","З","И","Й","К","Л","М","Н","О","П","Р","С","Т","У",
    "Ф","Х","Ц","Ч","Ш","Щ","Ъ", "Ы","Ь", "Э", "Ю","Я",
    NULL
  };

  static const char *STR_Lat[] =
  {
    "a","b","v","g","d","e","e","zh","z","i","y","k","l","m","n","o","p","r","s","t","u",
    "f" ,"h" ,"ts" ,"ch","sh" ,"sht" ,"i", "y", "y", "e" ,"yu" ,"ya","A","B","V","G","D","E","E","Zh",
    "Z","I","Y","K","L","M","N","O","P","R","S","T","U",
    "F" ,"H" ,"Ts" ,"Ch","Sh" ,"Sht" ,"I" ,"Y" ,"Y", "E", "Yu" ,"Ya",
    NULL
  };

  string StrCyr2Lat(const string &cyrtext)
  {
    string rv;

    if (cyrtext.size() < 2)
    {
      rv = cyrtext;
    }
    else
    {
      const char *src = cyrtext.c_str();
      const char *lat = NULL;

      for (int i = 0; i < cyrtext.size() - 1; i ++)
      {
        lat = NULL;

        for (int j = 0; STR_Cyr[j]; j++)
        {
          if (memcmp(src, STR_Cyr[j], 2) == 0)
          {
            lat = STR_Lat[j];

            break;
          }
        }

        if (lat)
        {
          rv  += lat;
          src += 2;
        }
        else
        {
          rv += src[0];
          src++;
        }
      }
    }

    return rv;
  }

  #ifdef WIN32

  //
  // Convert code page from <sourceCP> to <targetCP>.
  //
  // str      - string to convert (IN/OUT).
  // sourceCP - original code page of input string (IN).
  // targetCP - destination code page, we want convert to (IN).
  //
  // RETURNS: 0 if OK.
  //

  int StrConvertCodePage(string &str, int sourceCP, int targetCP)
  {
    string tmpWide;

    tmpWide.resize(str.size() * 3);

    str.resize(str.size() * 2);

    if (MultiByteToWideChar(sourceCP, MB_PRECOMPOSED, str.c_str(), -1,
                                (wchar_t *) tmpWide.c_str(), tmpWide.size()) <= 0)
    {
      return -1;
    }

    if (WideCharToMultiByte(targetCP, 0, (wchar_t *) tmpWide.c_str(), -1,
                                (char *) str.c_str(), str.size(), NULL, NULL) <= 0)
    {
      return -1;
    }

    str.resize(strlen(str.c_str()));

    return 0;
  }

  //
  // Replace all polish non-lating characters by lating equvalent.
  //
  // str      - string to convert (IN/OUT).
  // sourceCP - code page of input string (IN).
  //
  // RETURNS: 0 if OK.
  //

  int StrRemovePlChars(string &str, DWORD sourceCP)
  {
    string tmpWide;

    tmpWide.resize(str.size() * 3);

    str.resize(str.size() * 2);

    wchar_t *ws = (wchar_t *) &tmpWide[0];

    if (MultiByteToWideChar(sourceCP, 0, str.c_str(), -1,
                                (wchar_t *) tmpWide.c_str(), tmpWide.size()) <= 0)
    {
      return -1;
    }

    for (int i = 0; ws[i]; i++)
    {
      int c = ws[i];

           if (c == 0x105 || c == 0x104) c = 'a';
      else if (c == 0x119 || c == 0x118) c = 'e';
      else if (c == 0x107 || c == 0x106) c = 'c';
      else if (c == 0x142 || c == 0x141) c = 'l';
      else if (c == 0x144 || c == 0x143) c = 'n';
      else if (c == 0x0f3 || c == 0xd3) c = 'o';
      else if (c == 0x17a || c == 0x179) c = 'z';
      else if (c == 0x17c || c == 0x17b) c = 'z';

      ws[i] = c;
    }

    if (WideCharToMultiByte(sourceCP, 0, (wchar_t *) tmpWide.c_str(), -1,
                                (char *) str.c_str(), str.size(), NULL, NULL) <= 0)
    {
      return -1;
    }

    str.resize(strlen(&str[0]));

    return 0;
  }

  #endif /* WIN32 */
} /* namespace Tegenaria */
