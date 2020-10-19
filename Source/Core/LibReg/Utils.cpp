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

#ifdef WIN32

#include "Reg.h"

namespace Tegenaria
{
  //
  // Translate REG_XXX registry type into human readable string.
  //
  // type - one of REG_XXX (e.g. REG_DWORD) value (IN).
  //
  // RETURNS: human readable type's name
  //          or "UNKNOWN" if type not recognized.
  //

  const char *RegGetTypeName(DWORD type)
  {
    switch(type)
    {
      case REG_BINARY    : return "REG_BINARY";
      case REG_DWORD     : return "REG_DWORD";
      case REG_EXPAND_SZ : return "REG_EXPAND_SZ";
      case REG_LINK      : return "REG_LINK";
      case REG_MULTI_SZ  : return "REG_MULTI_SZ";
      case REG_NONE      : return "REG_NONE";
      case REG_QWORD     : return "REG_QWORD";
      case REG_SZ        : return "REG_SZ";
    }

    return "UNKNOWN";
  }

  //
  // Retrieve human readable name for one of HKEY_XXX predefined root keys
  // (e.g. HKEY_LOCAL_MACHINE).
  //
  // key - one of HKEY_XXX predefined keys (IN).
  //
  // RETURNS: Human readable key name
  //          or "UNKNOWN" if key not recognized.
  //

  const char *RegGetRootKeyName(HKEY key)
  {
    struct
    {
      HKEY key_;

      const char *name_;
    }
    knownKeys[] =
    {
      {HKEY_CLASSES_ROOT,     "HKEY_CLASSES_ROOT"},
      {HKEY_CURRENT_CONFIG,   "HKEY_CURRENT_CONFIG"},
      {HKEY_CURRENT_USER,     "HKEY_CURRENT_USER"},
      {HKEY_LOCAL_MACHINE,    "HKEY_LOCAL_MACHINE"},
      {HKEY_PERFORMANCE_DATA, "HKEY_PERFORMANCE_DATA"},
      {HKEY_USERS,            "HKEY_USERS"},
      {NULL,                  NULL}
    };

    for (int i = 0; knownKeys[i].key_; i++)
    {
      if (knownKeys[i].key_ == key)
      {
        return knownKeys[i].name_;
      }
    }

    return "UNKNOWN";
  }

} /* namespace Tegenaria */

#endif /* WIN32 */
