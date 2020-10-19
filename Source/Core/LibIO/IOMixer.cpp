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
// Purpose: Send many isolated channels over one CRT file descriptor.
//

#include "IOMixer.h"
#include "Utils.h"

namespace Tegenaria
{
  //
  // Static variables.
  //

  set<IOMixer *> IOMixer::instances_;

  Mutex IOMixer::instancesMutex_("IOMixer::instances_");

  //
  // Create new slave fd pair.
  //
  // All <data> written to 'slaveOut' will be wrapped to <slaveId> <len> <data>
  // inside slave thread. One slave = one 'slave thread'.
  //
  //
  // -> <data1> -> [Slave1] \
  //                        -> <1><len1><data1> <2><len2><data2> -> [master] ->
  // -> <data3> -> [Slave3] /
  //
  //
  // All <slaveId> <len> <data> readed from 'masterIn' will be dispatched
  // into 'slaveIn' with given ID inside 'master thread'.
  // One master thread is common to all slaves.
  //
  //                                              <data1> ->
  //                                             /
  // <1><len1><data1><2><len2><data2> -> [Master]
  //                                             \
  //                                              <data2> ->
  //
  //
  // callerFds[] - input/output CRT fds valid in caller context (OUT).
  //
  // slaveId     - ID for new slave. Optional, if skipped new unique ID
  //               will be generated (IN/OPT).
  //
  // RETURNS: ID assigned to slave or -1 if error.
  //

  int IOMixer::addSlave(int callerFds[2], int slaveId)
  {
    DBG_ENTER("IOMixer::addSlave");

    int exitCode = -1;

    int pin[2];
    int pout[2];

    IOMixerSlave *slave = NULL;

    char mutexName[64];

    //
    // Check args.
    //

    FAILEX(init_ == 0, "ERROR: IOMixer object was not initiated correctly.\n");
    FAILEX(slaveId == 0, "ERROR: Slave ID #0 is reseved for internal use.");
    FAILEX(dead_, "ERROR: IOMixer already dead while trying to add slave ID#%d.", slaveId);

    //
    // Crate pipe pairs for slave's in/out.
    //

    #ifdef WIN32
    {
      //
      // Security attributes for pin[] pair.
      //

      SECURITY_ATTRIBUTES sain[2] =
      {
        {sizeof(SECURITY_ATTRIBUTES), NULL, FALSE}, // pin[0] - don't inherit
        {sizeof(SECURITY_ATTRIBUTES), NULL, TRUE }, // pin[1] - inherit
      };

      //
      // Security attributes for pout[] pair.
      //

      SECURITY_ATTRIBUTES saout[2] =
      {
        {sizeof(SECURITY_ATTRIBUTES), NULL, TRUE }, // pout[0] - inherit
        {sizeof(SECURITY_ATTRIBUTES), NULL, FALSE}  // pout[1] - don't inherit
      };

      FAIL(CreatePipeEx(pin, sain, IOMIXER_MAX_PACKET,
                            FILE_FLAG_OVERLAPPED, FILE_FLAG_OVERLAPPED, 0));

      FAIL(CreatePipeEx(pout, saout, IOMIXER_MAX_PACKET,
                            FILE_FLAG_OVERLAPPED, FILE_FLAG_OVERLAPPED, 0));
    }
    #else
    {
      FAILEX(pipe(pin), "ERROR: Cannot create pipe pair.");
      FAILEX(pipe(pout), "ERROR: Cannot create pipe pair.");
    }
    #endif

    DBG_SET_ADD("CRT FD", pout[0], "IOMixer/caller/FD/0");
    DBG_SET_ADD("CRT FD", pout[1], "IOMixerSlave::fdout_");
    DBG_SET_ADD("CRT FD", pin[0], "IOMixerSlave::fdin_");
    DBG_SET_ADD("CRT FD", pin[1], "IOMixer/caller/FD/1");

    callerFds[0] = pout[0];
    callerFds[1] = pin[1];

    slavesMutex_.lock();

    //
    // Assign unique ID to slave if not specified by user.
    //

    if (slaveId == -1)
    {
      while(getSlave(slavesCount_))
      {
        slavesCount_ ++;
      }

      slaveId = slavesCount_;
    }

    //
    // If user specified own ID, check is it unique.
    //

    else
    {
      FAILEX(slaves_.find(slaveId) != slaves_.end(),
                 "IOMixer::addSlve : ERROR: ID [%d] is not unique.\n",
                     slaveId);
    }

    DEBUG1("IOMixer::addSlave : Assigned ID [%d] to slave FDs [%d][%d] for master [%d][%d].",
                slaveId, callerFds[1], callerFds[0], masterOut_, masterIn_);

    //
    // Allocate struct to store slave's context.
    //

    slave = new IOMixerSlave;

    slave -> id_          = slaveId;
    slave -> eofSent_     = 0;
    slave -> eofReceived_ = 0;
    slave -> fdin_        = pin[0];
    slave -> fdout_       = pout[1];
    slave -> this_        = this;
    slave -> flags_       = 0;

    //
    // Prepare overlapped I/O and cancel event on Windows.
    //

    #ifdef WIN32
    {
      memset(&slave -> ov_, 0, sizeof(OVERLAPPED));

      slave -> cancelEvent_ = CreateEvent(NULL, FALSE, FALSE, NULL);

      DBG_SET_ADD("event", slave -> cancelEvent_, "IOMixerSlave::cancelEvent_");
    }

    //
    // Prepare cancel FDs on Linux.
    //

    #else
    if (pipe(slave -> cancelFd_) < 0)
    {
      Error("ERROR: Cannot create cancel pipe for slave ID#%d.\n", slaveId);
    }
    DBG_SET_ADD("fd", slave -> cancelFd_[0], "IOMixerSlave::cancelFd_[0]");
    #endif

    snprintf(mutexName, sizeof(mutexName), "IOMixer::Slave#%d", slaveId);

    slave -> writeMutex_.setName(mutexName);

    //
    // Start up slave thread.
    // In this moment we're starting sending data
    // from slaves to master outputs.
    //

    slave -> thread_ = ThreadCreate((ThreadEntryProto) slaveLoop, slave);

    DBG_SET_RENAME("thread", slave -> thread_, "IOMixer::slaveLoop");

    //
    // Store {ID |-> slave} pair for quick search for slave with given ID.
    //

    slaves_[slave -> id_] = slave;

    //
    // Clean up.
    //

    exitCode = 0;

    fail:

    slavesMutex_.unlock();

    DBG_LEAVE("IOMixer::addSlave");

    if (exitCode == 0)
    {
      return slaveId;
    }
    else
    {
      return -1;
    }
  };

  //
  // Constructor using socket/FDs pair.
  //

  IOMixer::IOMixer(int masterOut, int masterIn, int masterOutType, int masterInType)
  {
    DBG_ENTER("IOMixer::IOMixer");

    unsigned long mode = 1;

    int tcpNoDelay = 1;

    char buf[128] = {0};

    DBG_SET_ADD("IOMixer", this);

    //
    // Track created instances.
    //

    instancesMutex_.lock();
    instances_.insert(this);
    instancesMutex_.unlock();

    DEBUG4("IOMixer::IOMixer : Creating I/O mixer from master"
               " FDs [%d][%d]...\n", masterOut, masterIn);

    init_       = 0;
    dead_       = 0;
    quietMode_  = 0;
    zlibLoaded_ = 0;
    refCount_   = 1;

    #ifdef WIN32
    memset(&masterInOV_, 0, sizeof(masterInOV_));
    memset(&masterOutOV_, 0, sizeof(masterOutOV_));
    #endif

    FAILEX(masterOut < 0, "ERROR: Wrong master OUT in IOMixer::IOMixer.\n");
    FAILEX(masterIn < 0, "ERROR: Wrong master IN in IOMixer::IOMixer.\n");

    slavesCount_        = 1;
    masterOut_          = masterOut;
    masterIn_           = masterIn;
    masterOutType_      = masterOutType;
    masterInType_       = masterInType;
    masterEofSent_      = 0;
    masterEofReceived_  = 0;


    slaveDeadCallback_ = NULL;

    #ifdef WIN32
    masterInOV_.hEvent  = WSACreateEvent();
    masterOutOV_.hEvent = WSACreateEvent();

    DBG_SET_ADD("event", masterInOV_.hEvent, "IOMixer::masterInOV_");
    DBG_SET_ADD("event", masterOutOV_.hEvent, "IOMixer::masterOutOV_");

    ioctlsocket(masterIn_, FIONBIO, &mode);
    ioctlsocket(masterOut_, FIONBIO, &mode);

    /*
      setsockopt(masterIn_, IPPROTO_TCP, TCP_NODELAY, (char *) &tcpNoDelay, sizeof(tcpNoDelay));
      setsockopt(masterOut_, IPPROTO_TCP, TCP_NODELAY, (char *) &tcpNoDelay, sizeof(tcpNoDelay));
    */
    #endif

    masterMutex_.setName("IOMixer::Master");
    slavesMutex_.setName("IOMixer::Slaves");

    //
    // Load ZLib for compression.
    //

    initZLib();

    //
    // Generate object name.
    //

    snprintf(buf, sizeof(buf) - 1, "IOMixer/0x%p", this);

    objectName_ = buf;

    //
    // Set init OK flag.
    //

    init_ = 1;

    //
    // Error handler.
    //

    fail:

    DBG_LEAVE("IOMixer::IOMixer");
  }

  //
  // Constructor using two caller specified read/write functions.
  //
  // readCallback     - function used to read data from master (IN).
  // writeCallback    - function used to write data to master (IN).
  // readCtx          - caller defined data passed to readCallback() (IN/OPT).
  // writeCtx         - caller defined data passed to writeCallback() (IN/OPT).
  // ioCancelCallback - function used to cancel pending IO on master (IN/OPT)
  //

  IOMixer::IOMixer(IOReadProto readCallback, IOWriteProto writeCallback,
                       void *readCtx, void *writeCtx, IOCancelProto ioCancelCallback)
  {
    DBG_ENTER("IOMixer::IOMixer");

    unsigned long mode = 1;

    int tcpNoDelay = 1;

    char buf[128] = {0};

    DBG_SET_ADD("IOMixer", this);

    //
    // Track created instances.
    //

    instancesMutex_.lock();
    instances_.insert(this);
    instancesMutex_.unlock();

    DEBUG4("IOMixer::IOMixer : Creating I/O mixer from master"
               " callbacks [%p/%p][%p/%p]...\n", readCallback, readCtx,
                   writeCallback, writeCtx);

    init_       = 0;
    dead_       = 0;
    quietMode_  = 0;
    zlibLoaded_ = 0;
    refCount_   = 1;

    #ifdef WIN32
    memset(&masterInOV_, 0, sizeof(masterInOV_));
    memset(&masterOutOV_, 0, sizeof(masterOutOV_));
    #endif

    FAILEX(readCallback == NULL, "ERROR: Wrong read callback in IOMixer::IOMixer.\n");
    FAILEX(writeCallback == NULL, "ERROR: Wrong write callback in IOMixer::IOMixer.\n");

    slavesCount_   = 1;
    masterOut_     = -1;
    masterIn_      = -1;
    masterOutType_ = IOMIXER_TYPE_CALLBACK ;
    masterInType_  = IOMIXER_TYPE_CALLBACK ;

    masterReadCallback_  = readCallback;
    masterWriteCallback_ = writeCallback;
    ioCancelCallback_    = ioCancelCallback;
    readCtx_             = readCtx;
    writeCtx_            = writeCtx;

    masterEofSent_       = 0;
    masterEofReceived_   = 0;

    slaveDeadCallback_ = NULL;

    masterMutex_.setName("IOMixer::Master");
    slavesMutex_.setName("IOMixer::Slaves");

    //
    // Generate object name.
    //

    snprintf(buf, sizeof(buf) - 1, "IOMixer/0x%p", this);

    objectName_ = buf;

    //
    // Load ZLib for compression.
    //

    initZLib();

    //
    // Set init OK flag.
    //

    init_ = 1;

    //
    // Error handler.
    //

    fail:

    DBG_LEAVE("IOMixer::IOMixer");
  }

  //
  // Retrieve pointer to slave object with given ID,
  // added by addSlave() before.
  //
  // RETURNS: Pointer to slave object or NULL if wrong ID.
  //

  IOMixerSlave *IOMixer::getSlave(int id)
  {
    map<int, IOMixerSlave *>::iterator it = slaves_.find(id);

    if (it != slaves_.end())
    {
      return it -> second;
    }

    DEBUG1("Slave ID [%d] for master FDs [%d][%d] does not exist.\n",
                id, masterIn_, masterOut_);

    return NULL;
  }

  //
  // Remove slave with given ID.
  //
  // id - ID of slave to remove, retrieved from addSlave() before (IN).
  //
  // RETURNS: 0 if OK.
  //

  int IOMixer::removeSlave(int id)
  {
    DBG_ENTER("IOMixer::removeSlave");

    slavesMutex_.lock();

    IOMixerSlave *slave = getSlave(id);

    if (slave)
    {
      #ifdef WIN32
      DWORD unused = 0;
      #endif

      DEBUG4("IOMixer::removeSlave : Closing slave ID [%d] FDs [%d][%d]"
                  " for master FDs [%d][%d]...\n", slave -> id_,
                      slave -> fdout_, slave -> fdin_, masterOut_, masterIn_);

      if (slave -> fdout_ != -1)
      {
        close(slave -> fdout_);

        DBG_SET_DEL("CRT FD", slave -> fdout_);

        slave -> fdout_ = -1;
      }

      if (slave -> fdin_ != -1)
      {
        close(slave -> fdin_);

        DBG_SET_DEL("CRT FD", slave -> fdin_);

        slave -> fdin_ = -1;
      }

      //
      // Cancel slave loop thread and wait until finished.
      //

      if (slave -> thread_)
      {
        DEBUG4("IOMixer::removeSlave : joining thread slave ID#%d...\n", id);



        #ifdef WIN32
        {
          SetEvent(slave -> cancelEvent_);
        }
        #else
        if (write(slave -> cancelFd_[1], "x", 1) <= 0)
        {
          DEBUG1("WARNING: Cannot cancel IO for slave ID#%d.\n", id);
        }
        #endif

        ThreadWait(slave -> thread_);
        ThreadClose(slave -> thread_);

        slave -> thread_ = NULL;
      }

      #ifdef WIN32
      {
        if (slave -> ov_.hEvent)
        {
          DEBUG4("IOMixer::removeSlave : clearing overlapped struct for slave ID#%d...\n", id);

          CloseHandle(slave -> ov_.hEvent);

          DBG_SET_DEL("event", slave -> ov_.hEvent);

          slave -> ov_.hEvent = NULL;
        }
      }
      #endif

      //
      // Close cancel event on windows.
      //

      #ifdef WIN32
      {
        if (slave -> cancelEvent_)
        {
          CloseHandle(slave -> cancelEvent_);

          DBG_SET_DEL("event", slave -> cancelEvent_);
        }
      }

      //
      // Close cancel FDs on linux.
      //

      #else
      {
        if (slave -> cancelFd_[0] != -1)
        {
          close(slave -> cancelFd_[0]);

          DBG_SET_DEL("fd", slave -> cancelFd_[0]);

          slave -> cancelFd_[0] = -1;
        }

        if (slave -> cancelFd_[1] != -1)
        {
          close(slave -> cancelFd_[1]);

          DBG_SET_DEL("fd", slave -> cancelFd_[1]);

          slave -> cancelFd_[1] = -1;
        }
      }
      #endif

      //
      // Free slave struct.
      //

      DEBUG4("IOMixer::removeSlave : destroying slave ID#%d.\n", id);

      delete slave;

      slaves_.erase(id);

      DEBUG2("IOMixer::removeSlave : slave ID#%d destroyed.\n", id);
    }

    slavesMutex_.unlock();

    DBG_LEAVE("IOMixer::removeSlave");

    return 0;
  }

  //
  // Start up master thread, which reading from <master>
  // and dispatching readed data to slaves.
  //
  // WARNING: If master thread will be started BEFORE registering
  //          needed slaves via addSlave(), all incoming data
  //          will be ignored with "ERROR: Unknown slave ID.".
  //
  // RETURNS: 0 if OK.
  //

  int IOMixer::start()
  {
    int exitCode = -1;

    DEBUG4("IOMixer::start : Starting master thread for"
                 " FDs [%d][%d]...\n", masterOut_, masterIn_);

    FAILEX(init_ == 0, "ERROR: IOMixer object was not initiated correctly.\n");

    masterThread_ = ThreadCreate((ThreadEntryProto) IOMixer::masterLoop, this);

    DBG_SET_RENAME("thread", masterThread_, "IOMixer::masterLoop");

    //
    // Error handler.
    //

    exitCode = 0;

    fail:

    return 0;
  }

  //
  // Stop master thread started before by start() method.
  //
  // RETURNS: 0 if OK.
  //

  int IOMixer::stop()
  {
    DEBUG4("IOMixer::stop : Stopping master thread for"
                 " FDs [%d][%d]...\n", masterOut_, masterIn_);

    dead_ = 1;

    if (ioCancelCallback_)
    {
      ioCancelCallback_(readCtx_);
      ioCancelCallback_(writeCtx_);
    }

    if (masterThread_)
    {
      DEBUG3("IOMixer::stop : Waiting for master thread for IOMixer PTR#%p.\n", this);

      ThreadWait(masterThread_);
      ThreadClose(masterThread_);

      masterThread_ = NULL;
    }
  }

  //
  // Thread routine to mix many <slave-outputs> into one <master-input>.
  // Used internally only.
  // One slave = one slave loop = one slave thread.
  //
  // -> slave1 \
  // -> slave2 -> master
  // -> slave3 /
  //
  // slave - pointer to related I/O slave (IN/OUT).
  //
  // RETURNS: 0 if OK.
  //

  int IOMixer::slaveLoop(IOMixerSlave *slave)
  {
    DBG_ENTER("IOMixer::slaveLoop");

    char buf[IOMIXER_MAX_PACKET];

    int readed = 1;

    int id = slave -> id_;

    int ret = -1;

    IOMixer *this_ = slave -> this_;

    this_ -> addRef();

    #ifdef WIN32
    HANDLE handle = (HANDLE) _get_osfhandle(slave -> fdin_);

    slave -> ov_.hEvent = CreateEvent(NULL, TRUE, TRUE, NULL);

    DBG_SET_ADD("event", slave -> ov_.hEvent, "IOMixerSlave::ov_");
    #endif

    //
    // Copy every
    //
    // - <DATA> readed from slave
    //
    // into:
    //
    // <ID><SIZE><DATA> written to master OUT.
    //

    while(readed > 0 && this_ -> dead_ == 0)
    {
      //
      // Read <data> from slave.
      //

      DEBUG4("IOMixer::slaveLoop : Waiting for data from slave"
                  " FD [%d] ID [%d]...\n", slave -> fdin_, slave -> id_);

      DBG_IO_READ_BEGIN(this_ -> objectName(), id, buf, sizeof(buf));

      #ifdef WIN32
      {
        HANDLE events[2] =
        {
          slave -> ov_.hEvent,
          slave -> cancelEvent_
        };

        ReadFile(handle, buf, sizeof(buf), NULL, &slave -> ov_);

        switch(WaitForMultipleObjects(2, events, FALSE, INFINITE))
        {
          case WAIT_OBJECT_0 + 0:
          {
            GetOverlappedResult(handle, &slave -> ov_, (PDWORD) &readed, FALSE);

            break;
          }

          case WAIT_OBJECT_0 + 1:
          {
            DEBUG1("IOMixer::slaveLoop: Read canceled on FD #%d.\n", slave -> fdin_);

            readed = 0;

            break;
          }
        }

        if (WaitForSingleObject(slave -> cancelEvent_, 0) == WAIT_OBJECT_0)
        {
          DEBUG1("IOMixer::slaveLoop: Read canceled on FD #%d.\n", slave -> fdin_);

          readed = 0;
        }
      }
      #else
      {
        fd_set rfd;

        int fdmax = std::max(slave -> fdin_, slave -> cancelFd_[0]);

        FD_ZERO(&rfd);

        FD_SET(slave -> fdin_, &rfd);
        FD_SET(slave -> cancelFd_[0], &rfd);

        select(fdmax + 1, &rfd, NULL, NULL, NULL);

        if (FD_ISSET(slave -> cancelFd_[0], &rfd))
        {
          DBG_INFO("Read canceled on slave #%d.\n", slave -> id_);

          readed = 0;
        }
        else if (FD_ISSET(slave -> fdin_, &rfd))
        {
          readed = read(slave -> fdin_, buf, sizeof(buf));
        }
      }
      #endif

      DEBUG4("IOMixer::slaveLoop : Readed [%d] bytes from slave"
                  " FD [%d] ID [%d].\n", readed, slave -> fdin_, slave -> id_);

      DBG_IO_READ_END(this_ -> objectName(), id, buf, readed);

      //
      // Write <id><flags><readed><data> into master.
      //

      DBG_IO_WRITE_BEGIN(this_ -> objectName(), 0, buf, readed);

      ret = this_ -> masterEncode(id, buf, readed, slave -> flags_);

      DBG_IO_WRITE_END(this_ -> objectName(), 0, buf, readed);

      FAIL(ret != 0);

      FAIL(readed <= 0);
    }

    //
    // EOF or error.
    // Worker finished.
    //

    fail:

    slave -> eofSent_ == 1;

    DEBUG1("IOMixer::slaveLoop : Job for slave ID [%d] for FDs [%d][%d]"
                " finished with code [%d].\n", slave -> id_, this_ -> masterOut_,
                    this_ -> masterIn_, GetLastError());

    #ifdef WIN32
    {
      CloseHandle(slave -> ov_.hEvent);

      DBG_SET_DEL("event", slave -> ov_.hEvent);

      slave -> ov_.hEvent = NULL;
    }
    #endif

    //
    // Tell caller about slave dead if he set callback.
    //

    if (this_ && this_ -> slaveDeadCallback_)
    {
      this_ -> slaveDeadCallback_(id, this_ -> slaveDeadCallbackCtx_);
    }

    this_ -> release();

    DBG_LEAVE("IOMixer::slaveLoop");

    return 0;
  };

  //
  // Thread routine to split one <master-input> into many <slave-outputs>.
  // Used internally only.
  //
  //           / slave1
  // -> master - slave2
  //           \ slave3
  //
  // this_ - pointer to related IOMixer object (IN/OUT).
  //
  // RETURNS: 0 if OK.
  //

  int IOMixer::masterLoop(IOMixer *this_)
  {
    DBG_ENTER("IOMixer::masterLoop");

    int readed  = 0;
    int len     = 0;
    int slaveId = 0;

    IOMixerSlave *slave = NULL;

    map<int, IOMixerSlave *>::iterator it;

    char data[IOMIXER_MAX_PACKET];

    int id   = -1;
    int size = -1;
    int ret  = -1;

    this_ -> addRef();

    //
    // Unpack one <master-input> into many <slaves-outputs>:
    //
    //           / slave1
    // -> master - slave2
    //           \ slave3
    //

    while(this_ -> masterEofReceived_ == 0)
    {
      //
      // Decode <id><size><data> packet from master IN.
      //

      DBG_IO_READ_BEGIN(this_ -> objectName(), 0, data, sizeof(data));

      ret = this_ -> masterDecode(&id, &size, data, sizeof(data));

      DBG_IO_READ_END(this_ -> objectName(), 0, data, size);

      FAIL(ret != 0);

      //
      // <size> <= 0 means EOF or ERROR.
      //

      if (size <= 0)
      {
        //
        // ID #0 is reserved for master.
        // It means close master connection and leave master loop.
        // EOF #0 is sent by remote via IOMixer::shutdown() function.
        //

        if (id == 0)
        {
          DEBUG1("IOMixer::masterLoop : Received EOF on master #0\n");

          this_ -> masterEofReceived_ = 1;
        }

        //
        // EOF on slave with id <ID>.
        // Remove slave and go on.
        //

        else
        {
          DEBUG1("IOMixer::masterLoop : Received EOF on slave ID #%d.\n", id);

          this_ -> slavesMutex_.lock();

          slave =  this_ -> getSlave(id);

          if (slave)
          {
            slave -> eofReceived_ = 1;

            if (slave -> fdout_ != -1)
            {
              close(slave -> fdout_);

              DBG_SET_DEL("CRT FD", slave -> fdout_);

              slave -> fdout_ = -1;
            }
          }

          this_ -> slavesMutex_.unlock();
        }
      }

      //
      // Otherwise, write <size> bytes of <data> to slave with ID <id>.
      //

      else
      {
        DBG_IO_WRITE_BEGIN(this_ -> objectName(), id, data, size);

        this_ -> slaveWrite(id, data, size);

        DBG_IO_WRITE_END(this_ -> objectName(), id, data, size);
      }
    }

    //
    // Clean up.
    //

    fail:

    DEBUG1("IOMixer::masterLoop : Finished with code [%d].\n", GetLastError());

    //
    // If we exited masterLoop before we received master EOF
    // it means connection brooken.
    // Then, set masterEofReceived_ manually to avoid infinite loop
    // in shutdown(), where we're waiting for remote EOF after
    // we sent own one.
    //

    if (this_ -> masterEofReceived_ == 0)
    {
      if (this_ -> quietMode_ == 0)
      {
        Error("IOMixer::masterLoop : connection broken.\n");
      }

      this_ -> masterEofReceived_ = 1;
    }

    //
    // Tell caller about master #0 dead.
    //

    if (this_ && this_ -> slaveDeadCallback_)
    {
      this_ -> slaveDeadCallback_(0, this_ -> slaveDeadCallbackCtx_);
    }

    this_ -> release();

    DBG_LEAVE("IOMixer::masterLoop");

    return 0;
  }

  //
  // Wait until 'master thread' and every 'slave threads' finished works.
  //
  // RETURNS: 0 if OK.
  //

  int IOMixer::join()
  {
    DBG_ENTER("IOMixer::join");

    //
    // Join to all slave threads.
    //

    flush();

    //
    // Join master thread first.
    //

    DEBUG4("IOMixer::join : Joining to master thread...\n");

    if (masterThread_)
    {
      ThreadWait(masterThread_);
      ThreadClose(masterThread_);

      masterThread_ = NULL;
    }

    DBG_LEAVE("IOMixer::join");

    return 0;
  }

  //
  // Desctructor.
  // Kill master thread and all slaves.
  //

  IOMixer::~IOMixer()
  {
    DBG_ENTER("IOMixer::~IOMixer");

    //
    // Check is this pointer correct.
    //

    if (isPointerCorrect(this) == 0)
    {
      Error("ERROR: Attemp to destroy non existing IOMixer PTR#%p.\n", this);

      return;
    }

    //
    // Track created instances.
    //

    instancesMutex_.lock();
    instances_.erase(this);
    instancesMutex_.unlock();

    shutdown();

    #ifdef WIN32
    {
      if (masterInOV_.hEvent)
      {
        CloseHandle(masterInOV_.hEvent);

        DBG_SET_DEL("event", masterInOV_.hEvent);

        masterInOV_.hEvent = NULL;
      }

      if (masterOutOV_.hEvent)
      {
        CloseHandle(masterOutOV_.hEvent);

        DBG_SET_DEL("event", masterOutOV_.hEvent);

        masterOutOV_.hEvent = NULL;
      }
    }
    #endif

    //
    // Free ZLib library if needed.
    //

    if (zlib_)
    {
      #ifdef WIN32
      FreeLibrary(zlib_);
      #else
      dlclose(zlib_);
      #endif
    }

    DBG_SET_DEL("IOMixer", this);

    DBG_LEAVE("IOMixer::~IOMixer");
  }

  //
  // Encode <data> into <id><flags><size><data> and write it into master OUT.
  //
  // TIP: If <size> is <= 0, then <data> part will be skipped.
  //      It's equal to sending EOF/error to other side, where remote
  //      read() will return -1/0.
  //
  // id    - channel id where to send data (IN).
  // buf   - buffer to send (IN).
  // size  - size of buf[] buffer in bytes (IN).
  // flags - combination of IOMIXER_FLAG_XXX flags (IN).
  //
  // RETURNS: 0 if OK.
  //

  int IOMixer::masterEncode(int id, void *buf, int size, uint8_t flags)
  {
    DBG_ENTER5("IOMixer::masterEncode");

    int exitCode = -1;

    int packetSize;

    struct Packet
    {
      int32_t channelId_;
      uint8_t flags_;
      int32_t dataSize_;

      char data_[1];
    }
    __attribute__((__packed__));

    if (zlibLoaded_)
    {
      packetSize = zlibCompressBound_(size) + sizeof(Packet);
    }
    else
    {
      packetSize = size + sizeof(Packet);
    }

    Packet *packet = (Packet *) malloc(packetSize);

    DEBUG4("IOMixer::masterEncode : Going to write [%d] bytes from slave ID [%d]"
              " to master [%d].\n", size, id, masterOut_);

    masterMutex_.lock();

    //
    // Generate <id><flags><size><data> packet.
    //

    packet -> channelId_ = id;
    packet -> flags_     = flags;
    packet -> dataSize_  = size;

    //
    // Compression enabled.
    //

    if (size > 256 && flags & IOMIXER_FLAG_COMPRESSION_ON && zlibLoaded_)
    {
      int compSize = packetSize - sizeof(Packet);

      int ret = zlibCompress_(packet -> data_, &compSize, buf, size);

      FAILEX(ret != 0, "ERROR: Compression failed with code [%d].\n", ret);

      packet -> dataSize_ = compSize;

      DEBUG5("IOMixer::masterEncode : Compressed [%d] bytes into [%d] (ratio %lf%%).\n",
                  size, compSize, double(compSize) / double(size) * 100.0);

      size = compSize;
    }

    //
    // Raw data.
    //

    else if (size > 0)
    {
      packet -> flags_ &= ~IOMIXER_FLAG_COMPRESSION_ON;

      memcpy(packet -> data_, buf, size);
    }

    //
    // Write <id><size> prefix.
    //

    FAIL(masterWrite(packet, size + 9));

    //
    // If <size> greater than zero write <size> bytes of <data>.
    //

    if (size > 0)
    {
      DEBUG3("IOMixer::masterEncode : Written [%d] bytes <data> with ID [%d]"
                  " to master [%d].", size, id, masterOut_);
    }
    else
    {
      DEBUG1("IOMixer::masterEncode : Sent EOF to slave ID [%d].", id);
    }

    exitCode = 0;

    //
    // Error handler.
    //

    fail:

    if (exitCode && quietMode_ == 0)
    {
      Error("ERROR: Cannot encode packet to master OUT [%d].\n"
                "Error code is : %d.\n", masterOut_, GetLastError());
    }

    masterMutex_.unlock();

    if (packet)
    {
      free(packet);
    }

    DBG_LEAVE5("IOMixer::masterEncode");

    return exitCode;
  }

  //
  // Pop <id><flags><size><data> packet from master IN.
  //
  // id       - buffer where to store decoded <id> (OUT).
  // size     - buffer where to store decoded <size> (OUT).
  // data     - buffer where to store decoded <data> (OUT).
  // dataSize - size of data[] buffer in bytes (IN).
  //
  // RETURNS: Number of decoded <data> bytes of -1 if error.
  //

  int IOMixer::masterDecode(int *id, int *size, void *data, int dataSize)
  {
    DBG_ENTER5("IOMixer::masterDecode");

    int exitCode = -1;

    uint8_t flags;

    //
    // Read <id><flags><size> head.
    //

    FAIL(masterRead(id, 4));
    FAIL(masterRead(&flags, 1));
    FAIL(masterRead(size, 4));

    DEBUG3("IOMixer::masterDecode : Readed <%d><%d><%d> head"
                " from master [%d].", *id, (int) flags, *size, masterIn_);

    //
    // Fail if data[] too small to store whole <DATA>.
    //

    FAILEX(dataSize < *size, "ERROR: Packet too big.\n");

    //
    // If <size> greater than 0 read <size> bytes of <data>.
    //

    if (*size > 0)
    {
      //
      // Incoming data compressed.
      //

      if (flags & IOMIXER_FLAG_COMPRESSION_ON)
      {
        unsigned char *compData = (unsigned char *) malloc(*size);

        int compSize = *size;

        //
        // Read compressed data.
        //

        if (masterRead(compData, *size) != 0)
        {
          free(compData);

          goto fail;
        }

        //
        // Decompress into caller buffer.
        //

        if (zlibLoaded_)
        {
          *size = dataSize;

          zlibUncompress_(data, size, compData, compSize);
        }
        else
        {
          Error("ERROR: ZLib not available, but compressed data received.\n");

          goto fail;
        }
      }

      //
      // Incoming data are uncompressed.
      //

      else
      {
        FAIL(masterRead(data, *size));
      }

      DEBUG3("IOMixer::masterDecode : Readed [%d] bytes <data> for ID [%d]"
                  " from from master [%d].", *size, *id, masterIn_);
    }

    exitCode = 0;

    //
    // Error handler.
    //

    fail:

    if (exitCode && quietMode_ == 0)
    {
      Error("ERROR: Cannot decode packet from master IN [%d].\n"
                "Error code is : %d.\n", masterIn_, GetLastError());
    }

    DBG_LEAVE5("IOMixer::masterDecode");

    return exitCode;
  }

  //
  // Atomic write <size> bytes from <buf> to master OUT.
  //
  // Wrapper to system write() to hide difference beetwen socket
  // and CRT FD on windows.
  //
  // buf          - buffer with data to write (IN).
  // bytesToWrite - number of bytes to be written (IN).
  //
  // RETURNS: 0 if all data written
  //         -1 if otherwise.
  //

  int IOMixer::masterWrite(void *buf, int bytesToWrite)
  {
    DBG_ENTER5("IOMixer::masterWrite");

    int exitCode = -1;

    int written = -1;

    char *src = (char *) buf;

    //
    // Reject if already sent master EOF to remote.
    // It meaans that we decalared - "we will not sent nothing anymore".
    //

    if (masterEofSent_)
    {
      DEBUG3("Master EOF already sent - write rejected.\n");
    }
    else
    {
      DBG_DUMP(buf, bytesToWrite);

      switch(masterOutType_)
      {
        //
        // Master OUT is a SOCKET.
        // Use send().
        //

        case IOMIXER_TYPE_SOCKET:
        {
          while(bytesToWrite > 0)
          {
            DEBUG4("IOMixer::masterWrite : writing [%d] bytes to master FD [%d]...\n",
                         bytesToWrite, masterOut_);

            //
            // Windows.
            //

            #ifdef WIN32
            {
              WSABUF wsabuf = {bytesToWrite, src};

              WSASend(masterOut_, &wsabuf, 1, NULL, 0, &masterOutOV_, NULL);

              WaitForSingleObject(masterOutOV_.hEvent, INFINITE);

              GetOverlappedResult(HANDLE(masterOut_), &masterOutOV_, PDWORD(&written), FALSE);
            }

            //
            // Linux, MacOS.
            //

            #else
            {
              written = send(masterOut_, src, bytesToWrite, 0);
            }
            #endif

            FAIL(written <= 0);

            bytesToWrite -= written;

            src += written;
          }

          break;
        }

        //
        // Master OUT is a CRT FD.
        // Use write().
        //

        case IOMIXER_TYPE_FD:
        {
          while(bytesToWrite > 0)
          {
            DEBUG4("IOMixer::masterWrite : writing [%d] bytes to master FD [%d]...\n",
                        bytesToWrite, masterOut_);

            written = write(masterOut_, src, bytesToWrite);

            FAIL(written <= 0);

            bytesToWrite -= written;

            src += written;
          }

          break;
        }

        //
        // Master OUT is a callback function.
        //

        case IOMIXER_TYPE_CALLBACK:
        {
          while(bytesToWrite > 0)
          {
            written = masterWriteCallback_(src, bytesToWrite, -1, writeCtx_);

            FAIL(written <= 0);

            bytesToWrite -= written;

            src += written;
          }

          break;
        }
      }
    }

    exitCode = 0;

    //
    // Error handler.
    //

    fail:

    if (exitCode && quietMode_ == 0)
    {
      Error("ERROR: Cannot write to master IN.\n"
                "Error code is : %d.\n", GetLastError());
    }

    DBG_LEAVE5("IOMixer::masterWrite");

    return exitCode;
  }

  //
  // Atomic read <size> bytes from master IN to <buf>.
  //
  // Wrapper to system read() to hide difference beetwen socket
  // and CRT FD on windows.
  //
  // buf         - buffer with data to write (IN).
  // bytesToRead - number of bytes to be readed (IN).
  //
  // RETURNS: 0 if all data readed
  //         -1 if otherwise.
  //

  int IOMixer::masterRead(void *buf, int bytesToRead)
  {
    DBG_ENTER5("IOMixer::masterRead");

    int exitCode = -1;

    int readed = -1;

    char *dst = (char *) buf;

    //
    // Reject if we already received master EOF from remote.
    // It means that remote machine declared, that it has no
    // any more data to sent to us.
    //

    FAILEX(masterEofReceived_,
               "ERROR: Master EOF already received - read rejected.\n");

    switch(masterInType_)
    {
      //
      // Master IN is a SOCKET.
      // Use recv().
      //

      case IOMIXER_TYPE_SOCKET:
      {
        while(bytesToRead > 0)
        {
          //
          // Windows.
          //

          #ifdef WIN32
          {
            readed = 0;

            WSANETWORKEVENTS wsaEvents = {0};

            WSAEventSelect(SOCKET(masterIn_),
                               masterInOV_.hEvent,
                                   FD_READ | FD_CLOSE);

            if (WaitForSingleObject(masterInOV_.hEvent, INFINITE) == WAIT_OBJECT_0)
            {
              WSAEnumNetworkEvents(SOCKET(masterIn_),
                                       masterInOV_.hEvent, &wsaEvents);

              if (wsaEvents.lNetworkEvents & FD_READ)
              {
                masterMutex_.lock();

                readed = recv(masterIn_, dst, bytesToRead, 0);

                masterMutex_.unlock();

                FAIL(readed <= 0);
              }
            }
          }

          //
          // Linux, MacOS.
          //

          #else
          {
            readed = recv(masterIn_, dst, bytesToRead, 0);

            FAIL(readed <= 0);
          }
          #endif

          if (readed > 0)
          {
            bytesToRead -= readed;

            dst += readed;
          }
        }

        break;
      }

      //
      // Master IN is a CRT FD.
      // Use read().
      //

      case IOMIXER_TYPE_FD:
      {
        while(bytesToRead > 0)
        {
          readed = read(masterIn_, dst, bytesToRead);

          FAIL(readed <= 0);

          bytesToRead -= readed;

          dst += readed;
        }

        break;
      }

      //
      // Master IN is a callback function.
      //

      case IOMIXER_TYPE_CALLBACK:
      {
        while(bytesToRead > 0)
        {
          readed = masterReadCallback_(dst, bytesToRead, -1, readCtx_);

          FAIL(readed <= 0);

          bytesToRead -= readed;

          dst += readed;
        }

        break;
      }
    }

    if (readed > 0)
    {
      DBG_DUMP(buf, readed);
    }

    exitCode = 0;

    //
    // Error handler.
    //

    fail:

    if (exitCode && quietMode_ == 0)
    {
      Error("ERROR: Cannot read from master IN.\n"
                "Error code is : %d.\n", GetLastError());
    }

    DBG_LEAVE5("IOMixer::masterRead");

    return exitCode;
  }

  //
  // Write <size> bytes from <buf> to slave with id <id>.
  //
  // id   - id of target slave (IN).
  // buf  - source buffer to write (IN).
  // size - number of bytes to be written (IN).
  //
  // RETURNS: 0 if all data written,
  //         -1 otherwise.
  //

  int IOMixer::slaveWrite(int id, void *buf, int size)
  {
    int exitCode = -1;
    int written  = -1;

    //
    // Search for slave with given ID.
    //

    IOMixerSlave *slave = getSlave(id);

    //
    // If exists write <size> bytes of <buf> on its output.
    //

    if (slave)
    {
      if (slave -> eofSent_)
      {
        DEBUG3("EOF already sent on slave ID#%d - write rejected.\n", id);
      }
      else
      {
        slave -> writeMutex_.lock();

        DEBUG4("IOMixer::slaveWrite : Writing [%d] bytes to slave FD [%d]"
                    " ID [%d]...\n", size, slave -> fdout_, id);

        written = write(slave -> fdout_, buf, size);

        slave -> writeMutex_.unlock();

        FAIL(written != size);
      }
    }

    exitCode = 0;

    //
    // Error handler.
    //

    fail:

    if (exitCode && quietMode_ == 0)
    {
      Error("ERROR: Cannot write to slave ID [%d].\n"
                "Error code is : %d.\n", id, GetLastError());
    }

    return exitCode;
  }

  //
  // Shutdown master FD. It sends EOF for ID #0,
  // where ID #0 is reserved for master.
  //

  int IOMixer::shutdown()
  {
    DBG_ENTER("IOMixer::shutdown");

    int exitCode = -1;

    int timeleft = 10;

    map<int, IOMixerSlave *>::iterator it;

    FAIL(dead_);

    dead_ = 1;

    //
    // Flush pending operations.
    //

    // flush();

    //
    // For every slave do:
    //
    // - Send <ID><0> to remote (it means slave #ID EOF).
    //

    slavesMutex_.lock();

    for (it = slaves_.begin(); it != slaves_.end(); it++)
    {
      int id = it -> second -> id_;

      DEBUG4("IOMixer::shutdown : Sending EOF on channel ID#%d...\n", id);

      masterEncode(id, NULL, 0, 0);

      it -> second -> eofSent_ = 1;
    }

    slavesMutex_.unlock();

    //
    // Send <0><0> packet on Master OUT.
    //

    DEBUG4("IOMixer::shutdown : Sending master EOF...\n");

    masterEncode(0, NULL, 0, 0);

    masterEofSent_ = 1;

    //
    // Wait until remote sent its master EOF.
    //

    DEBUG4("IOMixer::shutdown : Waiting for remote master EOF...\n");

    while(masterEofReceived_ == 0 && timeleft > 0)
    {
      #ifdef WIN32
      Sleep(100);
      #else
      usleep(100000);
      #endif

      timeleft --;
    }

    if (timeleft <= 0)
    {
      DEBUG1("IOMixer::shutdown : WARNING: Timeout while"
                 " waiting for remote master EOF.\n");

      masterEofReceived_ = 1;
    }

    //
    // Stop master thread.
    // We don't want to read anything from remote more.
    //

    flush();
    stop();

    //
    // Remove all slaves.
    //

    while(slaves_.size() > 0)
    {
      removeSlave(slaves_.begin() -> second -> id_);
    }

    exitCode = 0;

    //
    // Error handler.
    //

    fail:

    DBG_LEAVE("IOMixer::shutdown");

    return exitCode;
  }

  //
  // Wait until all slave threads finished work.
  //

  int IOMixer::flush()
  {
    DBG_ENTER("IOMixer::flush");

    map<int, IOMixerSlave *>::iterator it;

    //
    // Join slave threads.
    //

    slavesMutex_.lock();

    for (it = slaves_.begin(); it != slaves_.end(); it++)
    {
      IOMixerSlave *sl = slaves_.begin() -> second;

      DEBUG4("IOMixer::flush : flushing slave ID#%d...\n", sl -> id_);

      #ifdef WIN32
      {
        SetEvent(sl -> cancelEvent_);
      }
      #else
      if (write(sl -> cancelFd_[1], "x", 1) <= 0)
      {
        DEBUG1("WARNING: Cannot cancel IO for slave ID#%d.\n", sl -> id_);
      }
      #endif

      if (sl -> thread_)
      {
        ThreadWait(sl -> thread_);
        ThreadClose(sl -> thread_);

        sl -> thread_ = NULL;
      }
    }

    slavesMutex_.unlock();

    DBG_LEAVE("IOMixer::flush");

    return 0;
  }

  //
  // Register callback to inform caller when slave with given ID dead.
  //
  // callback - pointer to function, which will be called if one
  //            of slave dead (IN).
  //
  // ctx      - custom, caller specified data passed directly
  //            into callback (IN/OPT).
  //
  // RETURNS: none.
  //

  void IOMixer::setSlaveDeadCallback(IOSlaveDeadProto callback, void *ctx)
  {
    DEBUG1("IOMixer : Registered slave dead callback [%p][%p].\n", callback, ctx);

    slaveDeadCallback_    = callback;
    slaveDeadCallbackCtx_ = ctx;
  }

  //
  // Enable/disble quiet mode to avoid printint error messages longer.
  //

  void IOMixer::setQuietMode(int value)
  {
    quietMode_ = value;
  }

  //
  // Check is given this pointer points to correct IOMixer *object.
  //
  // ptr - this pointer to check (IN).
  //
  // RETURNS: 1 if given pointer points to correct IOMixer object,
  //          0 otherwise.
  //

  int IOMixer::isPointerCorrect(IOMixer *ptr)
  {
    int found = 0;

    instancesMutex_.lock();
    found = instances_.count(ptr);
    instancesMutex_.unlock();

    return found;
  }

  //
  // Enable or disable IOMIXER_FLAG_COMPRESSION_ON flag on selected
  // channel. After that outcoming data on this channel will be
  // compressed/uncompressed.
  //
  // id      - slave ID to change (IN).
  // enabled - set to 1 to enable compression, 0 to disable (IN).
  //
  // RETURNS: 0 if OK.
  //

  int IOMixer::setSlaveCompression(int id, int enabled)
  {
    int exitCode = -1;

    IOMixerSlave *slave = getSlave(id);

    FAILEX(slave == NULL, "ERROR: Incorrect slave ID#%d.\n", id);

    FAILEX(zlibCompress_ == NULL, "ERROR: ZLib library not available.\n");

    if (enabled)
    {
      slave -> flags_ |= IOMIXER_FLAG_COMPRESSION_ON;
    }
    else
    {
      slave -> flags_ &= ~IOMIXER_FLAG_COMPRESSION_ON;
    }

    DEBUG1("IOMixer : Enabled compression on channel #%d.\n.", id);

    //
    // Error handler.
    //

    exitCode = 0;

    fail:

    if (exitCode)
    {
      Error("ERROR: Cannot enable commpression on channel #%d.\n", id);
    }

    return exitCode;
  }

  //
  // Initialize ZLib library.
  // Called internally only.
  //
  // RETURNS: 0 if OK.
  //

  int IOMixer::initZLib()
  {
    int exitCode = -1;

    #ifdef WIN32
    {
      zlib_ = LoadLibrary("z.dll");

      zlibCompress_      = (ZLibCompressProto) GetProcAddress(zlib_, "compress");
      zlibUncompress_    = (ZLibUncompressProto) GetProcAddress(zlib_, "uncompress");
      zlibCompressBound_ = (ZLibCompressBoundProto) GetProcAddress(zlib_, "compressBound");

    }
    #else
    {
      zlib_ = dlopen("libz.so", RTLD_LAZY);

      zlibCompress_      = (ZLibCompressProto) dlsym(zlib_, "compress");
      zlibUncompress_    = (ZLibUncompressProto) dlsym(zlib_, "uncompress");
      zlibCompressBound_ = (ZLibCompressBoundProto) dlsym(zlib_, "compressBound");
    }
    #endif

    if (zlib_ && zlibCompress_ && zlibUncompress_ && zlibCompressBound_)
    {
      zlibLoaded_ = 1;

      exitCode = 0;

      DEBUG1("IOMixer::IOMixer : ZLib loaded.\n");
    }
    else
    {
      zlibLoaded_ = 0;

      DEBUG1("WARNING: Cannot to load ZLib library. Compression not available.\n");
    }

    return exitCode;
  }

  //
  // Get object name generated in constructor.
  //

  const char *IOMixer::objectName()
  {
    //
    // Check is this pointer correct.
    //

    if (isPointerCorrect(this) == 0)
    {
      Error("ERROR: Called objectName() on non existing IOMixer PTR#%p.\n", this);

      return "Incorrect";
    }
    else
    {
      return objectName_.c_str();
    }
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

  void IOMixer::addRef()
  {
    refCountMutex_.lock();

    refCount_ ++;

    DEBUG2("Increased refference counter to %d for IOMixer PTR#%p.\n",
               refCount_, this);

    refCountMutex_.unlock();
  }

  //
  // Decrease refference counter increased by addRef() before.
  //

  void IOMixer::release()
  {
    int deleteNeeded = 0;

    //
    // Check is this pointer correct.
    //

    if (isPointerCorrect(this) == 0)
    {
      Error("ERROR: Attemp to release non existing IOMixer PTR#%p.\n", this);

      return;
    }

    //
    // Decrease refference counter by 1.
    //

    refCountMutex_.lock();

    refCount_ --;

    DEBUG2("Decreased refference counter to %d for IOMIxer PTR#%p.\n",
               refCount_, this);

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
} /* namespace Tegenaria */
