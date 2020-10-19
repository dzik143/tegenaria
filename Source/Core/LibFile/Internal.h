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

#ifndef Tegenaria_Core_File_Internal_H
#define Tegenaria_Core_File_Internal_H

#ifdef WIN32
#include <windows.h>
#endif

#include <cstdio>

namespace Tegenaria
{
  //
  // FIXME! Set real maximum path's length
  //        on OSes other than Windows.
  //

  #ifndef MAX_PATH
  #define MAX_PATH 260
  #endif

  #ifdef WIN32

  //
  // TxF prototypes for Vista/Win7.
  //

  typedef HANDLE WINAPI (*CreateFileTransactedProto)(LPCTSTR, DWORD, DWORD,
                                                         LPSECURITY_ATTRIBUTES,
                                                             DWORD, DWORD,
                                                                 HANDLE, HANDLE,
                                                                     PUSHORT, PVOID);

  typedef HANDLE WINAPI (*CreateTransactionProto)(LPSECURITY_ATTRIBUTES, LPGUID,
                                                      DWORD, DWORD, DWORD, DWORD, LPWSTR);

  typedef BOOL WINAPI (*CommitTransactionProto)(HANDLE);

  //
  // Structures.
  //

  struct TFILE
  {
    HANDLE lockHandle_;
    HANDLE transaction_;
    HANDLE hFile_;

    int cookie_;
    int timestamp_;

    FILE *ftemp_;

    int fd_;

    char fname_[MAX_PATH];
  };

  #endif /* WIN32 */

} /* namespace Tegenaria */

#endif /* Tegenaria_Core_File_Internal_H */
