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
// Purpose: Convert variety types into string and vice versa.
//

#include "Str.h"

namespace Tegenaria
{
  //
  // Convert std::string to integer.
  //
  // str       - input string e.g. "1234" (IN).
  // algorithm - algorithm to use for conversion (IN).
  //
  // Possible <algorithms> are:
  //
  // - SimpleAtoi  : Call C atoi() function on string.
  //
  // - EveryDigits : Skip all non-decimal number e.g. it converts "1-234xx-56"
  //                 into 123456 number.
  //
  // - UntilAlpha  : stop converting on first alpha charcter e.g. it
  //                 converts "123-456xxx789" into 123456.
  //
  // RETURNS: Decoded integer.
  //

  int StringToInt(const string &str, StringToIntAlgorithm algorithm)
  {
    return StringToInt(str.c_str(), algorithm);
  }

  //
  // Convert std::string to integer.
  //
  // str       - input string e.g. "1234" (IN).
  // algorithm - algorithm to use for conversion (IN).
  //
  // Possible <algorithms> are:
  //
  // - SimpleAtoi  : Call C atoi() function on string.
  //
  // - EveryDigits : Skip all non-decimal number e.g. it converts "1-234xx-56"
  //                 into 123456 number.
  //
  // - UntilAlpha  : stop converting on first alpha charcter e.g. it
  //                 converts "123-456xxx789" into 123456.
  //
  // RETURNS: Decoded integer.
  //

  int StringToInt(const char *str, StringToIntAlgorithm algorithm)
  {
    if (str == NULL)
    {
      return 0;
    }

    switch(algorithm)
    {
      case SimpleAtoi:
      {
        return atoi(str);
      }

      case EveryDigits:
      {
        char tmp[64] = {0};

        int p = 0;

        for (int i = 0; str[i] && p < 63; i++)
        {
          if (str[i] >= '0' && str[i] <= '9')
          {
            tmp[p] = str[i];

            p ++;
          }
        }

        return atoi(tmp);
      }

      case UntilAlpha:
      {
        char tmp[64] = {0};

        int p = 0;

        for (int i = 0; str[i] && p < 63 && !isalpha(str[i]) &&
                            str[i] != ',' && str[i] != '.'; i++)
        {
          if (str[i] >= '0' && str[i] <= '9')
          {
            tmp[p] = str[i];

            p ++;
          }
        }

        return atoi(tmp);
      }
    }

    return 0;
  }

  //
  // Convert std::string into double value.
  //
  // RETURNS: Double value stored in input string.
  //

  double StringToDouble(const string &str)
  {
    return atof(str.c_str());
  }

  //
  // Create std::string from input integer value.
  //

  string StringFromInt(int x)
  {
    string ret;

    ret.resize(64);

    snprintf(&ret[0], 64, "%d", x);

    ret.resize(strlen(&ret[0]));

    return ret;
  }

  //
  // Create std::string from input double value.
  //

  string StringFromDouble(double x)
  {
    string ret;

    ret.resize(64);

    snprintf(&ret[0], 64, "%lf", x);

    ret.resize(strlen(&ret[0]));

    return ret;
  }
} /* namespace Tegenaria */
