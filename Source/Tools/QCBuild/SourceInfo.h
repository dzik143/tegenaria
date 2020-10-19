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

#ifndef SourceInfo_H
#define SourceInfo_H

#include <cstdio>
#include <set>
#include <map>
#include <string>

#include <Tegenaria/Str.h>
#include <Tegenaria/File.h>

#include "Utils.h"

using std::string;
using std::map;
using std::set;

#define TYPE_PROJECT        0
#define TYPE_PROGRAM        1
#define TYPE_LIBRARY        2
#define TYPE_SIMPLE_LIBRARY 3
#define TYPE_SIMPLE_PROGRAM 4
#define TYPE_MULTI_LIBRARY  5
#define TYPE_UNKNOWN        -1

class SourceInfo
{
  int type_;

  int dependsResolved_;

  std::map<string, string> vars_;

  std::set<string> dependsSet_;
  std::set<string> privateDirsSet_;

  public:

  SourceInfo();

  const char *get(const char *lvalue);

  int getInt(const char *lvalue);

  string getPathList(const char *lvalue);

  string getLowered(const char *lvalue);

  int set(const char *lvalue, const char *rvalue, int concat = 0);

  int cat(const char *lvalue, const char *rvalue);

  int isset(const char *lvalue);

  int getType()
  {
    return type_;
  }

  void print();

  int resolved();

  void resolve(const char *compTitle);

  void clear();

  string getDepends();

  const std::set<string> &getDependsSet();

  int doesDependOn(const char *compTitle);

  int loadProject();

  int loadComponent();

  int loadFromFile(const char *fname);

  int doesMatchPrivateDir(SourceInfo *ci);
};

#endif /* SourceInfo_H */
