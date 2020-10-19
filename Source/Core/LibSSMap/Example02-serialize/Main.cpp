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

#include <Tegenaria/SSMap.h>
#include <cstdio>
#include <vector>
#include <string>

using std::vector;
using std::string;

using namespace Tegenaria;

int main(int argc, char **argv)
{
  SSMap ssmap;

  string serial;

  //
  // Put example data into ssmap object.
  //

  ssmap["title"] = "Example01";
  ssmap["color"] = "red";
  ssmap["car"]   = "Porshe";

  ssmap.set("language", "assembler");

  ssmap.setInt("count", 666);

  //
  // Serialize object into one continous string.
  //

  ssmap.saveToString(serial);

  printf("Serialized data is: [%s].\n\n", serial.c_str());

  //
  // Clear object.
  //

  ssmap.clear();

  //
  // Load back object from serialized string.
  //

  ssmap.loadFromString(&serial[0]);

  printf("title    : [%s].\n", ssmap.get("title"));
  printf("color    : [%s].\n", ssmap.get("color"));
  printf("car      : [%s].\n", ssmap.get("car"));
  printf("language : [%s].\n", ssmap.get("language"));

  printf("count    : [%d].\n", ssmap.getInt("count"));

  return 0;
}
