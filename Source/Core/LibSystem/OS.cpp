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

#include "System.h"

namespace Tegenaria
{
  //
  // Retrieve name of current running OS (e.g. Windows).
  //

  const char *SystemGetOsName()
  {
    //
    // Windows.
    //

    #ifdef WIN32
    {
      return "Windows";
    }
    #endif

    //
    // Linux.
    //

    #ifdef __linux__
    {
      return "Linux";
    }
    #endif

    //
    // MacOS.
    //

    #ifdef __APPLE__
    {
      return "MacOS";
    }
    #endif

    return "Unknown";
  }

  //
  // Get version string of current running OS (e.g. XP 5.1.2600).
  //

  const char *SystemGetOsVersionString()
  {
    static char version[1024] = {0};

    if (version[0] == 0)
    {
      //
      // Windows.
      //

      #ifdef WIN32
      {
        OSVERSIONINFOEX info = {0};

        info.dwOSVersionInfoSize = sizeof(info);

        GetVersionEx(LPOSVERSIONINFO(&info));

        sprintf(version, "[%d.%d.%d], Service Pack [%d.%d]",
                    info.dwMajorVersion, info.dwMinorVersion,
                        info.dwBuildNumber, (int) info.wServicePackMajor,
                            (int) info.wServicePackMinor);
      }
      #endif

      //
      // Linux.
      //

      #ifdef __linux__
      {
        struct utsname utsnamebuf = {0};

        if (uname(&utsnamebuf) == 0)
        {
          strncpy(version, utsnamebuf.version, sizeof(version) - 1);
        }
        else
        {
          Error("ERROR: Cannot retrieve OS version. Error code is %d.\n", errno);
        }
      }
      #endif

      //
      // MacOS.
      //

      #ifdef __APPLE__
      {
        strcpy(version, "Unknown");

        Error("ERROR: SystemGetOsVersionString() not implemented on this platform.\n");
      }
      #endif
    }

    return version;
  }

  //
  // Get name of current running machine.
  //
  // machineName     - buffer, where to store retrieved machine name (OUT).
  // machineNameSize - size of machineName[] buffer in bytes (IN).
  //
  // RETURNS: 0 if OK.
  //

  int SystemGetMachineName(char *machineName, int machineNameSize)
  {
    int exitCode = -1;

    //
    // Check args.
    //

    FAILEX(machineName == NULL, "ERROR: 'machineName' cannot be NULL in SystemGetMachineName().\n");
    FAILEX(machineNameSize < 1, "ERROR: 'machineNameSize' cannot be < 1 in SystemGetMachineName().\n");

    //
    // Windows.
    //

    #ifdef WIN32
    {
      FAIL(GetComputerName(machineName, (PDWORD) &machineNameSize) == FALSE);
    }

    //
    // Linux, MacOS.
    //

    #else
    {
      FAIL(gethostname(machineName, machineNameSize));
    }
    #endif

    //
    // Error handler.
    //

    exitCode = 0;

    fail:

    if (exitCode)
    {
      Error("ERROR: Cannot get machine name."
                "Error code is : %d.\n", GetLastError());
    }

    return exitCode;
  }

  //
  // Generate unique string identyfing current running machine.
  // Output string has format:
  //
  // X:<machine-name>:<random-id>,
  //
  // where:
  // - X is 'W' for Windows, 'L' for Linux, 'M' for MacOS,
  //  'A' for Android, 'i' for iOS, 'U' for unknown.
  //
  // - machineName is name of current machine retreved from
  //   SystemGetMachineName()
  //
  // - random-id is random 4 characters
  //
  // Example: W:Office-12345678:fsd2
  //
  // clientId     - buffer, where to store generated client ID (OUT).
  // clientIdSize - size of clientId[] buffer in bytes (IN).
  //
  // RETURNS: 0 if OK.
  //

  int SystemGenerateClientId(char *clientId, int clientIdSize)
  {
    int exitCode = -1;

    char machineName[65] = {0};

    const char *os = NULL;

    char id[4 + 1] = {0};

    static int randomInited = 0;

    //
    // Check args.
    //

    FAILEX(clientId == NULL, "ERROR: 'clientId' cannot be NULL in SystemGenerateClientId().\n");
    FAILEX(clientIdSize < 1, "ERROR: 'clientIdSize' cannot be < 1 in SystemGenerateClientId().\n");

    //
    // Initialize random generator on first call.
    //

    if (randomInited == 0)
    {
      struct timeval tv;

      unsigned int seed;

      gettimeofday(&tv, NULL);

      seed = (tv.tv_sec * 1000) | (tv.tv_usec / 1000);

      srand(seed);

      randomInited = 1;
    }

    //
    // Get machine name.
    //

    FAIL(SystemGetMachineName(machineName, sizeof(machineName)));

    for (int i = 0; machineName[i]; i++)
    {
      if (machineName[i] == ':')
      {
        machineName[i] = '-';
      }
    }

    //
    // Get current Running OS.
    //

    os = SystemGetOsName();

    //
    // Generate random 4-characters random string.
    //

    for (int i = 0; i < 4; i++)
    {
      int type = rand() % 3;

      switch(type)
      {
        //
        // Big letter.
        //

        case 0:
        {
          id[i] = 'A' + rand() % ('Z' - 'A');

          break;
        }

        //
        // Low letter.
        //

        case 1:
        {
          id[i] = 'a' + rand() % ('z' - 'a');

          break;
        }

        //
        // Digits.
        //

        case 2:
        {
          id[i] = '0' + rand() % ('9' - '0');

          break;
        }
      }
    }

    //
    // Generate output 'X-<machine-name>-id' string in caller buffer.
    //

    snprintf(clientId, clientIdSize - 1, "%c:%s:%s", os[0], machineName, id);

    //
    // Error handler.
    //

    exitCode = 0;

    fail:

    if (exitCode)
    {
      Error("ERROR: Cannot generate client ID.\n");
    }

    return exitCode;
  }

  //
  // Split client ID into OS (operating system), machine name and ID part.
  // See SystemGenerateClientId() for more about client ID.
  //
  // os          - OS part found in given client ID (OUT).
  // machineName - machine name found in given client ID (OUT).
  // id          - random ID part found in given client ID (OUT).
  // clientId    - client ID to parse, generated by SystemGenerateClientId() before (IN).
  //
  // RETURNS: 0 if OK,
  //          -1 if error.
  //

  int SystemParseClientId(string &os, string &machineName,
                              string &id, const char *clientId)
  {
    int exitCode = -1;

    char *input = NULL;

    char *delim1 = NULL;
    char *delim2 = NULL;

    //
    // Check args.
    //

    FAILEX(clientId == NULL, "ERROR: 'clientId' cannot be NULL in SystemParseClientId().\n");

    //
    // Split client ID by ':' separator.
    // We expected 'X:machinename:abcd' string here.
    //

    input = strdup(clientId);

    delim1 = strchr(input, ':');

    FAIL(delim1 == NULL);

    delim2 = strchr(delim1 + 1, ':');

    FAIL(delim2 == NULL);

    *delim1 = 0;
    *delim2 = 0;

    //
    // Parse OS part.
    //

    switch(input[0])
    {
      case 'W': os = "Windows"; break;
      case 'L': os = "Linux"; break;
      case 'M': os = "MacOS"; break;
      case 'A': os = "Android"; break;
      case 'i': os = "iOS"; break;

      default: os = "Unknown";
    }

    //
    // Parse machine name part.
    //

    machineName = delim1 + 1;

    //
    // Parse random id part.
    //

    id = delim2 + 1;

    DEBUG3("Split client ID '%s' into [%s][%s][%s].\n",
               os.c_str(), machineName.c_str(), id.c_str());
    //
    // Clean up.
    //

    exitCode = 0;

    fail:

    if (exitCode)
    {
      Error("ERROR: Cannot parse client ID '%s'.\n", clientId);
    }

    if (input)
    {
      free(input);
    }

    return exitCode;
  }
} /* namespace Tegenaria */
