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

using namespace Tegenaria;

int Function(int arg1, const char *arg2)
{
  DBG_ENTER_EX("Function", "arg1=[%d], arg2=[%s]", arg1, arg2);

  DBG_MSG("Function : Working...\n");

  DBG_LEAVE("Function");
}

int main()
{
  DBG_INIT("example.log");
  DBG_HEAD("LibDebug - Example");

  DBG_SET_LEVEL("debug3");

  DBG_ENTER("main");

  double test = 1.1234;

  Function(12345678, "test");

  DBG_MSG("Hello from main.\n"
          "Bla bla bla bla bla.\n"
          "Bla bla bla.");

  DBG_MSG("Double : [%lf].\n", test);

  Error("ERROR: Cannot bla bla bla bla.\n"
        "Error code is : 1234.");

  DBG_LEAVE("main");

  return 0;
}
