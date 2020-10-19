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
// Purpose: Transacted I/O API.
//
// On Vista+ native TxF used.
// On WinXP and older - custom failback mechanism.
//

#ifdef WIN32

#include <Tegenaria/Debug.h>
#include "File.h"
#include "Internal.h"
#include <ctime>
#include <io.h>

namespace Tegenaria
{
  int TxF_Availible = -1;

  CreateFileTransactedProto CreateFileTransacted = NULL;
  CreateTransactionProto CreateTransaction       = NULL;
  CommitTransactionProto CommitTransaction       = NULL;

  //
  // Check is TxF functions availible and initiate it if yes.
  //
  // RETURNS: 1 if TxF availible.
  //

  int _FileCheckForTxF()
  {
    #pragma qcbuild_set_private(1)

    //
    // If TFile isn't initiated yet.
    //

    if (TxF_Availible == -1)
    {
      TxF_Availible = 0;

      #ifdef WIN32

      //
      // Get current OS version.
      //

      DWORD ver      = GetVersion();
      DWORD majorVer = (DWORD)(LOBYTE(LOWORD(ver)));
      DWORD minorVer = (DWORD)(HIBYTE(LOWORD(ver)));

      //
      // If at least Vista try import TxF functions.
      //

      if (majorVer >= 6)
      {
        CreateFileTransacted = (CreateFileTransactedProto) GetProcAddress(LoadLibrary("Kernel32.dll"), "CreateFileTransactedA");
        CreateTransaction    = (CreateTransactionProto) GetProcAddress(LoadLibrary("KtmW32.dll"), "CreateTransaction");
        CommitTransaction    = (CommitTransactionProto) GetProcAddress(LoadLibrary("KtmW32.dll"), "CommitTransaction");

        FAILEX(CreateFileTransacted == NULL, "Cannot import CreateFileTransacted().\n");
        FAILEX(CreateTransaction == NULL, "Cannot import CreateTransaction().\n");
        FAILEX(CommitTransaction == NULL, "Cannot import CommitTransaction().\n");

        DBG_MSG("TxF availible.\n");

        TxF_Availible = 1;

      #endif /* WIN32 */
      }
    }

    fail:

    return TxF_Availible;
  }

  //
  // Open file in transacted mode.
  //
  // fname - file path to open (IN).
  // mode  - C style open mode (see fopen) (IN).
  //
  // RETURNS: File handle or NULL if error.
  //

  TFILE *tfopen(const char *fname, const char *mode)
  {
    DBG_ENTER("tfopen");

    int exitCode = -1;

    int tmpCreated = 0;

    char tmpName[MAX_PATH] = "";

    TFILE *f = NULL;

    //
    // TxF functions availible.
    //

    if (_FileCheckForTxF())
    {
      DWORD desiredAccess     = 0;
      DWORD createDisposition = OPEN_EXISTING;

      //
      // Allocate TFILE buffer.
      //

      f = (TFILE *) calloc(sizeof(TFILE), 1);

      FAIL(f == NULL);

      //
      // Decode C style mode.
      //

      for (int i = 0; mode[i]; i++)
      {
        switch(mode[i])
        {
          case 'r': {desiredAccess |= GENERIC_READ; break;}
          case 'w': {desiredAccess |= GENERIC_WRITE; break;}
          case '+': {createDisposition = CREATE_ALWAYS; break;}
        }
      }

      //
      // Create transaction.
      //

      f -> transaction_ = CreateTransaction(NULL, NULL, 0, 0, 0, 0, NULL);

      FAILEX(f -> transaction_ == NULL, "Cannot create transaction");

      //
      // Open file.
      //

      f -> hFile_ = CreateFileTransacted(fname, desiredAccess, 0, NULL,
                                             createDisposition, FILE_ATTRIBUTE_NORMAL,
                                                 NULL, f -> transaction_, NULL, NULL);

      FAILEX(f -> hFile_ == INVALID_HANDLE_VALUE, "Cannot open file.\n");

      //
      // Convert HANDLE to C FILE*.
      //

      #if defined(WIN64)
      f -> fd_ = _open_osfhandle((intptr_t) f -> hFile_, 0);
      #else
      f -> fd_ = _open_osfhandle((long) f -> hFile_, 0);
      #endif

      FAIL(f -> fd_ < 0);

      f -> ftemp_ = fdopen(f -> fd_, mode);

      FAIL(f -> ftemp_ == NULL);
    }

    //
    // TxF unavailible. Use own mechanism.
    //

    else
    {
      HANDLE lockHandle = INVALID_HANDLE_VALUE;

      int cookie    = 0;
      int timestamp = 0;

      //
      // Generate cookie.
      //

      cookie    = rand();
      timestamp = time(0);

      //
      // Copy original file to temp.
      //

      snprintf(tmpName, MAX_PATH, "%s.tmp-%d%d", fname, cookie, timestamp);

      //DBG_MSG("Copying [%s] to [%s]...\n", fname, tmpName);

      tmpCreated = CopyFile(fname, tmpName, FALSE);

      FAIL(tmpCreated == 0);

      //
      // Allocate TFILE buffer.
      //

      f = (TFILE *) calloc(sizeof(TFILE), 1);

      FAIL(f == NULL);

      //
      // Lock file.
      //

      f -> lockHandle_ = CreateFile(fname, GENERIC_READ | GENERIC_WRITE, 0,
                                        NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
                                            NULL);

      //
      // Open temp file for real work.
      //

      //DBG_MSG("Opening [%s]...\n", tmpName);

      f -> ftemp_ = fopen(tmpName, mode);

      FAIL(f -> ftemp_ == NULL);

      //
      // Save original name and cookie number for close.
      //

      strncpy(f -> fname_, fname, MAX_PATH);

      f -> cookie_    = cookie;
      f -> timestamp_ = timestamp;
    }

    //
    // Error handler.
    //

    exitCode = 0;

    fail:

    if (exitCode)
    {
      DBG_MSG("ERROR: Cannot open [%s] file.\n", fname);

      if (f)
      {
        tfclose(f);
      }

      if (tmpCreated)
      {
        DeleteFile(tmpName);
      }
    }

    DBG_LEAVE("tfopen");

    return f;
  }

  //
  // Close transacted file.
  //
  // f - handle retrieved from tfopen function (IN).
  //

  void tfclose(TFILE *f)
  {
    DBG_ENTER("tfclose");

    if (f)
    {
      //
      // TxF functions availible.
      //

      if (_FileCheckForTxF())
      {
        if (f -> ftemp_)
        {
          fclose(f -> ftemp_);
        }

        CommitTransaction(f -> transaction_);

        CloseHandle(f -> hFile_);
        CloseHandle(f -> transaction_);
      }

      //
      // TxF unavailible. Use own mechanism.
      //

      else
      {
        char altName[MAX_PATH];
        char tmpName[MAX_PATH];

        //
        // Close temp file.
        //

        if (f -> ftemp_)
        {
          fclose(f -> ftemp_);
        }

        //
        // Remove lock.
        //

        CloseHandle(f -> lockHandle_);

        //
        // Rename original -> alt.
        //

        snprintf(altName, MAX_PATH, "%s.alt-%d%d",
                     f -> fname_, f -> cookie_, f -> timestamp_);

        //DBG_MSG("Renaming [%s] to [%s]...\n", f -> fname_, altName);

        FAIL(MoveFile(f -> fname_, altName) == FALSE);

        //
        // Rename tmp -> original.
        //

        snprintf(tmpName, MAX_PATH, "%s.tmp-%d%d",
                     f -> fname_, f -> cookie_, f -> timestamp_);


        FAIL(MoveFile(tmpName, f -> fname_) == FALSE);

        //
        // Delete alt.
        //

        DeleteFile(altName);
      }
    }

    fail:

    if (f)
    {
      free(f);
    }

    DBG_LEAVE("tfclose");
  }

  //
  // Recover corrupted I/O operations in given directory.
  //
  // directory - path, where scan for corrupted operations (IN).
  //
  // RETURNS: 0 if OK.
  //

  int FileRecoverFailedIO(const char *directory)
  {
    DBG_ENTER("FileRecoverFailedIO");

    int exitCode = -1;

    //
    // TxF functions availible.
    //

    if (_FileCheckForTxF())
    {
      //
      // Recover handled by TxF. No code needed.
      //
    }

    //
    // TxF unavailible. Use own mechanism.
    //

    else
    {
      vector<string> altFiles;
      vector<string> tmpFiles;

      string fname;
      string altName;
      string tmpName;

      vector<string>::iterator it;
      vector<string>::iterator jt;

      //
      // Iterate over tmp files.
      //

      ListFiles(directory, "*.tmp-*", tmpFiles, false);

      for (it = tmpFiles.begin(); it != tmpFiles.end(); it ++)
      {
        //
        // Restore original and alt name from tmp.
        //

        tmpName = *it;
        altName = tmpName;

        int lastDot = tmpName.find_last_of(".");

        memcpy(&altName[lastDot] + 1, "alt", 3);

        fname = tmpName.substr(0, lastDot);

        if (FileExists(fname.c_str()))
        {
          //
          // Exists : original, tmp
          // Remove all tmps related to file.
          //

          string mask = fname + ".tmp-";

          for (jt = tmpFiles.begin(); jt != tmpFiles.end(); jt ++)
          {
            if (jt -> find(mask) != -1)
            {
              DBG_MSG("Cleaning up [%s]...\n", jt -> c_str());

              DeleteFile(jt -> c_str());
            }
          }
        }
        else
        {
          //
          // Missing : original.
          // Exists  : Tmp, Alt.
          // Recover original.
          //

          if (FileExists(altName.c_str()))
          {
            DBG_MSG("Recovering [%s] from [%s]...\n",
                        fname.c_str(), tmpName.c_str());

            if (MoveFile(tmpName.c_str(), fname.c_str()))
            {
              DeleteFile(altName.c_str());
            }
          }

          //
          // Missing : original, alt.
          // Exists  : tmp

          else
          {
            DBG_MSG("Cleaning up [%s]...\n", tmpName.c_str());

            DeleteFile(tmpName.c_str());
          }
        }
      }

      //
      // Iterate over alt files.
      //

      ListFiles(directory, "*.alt-*", altFiles, false);

      for (it = altFiles.begin(); it != altFiles.end(); it ++)
      {
        //
        // Restore original and tmp name from alt.
        //

        altName = *it;
        tmpName = altName;

        int lastDot = altName.find_last_of(".");

        memcpy(&tmpName[lastDot] + 1, "tmp", 3);

        fname = altName.substr(0, lastDot);

        if (FileExists(fname.c_str()))
        {
          //
          // Exists : original, alt
          // Remove all alt related to file.
          //

          string mask = fname + ".alt-";

          for (jt = altFiles.begin(); jt != altFiles.end(); jt ++)
          {
            if (jt -> find(mask) != -1)
            {
              DBG_MSG("Cleaning up [%s]...\n", jt -> c_str());

              DeleteFile(jt -> c_str());
            }
          }
        }
        else
        {
          //
          // Missing : original.
          // Exists  : Tmp, Alt.
          // Recover original.
          //

          if (FileExists(altName.c_str()))
          {
            DBG_MSG("Recovering [%s] from [%s]...\n",
                        fname.c_str(), tmpName.c_str());

            if (MoveFile(tmpName.c_str(), fname.c_str()))
            {
              DeleteFile(altName.c_str());
            }
          }

          //
          // Missing : original, tmp.
          // Exists  : alt

          else
          {
            DBG_MSG("Cleaning up [%s]...\n", altName.c_str());

            DeleteFile(altName.c_str());
          }
        }
      }
    }

    exitCode = 0;

    fail:

    DBG_LEAVE("FileRecoverFailedIO");

    return exitCode;
  }
} /* namespace Tegenaria */

#endif /* WIN32 */
