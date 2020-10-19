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
// Functions to manipulate date strings (e.g. "2014-10-14").
//

#include "Str.h"

#ifdef WIN32
#include <strptime.h>
#endif

namespace Tegenaria
{
  //
  // Get local today date string in format: YYYY-MM-DD.
  //
  // RETURNS: String containing today date e.g. "2014-10-14".
  //

  const string StrDateGetToday()
  {
    time_t now = time(0);

    struct tm tstruct;

    char buf[80];

    tstruct = *localtime(&now);

    strftime(buf, sizeof(buf), "%Y-%m-%d", &tstruct);

    return buf;
  }

  //
  // Get UTC0 today date string in format: YYYY-MM-DD.
  //
  // RETURNS: String containing today date e.g. "2014-10-14".
  //

  const string StrDateGetTodayUTC0()
  {
    time_t now = time(0);

    struct tm tstruct;

    char buf[80];

    tstruct = *gmtime(&now);

    strftime(buf, sizeof(buf), "%Y-%m-%d", &tstruct);

    return buf;
  }

  //
  // Add N days to date string.
  //
  // Example: "2014-10-14" + 1 day = "2014-10-15"
  //
  // date  - input date string to increase (IN).
  // nDays - how many days to add (IN).
  //
  // RETURNS: Date string increased by given number of days.
  //

  const string StrDateAddDays(string date, int nDays)
  {
    int exitCode = -1;

    string ret = date;

    int yyyy = 0;
    int mm   = 0;
    int dd   = 0;

    struct tm tstruct = {0};

    time_t ts;

    char buf[80];

    //
    // Check args.
    //

    FAILEX(date.size() != 10, "ERROR: Invalid date format in StrDateAddDays().\n");

    //
    // Convert date string into tm struct.
    //

    FAILEX(strptime(date.c_str(), "%Y-%m-%d", &tstruct) == NULL,
               "ERROR: Invalid date format in StrDateAddDays().\n");

    //
    // Convert tm struct into unix timestamp.
    //

    ts = mktime(&tstruct);

    //
    // Add days to timestamp.
    //

    ts += nDays * 3600 * 24;

    //
    // Convert increased timestamp back to string.
    //

    tstruct = *localtime(&ts);

    strftime(buf, sizeof(buf), "%Y-%m-%d", &tstruct);

    ret = buf;

    //
    // Error handler.
    //

    exitCode = 0;

    fail:

    return ret;
  }

  //
  // Add N days to UTC0 date string.
  //
  // Example: "2014-10-14" + 1 day = "2014-10-15"
  //
  // date  - input date string to increase (IN).
  // nDays - how many days to add (IN).
  //
  // RETURNS: Date string increased by given number of days.
  //

  const string StrDateAddDaysUTC0(string date, int nDays)
  {
    int exitCode = -1;

    string ret = date;

    int yyyy = 0;
    int mm   = 0;
    int dd   = 0;

    struct tm tstruct = {0};

    time_t ts;

    char buf[80];

    //
    // Check args.
    //

    FAILEX(date.size() != 10, "ERROR: Invalid date format in StrDateAddDays().\n");

    //
    // Convert date string into tm struct.
    //

    FAILEX(strptime(date.c_str(), "%Y-%m-%d", &tstruct) == NULL,
               "ERROR: Invalid date format in StrDateAddDays().\n");

    //
    // Convert tm struct into unix timestamp.
    //

    ts = mktime(&tstruct);

    //
    // Add days to timestamp.
    //

    ts += nDays * 3600 * 24;

    //
    // Convert increased timestamp back to string.
    //

    tstruct = *gmtime(&ts);

    strftime(buf, sizeof(buf), "%Y-%m-%d", &tstruct);

    ret = buf;

    //
    // Error handler.
    //

    exitCode = 0;

    fail:

    return ret;
  }
} /* namespace Tegenaria */
