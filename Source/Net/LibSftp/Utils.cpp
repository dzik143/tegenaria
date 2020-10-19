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

#include <Tegenaria/Mutex.h>
#include "Utils.h"
#include "Sftp.h"

namespace Tegenaria
{
  //
  // Generate thread-safe, unique number.
  // Used internally to generate handles and session IDs.
  //
  // RETURNS: Number unique inside process.
  //

  int GenerateUniqueId()
  {
    static Mutex mutex("UniqueId");

    static int count = 1;

    int id = -1;

    mutex.lock();

    id = count;
    count ++;

    mutex.unlock();

    return id;
  }

  #ifdef WIN32

  //
  // Translate {access, create, flags} masks passed to CreateFile()
  // into SFTP access mask.
  //
  // access - dwDesiredAccess mask passed used with CreateFile (IN).
  // share  - dwShareMode mask passed used with CreateFile (IN).
  // create - dwCreationDisposition parameter used with CreateFile (IN).
  // flags  - dwFlagsAndAttributes parameter used with CreateFile (IN).
  //
  // RETURNS: SFTP access mask.
  //

  int GetSftpFlagFromWinapi(DWORD access, DWORD shared, DWORD create, DWORD flags)
  {
    int mode = 0;

    if (access & FILE_READ_DATA || access & GENERIC_READ)
    {
      mode |= SSH2_FXF_READ;
    }

    if (access & FILE_WRITE_DATA || access & GENERIC_WRITE)
    {
     mode |= SSH2_FXF_WRITE;
    }

    if (access & FILE_APPEND_DATA)
    {
      mode |= SSH2_FXF_APPEND;
    }

    //
    // Process create disposition arg.
    //

    switch(create)
    {
      //
      // Create always. If exists open, if not exist open for overwrite.
      //

      case CREATE_ALWAYS:
      {
        mode |= SSH2_FXF_TRUNC;
        mode |= SSH2_FXF_CREAT;

        break;
      }

      //
      // Create new. If exists fail, if not create new.
      //

      case CREATE_NEW:
      {
        mode |= SSH2_FXF_CREAT;

        break;
      }

      //
      // Open always. If exists open, if not craete new file.
      //

      case OPEN_ALWAYS:
      {
        mode |= SSH2_FXF_CREAT;

        break;
      }

      //
      // Open existing. If exists open, if not fail.
      //

      case OPEN_EXISTING:
      {
        break;
      }

      //
      // Truncate existing. If exists open and zero, if not fail.
      //

      case TRUNCATE_EXISTING:
      {
        mode |= SSH2_FXF_TRUNC;
      }
    }


    DEBUG3("Translated WINAPI mode {0x%x, 0x%x, 0x%x, 0x%x} into SFTP 0x%x.\n",
               access, shared, create, flags, mode);

    return mode;
  }

  #endif /* WIN32 */

  //
  // Translate SSH2_FX_XXX code into human readable string.
  //
  // code - one of SSH2_FX_XXX code defined in Sftp.h (IN).
  //
  // RETURNS: Error string related to given code,
  //          or "Unknown" if code not recognized.
  //

  const char *TranslateSftpStatus(int code)
  {
    const char *names[] =
    {
      "SSH2_FX_OK",                // 0
      "SH2_FX_EOF",                // 1
      "SSH2_FX_NO_SUCH_FILE",      // 2
      "SSH2_FX_PERMISSION_DENIED", // 3
      "SSH2_FX_FAILURE",           // 4
      "SSH2_FX_BAD_MESSAGE",       // 5
      "SSH2_FX_NO_CONNECTION",     // 6
      "SSH2_FX_CONNECTION_LOST",   // 7
      "SSH2_FX_OP_UNSUPPORTED"     // 8
    };

    if (code >= 0 && code <= SSH2_FX_MAX)
    {
      return names[code];
    }
    else
    {
      return "Unknown";
    }
  }

  //
  // Get current time in ms.
  //

  double GetTimeMs()
  {
    struct timeval tv;

    gettimeofday(&tv, NULL);

    return tv.tv_sec * 1000.0 + tv.tv_usec / 1000.0;
  }
} /* namespace Tegenaria */
