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

#include <Tegenaria/Str.h>
#include <cstdio>

using namespace Tegenaria;

int main(int argc, char **argv)
{
  string today     = StrDateGetToday();
  string todayUTC0 = StrDateGetTodayUTC0();
  string nextDay   = StrDateAddDays(today, 1);
  string nextMonth = StrDateAddDays(today, 31);
  string nextYear  = StrDateAddDays(today, 365);

  printf("Today            : [%s]\n", today.c_str());
  printf("Today UTC0       : [%s]\n", todayUTC0.c_str());
  printf("Today + 1 day    : [%s]\n", nextDay.c_str());
  printf("Today + 31 days  : [%s]\n", nextMonth.c_str());
  printf("Today + 365 days : [%s]\n", nextYear.c_str());

  return 0;
}
