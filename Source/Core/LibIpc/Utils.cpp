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

#include "Utils.h"

namespace Tegenaria
{
  //
  // Work like system read(), but with timeout functionality.
  //
  // fd      - CRT file descriptor where to read from (IN).
  // buf     - destination buffer (OUT).
  // len     - number of bytes to read (IN).
  // timeout - timeout in seconds (IN).
  //
  // RETURNS: Number of bytes readed or
  //          -1 if error.
  //

  int TimeoutReadSelect(int fd, void *buf, int len, int timeout)
  {
    int readed = 0;

    fd_set rfd;

    struct timeval tv;

    FD_ZERO(&rfd);
    FD_SET(fd, &rfd);

    tv.tv_sec  = timeout;
    tv.tv_usec = 0;

    if (select(fd + 1, &rfd, NULL, NULL, &tv) > 0 && FD_ISSET(fd, &rfd))
    {
      readed = read(fd, buf, len);
    }
    else
    {
      Error("ERROR: Timeout while reading from FD #%d.\n", fd);

      readed = -1;
    }

    return readed;
  }

  #ifdef WIN32

  //
  // Work like system ReadFile(), but with timeout functionality.
  //
  // WARNING: Handle MUST be opened in overlapped mode.
  //
  // handle  - native HANDLE to input device e.g. named pipe or socket (IN).
  // buf     - destination buffer (OUT).
  // len     - number of bytes to read (IN).
  // readed  - number of bytes readed (OUT).
  // timeout - timeout in miliseconds (IN).
  //
  // RETURNS: 1 if success and still something to read,
  //          0 otherwise.
  //

  int TimeoutReadFileOv(HANDLE handle, void *buf, int len, int *readed, int timeout)
  {
    OVERLAPPED ov = {0};

    *readed = 0;

    ov.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

    int goOn = ReadFile(handle, buf, len, NULL, &ov);

    if (goOn || GetLastError() == ERROR_IO_PENDING)
    {
      switch(WaitForSingleObject(ov.hEvent, timeout))
      {
        case WAIT_OBJECT_0:
        {
          GetOverlappedResult(handle, &ov, PDWORD(readed), FALSE);

          break;
        }

        case WAIT_TIMEOUT:
        {
          Error("Timeout while reading from handle #%d.\n", handle);

          goOn = 0;
        }
      }
    }

    CloseHandle(ov.hEvent);

    return goOn;
  }

  //
  // Allocating security attributes giving READ+WRITE rights to
  // everybody, and maximum allowed rights to administartors.
  //
  // WARNING: Output struct need to be uninitialized by
  // FreeSecurityAttributes() function!
  //
  // sa - security attributes struct to fill (OUT).
  //
  // RETURNS: 0 if OK.
  //

  int SetUpSecurityAttributesEverybody(SECURITY_ATTRIBUTES *sa)
  {
    DBG_ENTER3("SetUpSecurityAttributesEverybody");

    int exitCode = -1;

    //
    // Needed for set up security on communication pipe.
    //

    PSID everybodySid = NULL;
    PSID adminSid     = NULL;

    PACL acl = NULL;

    PSECURITY_DESCRIPTOR sd = NULL;

    EXPLICIT_ACCESS ea[2] = {{0}};

    SID_IDENTIFIER_AUTHORITY authWorldId = {SECURITY_WORLD_SID_AUTHORITY};
    SID_IDENTIFIER_AUTHORITY authNtId    = {SECURITY_NT_AUTHORITY};

    //
    // Create everybody sid.
    //

    DEBUG3("SetUpSecurityAttributesEverybody : Creating everybody sid...\n");

    FAIL(AllocateAndInitializeSid(&authWorldId, 1, SECURITY_WORLD_RID,
                                      0, 0, 0, 0, 0, 0, 0, &everybodySid) == FALSE);

    //
    // Create sid for BUILTIN\Administrators group.
    //

    DEBUG3("SetUpSecurityAttributesEverybody : Creating adminstrators sid...\n");

    FAIL(AllocateAndInitializeSid(&authNtId, 2, SECURITY_BUILTIN_DOMAIN_RID,
                                      DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0,
                                          &adminSid) == FALSE);

    //
    // Give READ+WRITE access to everybody.
    //

    ZeroMemory(&ea, 2 * sizeof(EXPLICIT_ACCESS));

    ea[0].grfAccessPermissions = GENERIC_WRITE | GENERIC_READ;
    ea[0].grfAccessMode = SET_ACCESS;
    ea[0].grfInheritance= NO_INHERITANCE;
    ea[0].Trustee.TrusteeForm = TRUSTEE_IS_SID;
    ea[0].Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
    ea[0].Trustee.ptstrName  = (LPTSTR) everybodySid;

    //
    // Give maximum allowed access to admin.
    //

    ea[1].grfAccessPermissions = MAXIMUM_ALLOWED;
    ea[1].grfAccessMode = SET_ACCESS;
    ea[1].grfInheritance= NO_INHERITANCE;
    ea[1].Trustee.TrusteeForm = TRUSTEE_IS_SID;
    ea[1].Trustee.TrusteeType = TRUSTEE_IS_GROUP;
    ea[1].Trustee.ptstrName  = (LPTSTR) adminSid;

    //
    // Create a new ACL that contains the new ACEs.
    //

    DEBUG3("SetUpSecurityAttributesEverybody : Creating new ACL...\n");

    FAIL(SetEntriesInAcl(2, ea, NULL, &acl) != ERROR_SUCCESS);

    //
    // Init new, empty security descriptor.
    //

    DEBUG3("SetUpSecurityAttributesEverybody : Allocating new security descriptor...\n");

    sd = (PSECURITY_DESCRIPTOR) LocalAlloc(LPTR, SECURITY_DESCRIPTOR_MIN_LENGTH);

    FAIL(sd == NULL);

    FAIL(InitializeSecurityDescriptor(sd, SECURITY_DESCRIPTOR_REVISION) == FALSE);

    //
    // Add the ACL to the security descriptor.
    //

    DEBUG3("SetUpSecurityAttributesEverybody : Setting up DACL...\n");

    FAIL(SetSecurityDescriptorDacl(sd, TRUE, acl, FALSE) == FALSE);

    //
    // Fill output security attributes struct.
    //

    sa -> nLength              = sizeof (SECURITY_ATTRIBUTES);
    sa -> lpSecurityDescriptor = sd;
    sa -> bInheritHandle       = TRUE;

    //
    // Error handler.
    //

    exitCode = 0;

    fail:

    if (exitCode)
    {
      int err = GetLastError();

      Error("ERROR: Cannot set up security attributes.\n"
                "Error code is : %d.\n", err);

      if (err != 0)
      {
        exitCode = err;
      }
    }

    DBG_LEAVE3("SetUpSecurityAttributesEverybody");

    return exitCode;
  }

  //
  // Free Security Attributes struct allocated by
  // SetUpSecurityAttributes() function.
  //

  void FreeSecurityAttributes(SECURITY_ATTRIBUTES *sa)
  {
    DBG_ENTER3("FreeSecurityAttributes");

    if (sa && sa -> lpSecurityDescriptor)
    {
      BOOL daclPresent = FALSE;
      BOOL daclDefault = FALSE;

      ACL *pACL = NULL;

      SECURITY_DESCRIPTOR *pSD = (SECURITY_DESCRIPTOR *) sa -> lpSecurityDescriptor;

      //
      // Get ACL if exists.
      //

      DEBUG3("FreeSecurityAttributes : Retrieving ACL...\n");

      GetSecurityDescriptorDacl(pSD, &daclPresent, &pACL, &daclDefault);

      //
      // Free security descriptor field.
      //

      DEBUG3("FreeSecurityAttributes : Freeing SD...\n");

      LocalFree(pSD);

      //
      // Free ACL if exist.
      //

      DEBUG3("FreeSecurityAttributes : Freeing ACL...\n");

      if (daclPresent)
      {
        LocalFree(pACL);
      }

    }

    DBG_LEAVE3("FreeSecurityAttributes");
  }

  #endif /* WIN32 */

} /* namespace Tegenaria */
