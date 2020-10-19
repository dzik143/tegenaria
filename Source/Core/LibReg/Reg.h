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

#ifndef Tegenaria_Core_LibReg_H
#define Tegenaria_Core_LibReg_H

#ifdef WIN32

#include <windows.h>
#include <list>
#include <vector>
#include <string>

#include <Tegenaria/Debug.h>

namespace Tegenaria
{
  using std::list;
  using std::vector;
  using std::string;

  //
  // Query functions.
  //

  int RegGetDword(DWORD *value, HKEY rootKey,
                      const char *path, const char *element);

  int RegGetString(char *value, int valueSize, HKEY rootKey,
                       const char *path, const char *element);

  int RegGetStringList(list<string> &values, HKEY rootKey,
                           const char *path, const char *element);

  int RegListSubkeys(vector<string> &subkeys, HKEY rootKey, const char *path);

  //
  // Write functions.
  //

  int RegSetDword(HKEY rootKey, const char *path,
                      const char *element, DWORD value, DWORD flags = 0);

  int RegSetString(HKEY rootKey, const char *path,
                       const char *element, const char *value, DWORD flags = 0);

  int RegSetStringW(HKEY rootKey, const char *path,
                       const char *element, const wchar_t *value, DWORD flags = 0);

  int RegSetStringList(HKEY rootKey, const char *path,
                           const char *element, list<string> values, DWORD flags = 0);

  int RegRemove(HKEY rootKey, const char *path);

  //
  // Internal use only.
  //

  const char *RegGetTypeName(DWORD type);
  const char *RegGetRootKeyName(HKEY key);

} /* namespace Tegenaria */

#endif /* WIN32 */
#endif /* Tegenaria_Core_LibReg_H */
