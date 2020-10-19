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

#ifndef Tegenaria_Core_CachedSftpClient_H
#define Tegenaria_Core_CachedSftpClient_H

#ifdef WIN32

#include <list>
#include <map>
#include <Tegenaria/Semaphore.h>
#include "SftpClient.h"

namespace Tegenaria
{
  using std::list;
  using std::map;

  //
  // Struct to store single cache element.
  //

  struct CacheElement
  {
    string path_;

    int64_t handle_;

    int refCount_;
    int closeTime_;
    int isDir_;
    int readdirCalled_;

    vector<WIN32_FIND_DATAW> findData_;

    int findTime_;
  };

  struct CacheStatvfsElement
  {
    Statvfs_t statvfs_;

    int timestamp_;
  };

  //
  // Wrapper class to wrap unached SftpClient into cached one.
  //

  class CachedSftpClient : public SftpClient
  {
    private:

    list<CacheElement> cache_;

    static int cleanerLoop(void *data);

    ThreadHandle_t *cleanerThread_;

    int cleanerLoopEnabled_;

    Semaphore cleanerLoopReset_;

    map<string, CacheStatvfsElement> cacheStatvfs_;

    Mutex mutex_;

    //
    // Private functions.
    //

    list<CacheElement>::iterator findElement(int64_t handle);
    list<CacheElement>::iterator findElementForUpdate(int64_t handle);

    list<CacheElement>::iterator findElement(const char *path);
    list<CacheElement>::iterator findElementForUpdate(const char *path);

    int realclose(int64_t handle);

    void removeUnusedHandle(const char *path);
    void removeUnusedHandle(int64_t handle);

    //
    // Exported functions.
    //

    public:

    //
    // Init.
    //

    CachedSftpClient(int fdin, int fdout,
                         int timeout = 30,
                             int fdType = SFTP_CLIENT_FD);

    ~CachedSftpClient();

    //
    // Thread sychronization.
    //

    void lock();
    void unlock();

    //
    // Wrappers for raw SftpClienet functions.
    //

    int64_t opendir(const char *name);

    int close(int64_t handle);

    int readdir(vector<WIN32_FIND_DATAW> &data, int64_t handle);

    int mkdir(const char *path);
    int rmdir(const char *path);
    int rename(const char *path1, const char *path2);

    int statvfs(Statvfs_t *stvfs, const char *path);

    int64_t createfile(const char *path, uint32_t access,
                           uint32_t shared, uint32_t create, uint32_t flags,
                               int *isDir);

    //
    // Widechar wrappers for DOKAN.
    //

    int64_t opendir(const wchar_t *path);

    int mkdir(const wchar_t *path);
    int rmdir(const wchar_t *path);
    int rename(const wchar_t *path1, const wchar_t *path2);

    int64_t createfile(const wchar_t *path, uint32_t access,
                           uint32_t shared, uint32_t create, uint32_t flags,
                               int *isDir);
  };

} /* namespace Tegenaria */

#endif /* WIN32 */
#endif /* Tegenaria_Core_CachedSftpClient_H */
