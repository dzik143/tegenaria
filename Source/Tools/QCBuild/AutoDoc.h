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

#ifndef AutoDoc_H
#define AutoDoc_H

#include "Scan.h"
#include "SourceInfo.h"
#include <algorithm>

#define MAX_DEEP 16

struct FunctionInfo
{
  string name_;
  string fname_;
  string component_;
  string comment_;
  string return_;
  string args_;
};

struct SourceFile
{
  string name_;
  string title_;

  vector<FunctionInfo> functions_;
};

struct QCBuildPragma
{
  string cmd_;

  vector<string> args_;
};

int GenerateHtmlFromSourceInfo(FILE *f, SourceInfo *si, int privateMode);
int GenerateHtmlTableOfContent(FILE *f, SourceInfo *si, int privateMode);
int AutoDocGenerate(int privateMode);

#endif /* AutoDoc_H */
