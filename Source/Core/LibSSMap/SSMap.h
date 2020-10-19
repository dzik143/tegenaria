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

#ifndef Tegenaria_Core_SSMap_H
#define Tegenaria_Core_SSMap_H

#include <string>
#include <vector>
#include <map>
#include <cstdio>
#include <cstring>
#include <cstdlib>

namespace Tegenaria
{
  #ifndef FAIL
  #define FAIL(X) if (X) goto fail
  #endif

  using namespace std;

  //
  // Flags to format output file.
  //

  #define SSMAP_FLAGS_WIDE 1 // align to '=' character

  //
  // General class to store string |-> string map.
  //

  class SSMap : public map<string, string>
  {
    public:

    //
    // Serialize object to disk.
    //

    int saveToFile(const string &fname);
    int saveToFileEx(const string &fname, int flags);
    int loadFromFile(const string &fname);

    //
    // Serialize object to single, continous string.
    //

    void saveToString(string &data);

    int loadFromString(char *data);

    //
    // Set value.
    //

    void set(const char *lvalue, const char *rvalue);
    void setInt(const char *lvalue, int rvalue);
    void setPtr(const char *lvalue, const void *rvalue);

    void setStringList(const char *lvalue, vector<string> &stringList);

    //
    // Get value.
    //

    const char *get(const char *lvalue);
    const char *get(const string &lvalue);

    const char *safeGet(const char *lvalue);
    const char *safeGet(const string &lvalue);

    int getInt(const char *lvalue);
    int getInt(const string &lvalue);

    void getStringList(vector<string> &stringList, const char *lvalue);

    void *getPtr(const char *ptr);

    //
    // Check is any value assigned to given lvalue key.
    //

    int isset(const char *lvalue);
    int isset(const string &lvalue);
  };

} /* namespace Tegenaria */

#endif /* Tegenaria_Core_SSMap_H */
