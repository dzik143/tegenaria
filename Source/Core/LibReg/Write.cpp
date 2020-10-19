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
// Purpose: High-level functions to modify windows registry.
//

#ifdef WIN32

#include "Reg.h"

namespace Tegenaria
{
  //
  // Write DWORD value to registry. If key not exist yet, function creates new one.
  //
  // rootKey - one of HKEY_XXX defines (e.g. HKEY_LOCAL_MACHINE) (IN).
  // path    - key path (e.g SYSTEM\CurrentControlSet\Control\FileSystem) (IN).
  // element - element name (e.g. Win95TruncatedExtensions) (IN).
  // value   - DWORD value to set (IN).
  // flags   - combination of REG_OPTION_XXX flags (e.g. REG_OPTION_VOLATILE) (IN/OPT).
  //
  // RETURNS: 0 if OK,
  //          WINAPI error code otherwise.
  //

  int RegSetDword(HKEY rootKey, const char *path,
                      const char *element, DWORD value, DWORD flags)
  {
    DBG_ENTER2("RegSetDword");

    int exitCode = -1;
    int regCode  = 0;

    HKEY key = NULL;

    DWORD keyType   = 0;
    DWORD keyValue  = 0;
    DWORD valueSize = sizeof(DWORD);

    //
    // Check args.
    //

    FAILEX(path == NULL,  "ERROR: path parameter cannot be NULL in RegSetDword.\n");
    FAILEX(element == NULL,  "ERROR: element parameter cannot be NULL in RegSetDword.\n");

    //
    // Open path (or create new one if does not exist yet).
    //

    regCode = RegCreateKeyEx(rootKey, path, 0, NULL, flags,
                                 KEY_READ | KEY_WRITE, NULL, &key, NULL);

    FAIL(regCode != ERROR_SUCCESS);

    //
    // Write element value to opened path.
    //

    regCode = RegSetValueEx(key, element, 0, REG_DWORD,
                                (PBYTE) &value, sizeof(value));

    FAIL(regCode != ERROR_SUCCESS);

    //
    // Show set value in debug log.
    //

    DEBUG1("RegSetDword : '%s\\%s\\%s' set to '%d'.\n",
               RegGetRootKeyName(rootKey), path, element, value);

    //
    // Clean up.
    //

    exitCode = 0;

    fail:

    if (exitCode)
    {
      if (regCode)
      {
        exitCode = regCode;
      }

      Error("ERROR: Cannot set REG_DWORD key '%s\\%s\\%s' to '%d'.\n"
                "Error code is %d.\n",
                    RegGetRootKeyName(rootKey), path, element, value, exitCode);
    }

    if (key)
    {
      RegCloseKey(key);
    }

    DBG_LEAVE2("RegSetDword");

    return exitCode;
  }

  //
  // Write string value to registry. If key not exist yet, function creates new one.
  //
  // rootKey - one of HKEY_XXX defines (e.g. HKEY_LOCAL_MACHINE) (IN).
  // path    - key path (e.g SYSTEM\CurrentControlSet\Control\FileSystem) (IN).
  // element - element name (e.g. Win95TruncatedExtensions) (IN).
  // value   - ASCIZ string to set (IN).
  // flags   - combination of REG_OPTION_XXX flags (e.g. REG_OPTION_VOLATILE) (IN/OPT).
  //
  // RETURNS: 0 if OK,
  //          WINAPI error code otherwise.
  //

  int RegSetString(HKEY rootKey, const char *path,
                       const char *element, const char *value, DWORD flags)
  {
    DBG_ENTER2("RegSetString");

    int exitCode = -1;
    int regCode  = 0;

    HKEY key = NULL;

    DWORD keyType   = 0;
    DWORD keyValue  = 0;
    DWORD valueSize = 0;

    //
    // Check args.
    //

    FAILEX(path == NULL,    "ERROR: path parameter cannot be NULL in RegSetString.\n");
    FAILEX(element == NULL, "ERROR: element parameter cannot be NULL in RegSetString.\n");
    FAILEX(value == NULL,   "ERROR: value parameter cannot be NULL in RegSetString.\n");

    //
    // Open path (or create new one if does not exist yet).
    //

    regCode = RegCreateKeyEx(rootKey, path, 0, NULL, flags,
                                 KEY_READ | KEY_WRITE, NULL, &key, NULL);

    FAIL(regCode != ERROR_SUCCESS);

    //
    // Compute string len including zero terminator.
    // We want to write last zero byte to registry too.
    //

    valueSize = strlen(value) + 1;

    //
    // Write element value to opened path.
    //

    regCode = RegSetValueEx(key, element, 0, REG_SZ,
                                (PBYTE) value, valueSize);

    FAIL(regCode != ERROR_SUCCESS);

    //
    // Show set value in debug log.
    //

    DEBUG1("RegSetString : '%s\\%s\\%s' set to '%s'.\n",
               RegGetRootKeyName(rootKey), path, element, value);

    //
    // Clean up.
    //

    exitCode = 0;

    fail:

    if (exitCode)
    {
      if (regCode)
      {
        exitCode = regCode;
      }

      Error("ERROR: Cannot set REG_SZ key '%s\\%s\\%s' to '%s'.\n"
                "Error code is %d.\n",
                    RegGetRootKeyName(rootKey), path, element, value, exitCode);
    }

    if (key)
    {
      RegCloseKey(key);
    }

    DBG_LEAVE2("RegSetString");

    return exitCode;
  }

  //
  // Write string value to registry. If key not exist yet, function creates new one.
  //
  // rootKey - one of HKEY_XXX defines (e.g. HKEY_LOCAL_MACHINE) (IN).
  // path    - key path (e.g SYSTEM\CurrentControlSet\Control\FileSystem) (IN).
  // element - element name (e.g. Win95TruncatedExtensions) (IN).
  // value   - ASCIZ string to set (IN).
  // flags   - combination of REG_OPTION_XXX flags (e.g. REG_OPTION_VOLATILE) (IN/OPT).
  //
  // RETURNS: 0 if OK,
  //          WINAPI error code otherwise.
  //

  int RegSetStringW(HKEY rootKey, const char *path,
                        const char *element, const wchar_t *value, DWORD flags)
  {
    DBG_ENTER2("RegSetStringW");

    int exitCode = -1;
    int regCode  = 0;

    HKEY key = NULL;

    DWORD keyType   = 0;
    DWORD keyValue  = 0;
    DWORD valueSize = 0;

    wchar_t elementW[MAX_PATH] = {0};

    //
    // Check args.
    //

    FAILEX(path == NULL,    "ERROR: path parameter cannot be NULL in RegSetString.\n");
    FAILEX(element == NULL, "ERROR: element parameter cannot be NULL in RegSetString.\n");
    FAILEX(value == NULL,   "ERROR: value parameter cannot be NULL in RegSetString.\n");

    //
    // Open path (or create new one if does not exist yet).
    //

    regCode = RegCreateKeyEx(rootKey, path, 0, NULL, flags,
                                 KEY_READ | KEY_WRITE, NULL, &key, NULL);

    FAIL(regCode != ERROR_SUCCESS);

    //
    // Compute string len including zero terminator.
    // We want to write last zero byte to registry too.
    //

    valueSize = wcslen(value) + 1;

    valueSize *= sizeof(wchar_t);

    //
    // Write element value to opened path.
    //

    swprintf(elementW, L"%hs", element);

    regCode = RegSetValueExW(key, elementW, 0, REG_SZ,
                                 (PBYTE) value, valueSize);

    FAIL(regCode != ERROR_SUCCESS);

    //
    // Show set value in debug log.
    //

    DEBUG1("RegSetStringW : '%s\\%s\\%s' set to '%ls'.\n",
               RegGetRootKeyName(rootKey), path, element, value);

    //
    // Clean up.
    //

    exitCode = 0;

    fail:

    if (exitCode)
    {
      if (regCode)
      {
        exitCode = regCode;
      }

      Error("ERROR: Cannot set REG_SZ key '%s\\%s\\%s' to '%ls'.\n"
                "Error code is %d.\n",
                    RegGetRootKeyName(rootKey), path, element, value, exitCode);
    }

    if (key)
    {
      RegCloseKey(key);
    }

    DBG_LEAVE2("RegSetStringW");

    return exitCode;
  }

  //
  // Write string list to registry. If key not exist yet, function creates new one.
  //
  // rootKey - one of HKEY_XXX defines (e.g. HKEY_LOCAL_MACHINE) (IN).
  // path    - key path (e.g SYSTEM\CurrentControlSet\Control\FileSystem) (IN).
  // element - element name (e.g. Win95TruncatedExtensions) (IN).
  // values  - list of strings to set (IN).
  // flags   - combination of REG_OPTION_XXX flags (e.g. REG_OPTION_VOLATILE) (IN/OPT).
  //
  // RETURNS: 0 if OK,
  //          WINAPI error code otherwise.
  //

  int RegSetStringList(HKEY rootKey, const char *path,
                           const char *element, list<string> values, DWORD flags)
  {
    DBG_ENTER2("RegSetStringList");

    int exitCode = -1;
    int regCode  = 0;

    HKEY key = NULL;

    DWORD keyType   = 0;
    DWORD keyValue  = 0;

    PBYTE buf     = NULL;
    PBYTE dst     = NULL;
    DWORD bufSize = 0;

    string debugList;

    //
    // Check args.
    //

    FAILEX(path == NULL,    "ERROR: path parameter cannot be NULL in RegSetString.\n");
    FAILEX(element == NULL, "ERROR: element parameter cannot be NULL in RegSetString.\n");

    //
    // Open path (or create new one if does not exist yet).
    //

    regCode = RegCreateKeyEx(rootKey, path, 0, NULL, flags,
                                 KEY_READ | KEY_WRITE, NULL, &key, NULL);

    FAIL(regCode != ERROR_SUCCESS);

    //
    // Compute whole list length including zero terminator.
    //


    bufSize = 0;

    for (list<string>::iterator it = values.begin(); it != values.end(); it++)
    {
      bufSize += it -> size() + 1;
    }

    bufSize ++;

    //
    // Allocate buffer to store raw list.
    //

    buf = (PBYTE) calloc(bufSize, 1);

    FAILEX(buf == NULL, "ERROR: Out of memory.\n");

    //
    // Copy elements from std::list to raw.
    //

    dst = buf;

    for (list<string>::iterator it = values.begin(); it != values.end(); it++)
    {
      memcpy(dst, it -> c_str(), it -> size());

      dst += it -> size() + 1;

      debugList += *it;
      debugList += ", ";
    }

    //
    // Write raw list to opened path.
    //

    regCode = RegSetValueEx(key, element, 0, REG_MULTI_SZ, buf, bufSize);

    FAIL(regCode != ERROR_SUCCESS);

    //
    // Show set value in debug log.
    //

    DEBUG1("RegSetStringList : '%s\\%s\\%s' set to '%s'.\n",
               RegGetRootKeyName(rootKey), path, element, debugList.c_str());

    //
    // Clean up.
    //

    exitCode = 0;

    fail:

    if (exitCode)
    {
      if (regCode)
      {
        exitCode = regCode;
      }

      Error("ERROR: Cannot set REG_MULTI_SZ key '%s\\%s\\%s'.\n"
                "Error code is %d.\n",
                    RegGetRootKeyName(rootKey), path, element, exitCode);
    }

    if (key)
    {
      RegCloseKey(key);
    }

    if (buf)
    {
      free(buf);
    }

    DBG_LEAVE2("RegSetStringList");

    return exitCode;
  }

  //
  // Remove given key from registry.
  //
  // rootKey - one of HKEY_XXX defines (e.g. HKEY_LOCAL_MACHINE) (IN).
  // path    - key path to remove (e.g SYSTEM\CurrentControlSet\Control\FileSystem) (IN).
  //
  // RETURNS: 0 if OK,
  //          WINAPI error code otherwise.
  //

  int RegRemove(HKEY rootKey, const char *path)
  {
    DBG_ENTER2("RegRemove");

    vector<string> subkeys;

    //
    // Remove subkeys first.
    //

    RegListSubkeys(subkeys, rootKey, path);

    for (int i = 0; i < subkeys.size(); i++)
    {
      string subpath = string(path) + "\\" + subkeys[i];

      RegRemove(rootKey, subpath.c_str());
    }

    //
    // Remove key itself.
    //

    int exitCode = RegDeleteKey(rootKey, path);

    if (exitCode != ERROR_SUCCESS)
    {
      Error("ERROR: Cannot remove key '%s\\%s'.\n"
                "Error code is : %d.\n",
                    RegGetRootKeyName(rootKey), path, exitCode);
    }
    else
    {
      DEBUG1("RegRemove : Key '%s\\%s' removed.\n", RegGetRootKeyName(rootKey), path);
    }

    DBG_LEAVE2("RegRemove");

    return exitCode;
  }
} /* namespace Tegenaria */

#endif /* WIN32 */
