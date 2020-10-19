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
  // Retrieve number of free bytes available in system.
  //

  int64_t SystemGetFreeMemory()
  {
    DBG_ENTER3("SystemGetFreeMemory");

    int64_t bytesFree = 0.0;

    //
    // Windows.
    //

    #ifdef WIN32
    {
      MEMORYSTATUSEX status = {0};

      status.dwLength = sizeof(status);

      GlobalMemoryStatusEx(&status);

      bytesFree = status.ullAvailVirtual;
    }

    //
    // Linux, MacOS.
    //

    #else
    {
      long pages = sysconf(_SC_PHYS_PAGES);

      long page_size = sysconf(_SC_PAGE_SIZE);

      bytesFree = int64_t(pages) * int64_t(page_size);
    }
    #endif

    DBG_LEAVE3("SystemGetFreeMemory");

    return bytesFree;
  }

  //
  // Retrieve numer of bytes allocated by current running process.
  //

  int64_t SystemGetUsedMemory()
  {
    DBG_ENTER3("SystemGetUsedMemory");

    int64_t bytesUsed = 0;

    //
    // Windows.
    //

    #ifdef WIN32
    {
      HANDLE process = NULL;

      PROCESS_MEMORY_COUNTERS pmc = {0};

      //
      // Open current running process.
      //

      process = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
                                FALSE, GetCurrentProcessId());

      if (process == NULL)
      {
        Error("ERROR: Cannot open current running process.\n"
                  "Error code is %d.\n", GetLastError());

        goto fail;
      }

      //
      // Get memory info for opened process.
      //

      if (GetProcessMemoryInfo(process, &pmc, sizeof(pmc)) == FALSE)
      {
        Error("ERROR: Cannot retrieve memory information for current process.\n"
                  "Error code is %d.\n", GetLastError());

        goto fail;
      }

      bytesUsed = pmc.WorkingSetSize + pmc.PagefileUsage;

      //
      // Clean up.
      //

      fail:

      CloseHandle(process);
    }

    //
    // Linux, MacOS.
    //

    #else
    {
      FILE *f = NULL;

      char fname[1024] = {0};
      char line[1024]  = {0};

      int vmSizeFound = 0;

      char *delim1       = NULL;
      char *delim2       = NULL;
      char *vmSizeString = NULL;
      char *unitString   = NULL;

      //
      // Open '/proc/<pid>/status' file.
      //

      snprintf(fname, sizeof(fname) - 1, "/proc/%d/status", getpid());

      f = fopen(fname, "r");

      FAILEX(f == NULL, "ERROR: Cannot open '%s' file.\n", fname);

      //
      // Read file line by line until "VmSize" found or eof reached.
      //

      while(vmSizeFound == 0 && fgets(line, sizeof(line) - 1, f))
      {
        if (strstr(line, "VmSize") != NULL)
        {
          DEBUG3("SystemGetUsedMemory : VmSize found at line [%s]\n", line);

          vmSizeFound = 1;
        }
      }

      FAILEX(vmSizeFound == 0, "ERROR: VmSize row not found in '%s' file.\n", fname);

      //
      // We expected 'VmSize: <number> <unit>'
      // for example 'VmSize: 1024 kB'
      //

      delim1 = strchr(line, ':');

      FAILEX(delim1 == NULL, "ERROR: ':' delimer not found in line '%s'\n", line);

      delim2 = strchr(delim1, ' ');

      FAILEX(delim2 == NULL, "ERROR: ' ' delimer not found after ':' in line '%s'\n", line);

      //
      // Split line into <number> and <unit>.
      //

      vmSizeString = delim1 + 1;
      unitString   = delim2 + 1;

      //
      // Convert number of free memory from string into integer.
      //

      bytesUsed = int64_t(atoi(vmSizeString));

      //
      // Lowerize unit string.
      //

      StrLowerize(unitString);

      //
      // Apply unit.
      //

      if (strstr(unitString, "kb"))
      {
        DEBUG3("SystemGetUsedMemory : Detected kB unit.\n");

        bytesUsed *= 1024;
      }
      else if (strstr(unitString, "mb"))
      {
        DEBUG3("SystemGetUsedMemory : Detected MB unit.\n");

        bytesUsed *= 1024 * 1024;
      }
      else if (strstr(unitString, "gb"))
      {
        DEBUG3("SystemGetUsedMemory : Detected GB unit.\n");

        bytesUsed *= 1024 * 1024 * 1024;
      }
      else if (strstr(unitString, "b"))
      {
        DEBUG3("SystemGetUsedMemory : Detected byte unit.\n");
      }
      else
      {
        DEBUG1("WARNING: Unknown unit '%s' at line '%s'. Assumed bytes.\n", unitString, line);
      }

      //
      // Clean up.
      //

      fail:

      if (f)
      {
        fclose(f);
      }
    }
    #endif

    DBG_LEAVE3("SystemGetUsedMemory");

    return bytesUsed;
  }
} /* namespace Tegenaria */
