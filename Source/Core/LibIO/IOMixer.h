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

#ifndef Tegenaria_Core_LibIO_H
#define Tegenaria_Core_LibIO_H

#define IOMIXER_MAX_PACKET (1024 * 64)

#ifdef WIN32
# include <Winsock2.h>
# include <windows.h>
# include <io.h>
#else
# include <unistd.h>
# include <sys/socket.h>
# include <dlfcn.h>
#endif

#include <cstdio>
#include <map>
#include <fcntl.h>
#include <cstring>
#include <set>

#include <Tegenaria/Debug.h>
#include <Tegenaria/Mutex.h>
#include <Tegenaria/Thread.h>

namespace Tegenaria
{
  using std::map;
  using std::set;
  using std::min;
  using std::max;

  #define IOMIXER_TYPE_AUTO     0
  #define IOMIXER_TYPE_FD       1
  #define IOMIXER_TYPE_SOCKET   2
  #define IOMIXER_TYPE_CALLBACK 3

  #define IOMIXER_FLAG_COMPRESSION_ON (1 << 0)
  #define IOMIXER_FLAG_ENCRYPTION_ON  (1 << 1)

  //
  // Typedef.
  //

  typedef int (*IOReadProto)(void *buf, int count, int timeout, void *ctx);
  typedef int (*IOWriteProto)(void *buf, int count, int timeout, void *ctx);
  typedef void (*IOCancelProto)(void *ctx);
  typedef void (*IOSlaveDeadProto)(int id, void *ctx);

  //
  // ZLib typedefs.
  //

  typedef int (*ZLibCompressProto)(void *dest, int *destLen,
                                       const void *source, int sourceLen);

  typedef int (*ZLibUncompressProto)(void *dest, int *destLen,
                                         const void *source, int sourceLen);

  typedef int (*ZLibCompressBoundProto)(int sourceLen);

  //
  // Forward declarations.
  //

  class IOMixer;

  //
  //  --------------     ------------     |   ...   |
  //  |            |     |          |     |         |
  //  |            |     |          |     |         |
  //  |         OUT|     |IN        |     |         |
  //  |         ----------->     -----------> ... ----> MasterOut
  //  |            |     |          |     |         |
  //  |   Caller   |     |   Slave  |     | IOMixer |
  //  |            |     |          |     |         |
  //  |          IN|     |OUT       |     |         |
  //  |          <----------- ... <---------- ... ----- MasterIn
  //  |            |     |          |     |         |
  //  --------------     ------------     |         |
  //                                          ...
  //

  struct IOMixerSlave
  {
    //
    // Slave ID unique locally - inside one IOMixer context.
    // NOT unique globally.
    //

    int id_;

    //
    // Save that remote side already sent <id><0> packet
    // that means input is closed.
    //

    int eofSent_;
    int eofReceived_;

    //
    // Slave in/out sinks.
    //
    // Used by:
    //
    // - IOMixer to communicate with caller.
    // - Caller gets another side of them from 'addSlave()' call.
    //

    int fdin_;
    int fdout_;

    //
    // Event to cancel pending read on Windows.
    //

    #ifdef WIN32

    HANDLE cancelEvent_;

    #else

    //
    // FD to cancel pending read on Linux.
    //

    int cancelFd_[2];

    #endif

    //
    // Save caller side in/out sinks.
    //
    // Used by:
    //
    // - Caller to communicate with master FDs.
    // - IOMixer to simulate writing EOF on flush/shutdown.
    //

    int callerIn_;
    int callerOut_;

    //
    // Mutex to synchronize many slaves writing to one master.
    //

    Mutex writeMutex_;

    #ifdef WIN32
    OVERLAPPED ov_;
    #endif

    //
    // Handle of worker thread, where 'slaveLoop' is running.
    //

    ThreadHandle_t *thread_;

    //
    // Pointer to related IOMixer object.
    //

    IOMixer *this_;

    //
    // Slave flags.
    // See IOMIXER_FLAG_XXX defines.
    //

    uint8_t flags_;
  };

  //
  //
  //

  class IOMixer
  {
    map<int, IOMixerSlave *> slaves_;

    Mutex slavesMutex_;

    int masterIn_;
    int masterOut_;
    int slavesCount_;

    int masterInType_;
    int masterOutType_;

    int dead_;

    int masterEofSent_;
    int masterEofReceived_;

    //
    // Refference counter.
    //

    int refCount_;

    Mutex refCountMutex_;

    //
    // Quiet mode to avoid reporting error longer.
    // Used when connection is shutted down in usual way.
    //

    int quietMode_;

    Mutex masterMutex_;

    ThreadHandle_t *masterThread_;

  #ifdef WIN32
    OVERLAPPED masterInOV_;
    OVERLAPPED masterOutOV_;
  #endif

    //
    // Callback funcions used to read/write from/to master FD.
    //

    void *readCtx_;
    void *writeCtx_;

    IOReadProto masterReadCallback_;
    IOWriteProto masterWriteCallback_;
    IOCancelProto ioCancelCallback_;

    //
    // Optional callbacks to inform caller about events.
    //

    IOSlaveDeadProto slaveDeadCallback_;

    void *slaveDeadCallbackCtx_;

    //
    // Main decode and encode loops.
    // Internal use only.
    //

    static int masterLoop(IOMixer *this_);
    static int slaveLoop(IOMixerSlave *slave);

    //
    // Low-level functions to read/write single
    // <id><size><data> packets from/to master FD.
    //
    // Internal use only.
    //

    int masterEncode(int id, void *data, int size, uint8_t flags);
    int masterDecode(int *id, int *size, void *data, int dataSize);

    //
    // Wrappers for system read/write with master FD to hide
    // differences beetwen SOCKET and CRT FD on Windows.
    //
    // Internal use only.
    //

    int masterRead(void *buf, int size);
    int masterWrite(void *buf, int size);

    //
    // Helper function to write <size> bytes of <data>
    // to slave with id <id>.
    //
    // Internal use only.
    //

    int slaveWrite(int id, void *buf, int size);

    //
    // Save that object is already corectly inited or not.
    //

    int init_;

    //
    // Track created instances to check is given this pointer correct or not.
    //

    static set<IOMixer *> instances_;

    static Mutex instancesMutex_;

    //
    // ZLib library loaded runtime.
    //

    #ifdef WIN32
    HMODULE zlib_;
    #else
    void *zlib_;
    #endif

    int zlibLoaded_;

    ZLibCompressProto zlibCompress_;

    ZLibUncompressProto zlibUncompress_;

    ZLibCompressBoundProto zlibCompressBound_;

    //
    // Object name for debug purpose.
    //

    string objectName_;

    //
    // Public exported functions.
    //

    public:

    IOMixerSlave *getSlave(int id);

    int addSlave(int callerFds[2], int id = -1);

    int setSlaveCompression(int id, int enabled);

    int removeSlave(int id);

    int start();
    int stop();
    int shutdown();
    int flush();
    int join();

    void setSlaveDeadCallback(IOSlaveDeadProto callback, void *ctx);

    //
    // Quiet mode to disable error reporting on usual exit.
    //

    void setQuietMode(int value);

    //
    // Check is given Session *pointer correct.
    //

    static int isPointerCorrect(IOMixer *);

    //
    // Refference counter.
    //

    void addRef();
    void release();

    //
    // Initialize ZLib library.
    //

    int initZLib();

    //
    // Get object name generated in constructor.
    //

    const char *objectName();

    //
    // Constructor using two Socket/FDs.
    //

    IOMixer(int masterIn = 0, int masterOut = 1,
                int masterInType = 0, int masterOutType = 0);

    //
    // Contsructor using caller specified read/write callback functions.
    //

    IOMixer(IOReadProto readCallback, IOWriteProto writeCallback,
                void *readCtx, void *writeCtx, IOCancelProto ioCancelCallback = NULL);

    //
    // Private constructor. Use release().
    //

    private:

    ~IOMixer();
  };

} /* namespace Tegenaria */

#endif /* Tegenaria_Core_LibIO_H */
