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

#ifndef Tegenaria_Core_Args_H
#define Tegenaria_Core_Args_H

#include <string>

namespace Tegenaria
{
  using std::string;

  enum ArgsType
  {
    ARGS_INT,
    ARGS_DOUBLE,
    ARGS_FLOAT,
    ARGS_STRING,
    ARGS_CHAR,
    ARGS_MODE,
    ARGS_FLAG,
    ARGS_NULL
  };

  struct ArgsObj
  {
    ArgsType type_;

    const char *pattern_;

    void *target_;

    union
    {
      int intVal_;

      char charVal_;

      char *stringVal_;

      double doubleVal_;

      float floatVal_;
    };
  };

  //
  // Process arguments.
  //

  int ArgsParse(ArgsObj *ao, int argc, char **argv);

  int ArgsParse(ArgsObj *ao, const char *cmd);

  int ArgsAlloc(int *argc, char ***argv, const char *cmd);

  void ArgsFree(int argc, char **argv);

} /* namespace Tegenaria */

#endif /* Tegenaria_Core_Args_H */
