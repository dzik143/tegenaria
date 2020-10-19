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

#ifndef Scan_H
#define Scan_H

#include <cstdio>
#include <cctype>
#include <string>
#include <vector>

#include <Tegenaria/File.h>
#include <Tegenaria/Str.h>
#include <Tegenaria/Debug.h>

using std::string;
using std::vector;

#define TOKEN_ERROR -1
#define TOKEN_EOF    0
#define TOKEN_NUMBER 1
#define TOKEN_CLASS  2
#define TOKEN_STRUCT 3
#define TOKEN_ENUM   4
#define TOKEN_IDENT  5
#define TOKEN_MACRO  6
#define TOKEN_EOL    7

#define TOKEN_BLOCK_BEGIN  8
#define TOKEN_BLOCK_END    9

#define TOKEN_PARENT_BEGIN 10
#define TOKEN_PARENT_END   11

#define TOKEN_OPERATOR     13
#define TOKEN_COMMENT      14
#define TOKEN_WHITE        15
#define TOKEN_DELIM        16
#define TOKEN_SEMICOLON    17

#define TOKEN_STRING       18
#define TOKEN_CHAR         19

#define TOKEN_INDEX_BEGIN  20
#define TOKEN_INDEX_END    21

void ScanInit(FILE *f);

const int ScanNextChar();
const int ScanPop();

const int ScanPeek();

const char *GetTokenName(int id);

int ScanPopToken(string &token);

#endif /* Scan_H */
