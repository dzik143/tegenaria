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

//
// Purpose: Manage string lists in "elem1;elem2;...;elemn" format.
//

#include "Str.h"
#include <Tegenaria/Error.h>

namespace Tegenaria
{
  //
  // Split input string list in format "elem1;elem2;..." into STL vector
  // containig [elem1,elem2,...] strings.
  //
  // vec - STL string vector, where to store retrieved elements list (OUT).
  // str - string list in "elem1;elem2;..." format (IN).
  //

  void StrListSplit(vector<string> &vec, const char *str)
  {
    DBG_ENTER3("StrListSplit");

    vec.clear();

    if (str)
    {
      char *tmp = strdup(str);

      char *token = strtok(tmp, ";");

      while(token)
      {
        vec.push_back(token);

        token = strtok(NULL, ";");
      }

      free(tmp);
    }

    DBG_LEAVE3("StrListSplit");
  }

  //
  // Create one conitous "elem1;elem2;..." string from stl vector
  // containig [elem1,elem2,...].
  //
  // str - STL string, where to store created list (OUT).
  // vec - STL vector containing string elements (IN).
  //

  void StrListInit(string &str, vector<string> &vec)
  {
    DBG_ENTER3("StrListInit");

    str.clear();

    for (vector<string>::iterator it = vec.begin(); it != vec.end(); it++)
    {
      str += (*it);
      str += ';';
    }

    DBG_LEAVE3("StrListInit");
  }

  //
  // Add element to list.
  //
  // Example:
  //
  //   Input str  : "jozek;janek;"
  //   Input elem : "maciek"
  //   Output str : "jozek;janek;maciek;"
  //
  //
  // TIP#1: Element is added only if not exists on list yet.
  //
  // Parameters:
  //
  // str  - string containing list in format "elem1;elem2;...", where to add
  //        new element (IN/OUT).
  //
  // elem - element to add (IN).
  //
  // RETURNS: 0 if element added,
  //          ERR_XXX code otherwise.
  //

  int StrListAdd(string &str, const char *elem)
  {
    DBG_ENTER3("StrListAdd");

    int exitCode = ERR_WRONG_PARAMETER;

    if (elem)
    {
      //
      // If friend doesn't exist yet, add it to list.
      //

      if (StrListExists(str, elem) == 0)
      {
        str += elem;
        str += ";";

        exitCode = 0;
      }
      else
      {
        exitCode = ERR_ALREADY_EXISTS;
      }
    }

    DBG_LEAVE3("StrListAdd");

    return exitCode;
  }

  //
  // Remove element from list.
  //
  // Example:
  //
  //   Input str  : "jozek;janek;maciek;"
  //   Input elem : "janek"
  //   Output str : "jozek;maciek;"
  //
  // Parameters:
  //
  // str  - string containing list in format "elem1;elem2;...", which we want
  //        to modify (IN/OUT).
  //
  // elem - element to remove (IN).
  //
  // RETURNS: 0 if element removed,
  //          ERR_XXX code otherwise.
  //

  int StrListRemove(string &str, const char *elem)
  {
    DBG_ENTER3("StrListRemove");

    int exitCode = ERR_WRONG_PARAMETER;

    int found = 0;

    if (elem)
    {
      char *tmp = strdup(str.c_str());

      string newList;

      char *token = strtok(tmp, ";");

      while(token)
      {
        if (stricmp(token, elem) != 0)
        {
          newList += token;
          newList += ';';
        }
        else
        {
          found = 1;
        }

        token = strtok(NULL, ";");
      }

      str = newList;

      free(tmp);

      if (found == 1)
      {
        exitCode = ERR_OK;
      }
      else
      {
        exitCode = ERR_NOT_FOUND;
      }
    }

    DBG_LEAVE3("StrListRemove");

    return exitCode;
  }

  //
  // Check is elem exists on list.
  //
  // str  - string containing list in format "elem1;elem2;...", which we want
  //        search (IN).
  //
  // elem - element to found (IN).
  //
  // RETURNS: 1 if element exists in the list,
  //          0 otherwise.
  //

  int StrListExists(const string &str, const char *elem)
  {
    DBG_ENTER3("StrListExists");

    int found = 0;

    if (elem)
    {
      char *tmp = strdup(str.c_str());

      char *token = strtok(tmp, ";");

      while(found == 0 && token)
      {
        if (stricmp(token, elem) == 0)
        {
          found = 1;
        }

        token = strtok(NULL, ";");
      }

      free(tmp);
    }

    DBG_LEAVE3("StrListExists");

    return found;
  }
} /* namespace Tegenaria */
