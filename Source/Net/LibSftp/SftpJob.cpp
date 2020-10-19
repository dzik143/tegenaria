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

#include "SftpJob.h"

namespace Tegenaria
{
  //
  // SftpJob constructor.
  //
  // type           - job's type, see SFTP_JOB_TYPE_XXX defines in SftpJob.h (IN).
  // sftp           - related SFTP Client object (IN).
  // localName      - related local path, e.g. source local file in upload job (IN).
  // remoteName     - related remote path e.g. destination remote path in upload job (IN).
  //
  // notifyCallback - function to be called when new transfer statistics arrives
  //                  or job's state changed. Optional, can be NULL (IN/OPT).
  //

  SftpJob::SftpJob(int type, SftpClient *sftp,
                       const char *localName, const char *remoteName,
                           SftpJobNotifyCallbackProto notifyCallback)
  {
    DBG_ENTER3("SftpJob::SftpJob");

    DBG_SET_ADD("SftpJob", this);

    if (localName == NULL)
    {
      localName = "";
    }

    if (remoteName == NULL)
    {
      remoteName = "";
    }

    refCount_       = 1;
    type_           = type;
    sftp_           = sftp;
    localName_      = localName;
    remoteName_     = remoteName;
    totalBytes_     = 0;
    processedBytes_ = 0;
    avgRate_        = 0.0;
    startTime_      = this -> getTimeMs();
    thread_         = NULL;
    state_          = SFTP_JOB_STATE_INITIALIZING;
    notifyCallback_ = notifyCallback;

    switch(type)
    {
      case SFTP_JOB_TYPE_DOWNLOAD:
      {
        DBG_INFO("Created SFTP download job. PTR is '%p', remote path is '%s', local path is '%s'",
                     this, remoteName, localName);

        break;
      }

      case SFTP_JOB_TYPE_UPLOAD:
      {
        DBG_INFO("Created SFTP upload job. PTR is '%p', remote path is '%s', local path is '%s'",
                     this, remoteName, localName);

        break;
      }

      case SFTP_JOB_TYPE_LIST:
      {
        DBG_INFO("Created SFTP list job. PTR is '%p', remote path is '%s'", this, remoteName);

        break;
      }
    }

    DBG_LEAVE3("SftpJob::SftpJob");
  }

  //
  // SftpJob destructor.
  //

  SftpJob::~SftpJob()
  {
    DBG_SET_DEL("SftpJob", this);
  }

  //
  // Get current time in ms.
  //

  double SftpJob::getTimeMs()
  {
    struct timeval tv;

    gettimeofday(&tv, NULL);

    return tv.tv_sec * 1000.0 + tv.tv_usec / 1000.0;
  }

  //
  // Increase refference counter.
  //
  // WARNING! Every call to addRef() MUSTS be followed by one release() call.
  //
  // TIP #1: Object will not be destroyed until refference counter is greater
  //         than 0.
  //
  // TIP #2: Don't call destructor directly, use release() instead. If
  //         refference counter achieve 0, object will be destroyed
  //         automatically.
  //

  void SftpJob::addRef()
  {
    refCountMutex_.lock();

    refCount_ ++;

    DEBUG2("Increased refference counter to %d for SftpJob PTR#%p.\n", refCount_, this);

    refCountMutex_.unlock();
  }

  //
  // Decrease refference counter increased by addRef() before.
  //

  void SftpJob::release()
  {
    int deleteNeeded = 0;

    //
    // Decrease refference counter by 1.
    //

    refCountMutex_.lock();

    refCount_ --;

    DEBUG2("Decreased refference counter to %d for SftpJob PTR#%p.\n", refCount_, this);

    if (refCount_ == 0)
    {
      deleteNeeded = 1;
    }

    refCountMutex_.unlock();

    //
    // Delete object if refference counter goes down to 0.
    //

    if (deleteNeeded)
    {
      delete this;
    }
  }

  //
  // Change current state. See SFTP_STATE_XXX defines in SftpJob.h.
  //
  // state - new state to set (IN).
  //

  void SftpJob::setState(int state)
  {
    state_ = state;

    DEBUG2("SftpJob PTR#%p changed state to [%s].\n", this, this -> getStateString());

    switch(state)
    {
      case SFTP_JOB_STATE_ERROR:        DBG_INFO("SFTP job PTR#%p finished with error.\n", this); break;
      case SFTP_JOB_STATE_INITIALIZING: DEBUG2("SFTP job PTR#%p initializing.\n", this); break;
      case SFTP_JOB_STATE_PENDING:      DEBUG2("SFTP job PTR#%p pending.\n", this); break;
      case SFTP_JOB_STATE_FINISHED:     DBG_INFO("SFTP job PTR#%p finished with success.\n", this); break;
      case SFTP_JOB_STATE_STOPPED:      DBG_INFO("SFTP job PTR#%p stopped.\n", this); break;
    }

    //
    // Call notify callback if set.
    //

    if (notifyCallback_)
    {
      notifyCallback_(SFTP_JOB_NOTIFY_STATE_CHANGED, this);
    }
  }

  //
  // Set thread handle related with job.
  //
  // thread - thread handle to set (IN).
  //

  void SftpJob::setThread(ThreadHandle_t *thread)
  {
    thread_ = thread;
  }

  //
  // Get current state code. See SFTP_JOB_STATE_XXX defines in SftpJob.h.
  //
  // RETURNS: Current state code.
  //

  int SftpJob::getState()
  {
    return state_;
  }

  //
  // Get current state as human readable string.
  //
  // RETURNS: Name of current job's state.
  //

  const char *SftpJob::getStateString()
  {
    switch(state_)
    {
      case SFTP_JOB_STATE_ERROR:        return "Error";
      case SFTP_JOB_STATE_INITIALIZING: return "Initializing";
      case SFTP_JOB_STATE_PENDING:      return "Pending";
      case SFTP_JOB_STATE_FINISHED:     return "Finished";
      case SFTP_JOB_STATE_STOPPED:      return "Stopped";
    };

    return "Unknown";
  }

  //
  // Wait until job finished or stopped with error.
  //
  // timeout - maximum time to wait in ms. Set to -1 for infinite (IN).
  //
  // RETURNS: 0 if OK (job finished/stopped on exit),
  //         -1 otherwise (job still active on exit).

  int SftpJob::wait(int timeout)
  {
    DBG_ENTER2("SftpJob::waitForFinish");

    int exitCode = -1;

    int timeLeft = timeout;

    while(state_ == SFTP_JOB_STATE_INITIALIZING || state_ == SFTP_JOB_STATE_PENDING)
    {
      ThreadSleepMs(50);

      if (timeout > 0)
      {
        timeLeft -= 50;

        if (timeLeft <= 0)
        {
          Error("ERROR: Timeout while waiting for SftpJob PTR#%p.\n", this);

          goto fail;
        }
      }
    }

    //
    // Error handler.
    //

    exitCode = 0;

    fail:

    DBG_LEAVE2("SftpJob::waitForFinish");

    return exitCode;
  }

  //
  // Send stop signal for pending SFTP job.
  // After that related thread should stop working and state
  // should change to SFTP_JOB_STATE_STOPPED.
  //
  // WARNING#1: SFTP job object MUSTS be still released with release() method.
  //
  // TIP#1: To stop and release resources related with SFTP job use below code:
  //
  //        job -> cancel();
  //        job -> release();
  //

  void SftpJob::cancel()
  {
    this -> setState(SFTP_JOB_STATE_STOPPED);
  }

  //
  // Retrieve SftpClient object related with job.
  //

  SftpClient *SftpJob::getSftpClient()
  {
    return sftp_;
  }

  //
  // Retrieve remote name related with job.
  //

  const char *SftpJob::getRemoteName()
  {
    return remoteName_.c_str();
  }

  //
  // Retrieve local name related with job.
  //

  const char *SftpJob::getLocalName()
  {
    return localName_.c_str();
  }

  //
  // Update transfer statistics for job.
  // Internal use only.
  //
  // processedBytes - number of bytes already processed (IN).
  // totalBytes     - number of total bytes to process (IN).
  //

  void SftpJob::updateStatistics(double processedBytes, double totalBytes)
  {
    DBG_ENTER3("SftpJob::updateTransferStatistics");

    double currentTime = 0.0;
    double dt          = 0.0;

    //
    // Save total and processed fields.
    //

    processedBytes_ = processedBytes;
    totalBytes_     = totalBytes;

    //
    // Compute new averange rate.
    //

    currentTime = getTimeMs();
    dt          = (currentTime - startTime_) / 1000.0;
    avgRate_    = double(processedBytes) / dt;

    //
    // Call notify callback if set.
    //

    if (notifyCallback_)
    {
      notifyCallback_(SFTP_JOB_NOTIFY_TRANSFER_STATISTICS, this);
    }

    DBG_LEAVE3("SftpJob::updateTransferStatistics");
  }

  //
  // Retrieve job type. See SFTP_JOB_TYPE_XXX defines in SftpJob.h.
  //

  int SftpJob::getType()
  {
    return type_;
  }

  //
  // Get averange job's rate in bytes per seconds.
  //

  double SftpJob::getAvgRate()
  {
    return avgRate_;
  }

  //
  // Get total bytes needed to be processed to finish job e.g. total size
  // of file to download in bytes.
  //

  int64_t SftpJob::getTotalBytes()
  {
    return totalBytes_;
  }

  //
  // Get number of bytes already processed by job.
  //

  int64_t SftpJob::getProcessedBytes()
  {
    return processedBytes_;
  }

  //
  // Get completion status in percentes (0-100%).
  //

  double SftpJob::getPercentCompleted()
  {
    double perc = 0.0;

    if (totalBytes_ > 0)
    {
      perc = double(processedBytes_) / double(totalBytes_) * 100.0;
    }

    return perc;
  }

  //
  // Manage files list storing inside SftpJob object while performing
  // list job.
  //

  //
  // Clear list of files stored inside job object.
  // Used internally only.
  //

  void SftpJob::clearFiles()
  {
    files_.clear();
  }

  //
  // Add file to files list stored inside job object.
  // Used internally only.
  //

  void SftpJob::addFile(SftpFileInfo &file)
  {
    files_.push_back(file);
  }

  //
  // Retrieve list of files stored inside job object.
  //
  // TIP#1: Use this function to retrieve next part of files while performing
  //        SFTP_JOB_TYPE_LIST job. This function should be called inside
  //        notify callback passed to SftpClient::listFiles() function.
  //
  // RETURNS: Refference to files list currently stored inside job object.
  //

  vector<SftpFileInfo> &SftpJob::getFiles()
  {
    return files_;
  }

  //
  // Tell sftp job object to call underlying notify callback set in constructor.
  // Used internally only.
  //
  // code - one of SFTP_JOB_NOTIFY_XXX code passed to callback notify (IN).
  //

  void SftpJob::triggerNotifyCallback(int code)
  {
    if (notifyCallback_)
    {
      notifyCallback_(code, this);
    }
  }
} /* namespace Tegenaria */
