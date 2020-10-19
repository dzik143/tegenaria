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
  string str;

  vector<string> vec;

  //
  // Add 3 elements.
  //

  StrListAdd(str, "jozek");
  StrListAdd(str, "maciek");
  StrListAdd(str, "janek");

  printf("List after add : [%s]\n", str.c_str());

  //
  // Remove element from the middle.
  //

  StrListRemove(str, "maciek");

  printf("List after remove : [%s]\n", str.c_str());

  //
  // Check is given lement exists.
  //

  printf("Jozek exists : [%d].\n", StrListExists(str, "jozek"));
  printf("Kuba exists : [%d].\n", StrListExists(str, "kuba"));

  //
  // Convert string list into STL vector.
  //

  StrListSplit(vec, str.c_str());

  printf("Splitted list:\n");

  for (int i = 0; i < vec.size(); i++)
  {
    printf("  Element no %d : [%s]\n", i, vec[i].c_str());
  }

  return 0;
}
