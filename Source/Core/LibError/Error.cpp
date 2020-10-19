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

#include <cstdio>
#include <cstdlib>
#include <errno.h>
#include <cstring>
#include "Error.h"

#ifdef WIN32
# include <windows.h>
#endif

namespace Tegenaria
{
  //
  // Translate ERR_XXX codes into human readable
  // string.
  //
  // code - one of ERR_XXX codes defined in LibServer.h (IN).
  //
  // RETURNS: Human readable message or empty string if
  //          unknown code.
  //

  const char *ErrGetString(int code)
  {
    static struct
    {
      int code_;

      const char *msg_;
    }

    knownCodes[] =
    {
      //
      // 0-99
      // Generic errors.
      //

      {ERR_OK,                   "OK"},
      {ERR_GENERIC,              "Generic error"},
      {-1,                       "Generic error"},

      {ERR_NOT_IMPLEMENTED,      "Not implemented"},
      {ERR_NOT_FOUND,            "Element not found"},
      {ERR_ALREADY_EXISTS,       "Element already exists"},
      {ERR_WRONG_PARAMETER,      "Invalid parameter(s)"},
      {ERR_ACCESS_DENIED,        "Access denied"},
      {ERR_TIMEOUT,              "Timeout reached"},
      {ERR_SYNTAX_ERROR,         "Syntaxt error"},
      {ERR_WRONG_PATH,           "Invalid or missing path parameter" },
      {ERR_WRONG_ALIAS,          "Invalid or missing alias parameter"},
      {ERR_WRONG_USER,           "Invalid or missing user parameter"},
      {ERR_WRONG_MODE,           "Invalid or missing mode parameter"},
      {ERR_WRONG_PIPENAME,       "Invalid or missing pipename parameter"},
      {ERR_WRONG_TEXT,           "Invalid or missing text parameter"},
      {ERR_TEXT_TOO_LONG,        "Text is too long"},
      {ERR_WRONG_TYPE,           "Invalid or missing type parameter"},
      {ERR_WRONG_ACL,            "Invalid or missing accesslist parameter"},
      {ERR_WRONG_RESULT,         "Invalid or missing result parameter"},
      {ERR_ACL_TOO_LONG,         "Accesslist is too long"},
      {ERR_COULD_NOT_CONNECT,    "Could not connect to server"},
      {ERR_USER_CONN_FAILED,     "Could not connect with remote user"},
      {ERR_PATH_NOT_FOUND,       "Path not found"},
      {ERR_INVALID_SESSION_ID,   "Invalid session ID"},
      {ERR_AUTH_ERROR,           "Authentification error"},
      {ERR_NOT_CONNECTED,        "You are not connected"},
      {ERR_FILE_TOO_BIG,         "File is too big"},

      //
      // 100-149
      // Dlserver errors.
      //

      {ERR_DLSERVER_START_FAILED,  "Failed to initialize the IPC connection"},
      {ERR_DLSERVER,               "Internal error"},
      {ERR_SELF_ACCESS,            "Access to own data" },
      {ERR_ALREADY_ONLINE,         "User already online"},
      {ERR_ALIAS_IN_USE,           "Selected alias already in use"},
      {ERR_PATH_ALREADY_SHARED,    "Selected path is already shared under another alias"},
      {ERR_ALREADY_MOUNTED,        "Selected resource is already mounted"},
      {ERR_FORBIDDEN_FOR_TEMP,     "Operation forbidden for not registered user"},
      {ERR_ALIAS_NOT_FOUND,        "Alias not found"},
      {ERR_NOT_MOUNTED,            "Selected alias is not mounted"},
      {ERR_TOO_MANY_SHARES,        "Too many shares"},
      {ERR_ALIAS_TOO_LONG,         "Alias too long"},
      {ERR_COULD_NOT_MOUNT,        "Could not mount remote resource"},
      {ERR_DLMOUNT_FAIL,           "Dirligo Mounter process failed"},
      {ERR_DLMOUNT_TIMEOUT,        "Timeout while mounting"},
      {ERR_P2P_INIT_FAILED,        "Could not connect with remote user"},
      {ERR_NO_P2P_SESSION,         "There is no peer to peer connection with selected user"},
      {ERR_COULT_NOT_SEND_REQUEST, "Could not send request"},


      {ERR_MOUNT_REQUEST_ERROR,  "Error while processing mount request"},
      {ERR_MOUNT_REMOTE_ERROR,   "Mount failed on remote side"},
      {ERR_DLMOUNT_CREATE_ERROR, "Could not start dlmount process"},

      {ERR_DOKAN_NOT_FOUND,      "Could not found dokan driver"},
      {ERR_DOKAN_DISABLED,       "Dokan driver is disabled"},
      {ERR_DOKAN_DLL_ERROR,      "Could not load dokan.dll library"},

      {ERR_DLVDRIVE_NOT_FOUND,   "Could not found Dirligo Virtual Drive driver"},
      {ERR_DLVDRIVE_DISABLED,    "Dirligo Virtual Drive is disabled"},
      {ERR_DLVDRIVE_DLL_ERROR,   "Could not load dlvdrive.dll library"},

      //
      // 200-249
      // Data base.
      //

      {ERR_DB_ERROR,                "Database error"},
      {ERR_DB_ALREADY_EXISTS,       "Element already exists in database"},
      {ERR_DB_NOT_FOUND,            "Element not found in database"},
      {ERR_DB_FRIEND_EXISTS,        "Friend alredy exists"},
      {ERR_DB_FRIEND_NOT_FOUND,     "Friend not found"},
      {ERR_DB_SQL_INJECTION,        "Attemp to SQL injection detected"},
      {ERR_DB_WRONG_PARAMETER,      "Wrong parrameter(s) passed to database function"},
      {ERR_DB_INVITATION_EXISTS,    "Friend already invitated"},
      {ERR_DB_INVITATION_NOT_FOUND, "Friend does not exists on invitation list"},
      {ERR_DB_TOO_LESS_LICENSES,    "Too less licenses"},
      {ERR_DB_LOGIN_ALREADY_EXISTS, "Login already exists in database"},
      {ERR_DB_LOGIN_NOT_FOUND,      "Login not found in database"},

      //
      // 250-299
      // Data validation.
      //

      {ERR_VERIFY_TOO_LONG,          "Parameter is too long"},
      {ERR_VERIFY_TOO_SHORT,         "Parameter is too short"},
      {ERR_VERIFY_WRONG_CHAR,        "Parameter contains forbidden character(s)"},
      {ERR_VERIFY_WRONG_FORMAT,      "Parameter has incorrect format"},
      {ERR_VERIFY_ALIAS_FORBIDDEN,   "Aliases are not allowed"},
      {ERR_VERIFY_COMMENT_FORBIDDEN, "Comments are not allowed"},

      //
      // 300-349
      // Accserver.

      {ERR_ACCSERVER,                  "Internal error"},
      {ERR_ACCSERVER_CAPTCHA_ERROR,    "Wrong captcha"},
      {ERR_ACCSERVER_PASS_ERROR,       "Incorrect or empty password"},
      {ERR_ACCSERVER_AUTH_ERROR,       "Authentification failed"},
      {ERR_ACCSERVER_EMAIL_ERROR,      "Incorrect or missing email address"},
      {ERR_ACCSERVER_EMAIL_EXISTS,     "Email already exists in database"},
      {ERR_ACCSERVER_EMAIL_SEND_ERROR, "Cannot to send email"},
      {ERR_ACCSERVER_LOGIN_ERROR,      "Incorrect or empty login"},
      {ERR_ACCSERVER_LOGIN_EXISTS,     "Login already exists in database"},
      {ERR_ACCSERVER_LOGIN_NOT_EXIST,  "Login not found"},
      {ERR_ACCSERVER_ALREADY_ACTIVE,   "Account already activated"},
      {ERR_ACCSERVER_ACCOUNT_INACTIVE, "Account inactive"},

      //
      // 500-549
      // P2PServer codes.
      //

      {ERR_P2PSERVER,                       "Internal error"},
      {ERR_P2PSERVER_ALREADY_ONLINE,        "User is already online"},
      {ERR_P2PSERVER_USER_OFFLINE,          "User is offline"},
      {ERR_P2PSERVER_AUTH_ERROR,            "Authentication failed"},
      {ERR_P2PSERVER_SHARES_NOT_SAVED,      "Could not save the share list"},
      {ERR_P2PSERVER_SHARES_NOT_LOADED,     "Could not load share list"},
      {ERR_P2PSERVER_PACKET_PARTIAL,        "Partial packet received"},
      {ERR_P2PSERVER_PACKET_INVALID,        "Invalid packet received"},
      {ERR_P2PSERVER_LICENSE_INACTIVE,      "License is not activated"},
      {ERR_P2PSERVER_LICENSE_EXPIRED,       "License has expired"},
      {ERR_P2PSERVER_LICENSE_TRIAL_EXPIRED, "Trial period has expired"},

      //
      // End of table terminator.
      //

      {0, NULL}
    };

    for (int i = 0; knownCodes[i].msg_; i++)
    {
      if (knownCodes[i].code_ == code)
      {
        return knownCodes[i].msg_;
      }
    }

    return "Unknown error";
  }

  //
  // Get last saved system error code.
  // It calls internally:
  // - GetLastError() on Windows
  // - errno() on Linux
  //
  // RETURNS: System error code saved from last system function.
  //

  int ErrGetSystemError()
  {
    #ifdef WIN32
    return int(GetLastError());
    #else
    return errno;
    #endif
  }

  //
  // Get last saved system error as human readable string.
  // It calls internally:
  // - GetLastError() on Windows
  // - errno on Linux
  //
  // RETURNS: System error code saved from last system function converted
  //          to human readable string (eg. "access denied").
  //

  const string ErrGetSystemErrorString()
  {
    string ret = "Unknown";

    //
    // Windows.
    //

    #ifdef WIN32
    {
      DWORD lastError = GetLastError();

      size_t size = 0;

      char *buf = NULL;

      DWORD flags = FORMAT_MESSAGE_ALLOCATE_BUFFER
                  | FORMAT_MESSAGE_FROM_SYSTEM
                  | FORMAT_MESSAGE_IGNORE_INSERTS;

      size = FormatMessage(flags, NULL, lastError,
                               MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                                   (LPSTR) &buf, 0, NULL);

      if (size > 0)
      {
        //
        // Remove end of line from retrieved message.
        //

        while(size > 0 && (buf[size - 1] == 13 || buf[size - 1] == 10))
        {
          buf[size - 1] = 0;

          size --;
        }

        //
        // Pass message to caller output.
        //

        ret = buf;
      }

      if (buf)
      {
        LocalFree(buf);
      }
    }

    //
    // Linux, MacOS.
    //
    // FIXME: Not implemented.
    //

    #else
    {
      char buf[128] = "Unknown";

      int lastError = errno;

      const char *lastErrorString = NULL;

      #ifndef __APPLE__
      lastErrorString = strerror_r(lastError, buf, sizeof(buf) - 1);

      if (lastErrorString)
      {
        ret = lastErrorString;
      }
      #else //ifndef __APPLE__
      if (strerror_r(lastError, buf, sizeof(buf) - 1) == 0)
      {
        ret = buf;
      }
      #endif //ifndef __APPLE__
    }
    #endif

    return ret;
  }
} /* namespace Tegenaria */
