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

#ifndef QCBuild_H
#define QCBuild_H

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cctype>
#include <set>
#include "SourceInfo.h"
#include "AutoDoc.h"

//
// FIXME: Move target specified variables into
//        configuration files.
//

//
// Linux defines.
//

#ifdef __linux__
# define EXE_POSTFIX    ""
# define DLL_PREFIX     "lib"
# define DLL_POSTFIX    ".so"
# define HOST_MACHINE   "Linux"

# define CXX_COMPILER   "g++"
# define C_COMPILER     "gcc"

# define SCRIPT_POSTFIX      ".sh"
# define SCRIPT_DEL          "rm -rf"
# define SCRIPT_RMDIR        "rmdir"
# define SCRIPT_MKDIR        "mkdir"
# define SCRIPT_COPY         "cp"
# define SCRIPT_COMMENT      "#"
# define SCRIPT_INTRO        "#!/bin/sh"
# define SCRIPT_FAIL         "if [ $? -ne 0 ]\nthen\n  exit -1\nfi"
# define SCRIPT_MAKE_INSTALL "make install"
#endif

//
// Windows defines.
//

#ifdef WIN32
# define EXE_POSTFIX         ".exe"
# define DLL_PREFIX          ""
# define DLL_POSTFIX         ".dll"
# define HOST_MACHINE        "MinGW"
# define CXX_COMPILER        "g++"
# define C_COMPILER          "gcc"

# define SCRIPT_POSTFIX      ".bat"
# define SCRIPT_DEL          "del"
# define SCRIPT_RMDIR        "rmdir"
# define SCRIPT_MKDIR        "mkdir"
# define SCRIPT_COPY         "copy"
# define SCRIPT_COMMENT      "rem"
# define SCRIPT_INTRO        "@echo off"
# define SCRIPT_FAIL         "if %ERRORLEVEL% neq 0 exit /b %ERRORLEVEL%"
# define SCRIPT_MAKE_INSTALL "call make install"
#endif /* WIN32 */

//
// MacOS defines.
//

#ifdef __APPLE__
# define EXE_POSTFIX         ""
# define DLL_PREFIX          "lib"
# define DLL_POSTFIX         ".dylib"
# define HOST_MACHINE        "MacOS"

# define CXX_COMPILER        "g++"
# define C_COMPILER          "gcc"

# define SCRIPT_POSTFIX      ".sh"
# define SCRIPT_DEL          "rm"
# define SCRIPT_RMDIR        "rmdir"
# define SCRIPT_MKDIR        "mkdir"
# define SCRIPT_COPY         "cp"
# define SCRIPT_COMMENT      "#"
# define SCRIPT_INTRO        "#!/bin/sh"
# define SCRIPT_FAIL         "if [ $? -ne 0 ]\n  then\n  exit(-1)  \nfi"
# define SCRIPT_MAKE_INSTALL "make install"
#endif /* __APPLE__ */


#endif /* QCBuild_H */
