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
  // Retrieve number of CPU cores installed on system.
  //

  int SystemGetCpuCores()
  {
    DBG_ENTER3("SystemGetCpuNumber");

    static int cpuCount = -1;

    //
    // Windows.
    //

    #ifdef WIN32
    {
      if (cpuCount == -1)
      {
        SYSTEM_INFO si = {0};

        GetSystemInfo(&si);

        cpuCount = si.dwNumberOfProcessors;
      }
    }

    //
    // Linux, MacOS.
    //

    #else
    {
      cpuCount = sysconf(_SC_NPROCESSORS_ONLN);
    }
    #endif

    DBG_LEAVE3("SystemGetCpuNumber");

    return cpuCount;
  }

  //
  // Run native cpuid instruction.
  //
  // func - CPU id function, see INTEL or AMD manual for more (IN).
  // eax  - copy of returned eax register (OUT).
  // ebx  - copy of returned ebx register (OUT).
  // ecx  - copy of returned ecx register (OUT).
  // edx  - copy of returned edx register (OUT).
  //

  void SystemCpuId(unsigned int func, unsigned int *eax,
                       unsigned int *ebx, unsigned int *ecx, unsigned int *edx)
  {
    DBG_ENTER3("SystemCpuId");

    static int firstCall = 1;

    static unsigned int cachedEax = 0;
    static unsigned int cachedEbx = 0;
    static unsigned int cachedEcx = 0;
    static unsigned int cachedEdx = 0;

    //
    // Check args.
    //

    FAILEX(eax == NULL, "ERROR: eax paramater cannot be NULL in SystemCpuId().\n");
    FAILEX(ebx == NULL, "ERROR: ebx paramater cannot be NULL in SystemCpuId().\n");
    FAILEX(ecx == NULL, "ERROR: ecx paramater cannot be NULL in SystemCpuId().\n");
    FAILEX(edx == NULL, "ERROR: edx paramater cannot be NULL in SystemCpuId().\n");

    //
    // If already called use cached values.
    //

    if (firstCall == 0)
    {
      *eax = cachedEax;
      *ebx = cachedEbx;
      *ecx = cachedEcx;
      *edx = cachedEdx;
    }

    //
    // First call. Run cpuid instruction.
    //

    else
    {
      //
      // Sun or unknown CPU.
      //

      #if defined(__sun) || (!defined(__i386) && !defined(__x86_64))
      {
        *eax = 0;
        *ebx = 0;
        *ecx = 0;
        *edx = 0;

        Error("ERROR: SystemCpuId() not implemented on this platform.\n");
      }

      //
      // i386 and AMD64.
      //

      #else
      {
        *eax = func;

        //
        // AMD64.
        //

        #if defined(__x86_64)
        {
          __asm volatile
          (
            "cpuid;"
            "mov %%ebx, %%esi;"
            :"+a" (*eax), "=S" (*ebx), "=c" (*ecx), "=d" (*edx)
            :
            :"ebx"
          );
        }
        #endif

        //
        // i386.
        //

        #if defined(__i386)
        {
          __asm volatile
          (
            "mov %%ebx, %%edi;"
            "cpuid;"
            "mov %%ebx, %%esi;"
            "mov %%edi, %%ebx;"
            :"+a" (*eax), "=S" (*ebx), "=c" (*ecx), "=d" (*edx)
            :
            :"edi"
          );
        }
        #endif
      }
      #endif

      //
      // Cache result to avoid call cpuid at next calls.
      //

      cachedEax = *eax;
      cachedEbx = *ebx;
      cachedEcx = *ecx;
      cachedEdx = *edx;

      firstCall = 0;
    }

    //
    // Error handler.
    //

    fail:

    return;
  }

  //
  // Check does current running CPU has SSE3 extension.
  //

  int SystemCpuHasSSE3()
  {
    unsigned int eax;
    unsigned int ebx;
    unsigned int ecx;
    unsigned int edx;

    SystemCpuId(1, &eax, &ebx, &ecx, &edx);

    return (ecx & SYSTEM_SSE3_AVAILABLE);
  }

  //
  // Check does current running CPU has SSE2 extension.
  //

  int SystemCpuHasSSE2()
  {
    unsigned int eax;
    unsigned int ebx;
    unsigned int ecx;
    unsigned int edx;

    SystemCpuId(1, &eax, &ebx, &ecx, &edx);

    return (edx & SYSTEM_SSE2_AVAILABLE);
  }

  //
  // Check does current running CPU has MMX extension.
  //

  int SystemCpuHasMMX()
  {
    unsigned int eax;
    unsigned int ebx;
    unsigned int ecx;
    unsigned int edx;

    SystemCpuId(1, &eax, &ebx, &ecx, &edx);

    return (edx & SYSTEM_MMX_AVAILABLE);
  }
} /* namespace Tegenaria */
