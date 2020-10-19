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

#ifndef Tegenaria_Core_SftpJob_H
#define Tegenaria_Core_SftpJob_H

#include <vector>

#include "Sftp.h"
#include <Tegenaria/Thread.h>
#include <Tegenaria/Mutex.h>
#include <sys/time.h>

namespace Tegenaria
{
  using std::vector;

  //
  // Defines.
  //

  #define SFTP_JOB_TYPE_DOWNLOAD 1
  #define SFTP_JOB_TYPE_UPLOAD   2
  #define SFTP_JOB_TYPE_LIST     3

  #define SFTP_JOB_STATE_ERROR        1
  #define SFTP_JOB_STATE_INITIALIZING 2
  #define SFTP_JOB_STATE_PENDING      4
  #define SFTP_JOB_STATE_FINISHED     8
  #define SFTP_JOB_STATE_STOPPED      16

  #define SFTP_JOB_NOTIFY_STATE_CHANGED       1
  #define SFTP_JOB_NOTIFY_TRANSFER_STATISTICS 2
  #define SFTP_JOB_NOTIFY_FILES_LIST_ARRIVED  3

  //
  // Forward declaration.
  //

  class SftpJob;
  class SftpClient;

  //
  // Typedef.
  //

  typedef void (*SftpJobNotifyCallbackProto)(int type, SftpJob *job);

  //
  // Classes.
  //

  class SftpJob
  {
    private:

    int type_;

    string localName_;
    string remoteName_;

    int64_t totalBytes_;
    int64_t processedBytes_;

    double avgRate_;
    double startTime_;

    SftpClient *sftp_;

    int state_;

    Mutex refCountMutex_;

    int refCount_;

    ThreadHandle_t *thread_;

    SftpJobNotifyCallbackProto notifyCallback_;

    vector<SftpFileInfo> files_;

    //
    // Exported public functions.
    //

    public:

    SftpJob(int type, SftpClient *sftp, const char *localName,
                const char *remoteName, SftpJobNotifyCallbackProto notifyCallback);

    void triggerNotifyCallback(int code);

    //
    // Refference counter.
    //

    void addRef();
    void release();

    //
    // Setters.
    //

    void setState(int state);

    void setThread(ThreadHandle_t *thread);

    const char *getStateString();

    void updateStatistics(double processedBytes, double totalBytes);

    void clearFiles();

    void addFile(SftpFileInfo &file);

    //
    // Getters.
    //

    int getType();
    int getState();

    SftpClient *getSftpClient();

    const char *getRemoteName();
    const char *getLocalName();

    double getAvgRate();

    double getPercentCompleted();

    int64_t getTotalBytes();
    int64_t getProcessedBytes();

    vector<SftpFileInfo> &getFiles();

    //
    // Synchronize functinos.
    //

    int wait(int timeout = -1);

    void cancel();

    //
    // Private functions.
    //

    private:

    ~SftpJob();

    double getTimeMs();
  };

} /* namespace Tegenaria */

#endif /* Tegenaria_Core_SftpJob_H */
