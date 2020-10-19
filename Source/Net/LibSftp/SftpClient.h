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

#ifndef Tegenaria_Core_SftpClient_H
#define Tegenaria_Core_SftpClient_H

//
// Includes.
//

#ifdef WIN32
# include <windows.h>
#endif

#include <Tegenaria/Mutex.h>
#include <Tegenaria/RequestPool.h>
#include <Tegenaria/Thread.h>
#include <Tegenaria/Math.h>
#include <string>
#include <vector>

#include "Sftp.h"
#include "SftpJob.h"

namespace Tegenaria
{
  using std::string;
  using std::vector;

  //
  // Defines.
  //

  #ifdef WIN32
  # define SFTP_ISDIR(mode) (((mode) & (_S_IFMT)) == (_S_IFDIR))
  #else
  # define SFTP_ISDIR(mode) (((mode) & (S_IFMT)) == (S_IFDIR))
  #endif

  #define SFTP_CLIENT_FD     0
  #define SFTP_CLIENT_SOCKET 1

  #define SFTP_CLIENT_AVG_ALFA 0.9

  //
  // Forward declarations.
  //

  class SftpClient;

  //
  // Typedef.
  //

  typedef void (*SftpNetStatCallbackProto)(NetStatistics *netstat, void *ctx);
  typedef void (*SftpConnectionDroppedProto)(SftpClient *SftpClient, void *ctx);

  //
  // Sftp client class.
  //

  class SftpClient
  {
    protected:

    int fdin_;
    int fdout_;

    int timeout_;
    int counter_;

    int sectorSize_;

    int fdType_;

    int dead_;

    Mutex mutex_;

    RequestPool *rpool_;

    //
    // Network statistics.
    //

    NetStatistics netstat_;

    //
    // Thread reading incoming packets.
    //

    ThreadHandle_t *readThread_;

    //
    // Callback to inform owner when underlying network conenction dead.
    //

    SftpConnectionDroppedProto connectionDroppedCallback_;

    void *connectionDroppedCallbackCtx_;

    //
    // Shutdown object.
    // After that object reach dead stead and become unusable.
    //

    void shutdown();

    //
    // Partial operation detection.
    //

    int partialReadThreshold_;
    int partialWriteThreshold_;

    //
    // Callback to pass network statistics to caller.
    //

    SftpNetStatCallbackProto netStatCallback_;

    void *netStatCallbackCtx_;

    int netStatTick_;

    //
    // Exported functions.
    //

    public:

    SftpClient(int fdin, int fdout,
                   int timeout = 30,
                       int fdType = SFTP_CLIENT_FD);

    ~SftpClient();

    //
    // Net statisticts
    //

    void setNetStatTick(int tick);

    void registerNetStatCallback(SftpNetStatCallbackProto callback, void *ctx);

    void setPartialThreshold(int readThreshold, int writeThreshold);

    //
    // Callback to inform owner when underlying network conenction dead.
    //

    void registerConnectionDroppedCallback(SftpConnectionDroppedProto callback, void *ctx);

    //
    // Negotiate connection with server.
    //

    int connect();
    int reconnect();

    void disconnect();

    //
    // Manage sector size.
    //

    void setSectorSize(int size);

    //
    // Wrappers for standard sftp commands.
    //

    int64_t open(const char *path, int mode = SSH2_FXP_OPEN, int isDir = 0);

    int64_t opendir(const char *path);

    int close(int64_t handle);

    //
    // Windows optimized functions.
    //

  #ifdef WIN32
    int stat(const char *path, BY_HANDLE_FILE_INFORMATION *extendInfo = NULL);
    int readdir(vector<WIN32_FIND_DATAW> &data, int64_t handle);
  #endif

    //
    // Generic functions.
    //

    int stat(const char *path, SftpFileAttr *attr = NULL);

    int readdir(vector<SftpFileInfo> &files, int64_t handle);

    int read(int64_t handle, char *buffer, uint64_t offset, int size);
    int write(int64_t handle, char *buffer, uint64_t offset, int size);

    int mkdir(const char *path);
    int remove(const char *path);
    int rmdir(const char *path);
    int rename(const char *path1, const char *path2);

    int statvfs(Statvfs_t *stvfs, const char *path);

    //
    // Custom SFTP commands to fit protocol with WINAPI better.
    //

    int64_t createfile(const char *path, uint32_t access,
                           uint32_t shared, uint32_t create, uint32_t flags,
                               int *isDir);

    int multiclose(vector<int64_t> &handle);

    int resetdir(int64_t handle);

    int append(int64_t handle, char *buffer, int size);

    //
    // Widechar version of above, because DOKAN uses WCHAR.
    // Windows only.
    //

  #ifdef WIN32
    int64_t open(const wchar_t *path, int mode = SSH2_FXP_OPEN, int isDir = 0);
    int64_t opendir(const wchar_t *path);

    int stat(const wchar_t *path, BY_HANDLE_FILE_INFORMATION *extendInfo = NULL);
    int mkdir(const wchar_t *path);
    int remove(const wchar_t *path);
    int rmdir(const wchar_t *path);
    int rename(const wchar_t *path1, const wchar_t *path2);
  #endif

    int64_t createfile(const wchar_t *path, uint32_t access,
                           uint32_t shared, uint32_t create, uint32_t flags,
                               int *isDir);

    int statvfs(Statvfs_t *stvfs, const wchar_t *path);

    //
    // High level API.
    //

    SftpJob *downloadFile(const char *localPath,
                            const char *remotePath,
                                SftpJobNotifyCallbackProto notifyCallback);

    SftpJob *uploadFile(const char *remotePath,
                          const char *localPath,
                              SftpJobNotifyCallbackProto notifyCallback);

    SftpJob *listFiles(const char *remotePath,
                           SftpJobNotifyCallbackProto notifyCallback);

    static int DownloadFileWorker(void *data);
    static int UploadFileWorker(void *data);
    static int ListFilesWorker(void *data);

    //
    // Packet trasmission.
    //

    int processPacketSimple(string &answer, string &packet);

    int processPacket(string &answer, string &packet);

    //
    // Read thread handler.
    //

    static int readLoop(void *data);

    //
    // Helpers.
    //

    int parseStatusPacket(uint32_t *status, string &packet);

    int packetComplete(string &packet, int realSize);

    int decodePacketHead(uint32_t *size, uint32_t *id, uint8_t *type,
                             string &packet, int realSize);

  #ifdef WIN32
    int popAttribs(BY_HANDLE_FILE_INFORMATION *info, string &packet);
  #endif

    int popAttribs(SftpFileAttr *info, string &packet);

    uint32_t popStatusPacket(string &packet, uint32_t expectedId);
  };

} /* namespace Tegenaria */

#endif /* Tegenaria_Core_SftpClient_H */
