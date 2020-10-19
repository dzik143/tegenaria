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

#ifndef Tegenaria_Core_Str_H
#define Tegenaria_Core_Str_H

//
// Includes.
//

#include <cstdio>
#include <string>
#include <cstdlib>
#include <vector>
#include <cstring>
#include <set>
#include <ctime>
#include <Tegenaria/Debug.h>

#ifdef WIN32
#include <windows.h>
#endif

namespace Tegenaria
{
  using namespace std;


  //
  // Defines.
  //

  #define STR_HTML_NON_BREAK_SPACES (1 << 0)

  #ifndef WIN32
  # undef stricmp
  # define stricmp strcasecmp
  #endif

  #define STR_LITTLE_ENDIAN 0
  #define STR_BIG_ENDIAN    1

  #define STR_MIN_LOGIN_LEN 1
  #define STR_MAX_LOGIN_LEN 60

  #define STR_MIN_PASS_LEN 6
  #define STR_MAX_PASS_LEN 20

  //
  // Enums.
  //

  enum StringToIntAlgorithm
  {
    SimpleAtoi,
    EveryDigits,
    UntilAlpha
  };

  //
  // Exported functions.
  //

  inline string StringFromChar(const char *p)
  {
    if (p)
    {
      return p;
    }
    else
    {
      return "N/A";
    }
  }

  void StringSet(char *&destination, const char *source);

  void StringAdd(char *&destination, const char *source1 = NULL,
                     const char *source2 = NULL, const char *source3 = NULL,
                         const char *source4 = NULL);

  void StringReset(char *&destination);

  //
  // Conversion.
  //

  int StringToInt(const string &, StringToIntAlgorithm algorithm = SimpleAtoi);
  int StringToInt(const char *, StringToIntAlgorithm algorithm = SimpleAtoi);

  double StringToDouble(const string &);

  string StringFromInt(int);
  string StringFromDouble(double);

  //
  // Tokenize.
  //

  void StrTokenize(vector<string> &tokens, const string &inputStr,
                       const char *delims = "\t\n ");

  void StrTokenize(vector<string> &tokens, const string &inputStr,
                       const char *delims, const char *delimsExtra);

  void StrTokenize(vector<char *> &tokens, char *inputStr,
                       const char *delims, const char *delimsExtra);

  void StrTokenize(set<string> &tokens, const string &inputStr,
                       const char *delims = "\t\n ");

  void StrTokenize(set<string> &tokens, const string &inputStr,
                       const char *delims, const char *delimsExtra);

  void StrStrTokenize(vector<string> &tokens,
                          const string &inputStr, const char *delim);

  void StrStrTokenize(vector<char *> &tokens, char *inputStr, const char *delim);

  int StrSplit(char *token, char **left, char **right, char splitChar);

  //
  // Capitalize.
  //

  void StrCapitalize(string &str);

  void StrCapitalize(char *str);

  string &StrLowerize(string &s);

  void StrLowerize(char *str);

  //
  // Parser.
  //

  const char *StrMatch(const char **it, const char *pattern);

  const char *StrSkipWhites(const char *);

  int StrCountChar(const char *s, char c);
  int StrCountDigits(const char *s);
  int StrCountAlpha(const char *s);

  int StrPopInt(const char **it);
  int StrPopInt(char **it);

  double StrPopDouble(const char **it);
  double StrPopDouble(char **it);

  char StrPopChar(const char **it);
  char StrPopChar(char **it);

  int StrMatchChar(const char **it, char expected);
  int StrMatchChar(char **it, char expected);

  void StrPopAlphaWord(const char **it, char *word, int wordSize);
  void StrPopAlphaWord(char **it, char *word, int wordSize);

  void StrPopAlphaWordBound(const char **it, char *word, int wordSize);
  void StrPopAlphaWordBound(char **it, char *word, int wordSize);

  string StrNormalizeWhiteSpaces(string s);

  //
  // Find.
  //

  char *stristr(const char *arg1, const char *arg2);
  char *stristr_utf8(const char *arg1, const char *arg2, int *byteLen);

  char *StrFindIWord(const char *arg1, const char *arg2);
  char *StrFindWord(const char *arg1, const char *arg2);

  char *StrFindIWord_utf8(const char *arg1, const char *arg2);

  string StrGetTextBeetwen(const char *buf, const char *startStr, const char *endStr);

  char *strstrex(const char *s, const char *pattern);
  char *strchrex(const char *s, char pattern);

  const char *StrFindCharMulti(const char *p, const char *chars, int zeroByteMatch);

  //
  // Remove.
  //

  void StrReplace(char *str, char oldChar, char newChar);
  void StrReplace(string &str, char oldChar, char newChar);

  char *StrRemoveChar(char *str, char c);

  string StrRemoveChar(const string &str, char c);

  string StrRemoveWhiteSpaces(const string &str);

  char *StrRemoveString(char *buf, const char *pattern);

  char *StrRemoveCaseString(char *buf, const char *pattern);

  string &StrReplaceString(string &buf, const char *oldStr, const char *newStr);

  int StrRemovePlChars_utf8(unsigned char *dst, unsigned char *src);

  string StrRemoveDeclensionPostfixPL(string word);

  //
  // Random.
  //

  string RandomIntString();

  string RandomString(int len, int minChar, int maxChar);

  //
  // Mask.
  //

  int StrMaskPhone(char *p);
  int StrMaskEmail(char *p);

  //
  // Verify.
  //

  int StrEmailVerify(const char *email);
  int StrLoginVerify(const char *login);
  int StrPassStrength(const char *pass);
  int StrPassVerify(const char *pass);

  //
  // String lists.
  //

  void StrListSplit(vector<string> &vec, const char *str);

  void StrListInit(string &str, vector<string> &vec);

  int StrListAdd(string &str, const char *elem);

  int StrListRemove(string &str, const char *elem);

  int StrListExists(const string &str, const char *elem);

  //
  // Html.
  //

  string StrEncodeToHtml(const string &str, unsigned int flags = 0);

  //
  // Local characters.
  //

  string StrCyr2Lat(const string &cyrtext);

  #ifdef WIN32

  int StrConvertCodePage(string &str, int sourceCP, int targetCP);

  int StrRemovePlChars(string &str, DWORD sourceCP = -1);

  #endif

  //
  // Binary string.
  //

  int StrPopByte(uint8_t *value, string &buf);
  int StrPopDword(uint32_t *value, string &buf, int flags = 0);
  int StrPopQword(uint64_t *value, string &buf, int flags = 0);
  int StrPopRaw(void *raw, int rawSize, string &buf);

  int StrPushByte(string &buf, uint8_t value);
  int StrPushDword(string &buf, uint32_t value, int flags = 0);
  int StrPushQword(string &buf, uint64_t value, int flags = 0);
  int StrPushRaw(string &buf, const void *raw, int rawSize);
  int StrPushString(string &buf, const char *str);

  //
  // C-style binary.
  //

  int StrPopRaw(void *raw, int rawSize, char **it, int *bytesLeft);
  int StrPopQword(uint64_t *value, char **it, int *bytesLeft, int flags = 0);
  int StrPopDword(uint32_t *value, char **it, int *bytesLeft, int flags = 0);
  int StrPopByte(uint8_t *value, char **it, int *bytesLeft);
  int StrPopString(const char **str, int *strLen, char **it, int *bytesLeft);

  //
  // Date strings.
  //

  const string StrDateGetToday();
  const string StrDateGetTodayUTC0();

  const string StrDateAddDays(string date, int nDays);
  const string StrDateAddDaysUTC0(string date, int nDays);

  //
  // Clean code.
  //

  string StrRemoveSingleLineComments(string code, const char *commentString);

  string StrRemoveMultiLineComments(const string &code,
                                        const char *commentBegin,
                                            const char *commentEnd);

  string StrMinifyCode(const string &code, const char *singleComment,
                           const char *multiCommentBegin, const char *multiCommentEnd,
                               int removeWhites = 0);

  //
  // Money.
  //

  int StrFormatMoney(char *money, int moneySize, const char *value);

} /* namespace Tegenaria */

#endif /* Tegenaria_Core_Str_H */
