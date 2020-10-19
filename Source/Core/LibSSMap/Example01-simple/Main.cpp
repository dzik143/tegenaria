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

  vector<string> friends;

  //
  // Put example data into ssmap object.
  //

  ssmap["title"] = "Example01";
  ssmap["color"] = "red";
  ssmap["car"]   = "Porshe";

  ssmap.set("language", "assembler");

  ssmap.setInt("count", 666);

  //
  // Put example string list to ssmap object.
  //

  friends.push_back("john");
  friends.push_back("janek");
  friends.push_back("jozek");

  ssmap.setStringList("friends", friends);

  //
  // Dump ssmap to file.
  //

  ssmap.saveToFile("ssmap.dat");

  //
  // Clear objects.
  //

  ssmap.clear();
  friends.clear();

  //
  // Try load data back from file.
  //

  ssmap.loadFromFile("ssmap.dat");

  printf("title    : [%s].\n", ssmap.get("title"));
  printf("color    : [%s].\n", ssmap.get("color"));
  printf("car      : [%s].\n", ssmap.get("car"));
  printf("language : [%s].\n", ssmap.get("language"));

  printf("count    : [%d].\n", ssmap.getInt("count"));
  printf("friends  : [%s].\n", ssmap.get("friends"));

  ssmap.getStringList(friends, "friends");

  printf("\nfriends:\n");

  for (int i = 0; i < friends.size(); i++)
  {
    printf("  [%s]\n", friends[i].c_str());
  }

  return 0;
}
