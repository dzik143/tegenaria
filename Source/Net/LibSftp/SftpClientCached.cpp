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

#pragma qcbuild_set_private(1)

#include <map>
#include "SftpClientCached.h"

namespace Tegenaria
{
  #ifdef WIN32

  using std::map;

  //
  // Cleaner loop monitoring all cached handles and performs delayed
  // close.
  //

  int CachedSftpClient::cleanerLoop(void *data)
  {
    CachedSftpClient *this_ = (CachedSftpClient *) data;

    //
    // Fall into main cleaner loop.
    //

    while(this_ -> cleanerLoopEnabled_)
    {
      vector<int64_t> handlesToClose;

      list<CacheElement>::iterator it;

      int currentTime = time(0);

      //
      // Activate cleaner aevery 5s or when cleanerLoopReset_
      // semaphore signaled.
      //

      this_ -> cleanerLoopReset_.wait(5000);

      //
      // Prepare list of handles needed to be close on server.
      //

      this_ -> lock();

      for (it = this_ -> cache_.begin(); it != this_ -> cache_.end();)
      {
        if (it -> refCount_ == 0
                && (currentTime - it -> closeTime_) > 5)
        {
          handlesToClose.push_back(it -> handle_);

          it = this_ -> cache_.erase(it);
        }
        else
        {
          it++;
        }
      }

      this_ -> unlock();

      //
      // Close handles on SFTP server.
      //

      if (handlesToClose.size() > 0)
      {
        this_ -> multiclose(handlesToClose);
      }
    }
  }

  //
  // Constructor.
  //

  CachedSftpClient::CachedSftpClient(int fdin, int fdout, int timeout, int fdType)
  : SftpClient(fdin, fdout, timeout, fdType)
  {
    DBG_ENTER3("CachedSftpClient::CachedSftpClient");

    cleanerLoopEnabled_ = 1;

    cleanerThread_ = ThreadCreate(cleanerLoop, this);

    DBG_LEAVE3("CachedSftpClient::CachedSftpClient");
  }

  //
  // Destructor.
  //

  CachedSftpClient::~CachedSftpClient()
  {
    DBG_ENTER3("CachedSftpClient::~CachedSftpClient");

    cleanerLoopEnabled_ = 0;

    cleanerLoopReset_.signal();

    ThreadWait(cleanerThread_);
    ThreadClose(cleanerThread_);

    cleanerThread_ = NULL;

    DBG_LEAVE3("CachedSftpClient::~CachedSftpClient");
  }

  //
  // Find given SFTP handle in cache.
  //
  // handle - SFTP handle retrive from open() or opendir() function before (IN).
  //
  // RETURNS: Iterator to CacheElement struct,
  //          or CacheElement::end() if element not found.
  //

  list<CacheElement>::iterator CachedSftpClient::findElement(int64_t handle)
  {
    list<CacheElement>::iterator it;

    lock();

    for (it = cache_.begin(); it != cache_.end(); it++)
    {
      if (it -> handle_ == handle)
      {
        break;
      }
    }

    unlock();

    return it;
  }

  //
  // Find given SFTP handle in cache and lock Cache.
  //
  // WARNING: Caller MUSTS unlock() cache by calling unlock() method
  //          when found element not needed longer.
  //
  // TIP: If element not found in cache object is not locked.
  //
  // Parameters:
  //
  // handle - SFTP handle retrive from open() or opendir() function before (IN).
  //
  // RETURNS: Iterator to CacheElement struct,
  //          or CacheElement::end() if element not found.
  //

  list<CacheElement>::iterator CachedSftpClient::findElementForUpdate(int64_t handle)
  {
    list<CacheElement>::iterator it;

    lock();

    for (it = cache_.begin(); it != cache_.end(); it++)
    {
      if (it -> handle_ == handle)
      {
        break;
      }
    }

    if (it == cache_.end())
    {
      unlock();
    }

    return it;
  }

  //
  // Find given remote path in cache.
  //
  // handle - full, remote path on server side (IN).
  //
  // RETURNS: Iterator to CacheElement struct,
  //          or CacheElement::end() if element not found.
  //

  list<CacheElement>::iterator CachedSftpClient::findElement(const char *path)
  {
    list<CacheElement>::iterator it;

    lock();

    for (it = cache_.begin(); it != cache_.end(); it++)
    {
      if (it -> path_ == path)
      {
        break;
      }
    }

    unlock();

    return it;
  }

  //
  // Find given remote path in cache and lock cache.
  //
  // TIP: If element not found in cache object is not locked.
  //
  // Parameters:
  //
  // handle - full, remote path on server side (IN).
  //
  // RETURNS: Iterator to CacheElement struct,
  //          or CacheElement::end() if element not found.
  //

  list<CacheElement>::iterator CachedSftpClient::findElementForUpdate(const char *path)
  {
    list<CacheElement>::iterator it;

    lock();

    for (it = cache_.begin(); it != cache_.end(); it++)
    {
      if (it -> path_ == path)
      {
        break;
      }
    }

    if (it == cache_.end())
    {
      unlock();
    }

    return it;
  }

  //
  // Remove and close cached handle, but ONLY if it's unused.
  //

  void CachedSftpClient::removeUnusedHandle(const char *path)
  {
    list<CacheElement>::iterator it = findElementForUpdate(path);

    if (it != cache_.end())
    {
      int64_t handleToClose = -1;

      if (it -> refCount_ == 0)
      {
        handleToClose = it -> handle_;

        cache_.erase(it);
      }

      unlock();

      if (handleToClose != -1)
      {
        realclose(handleToClose);
      }
    }
  }

  //
  // Remove and close cached handle, but ONLY if it's unused.
  //

  void CachedSftpClient::removeUnusedHandle(int64_t handle)
  {
    list<CacheElement>::iterator it = findElementForUpdate(handle);

    if (it != cache_.end())
    {
      int64_t handleToClose = -1;

      if (it -> refCount_ == 0)
      {
        handleToClose = it -> handle_;

        cache_.erase(it);
      }

      unlock();

      if (handleToClose != -1)
      {
        realclose(handleToClose);
      }
    }
  }

  //
  // Lock whole object.
  //
  // WARNING: Every calls to lock() MUSTS be folowed by one unlock() method.
  //

  void CachedSftpClient::lock()
  {
    mutex_.lock();
  }

  //
  // Unlock object, locked by lock() function before.
  //

  void CachedSftpClient::unlock()
  {
    mutex_.unlock();
  }

  //
  // ----------------------------------------------------------------------------
  //
  //                 Wrappers for unchached SftpClient methods
  //
  // ----------------------------------------------------------------------------
  //

  //
  // Wrapper for SftpClient::readdir().
  //

  int CachedSftpClient::readdir(vector<WIN32_FIND_DATAW> &data, int64_t handle)
  {
    int ret = -1;

    list<CacheElement>::iterator it = findElementForUpdate(handle);

    if (it != cache_.end())
    {
      if (it -> readdirCalled_ == 1)
      {
        if (time(0) - it -> findTime_ < 1)
        {
          data = it -> findData_;

          ret = 0;

          DEBUG1("SFTP-CACHE: Reused dir content [%I64d].\n", handle);
        }
        else
        {
          resetdir(handle);

          DEBUG1("SFTP-CACHE: Reset dir [%I64d].\n", handle);
        }
      }

      it -> readdirCalled_ = 1;

      DEBUG1("SFTP-CACHE: Called readdir on [%I64d].\n", handle);

      unlock();
    }

    if (ret == -1)
    {
      ret = SftpClient::readdir(data, handle);

      if (ret == 0)
      {
        it = findElementForUpdate(handle);

        if (it != cache_.end())
        {
          it -> findData_ = data;
          it -> findTime_ = time(0);

          DEBUG1("SFTP-CACHE: Cached dir content [%I64d].\n", handle);
        }

        unlock();
      }
    }

    return ret;
  }

  //
  // Wrapper for SftpClient::opendir().
  //

  int64_t CachedSftpClient::opendir(const char *path)
  {
    DBG_ENTER3("CachedSftpClient::opendir");

    list<CacheElement>::iterator it;

    int64_t handle = -1;

    //
    // Find element in cache.
    //

    it = findElementForUpdate(path);

    //
    // Element already cached.
    // Try use it.
    //

    if (it != cache_.end())
    {
      if (it -> isDir_)
      {
        handle = it -> handle_;

        it -> refCount_ ++;

        DEBUG1("SFTP-CACHE: Reused dir [%I64d][%s]"
                   ", new ref count is [%d]", handle, path, it -> refCount_);
      }

      unlock();
    }

    //
    // Element not cached.
    // Open dir on server and cache retrieved handle.
    //

    if (handle == -1)
    {
      handle = SftpClient::opendir(path);

      if (handle != -1)
      {
        CacheElement e;

        e.path_          = path;
        e.handle_        = handle;
        e.refCount_      = 1;
        e.isDir_         = 1;
        e.readdirCalled_ = 0;

        mutex_.lock();
        cache_.push_back(e);
        mutex_.unlock();

        DEBUG1("SFTP-CACHE: Added dir [%s][%I64d].\n", path, handle);
      }
    }

    DBG_LEAVE3("CachedSftpClient::opendir");

    return handle;
  }

  //
  // Wrapper for SftpClient::close().
  //

  int CachedSftpClient::close(int64_t handle)
  {
    list<CacheElement>::iterator it;

    int realCloseNeeded = 1;

    //
    // Try find element in cache.
    //

    it = findElementForUpdate(handle);

    if (it != cache_.end())
    {
      //
      // If cached element is dir decrease its refference count.
      //

      if (it -> isDir_)
      {
        it -> refCount_ --;

        it -> closeTime_ = time(0);

        DEBUG1("SFTP-CACHE: Released [%I64d]"
                   ", new ref count is [%d].\n", handle, it -> refCount_);

        realCloseNeeded = 0;
      }

      unlock();
    }

    //
    // If element not found in cache pass to underlying SFTP directly.
    //

    if (realCloseNeeded)
    {
      DEBUG1("SFTP-CACHE: Real close [%I64d]\n", handle);

      realclose(handle);
    }

    mutex_.unlock();
  }

  //
  // Wrapper for SftpClient::mkdir().
  //

  int CachedSftpClient::mkdir(const char *path)
  {
    removeUnusedHandle(path);

    return SftpClient::mkdir(path);
  }

  //
  // Wrapper for SftpClient::rmdir().
  //

  int CachedSftpClient::rmdir(const char *path)
  {
    removeUnusedHandle(path);

    return SftpClient::rmdir(path);
  }

  //
  // Wrapper for SftpClient::rename().
  //

  int CachedSftpClient::rename(const char *path1, const char *path2)
  {
    removeUnusedHandle(path1);
    removeUnusedHandle(path2);

    return SftpClient::rename(path1, path2);
  }

  //
  // Wrapper for SftpClient::statvfs().
  //

  int CachedSftpClient::statvfs(Statvfs_t *stvfs, const char *path)
  {
    map<string, CacheStatvfsElement>::iterator it;

    int found = 0;

    int ret = -1;

    //
    // Try use cache first.
    //

    lock();

    it = cacheStatvfs_.find(path);

    if (it != cacheStatvfs_.end()
            && (time(0) - it -> second.timestamp_) < 5)
    {
      memcpy(stvfs, &it -> second.statvfs_, sizeof(*stvfs));

      DEBUG1("SFTP-CACHE: Reusing statvfs for [%s].\n", path);

      found = 1;

      ret = 0;
    }

    unlock();

    //
    // Path not found in cache or too old.
    // Get info from server and put retrieved data into cache.
    //

    if (found == 0)
    {
      ret = SftpClient::statvfs(stvfs, path);

      if (ret == 0)
      {
        lock();
        cacheStatvfs_[path].statvfs_   = *stvfs;
        cacheStatvfs_[path].timestamp_ = time(0);
        unlock();

        DEBUG1("SFTP-CACHE: Cached statvfs for [%s].\n", path);
      }
    }

    return ret;
  }

  //
  // Wrapper on SftpClient::craetefile().
  //

  int64_t CachedSftpClient::createfile(const char *path, uint32_t access,
                                           uint32_t shared, uint32_t create,
                                               uint32_t flags, int *isDir)
  {
    DBG_ENTER3("CachedSftpClient::createfile");

    list<CacheElement>::iterator it;

    int64_t handle = -1;

    //
    // Find element in cache.
    //

    it = findElementForUpdate(path);

    //
    // Element already cached.
    // Try use it.
    //

    if (it != cache_.end())
    {
      if (it -> isDir_)
      {
        handle = it -> handle_;

        it -> refCount_ ++;

        DEBUG1("SFTP-CACHE: Reused craetefile dir [%I64d][%s]"
                   ", new ref count is [%d].\n", handle, path, it -> refCount_);
      }

      unlock();
    }

    //
    // Element not cached.
    // Open dir on server and cache retrieved handle.
    //

    if (handle == -1)
    {
      int isDirLocal = 0;

      handle = SftpClient::createfile(path, access, shared,
                                          create, flags, &isDirLocal);

      if (isDir)
      {
        *isDir = isDirLocal;
      }

      //
      // If retrieved handle redirects to directory cache it.
      //

      if (handle != -1 && isDirLocal)
      {
        CacheElement e;

        e.path_          = path;
        e.handle_        = handle;
        e.isDir_         = 1;
        e.readdirCalled_ = 0;
        e.refCount_      = 1;

        mutex_.lock();
        cache_.push_back(e);
        mutex_.unlock();

        DEBUG1("SFTP-CACHE: Added createfile dir [%s][%I64d].\n", path, handle);
      }
    }

    DBG_LEAVE3("CachedSftpClient::createfile");

    return handle;
  }

  //
  // ----------------------------------------------------------------------------
  //
  //                       Widechar variants for DOKAN
  //
  // ----------------------------------------------------------------------------
  //

  //
  // Widechar wrapper for open().
  // See utf8 version of SftpClient::open().
  //

  int64_t CachedSftpClient::opendir(const wchar_t *path16)
  {
    char path[MAX_PATH] = {0};

    WideCharToMultiByte(CP_UTF8, 0, path16, -1, path, sizeof(path), NULL, NULL);

    return opendir(path);
  }

  //
  // ----------------------------------------------------------------------------
  //
  //                             Helper functions
  //
  // ----------------------------------------------------------------------------
  //

  int CachedSftpClient::realclose(int64_t handle)
  {
    return SftpClient::close(handle);
  }

  //
  // Widechar wrapper for mkdir().
  // See utf8 version of SftpClient::mkdir().
  //

  int CachedSftpClient::mkdir(const wchar_t *path16)
  {
    char path[MAX_PATH] = {0};

    WideCharToMultiByte(CP_UTF8, 0, path16, -1, path, sizeof(path), NULL, NULL);

    return mkdir(path);
  }

  //
  // Widechar wrapper for rmdir().
  // See utf8 version of SftpClient::rmdir().
  //

  int CachedSftpClient::rmdir(const wchar_t *path16)
  {
    char path[MAX_PATH] = {0};

    WideCharToMultiByte(CP_UTF8, 0, path16, -1, path, sizeof(path), NULL, NULL);

    return rmdir(path);
  }

  //
  // Widechar wrapper for rename().
  // See utf8 version of SftpClient::rename().
  //

  int CachedSftpClient::rename(const wchar_t *path1_16, const wchar_t *path2_16)
  {
    char path1[MAX_PATH] = {0};
    char path2[MAX_PATH] = {0};

    WideCharToMultiByte(CP_UTF8, 0, path1_16, -1, path1, sizeof(path1), NULL, NULL);
    WideCharToMultiByte(CP_UTF8, 0, path2_16, -1, path2, sizeof(path2), NULL, NULL);

    return rename(path1, path2);
  }

  //
  // Widechar wrapper for createfile().
  // See utf8 version of SftpClient::createfile().
  //

  int64_t CachedSftpClient::createfile(const wchar_t *path16, uint32_t access,
                                           uint32_t shared, uint32_t create,
                                               uint32_t flags, int *isDir)
  {
    char path[MAX_PATH] = {0};

    WideCharToMultiByte(CP_UTF8, 0, path16, -1, path, sizeof(path), NULL, NULL);

    return createfile(path, access, shared, create, flags, isDir);
  }
  #endif /* WIN32 */

} /* namespace Tegenaria */
