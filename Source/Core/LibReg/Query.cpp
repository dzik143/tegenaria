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
// Purpose: High-level functions to query windows registry.
//

#ifdef WIN32

#include "Reg.h"

namespace Tegenaria
{
  //
  // Read DWORD value from registry. Quered key should be REG_DWORD type.
  //
  // value   - output buffer, where to store readed value (OUT).
  // rootKey - one of HKEY_XXX defines (e.g. HKEY_LOCAL_MACHINE) (IN).
  // path    - key path (e.g SYSTEM\CurrentControlSet\Control\FileSystem) (IN).
  // element - element name (e.g. Win95TruncatedExtensions) (IN).
  //
  // RETURNS: 0 if OK,
  //          WINAPI error code otherwise.
  //

  int RegGetDword(DWORD *value, HKEY rootKey, const char *path, const char *element)
  {
    DBG_ENTER2("RegGetDword");

    int exitCode = -1;
    int regCode  = 0;

    HKEY key = NULL;

    DWORD keyType   = 0;
    DWORD keyValue  = 0;
    DWORD valueSize = sizeof(DWORD);

    //
    // Check args.
    //

    FAILEX(value == NULL, "ERROR: value parameter cannot be NULL in RegGetDword.\n");
    FAILEX(path == NULL,  "ERROR: path parameter cannot be NULL in RegGetDword.\n");
    FAILEX(element == NULL,  "ERROR: element parameter cannot be NULL in RegGetDword.\n");

    //
    // Open path.
    //

    regCode = RegOpenKeyEx(rootKey, path, 0, KEY_READ, &key);

    FAIL(regCode != ERROR_SUCCESS);

    //
    // Read element value from opened path.
    //

    regCode = RegQueryValueEx(key, element, NULL,
                                  &keyType, (PBYTE) &keyValue, &valueSize);

    FAIL(regCode != ERROR_SUCCESS);

    //
    // Check readed key type. It should be DWORD.
    //

    if (keyType != REG_DWORD)
    {
      Error("ERROR: Registry key '%s\\%s\\%s' has type '%s', but 'REG_DWORD' expected.\n",
                RegGetRootKeyName(rootKey), path, element, RegGetTypeName(keyType));

      exitCode = ERROR_INVALID_DATA;

      goto fail;
    }

    //
    // Pass readed value to caller parameter.
    //

    *value = keyValue;

    //
    // Show quered value in debug log.
    //

    DEBUG1("RegGetDword : '%s\\%s\\%s' quered with value '%d'.\n",
               RegGetRootKeyName(rootKey), path, element, exitCode, keyValue);

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

      DEBUG1("RegGetDword : Querying '%s\\%s\\%s' failed with code '%d'.\n",
               RegGetRootKeyName(rootKey), path, element, exitCode);
    }

    if (key)
    {
      RegCloseKey(key);
    }

    DBG_LEAVE2("RegGetDword");

    return exitCode;
  }

  //
  // Read ASCIZ string value from registry. Quered key should be REG_SZ type.
  //
  // value     - output buffer, where to store readed string (OUT).
  // valueSize - size of value[] buffer in bytes (IN).
  // rootKey   - one of HKEY_XXX defines (e.g. HKEY_LOCAL_MACHINE) (IN).
  // path      - key path (e.g SYSTEM\CurrentControlSet\Control\BiosInfo) (IN).
  // element   - element name (e.g. SystemBiosDate) (IN).
  //
  // RETURNS: 0 if OK,
  //          WINAPI error code otherwise.
  //

  int RegGetString(char *value, int valueSize, HKEY rootKey,
                       const char *path, const char *element)
  {
    DBG_ENTER2("RegGetString");

    int exitCode = -1;
    int regCode  = 0;

    HKEY key = NULL;

    DWORD keyType   = 0;

    //
    // Check args.
    //

    FAILEX(value == NULL,   "ERROR: value parameter cannot be NULL in RegGetString.\n");
    FAILEX(valueSize <= 0,  "ERROR: valueSize parameter cannot be <= 0 in RegGetString.\n");
    FAILEX(path == NULL,    "ERROR: path parameter cannot be NULL in RegGetString.\n");
    FAILEX(element == NULL, "ERROR: element parameter cannot be NULL in RegGetString.\n");

    //
    // Default output string to empty.
    //

    value[0] = 0;

    //
    // Reserve one byte for string terminator.
    //

    valueSize --;

    //
    // Open path.
    //

    regCode = RegOpenKeyEx(rootKey, path, 0, KEY_READ, &key);

    FAIL(regCode != ERROR_SUCCESS);

    //
    // Read element value from opened path.
    //

    regCode = RegQueryValueEx(key, element, NULL,
                                  &keyType, (PBYTE) value, (PDWORD) &valueSize);

    FAIL(regCode != ERROR_SUCCESS);

    //
    // Check readed key type. It should be DWORD.
    //

    if (keyType != REG_SZ)
    {
      Error("ERROR: Registry key '%s\\%s\\%s' has type '%s', but 'REG_SZ' expected.\n",
                RegGetRootKeyName(rootKey), path, element, RegGetTypeName(keyType));

      exitCode = ERROR_INVALID_DATA;

      goto fail;
    }

    //
    // Show quered value in debug log.
    //

    DEBUG1("RegGetString : '%s\\%s\\%s' quered with value '%s'.\n",
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

      DEBUG1("RegGetString : Querying '%s\\%s\\%s' failed with code '%d'.\n",
                 RegGetRootKeyName(rootKey), path, element, exitCode);
    }

    if (key)
    {
      RegCloseKey(key);
    }

    DBG_LEAVE2("RegGetString");

    return exitCode;
  }

  // Read ASCIZ string list from registry. Quered key should be REG_MULTI_SZ type.
  //
  // values    - string list where to store readed list (OUT).
  // rootKey   - one of HKEY_XXX defines (e.g. HKEY_LOCAL_MACHINE) (IN).
  // path      - key path (e.g SYSTEM\CurrentControlSet\Control\ContentIndex) (IN).
  // element   - element name (e.g. DllsToRegister) (IN).
  //
  // RETURNS: 0 if OK,
  //          WINAPI error code otherwise.
  //

  int RegGetStringList(list<string> &values, HKEY rootKey,
                             const char *path, const char *element)
  {
    DBG_ENTER2("RegGetStringList");

    int exitCode = -1;
    int regCode  = 0;

    HKEY key = NULL;

    DWORD keyType = 0;

    BYTE *buf = NULL;

    DWORD bufSize = 0;

    char *token = NULL;

    string debugList;

    //
    // Check args.
    //

    FAILEX(path == NULL,    "ERROR: path parameter cannot be NULL in RegGetStringList.\n");
    FAILEX(element == NULL, "ERROR: element parameter cannot be NULL in RegGetStringList.\n");

    //
    // Default output list to empty.
    //

    values.clear();

    //
    // Open path.
    //

    regCode = RegOpenKeyEx(rootKey, path, 0, KEY_READ, &key);

    FAIL(regCode != ERROR_SUCCESS);

    //
    // Read element size.
    //

    regCode = RegQueryValueEx(key, element, NULL, &keyType, buf, &bufSize);

    FAIL(regCode != ERROR_SUCCESS);

    //
    // Check readed key type. It should be DWORD.
    //

    if (keyType != REG_MULTI_SZ)
    {
      Error("ERROR: Registry key '%s\\%s\\%s' has type '%s', but 'REG_MULTI_SZ' expected.\n",
                RegGetRootKeyName(rootKey), path, element, RegGetTypeName(keyType));

      exitCode = ERROR_INVALID_DATA;

      goto fail;
    }

    //
    // Allocate buffer to store readed list.
    // Reserve 2 extra bytes for 0 bytes terminator as emergency for corrupted
    // registry key.
    //

    buf = (BYTE *) malloc(bufSize + 2);

    FAILEX(buf == NULL, "ERROR: Out of memory.\n");

    //
    // Read raw list to buffer.
    //

    regCode = RegQueryValueEx(key, element, NULL,
                                  &keyType, (PBYTE) buf, &bufSize);

    FAIL(regCode != ERROR_SUCCESS);

    //
    // Make sure there are last string and list terminators.
    // It catches corrupted registry keys.
    //

    buf[bufSize]     = 0;
    buf[bufSize + 1] = 0;

    //
    // Put loaded list into output std list.
    //

    token = (char *) buf;

    while((token - (char *) buf + 1) < bufSize)
    {
      values.push_back(token);

      debugList += token;
      debugList += ", ";

      token = strchr(token, 0) + 1;
    }

    //
    // Show readed list in debug log.
    //

    DEBUG1("RegGetStringList : '%s\\%s\\%s' quered with value '%s'.\n",
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

      DEBUG1("RegGetStringList : Querying '%s\\%s\\%s' failed with code '%d'.\n",
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

    DBG_LEAVE2("RegGetStringList");

    return exitCode;
  }

  //
  // List subkeys inside given key.
  //
  //
  //

  int RegListSubkeys(vector<string> &subkeys, HKEY rootKey, const char *path)
  {
    DBG_ENTER2("RegListSubkeys");

    int exitCode = -1;
    int regCode  = -1;

    HKEY key = NULL;

    char subkey[1024] = {0};

    //
    // Check args.
    //

    FAILEX(path == NULL, "ERROR: path parameter cannot be NULL in RegListSubkeys.\n");

    //
    // Default output list to empty.
    //

    subkeys.clear();

    //
    // Open path.
    //

    regCode = RegOpenKeyEx(rootKey, path, 0, KEY_READ, &key);

    FAIL(regCode != ERROR_SUCCESS);

    //
    //
    //

    for (int i = 0;
             RegEnumKey(key, i, subkey, sizeof(subkey) - 1) == ERROR_SUCCESS;
                 i++)
    {
      subkeys.push_back(subkey);
    }

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

      Error("ERROR: Cannot list keys under '%s\\%s'.\nError code is '%d'.\n",
                 RegGetRootKeyName(rootKey), path, exitCode);
    }

    if (key)
    {
      RegCloseKey(key);
    }

    DBG_LEAVE2("RegListSubkeys");

    return exitCode;
  }

} /* namespace Tegenaria */

#endif /* WIN32 */
