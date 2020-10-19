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

#include <cstdio>
#include <stdint.h>
#include <string>
#include <cstdlib>

#include <Tegenaria/Debug.h>
#include <Tegenaria/Str.h>

#ifdef WIN32
# include <windows.h>
# include <io.h>
#else
# include <sys/socket.h>
#endif

#include <fcntl.h>
#include <sys/stat.h>

#include "SftpClient.h"
#include "SftpJob.h"
#include "Sftp.h"
#include "Utils.h"

namespace Tegenaria
{
  using std::string;
  using std::min;

  #ifndef MAX_PATH
  #define MAX_PATH 260
  #endif

  //
  // ----------------------------------------------------------------------------
  //
  //                          Constructors and destructors.
  //
  // ----------------------------------------------------------------------------
  //

  //
  // Create SFTP client over given CRT FD pair.
  //
  // TIP: To initiate session with server on other side
  //      use connect() method.
  //
  // fdin    - FD for input data (IN).
  // fdout   - FD for output data (IN).
  // timeout - I/O timeout in ms, -1 means infinite (IN).
  //
  // fdType  - set SFTP_CLIENT_FD if fdin/fdout are CRT FDs and
  //           SFTP_CLIENT_SOCKET if fdin/fdout are sockets.
  //           Defaulted to SFTP_CLIENT_FD if skipped (IN/OPT).
  //

  SftpClient::SftpClient(int fdin, int fdout, int timeout, int fdType)
  {
    DBG_ENTER3("SftpClient");

    DBG_SET_ADD("SftpClient", this, "%d/%d", fdin, fdout);

    fdin_        = fdin;
    fdout_       = fdout;
    timeout_     = timeout;
    fdType_      = fdType;
    sectorSize_  = SFTP_DEFAULT_SECTOR_SIZE;
    dead_        = 0;

    connectionDroppedCallback_    = NULL;
    connectionDroppedCallbackCtx_ = NULL;

    mutex_.setName("SftpClient");

    //
    // Init request pool.
    //

    rpool_ = new RequestPool(8, "SftpClient");

    //
    // Init network statistics.
    //

    netStatTick_ = SFTP_NETSTAT_DEFAULT_TICK;

    netStatCallback_   = NULL;
    netStatCallbackCtx_ = NULL;

    partialReadThreshold_  = SFTP_PARTIAL_READ_TIMEOUT * 1000;
    partialWriteThreshold_ = SFTP_PARTIAL_WRITE_TIMEOUT * 1000;

    netstat_.reset();

    DBG_LEAVE3("SftpClient");
  }

  //
  // Shutdown connection if any.
  //

  SftpClient::~SftpClient()
  {
    DBG_ENTER3("SftpClient::~SftpClient");

    disconnect();

    delete rpool_;

    DBG_SET_DEL("SftpClient", this);

    DBG_LEAVE3("SftpClient::~SftpClient");
  }

  //
  // ----------------------------------------------------------------------------
  //
  //                        SFTP session negotiation
  //
  // ----------------------------------------------------------------------------
  //

  //
  // Establish connection with sftp-server.
  //
  // Sends   : SSH2_FXP_INIT packet.
  // Expects : SSH2_VERSION packet.
  //
  // RETURNS: 0 if OK.
  //

  int SftpClient::connect()
  {
    DBG_ENTER3("Connect::connect");

    int exitCode = -1;

    string packet;

    //
    // Process SSH2_FXP_INIT packet.
    //

    StrPushDword(packet, 5, STR_BIG_ENDIAN);
    StrPushByte(packet, SSH2_FXP_INIT);
    StrPushDword(packet, SSH2_FILEXFER_VERSION, STR_BIG_ENDIAN);

    FAIL(processPacketSimple(packet, packet));

    DBG_INFO("Established connection with server.\n");

    netstat_.reset();

    //
    // Start thread loop.
    //

    readThread_ = ThreadCreate(readLoop, this);

    //
    // Error handler.
    //

    exitCode = 0;

    fail:

    if (exitCode)
    {
      Error("ERROR: Cannot connect to server.\n");
    }

    DBG_LEAVE3("Connect::connect");

    return exitCode;
  }

  //
  // Disconnect existing connection with sftp server.
  //

  void SftpClient::disconnect()
  {
    DEBUG1("WARNING: SftpClient::disconnect not implemented.");
  }

  //
  // Close existing connection with server and reinit it once again
  // from begin.
  //

  int SftpClient::reconnect()
  {
    Error("ERROR: SftpClient::reconnect is not implemented.\n");

    return -1;
  }

  //
  // ----------------------------------------------------------------------------
  //
  //                   Wrapper for standard SSH2_FXP_XXX commands.
  //
  // ----------------------------------------------------------------------------
  //

  //
  // Open remote file or directory.
  //
  // Sends  : SSH2_FXP_OPEN or SSH2_FXP_OPENDIR packet.
  // Expect : SSH2_FXP_HANDLE or SSSH2_FXP_STATUS packet.
  //
  // path  - path to remote file or directory (IN).
  // mode  - sftp access mode (IN).
  // isDir - set to 1 for directory, 0 otherwise (IN).
  //
  // RETURNS: handle assigned by sftp-server
  //          or -1 if error.
  //

  int64_t SftpClient::open(const char *path, int mode, int isDir)
  {
    DBG_ENTER3("SftpClient::open");

    int exitCode = -1;

    string packet;

    int64_t handle      = -1;
    uint32_t handleSize = -1;

    int pathLen = strlen(path);

    uint32_t id           = GenerateUniqueId();
    uint32_t idRet        = 0;
    uint32_t serverStatus = SSH2_FX_FAILURE;
    uint32_t size         = 0;

    uint8_t type = 0;

    DEBUG1("SFTP #%d: opening %s [%s] mode [0x%x].",
               id, isDir ? "directory" : "file", path, mode);

    FAILEX(dead_, "SFTP #%d: rejected because session dead.\n", id);

    //
    // Prepare SSH2_FXP_OPEN packet:
    //
    // size          4
    // SSH2_FXP_OPEN 1
    // id            4
    // pathLen       4
    // path          pathLen
    // mode          4 (for file only)
    // attribs       4 (for file only)
    // ----------------------------------
    //          total: 17 + 4 + <pathLen>
    //

    if (isDir)
    {
      DEBUG2("SFTP #%d: Preparing SSH2_FXP_OPENDIR packet...", id);

      type = SSH2_FXP_OPENDIR;
      size = 9 + pathLen;
    }
    else
    {
      DEBUG2("SFTP #%d: Preparing SSH2_FXP_OPEN packet...", id);

      type = SSH2_FXP_OPEN;
      size = 17 + pathLen;
    }

    StrPushDword(packet, size, STR_BIG_ENDIAN);     // size    4
    StrPushByte(packet, type);                      // type    1
    StrPushDword(packet, id, STR_BIG_ENDIAN);       // id      4
    StrPushDword(packet, pathLen, STR_BIG_ENDIAN);  // pathLen 4
    StrPushRaw(packet, path, pathLen);              // path    pathLen

    if (isDir == 0)
    {
      StrPushDword(packet, mode, STR_BIG_ENDIAN);   // mode    4
      StrPushDword(packet, 0, STR_BIG_ENDIAN);      // attr    4
    }

    //
    // Send packet and wait for answer.
    //

    FAIL(processPacket(packet, packet));

    //
    // Parse answer.
    //
    // size 4
    // type 1
    // id   4
    // ...
    //

    FAIL(StrPopDword(&size, packet, STR_BIG_ENDIAN));  // size 4
    FAIL(StrPopByte(&type, packet));                   // type 1
    FAIL(StrPopDword(&idRet, packet, STR_BIG_ENDIAN)); // id   4

    if (idRet != id)
    {
      Error("ERROR: Packet ID mismatch.\n");

      shutdown();

      goto fail;
    }

    //
    // Check packet type.
    //

    switch(type)
    {
      //
      // SSH2_FXP_HANDLE with opened server side handle.
      // It means success.
      //
      // handleSize 4
      // handle     handleSize
      //

      case SSH2_FXP_HANDLE:
      {
        uint32_t tmp;

        DEBUG2("SFTP #%d: Received [SSH2_FXP_HANDLE].", id);

        //
        // FIXME: Handle 64-bit handle.
        //

        FAIL(StrPopDword(&handleSize, packet, STR_BIG_ENDIAN));

        FAILEX(handleSize != 4, "ERROR: Unsupported handle size [%d].\n", handleSize);

        FAIL(StrPopDword(&tmp, packet, STR_BIG_ENDIAN));

        handle = tmp;

        DEBUG1("SFTP #%d: Opened [%s] as remote handle [%"PRId64"].\n", id, path, handle);

        break;
      }

      //
      // SSH2_FXP_STATUS feans errors on server side.
      // Probably access denied.
      //

      case SSH2_FXP_STATUS:
      {
        DEBUG2("SFTP #%d: Received [SSH2_FXP_STATUS].", id);

        FAIL(StrPopDword(&serverStatus, packet, STR_BIG_ENDIAN));

        DEBUG1("SFTP #%d: Open [%s] failed with server code [%d][%s].\n",
                   id, path, serverStatus, TranslateSftpStatus(serverStatus));

        break;
      }

      //
      // Fatal error. Unexpected packet type.
      //

      default:
      {
        Error("ERROR: Unexpected packet type [%d].\n", type);

        shutdown();

        goto fail;
      }
    }

    if (handle >= 0)
    {
      DBG_SET_ADD("sftp handle", handle, "[%s]", path);
    }

    //
    // Cleanup.
    //

    exitCode = 0;

    fail:

    if (exitCode < 0)
    {
      DBG_MSG("Cannot open remote resource.\n"
                  "Error code is: %u.\nServer status is : [%d][%s].",
                      GetLastError(), serverStatus,
                          TranslateSftpStatus(serverStatus));

      handle = -1;
    }

    DBG_LEAVE3("SftpClient::open");

    return handle;
  }

  //
  // Open remote directory.
  // Works as open with isDir set to 1.
  // See SftpClient::open().
  //

  int64_t SftpClient::opendir(const char *path)
  {
    return open(path, 0, 1);
  }

  //
  // Close remote handle on server.
  //
  // Sends  : SSH2_FXP_CLOSE packet.
  // Expect : SSH2_FXP_STATUS packet.
  //
  // handle - handle value retrieved from open() before (IN).
  //
  // RETURNS: 0 if OK.
  //

  int SftpClient::close(int64_t handle)
  {
    DBG_ENTER3("SftpClient::close");

    uint32_t serverStatus = SSH2_FX_FAILURE;

    uint32_t id    = GenerateUniqueId();
    uint32_t size  = 0;
    uint32_t idRet = 0;

    uint8_t type = 0;

    string packet;

    DEBUG1("SFTP #%d: close handle [%"PRId64"].", id, handle);

    FAILEX(dead_, "SFTP #%d: rejected because session dead.\n", id);

    //
    // Prepare SSH2_FXP_CLOSE packet:
    //
    // size           4
    // SSH2_FXP_CLOSE 1
    // id             4
    // handleLen      4
    // handle         4
    // ----------------------------------
    //         total: 13 + 4
    //

    DEBUG2("SFTP #%d: Preparing SSH2_FXP_CLOSE packet...", id);

    StrPushDword(packet, 13, STR_BIG_ENDIAN);               // size      4
    StrPushByte(packet, SSH2_FXP_CLOSE);                    // type      1
    StrPushDword(packet, id, STR_BIG_ENDIAN);               // id        4
    StrPushDword(packet, 4, STR_BIG_ENDIAN);                // handleLen 4
    StrPushDword(packet, uint32_t(handle), STR_BIG_ENDIAN); // handle    4

    //
    // Send packet and wait for answer.
    //

    FAIL(processPacket(packet, packet));

    //
    // Receive status packet.
    //

    serverStatus = popStatusPacket(packet, id);

    FAIL(serverStatus != SSH2_FX_OK);

    DBG_SET_DEL("sftp handle", handle);

    //
    // Error handler.
    //

    fail:

    DEBUG1("SFTP #%d: Close handle [%"PRId64"] finished with code [%d].\n",
               id, handle, serverStatus);

    DBG_LEAVE3("SftpClient::close");

    return serverStatus;
  }

  //
  // Close many remote handles in one request.
  //
  // Sends  : SSH2_FXP_DIRLIGO_MULTICLOSE packet.
  // Expect : SSH2_FXP_STATUS packet.
  //
  // handle - list of handles value retrieved from open() before (IN).
  //
  // RETURNS: 0 if OK.
  //

  int SftpClient::multiclose(vector<int64_t> &handles)
  {
    DBG_ENTER3("SftpClient::multiclose");

    uint32_t serverStatus = SSH2_FX_FAILURE;

    uint32_t id    = GenerateUniqueId();
    uint32_t size  = 0;
    uint32_t idRet = 0;

    uint32_t packetSize;

    uint8_t type = 0;

    string packet;

    #ifdef DEBUG
    {
      string handlesListStr;

      for (int i = 0; i < handles.size(); i++)
      {
        char buf[32];

        snprintf(buf, sizeof(buf) - 1, "%I64d, ", handles[i]);

        handlesListStr += buf;
      }

      DEBUG1("SFTP #%d: multi-close {%s}.", id, handlesListStr.c_str());
    }
    #endif

    FAILEX(dead_, "SFTP #%d: rejected because session dead.\n", id);

    //
    // Don't run request if handles lilst is empty.
    //

    if (handles.size() > 0)
    {
      //
      // Prepare SSH2_FXP_DIRLIGO_MULTICLOSE packet:
      //
      // size                4
      // SSH2_FXP_MULTICLOSE 1
      // id                  4
      // count               4
      // handle              4 x n times
      // ----------------------------------
      //         total: 13 + 4 * nHandles
      //

      DEBUG2("SFTP #%d: Preparing SSH2_FXP_MULTICLOSE packet...", id);

      packetSize = 9 + 4 * handles.size();

      StrPushDword(packet, packetSize, STR_BIG_ENDIAN);       // size      4
      StrPushByte(packet, SSH2_FXP_DIRLIGO_MULTICLOSE);       // type      1
      StrPushDword(packet, id, STR_BIG_ENDIAN);               // id        4
      StrPushDword(packet, handles.size(), STR_BIG_ENDIAN);   // count     4

      for (int i = 0; i < handles.size(); i++)
      {
        StrPushDword(packet, uint32_t(handles[i]), STR_BIG_ENDIAN); // handle    4
      }

      //
      // Send packet and wait for answer.
      //

      FAIL(processPacket(packet, packet));

      //
      // Receive status packet.
      //

      serverStatus = popStatusPacket(packet, id);

      FAIL(serverStatus != SSH2_FX_OK);

      for (int i = 0; i < handles.size(); i++)
      {
        DBG_SET_DEL("sftp handle", handles[i]);
      }
    }

    //
    // Error handler.
    //

    fail:

    DEBUG1("SFTP #%d: multi-close finished with code [%d].\n", id, serverStatus);

    DBG_LEAVE3("SftpClient::multiclose");

    return serverStatus;
  }

  //
  // Reset directory handle. If readdir() finished with EOF on given handle
  // resetdir() request will reopen directory on server to make the same handle
  // possible to reuse in another readdir() request.
  //
  // Sends  : SSH2_FXP_DIRLIGO_RESETDIR packet.
  // Expect : SSH2_FXP_STATUS packet.
  //
  // handle - handle value retrieved from opendir() before (IN).
  //
  // RETURNS: 0 if OK.
  //

  int SftpClient::resetdir(int64_t handle)
  {
    DBG_ENTER3("SftpClient::resetdir");

    uint32_t serverStatus = SSH2_FX_FAILURE;

    uint32_t id    = GenerateUniqueId();
    uint32_t size  = 0;
    uint32_t idRet = 0;

    uint32_t packetSize;

    uint8_t type = 0;

    string packet;

    DEBUG1("SFTP #%d: resetdir [%I64d].", id, handle);

    FAILEX(dead_, "SFTP #%d: rejected because session dead.\n", id);

    //
    // Prepare SSH2_FXP_DIRLIGO_RESETDIR packet:
    //
    // size              4
    // SSH2_FXP_RESETDIR 1
    // id                4
    // handle            4
    // ----------------------------------
    //           total: 13
    //

    DEBUG2("SFTP #%d: Preparing SSH2_FXP_RESETDIR packet...", id);

    StrPushDword(packet, 9, STR_BIG_ENDIAN);                // size   4
    StrPushByte(packet, SSH2_FXP_DIRLIGO_RESETDIR);         // type   1
    StrPushDword(packet, id, STR_BIG_ENDIAN);               // id     4
    StrPushDword(packet, int32_t(handle), STR_BIG_ENDIAN);  // handle 4

    //
    // Send packet and wait for answer.
    //

    FAIL(processPacket(packet, packet));

    //
    // Receive status packet.
    //

    serverStatus = popStatusPacket(packet, id);

    FAIL(serverStatus != SSH2_FX_OK);

    //
    // Error handler.
    //

    fail:

    DEBUG1("SFTP #%d: resetdir finished with code [%d].\n", id, serverStatus);

    DBG_LEAVE3("SftpClient::resetdir");

    return serverStatus;
  }

  //
  // Read data from remote file.
  //
  // Sends  : many SSH2_FXP_READ.
  // Expect : many SSH2_FXP_DATA and one SSH2_FXP_STATUS for EOF signal.
  //
  // handle      - handle retrieved from open() before (IN).
  // buffer      - buffer, where to store readed data (OUT).
  // offset      - file position of first byte to read (IN).
  // bytesToRead - number of bytes to read (IN).
  //
  // RETURNS: Number of bytes readed,
  //          or -1 if error.
  //

  int SftpClient::read(int64_t handle, char *buffer, uint64_t offset, int bytesToRead)
  {
    DBG_ENTER3("SftpClient::read");

    int exitCode = -1;

    int goOn   = 1;
    int readed = 0;

    double startTime    = 0.0;
    double endTime      = 0.0;
    double elapsed      = 0.0;
    double elapsedTotal = 0.0;

    int pieceSize = 0;

    uint32_t serverStatus = 0;

    string packet;

    DEBUG1("SFTP: read(%"PRId64", %p, %"PRId64", %d).", handle, buffer, offset, bytesToRead);

    FAILEX(dead_, "SFTP: read() rejected because session dead.\n");

    //
    // Read packets unitil:
    //
    // - EOF.
    // - error.
    // - read time can exceed system I/O timeout
    //   (for big caller buffer or slow network).
    //

    while(goOn)
    {
      packet.clear();

      uint32_t id    = GenerateUniqueId();
      uint32_t idRet = 0;
      uint32_t size  = 0;

      uint8_t type = 0;

      //
      // Compute size of current piece.
      //

      pieceSize = min(bytesToRead - readed, sectorSize_);

      //
      // Prepare next SSH2_FXP_READ message.
      //
      // size        4
      // type        1
      // id          4
      // handleLen   4
      // handle      4 (32-bit)
      // offset      8 (64-bit)
      // bytesToRead 4
      // -----------------------
      //      total: 25 + 4
      //

      DEBUG3("SFTP #%d: Preparing SSH2_FXP_READ packet...\n", id);

      StrPushDword(packet, 25, STR_BIG_ENDIAN);                   // size      4
      StrPushByte(packet, SSH2_FXP_READ);                         // type      1
      StrPushDword(packet, id, STR_BIG_ENDIAN);                   // id        4
      StrPushDword(packet, 4, STR_BIG_ENDIAN);                    // handleLen 4
      StrPushDword(packet, uint32_t(handle), STR_BIG_ENDIAN);     // handle    4
      StrPushQword(packet, offset, STR_BIG_ENDIAN);               // offset    8
      StrPushDword(packet, pieceSize, STR_BIG_ENDIAN);            // toRead    4

      //
      // Send SSH2_FXP_READ packet.
      // Expect SSH2_FXP_DATA or SSH2_FXP_STATUS.
      //

      startTime = GetTimeMs();

      FAIL(processPacket(packet, packet));

      endTime = GetTimeMs();

      elapsed = endTime - startTime;

      DEBUG2("SFTP #%d: Sent [SSH2_FXP_READ] packet.\n", id);

      //
      // Pop header:
      //
      // size 4
      // type 1
      // id   4
      //

      FAIL(StrPopDword(&size, packet, STR_BIG_ENDIAN));  // size 4
      FAIL(StrPopByte(&type, packet));                   // type 1
      FAIL(StrPopDword(&idRet, packet, STR_BIG_ENDIAN)); // id   4

      if (idRet != id)
      {
        Error("ERROR: Packet ID mismatch.\n");

        shutdown();

        goto fail;
      }

      //
      // Parse reveived packet.
      //

      switch(type)
      {
        //
        // Next portion of readed data.
        //

        case SSH2_FXP_DATA:
        {
          DEBUG2("SFTP #%d: received [SSH2_FXP_DATA].", id);

          uint32_t len = 0;

          //
          // Put reveived data to output buffer.
          //

          FAIL(StrPopDword(&len, packet, STR_BIG_ENDIAN));

          DEBUG3("SFTP #%d: Excepting [%d] data bytes...", id, len);

          if (packet.size() < len)
          {
            Error("ERROR: Received [%d] data, but [%d] expected.", packet.size(), len);

            shutdown();

            goto fail;
          }

          memcpy(buffer, &packet[0], len);

          readed += len;
          buffer += len;
          offset += len;

          //
          // If we readed all expected data stop listening.
          //

          if (readed == bytesToRead)
          {
            DEBUG3("SFTP #%d: Reading completed.", id);

            goOn = 0;
          }

          //
          // Update statistics.
          //

          netstat_.insertDownloadEvent(len, elapsed);

          break;
        }

        //
        // Status message means EOF or error.
        //

        case SSH2_FXP_STATUS:
        {
          DEBUG2("SFTP #%d: received [SSH2_FXP_STATUS].\n", id);

          FAIL(StrPopDword(&serverStatus, packet, STR_BIG_ENDIAN));

          FAILEX(serverStatus != SSH2_FX_EOF,
                     "Read failed with server code [%d][%s].\n",
                         serverStatus, TranslateSftpStatus(serverStatus));

          goOn = 0;

          break;
        }

        //
        // Unexpected packet's type.
        //

        default:
        {
          Error("SFTP #%d: ERROR: Unexpected packet type received [%d].", id, type);

          shutdown();

          goto fail;
        }
      }

      //
      // Check for timeout.
      //

      elapsedTotal += elapsed;

      if (elapsedTotal > partialReadThreshold_)
      {
        Error("WARNING: Partial read triggered after %lf seconds.\n", elapsedTotal / 1000.0);

        netstat_.triggerPartialRead();

        if (netStatCallback_)
        {
          netStatCallback_(&netstat_, netStatCallbackCtx_);
        }

        goOn = 0;
      }
    }

    //
    // Clean up.
    //

    exitCode = 0;

    fail:

    if (exitCode)
    {
      Error("ERROR: Cannot read from remote file. Error code is: %u.\n"
                "Server status: is [%d][%s].", GetLastError(),
                    serverStatus, TranslateSftpStatus(serverStatus));
    }

    DBG_LEAVE3("SftpClient::read");

    return exitCode ? -1 : readed;
  }

  //
  // Write data to remote file.
  //
  // Sends  : many SSH2_FXP_WRITE packets.
  // Expect : many SSH2_FXP_STATUS packets.
  //
  // handle       - handle retrieved from open() before (IN).
  // buffer       - buffer with data, which we want to write (IN).
  // offset       - file position, where to start writing (IN).
  // bytesToWrite - number of bytes to write (IN).
  //
  // RETURNS: Number of bytes written,
  //          or -1 if error.
  //

  int SftpClient::write(int64_t handle, char *buffer, uint64_t offset, int bytesToWrite)
  {
    DBG_ENTER3("SftpClient::write");

    int exitCode = -1;

    int written   = 0;
    int pieceSize = 0;

    int goOn    = 1;

    double startTime    = 0.0;
    double endTime      = 0.0;
    double elapsed      = 0.0;
    double elapsedTotal = 0.0;

    uint32_t serverStatus = 0;

    string packet;

    DEBUG1("SFTP: write(%"PRId64", %p, %"PRId64", %d).", handle, buffer, offset, bytesToWrite);

    FAILEX(dead_, "SFTP: write() rejected because session dead.\n");

    //
    // Write packets until all data sent or error.
    //

    while(goOn)
    {
      uint32_t id    = GenerateUniqueId();
      uint32_t idRet = 0;
      uint32_t size  = 0;

      uint8_t type = 0;

      //
      // Compute size of current piece.
      //

      pieceSize = min(bytesToWrite - written, sectorSize_);

      //
      // Prepare next SSH2_FXP_WRITE message.
      //
      // size       4
      // type       1
      // id         4
      // handleLen  4
      // handle     4 (32-bit)
      // offset     8 (64-bit)
      // pieceSize  4
      // data       pieceSize
      // -----------------------
      //     total: 25 + 4 + pieceSize
      //

      DEBUG3("SFTP #%d: Preparing SSH2_FXP_WRITE packet...\n", id);

      packet.clear();

      StrPushDword(packet, 25 + pieceSize, STR_BIG_ENDIAN);       // size      4
      StrPushByte(packet, SSH2_FXP_WRITE);                        // type      1
      StrPushDword(packet, id, STR_BIG_ENDIAN);                   // id        4
      StrPushDword(packet, 4, STR_BIG_ENDIAN);                    // handleLen 4
      StrPushDword(packet, uint32_t(handle), STR_BIG_ENDIAN);     // handle    4
      StrPushQword(packet, offset, STR_BIG_ENDIAN);               // offset    8
      StrPushDword(packet, pieceSize, STR_BIG_ENDIAN);            // pieceSize 4

      packet.resize(packet.size() + pieceSize);

      memcpy(&packet[29], buffer, pieceSize);

      //
      // Send SSH2_FXP_WRITE packet.
      // Expect SSH2_FXP_STATUS.
      //

      startTime = GetTimeMs();

      FAIL(processPacket(packet, packet));

      endTime = GetTimeMs();

      DEBUG2("SFTP #%d: Sent [SSH2_FXP_WRITE] packet.\n", id);

      //
      // Pop header:
      //
      // size   4
      // type   1
      // id     4
      // status 4
      //

      FAIL(StrPopDword(&size, packet, STR_BIG_ENDIAN));         // size   4
      FAIL(StrPopByte(&type, packet));                          // type   1
      FAIL(StrPopDword(&idRet, packet, STR_BIG_ENDIAN));        // id     4
      FAIL(StrPopDword(&serverStatus, packet, STR_BIG_ENDIAN)); // status 4

      //
      // Check packet ID.
      //

      if (idRet != id)
      {
        Error("ERROR: Packet ID mismatch.\n");

        shutdown();

        goto fail;
      }

      //
      // Check packet type.
      //

      if (type != SSH2_FXP_STATUS)
      {
        Error("ERROR: Unexpected packet type [%d]"
                  " when [%d] expected.\n", type, SSH2_FXP_STATUS);

        shutdown();

        goto fail;
      }

      FAIL(serverStatus != SSH2_FX_OK);

      //
      // Move to next piece to send.
      //

      written += pieceSize;
      buffer  += pieceSize;
      offset  += pieceSize;

      //
      // Update statistics.
      //

      elapsed = endTime - startTime;

      netstat_.insertUploadEvent(pieceSize, elapsed);

      //
      // Check for timeout.
      //

      elapsedTotal += elapsed;

      if (elapsedTotal > partialWriteThreshold_)
      {
        Error("WARNING: Partial write triggered after %lf seconds.\n", elapsedTotal / 1000.0);

        netstat_.triggerPartialWrite();

        if (netStatCallback_)
        {
          netStatCallback_(&netstat_, netStatCallbackCtx_);
        }

        goOn = 0;
      }

      //
      // Check do we still have some to write.
      //

      if (written >= bytesToWrite)
      {
        goOn = 0;
      }
    }

    //
    // Clean up.
    //

    exitCode = 0;

    fail:

    if (exitCode)
    {
      Error("ERROR: Cannot write to remote file. Error code is: %u.\n"
                "Server status is: [%d][%s].", GetLastError(),
                    serverStatus, TranslateSftpStatus(serverStatus));
    }

    DBG_LEAVE3("SftpClient::write");

    return exitCode ? -1 : written;
  }

  //
  // Append data to remote file.
  //
  // Sends  : many SSH2_FXP_DIRLIGO_APPEND packets.
  // Expect : many SSH2_FXP_STATUS packets.
  //
  // handle       - handle retrieved from open() before (IN).
  // buffer       - buffer with data, which we want to write (IN).
  // bytesToWrite - number of bytes to write (IN).
  //
  // RETURNS: Number of bytes written,
  //          or -1 if error.
  //

  int SftpClient::append(int64_t handle, char *buffer, int bytesToWrite)
  {
    DBG_ENTER3("SftpClient::append");

    int exitCode = -1;

    Error("SftpClient::append() not implemented.\n");

    DBG_LEAVE3("SftpClient::append");

    return exitCode;
  }

  #ifdef WIN32

  //
  // Windows implementation of stat() command.
  // Check is file exists and optionally retrieve file attributes.
  //
  // Sends   : SSH2_FXP_STAT_VERSION_0 packet.
  // Expects : SSH2_FXP_ATTR or SSH2_FXP_STATUS.
  //
  // path - full remote path on server (IN).
  // info - info about remote file. (OUT).
  //
  // RETURNS: 0 if OK.
  //

  int SftpClient::stat(const char *path, BY_HANDLE_FILE_INFORMATION *info)
  {
    DBG_ENTER3("SftpClient::stat");

    int exitCode = -1;

    uint32_t serverStatus = 0;
    uint32_t id           = GenerateUniqueId();
    uint32_t idRet        = 0;
    uint32_t pathLen      = 0;
    uint32_t size         = 0;

    uint8_t type = 0;

    string packet;

    DEBUG1("SFTP #%d: stat(%s).", id, path);

    FAILEX(dead_, "SFTP #%d: rejected because session dead.\n", id);

    //
    // Check args.
    //

    FAILEX(path == NULL, "ERROR: Null 'path' passed to SftpClient::stat().\n");
    FAILEX(info == NULL, "ERROR: Null 'info' passed to SftpClient::stat().\n");

    //
    // Prepare SSH2_FXP_STAT_VERSION_0 packet.
    //
    // size       4
    // type       1
    // id         4
    // pathLen    4
    // path       pathLen
    // -----------------------
    //     total: 9 + pathLen + 4
    //

    DEBUG3("SFTP #%d: preparing SSH2_FXP_STAT_VERSION_0 packet...", id);

    pathLen = strlen(path);

    StrPushDword(packet, 9 + pathLen, STR_BIG_ENDIAN); // size      4
    StrPushByte(packet, SSH2_FXP_STAT_VERSION_0);      // type      1
    StrPushDword(packet, id, STR_BIG_ENDIAN);          // id        4
    StrPushDword(packet, pathLen, STR_BIG_ENDIAN);     // pathLen   4
    StrPushRaw(packet, path, pathLen);                 // path      pathLen

    //
    // Send packet, wait for answer.

    FAIL(processPacket(packet, packet));

    DEBUG2("SFTP #%d: [SSH2_FXP_STAT_VERSION_0][%s] packet.\n", id, path);

    //
    // Pop header:
    //
    // size   4
    // type   1
    // id     4
    // status 4
    //

    FAIL(StrPopDword(&size, packet, STR_BIG_ENDIAN));         // size   4
    FAIL(StrPopByte(&type, packet));                          // type   1
    FAIL(StrPopDword(&idRet, packet, STR_BIG_ENDIAN));        // id     4

    if (idRet != id)
    {
      Error("ERROR: Packet ID mismatch.\n");

      shutdown();

      goto fail;
    }

    //
    // Process received answer.
    //

    switch(type)
    {
      //
      // SSH2_FXP_ATTR message. We should get resource's
      // attributes here.
      //

      case SSH2_FXP_ATTRS:
      {
        DEBUG2("SFTP #%d: received [SSH2_FXP_ATTRS].", id);

        //
        // If extended request decode reveived attributes.
        //

        if (info)
        {
          FAIL(popAttribs(info, packet));
        }

        DEBUG1("SFTP #%d: Stat [%s] finished with success.", id, path);

        break;
      }

      //
      // SSH2_FXP_STATUS message. This means error or resource
      // not exists.
      //

      case SSH2_FXP_STATUS:
      {
        DBG_MSG("SFTP #%d: received [SSH2_FXP_STATUS].", id);

        FAIL(StrPopDword(&serverStatus, packet, STR_BIG_ENDIAN));

        FAIL(serverStatus != SSH2_FX_OK);

        break;
      }

      //
      // Unexpected message type.
      //

      default:
      {
        Error("SFTP #%d: ERROR: Unexpected message type [0x%x].", id, type);

        shutdown();

        goto fail;
      }
    }

    //
    // Clean up.
    //

    exitCode = 0;

    fail:

    if (exitCode)
    {
      Error("Cannot stat remote file.\n"
                "Error code is: %u.\nServer status is: [%d][%s].",
                      GetLastError(), serverStatus,
                          TranslateSftpStatus(serverStatus));
    }

    DBG_LEAVE3("SftpClient::stat");

    return exitCode;
  }

  //
  // Windows implementation of readdir().
  // List content of remote directory.
  //
  // Sends   : many SSH2_FXP_DIRLIGO_READDIR_SHORT packets.
  // Expects : many SSH2_FXP_NAME and one SSH2_FXP_STATUS for EOF.
  //
  // data   - list of files/dirs living inside selected directory (OUT).
  // handle - value retrieved from open() function before (IN).
  //
  // RETURNS: 0 if OK.
  //

  int SftpClient::readdir(vector<WIN32_FIND_DATAW> &data, int64_t handle)
  {
    DBG_ENTER3("SftpClient::readdir");

    int exitCode = -1;
    int goOn     = 1;
    int capacity = 0;
    int iter     = 0;

    uint32_t serverStatus = 0;

    string packet;

    data.clear();

    DEBUG1("SFTP: readdir(%"PRId64").", handle);

    FAILEX(dead_, "SFTP: readdir() rejected because session dead.\n");

    //
    // Send requests for data until EOF or error.
    //

    while(goOn)
    {
      uint32_t id    = GenerateUniqueId();
      uint32_t idRet = 0;
      uint32_t size  = 0;
      uint32_t count = 0;

      uint8_t type = 0;

      iter ++;

      //
      // Prepare SSH2_FXP_DIRLIGO_READDIR_SHORT message.
      //
      //
      // size      4
      // type      1
      // id        4
      // handleLen 4
      // handle    4
      // ----------------------------------
      //    total: 13 + 4
      //

      packet.clear();

      StrPushDword(packet, 13, STR_BIG_ENDIAN);               // size      4
      StrPushByte(packet, SSH2_FXP_DIRLIGO_READDIR_SHORT);    // type      1
      StrPushDword(packet, id, STR_BIG_ENDIAN);               // id        4
      StrPushDword(packet, 4, STR_BIG_ENDIAN);                // handleLen 4
      StrPushDword(packet, uint32_t(handle), STR_BIG_ENDIAN); // handle    4

      //
      // Send packet and wait for answer.
      //

      FAIL(processPacket(packet, packet));

      DEBUG2("SFTP #%d: Sent [SSH2_FXP_DIRLIGO_READDIR_SHORT][%"PRId64"].\n", id, handle);

      //
      // Pop header:
      //
      // size 4
      // type 1
      // id   4
      //

      FAIL(StrPopDword(&size, packet, STR_BIG_ENDIAN));  // size 4
      FAIL(StrPopByte(&type, packet));                   // type 1
      FAIL(StrPopDword(&idRet, packet, STR_BIG_ENDIAN)); // id   4

      if (idRet != id)
      {
        Error("ERROR: Packet ID mismatch.\n");

        shutdown();

        goto fail;
      }

      //
      // Interprete received message.
      //

      switch(type)
      {
        //
        // Status message. This means EOF or error.
        //

        case SSH2_FXP_STATUS:
        {
          DEBUG2("SFTP #%d: received [SSH2_FXP_STATUS].", id);

          goOn = 0;

          FAIL(StrPopDword(&serverStatus, packet, STR_BIG_ENDIAN));

          FAIL(serverStatus != SSH2_FX_EOF);

          DEBUG1("SFTP #%d: Readdir handle [%"PRId64"] finished with code [%d][%s].\n",
                     id, handle, serverStatus, TranslateSftpStatus(serverStatus));

          break;
        }

        //
        // SSH2_FXP_NAME. Next portion of data.
        //

        case SSH2_FXP_NAME:
        {
          DEBUG2("SFTP #%d: received [SSH2_FXP_NAME].", id);

          //
          // Pop number of elements.
          //

          FAIL(StrPopDword(&count, packet, STR_BIG_ENDIAN));

          DEBUG2("SFTP #%d: Popped elements count [%d].\n", id, count);

          //
          // Pop <count> elements from packet.
          //

          for (int i = 0; i < count; i++)
          {
            WIN32_FIND_DATAW buf = {0};

            BY_HANDLE_FILE_INFORMATION info = {0};

            char name[MAX_PATH]     = {0};
            char longName[MAX_PATH] = {0};

            uint32_t nameLen     = 0;
            uint32_t longNameLen = 0;

            //
            // Pop element's name.
            //

            FAIL(StrPopDword(&nameLen, packet, STR_BIG_ENDIAN));
            FAIL(StrPopRaw(name, nameLen, packet));

            FAIL(StrPopDword(&longNameLen, packet, STR_BIG_ENDIAN));
            FAIL(StrPopRaw(longName, longNameLen, packet));

            DEBUG2("SFTP #%d: Popped element [%s] / [%s].\n", id, name, longName);

            //
            // We expecting UTF8 from network.
            // Convert UTF8 to UTF16.
            //

            MultiByteToWideChar(CP_UTF8, 0, name, -1,
                                    buf.cFileName, sizeof(buf.cFileName));

            //
            // Pop attributes.
            //

            FAIL(popAttribs(&info, packet));

            buf.dwFileAttributes = info.dwFileAttributes;
            buf.ftCreationTime   = info.ftCreationTime;
            buf.ftLastAccessTime = info.ftLastAccessTime;
            buf.ftLastWriteTime  = info.ftLastWriteTime;
            buf.nFileSizeHigh    = info.nFileSizeHigh;
            buf.nFileSizeLow     = info.nFileSizeLow;

            //
            // Last entry marker. Don't go on.
            //

            if (strcmp(name, "...") == 0)
            {
              goOn = 0;
            }

            //
            // Put another element to list.
            //

            else
            {
              data.push_back(buf);
            }
          }

          break;
        }

        //
        // Unexpected packet type. Break.
        //

        default:
        {
          Error("SFTP #%d: ERROR: Unexpected packet type [%d].", id, type);

          shutdown();

          goto fail;
        }
      }
    }

    DEBUG2("SFTP: %d elements listed.", data.size());

    //
    // Cleanup.
    //

    exitCode = 0;

    fail:

    if (exitCode)
    {
      Error("ERROR: Cannot read remote directory.\n"
                " Server status is: [%d][%s].", serverStatus,
                    TranslateSftpStatus(serverStatus));

    }

    DBG_LEAVE3("SftpClient::readdir");

    return exitCode;
  }

  #endif /* WIN32 */

  // Generic implementation of stat() command.
  // Check is file exists and optionally retrieve file attributes.
  //
  // Sends   : SSH2_FXP_STAT_VERSION_0 packet.
  // Expects : SSH2_FXP_ATTR or SSH2_FXP_STATUS.
  //
  // path - full remote path on server (IN).
  // attr - info about remote file. (OUT).
  //
  // RETURNS: 0 if OK.
  //

  int SftpClient::stat(const char *path, SftpFileAttr *attr)
  {
    DBG_ENTER3("SftpClient::stat");

    int exitCode = -1;

    uint32_t serverStatus = 0;
    uint32_t id           = GenerateUniqueId();
    uint32_t idRet        = 0;
    uint32_t pathLen      = 0;
    uint32_t size         = 0;

    uint8_t type = 0;

    string packet;

    DEBUG1("SFTP #%d: stat(%s).", id, path);

    FAILEX(dead_, "SFTP #%d: rejected because session dead.\n", id);

    //
    // Check args.
    //

    FAILEX(path == NULL, "ERROR: Null 'path' passed to SftpClient::stat().\n");
    FAILEX(attr == NULL, "ERROR: Null 'attr' passed to SftpClient::stat().\n");

    //
    // Prepare SSH2_FXP_STAT_VERSION_0 packet.
    //
    // size       4
    // type       1
    // id         4
    // pathLen    4
    // path       pathLen
    // -----------------------
    //     total: 9 + pathLen + 4
    //

    DEBUG3("SFTP #%d: preparing SSH2_FXP_STAT_VERSION_0 packet...", id);

    pathLen = strlen(path);

    StrPushDword(packet, 9 + pathLen, STR_BIG_ENDIAN); // size      4
    StrPushByte(packet, SSH2_FXP_STAT_VERSION_0);      // type      1
    StrPushDword(packet, id, STR_BIG_ENDIAN);          // id        4
    StrPushDword(packet, pathLen, STR_BIG_ENDIAN);     // pathLen   4
    StrPushRaw(packet, path, pathLen);                 // path      pathLen

    //
    // Send packet, wait for answer.

    FAIL(processPacket(packet, packet));

    DEBUG2("SFTP #%d: [SSH2_FXP_STAT_VERSION_0][%s] packet.\n", id, path);

    //
    // Pop header:
    //
    // size   4
    // type   1
    // id     4
    // status 4
    //

    FAIL(StrPopDword(&size, packet, STR_BIG_ENDIAN));         // size   4
    FAIL(StrPopByte(&type, packet));                          // type   1
    FAIL(StrPopDword(&idRet, packet, STR_BIG_ENDIAN));        // id     4

    if (idRet != id)
    {
      Error("ERROR: Packet ID mismatch.\n");

      shutdown();

      goto fail;
    }

    //
    // Process received answer.
    //

    switch(type)
    {
      //
      // SSH2_FXP_ATTR message. We should get resource's
      // attributes here.
      //

      case SSH2_FXP_ATTRS:
      {
        DEBUG2("SFTP #%d: received [SSH2_FXP_ATTRS].", id);

        //
        // If extended request decode reveived attributes.
        //

        if (attr)
        {
          FAIL(popAttribs(attr, packet));
        }

        DEBUG1("SFTP #%d: Stat [%s] finished with success.", id, path);

        break;
      }

      //
      // SSH2_FXP_STATUS message. This means error or resource
      // not exists.
      //

      case SSH2_FXP_STATUS:
      {
        DBG_MSG("SFTP #%d: received [SSH2_FXP_STATUS].", id);

        FAIL(StrPopDword(&serverStatus, packet, STR_BIG_ENDIAN));

        FAIL(serverStatus != SSH2_FX_OK);

        break;
      }

      //
      // Unexpected message type.
      //

      default:
      {
        Error("SFTP #%d: ERROR: Unexpected message type [0x%x].", id, type);

        shutdown();

        goto fail;
      }
    }

    //
    // Clean up.
    //

    exitCode = 0;

    fail:

    if (exitCode)
    {
      Error("Cannot stat remote file.\n"
                "Error code is: %u.\nServer status is: [%d][%s].",
                      GetLastError(), serverStatus,
                          TranslateSftpStatus(serverStatus));
    }

    DBG_LEAVE3("SftpClient::stat");

    return exitCode;
  }

  //
  // Generic implementation of readdir().
  // List content of remote directory.
  //
  // Sends   : many SSH2_FXP_DIRLIGO_READDIR_SHORT packets.
  // Expects : many SSH2_FXP_NAME and one SSH2_FXP_STATUS for EOF.
  //
  // files  - list of files/dirs living inside selected directory (OUT).
  // handle - value retrieved from open() function before (IN).
  //
  // RETURNS: 0 if OK.
  //

  int SftpClient::readdir(vector<SftpFileInfo> &files, int64_t handle)
  {
    DBG_ENTER3("SftpClient::readdir");

    int exitCode = -1;
    int goOn     = 1;
    int capacity = 0;
    int iter     = 0;

    uint32_t serverStatus = 0;

    string packet;

    files.clear();

    DEBUG1("SFTP: readdir(%"PRId64").", handle);

    FAILEX(dead_, "SFTP: readdir() rejected because session dead.\n");

    //
    // Send requests for data until EOF or error.
    //

    while(goOn)
    {
      uint32_t id    = GenerateUniqueId();
      uint32_t idRet = 0;
      uint32_t size  = 0;
      uint32_t count = 0;

      uint8_t type = 0;

      iter ++;

      //
      // Prepare SSH2_FXP_DIRLIGO_READDIR_SHORT message.
      //
      //
      // size      4
      // type      1
      // id        4
      // handleLen 4
      // handle    4
      // ----------------------------------
      //    total: 13 + 4
      //

      packet.clear();

      StrPushDword(packet, 13, STR_BIG_ENDIAN);               // size      4
      StrPushByte(packet, SSH2_FXP_DIRLIGO_READDIR_SHORT);    // type      1
      StrPushDword(packet, id, STR_BIG_ENDIAN);               // id        4
      StrPushDword(packet, 4, STR_BIG_ENDIAN);                // handleLen 4
      StrPushDword(packet, uint32_t(handle), STR_BIG_ENDIAN); // handle    4

      //
      // Send packet and wait for answer.
      //

      FAIL(processPacket(packet, packet));

      DEBUG2("SFTP #%d: Sent [SSH2_FXP_DIRLIGO_READDIR_SHORT][%"PRId64"].\n", id, handle);

      //
      // Pop header:
      //
      // size 4
      // type 1
      // id   4
      //

      FAIL(StrPopDword(&size, packet, STR_BIG_ENDIAN));  // size 4
      FAIL(StrPopByte(&type, packet));                   // type 1
      FAIL(StrPopDword(&idRet, packet, STR_BIG_ENDIAN)); // id   4

      if (idRet != id)
      {
        Error("ERROR: Packet ID mismatch.\n");

        shutdown();

        goto fail;
      }

      //
      // Interprete received message.
      //

      switch(type)
      {
        //
        // Status message. This means EOF or error.
        //

        case SSH2_FXP_STATUS:
        {
          DEBUG2("SFTP #%d: received [SSH2_FXP_STATUS].", id);

          goOn = 0;

          FAIL(StrPopDword(&serverStatus, packet, STR_BIG_ENDIAN));

          FAIL(serverStatus != SSH2_FX_EOF);

          DEBUG1("SFTP #%d: Readdir handle [%"PRId64"] finished with code [%d][%s].\n",
                     id, handle, serverStatus, TranslateSftpStatus(serverStatus));

          break;
        }

        //
        // SSH2_FXP_NAME. Next portion of data.
        //

        case SSH2_FXP_NAME:
        {
          DEBUG2("SFTP #%d: received [SSH2_FXP_NAME].", id);

          //
          // Pop number of elements.
          //

          FAIL(StrPopDword(&count, packet, STR_BIG_ENDIAN));

          DEBUG2("SFTP #%d: Popped elements count [%d].\n", id, count);

          //
          // Pop <count> elements from packet.
          //

          for (int i = 0; i < count; i++)
          {
            SftpFileInfo info;

            memset(&info.attr_, 0, sizeof(info.attr_));

            char name[MAX_PATH]     = {0};
            char longName[MAX_PATH] = {0};

            uint32_t nameLen     = 0;
            uint32_t longNameLen = 0;

            //
            // Pop element's name.
            //

            FAIL(StrPopDword(&nameLen, packet, STR_BIG_ENDIAN));
            FAIL(StrPopRaw(name, nameLen, packet));

            FAIL(StrPopDword(&longNameLen, packet, STR_BIG_ENDIAN));
            FAIL(StrPopRaw(longName, longNameLen, packet));

            DEBUG2("SFTP #%d: Popped element [%s] / [%s].\n", id, name, longName);

            info.name_ = name;

            //
            // Pop attributes.
            //

            FAIL(popAttribs(&info.attr_, packet));

            //
            // Last element marker. Don't go on longer.
            //

            if (strcmp(name, "...") == 0)
            {
              goOn = 0;
            }

            //
            // Put another element to list.
            //

            else
            {
              files.push_back(info);
            }
          }

          break;
        }

        //
        // Unexpected packet type. Break.
        //

        default:
        {
          Error("SFTP #%d: ERROR: Unexpected packet type [%d].", id, type);

          shutdown();

          goto fail;
        }
      }
    }

    DEBUG2("SFTP: %d elements listed.", files.size());

    //
    // Cleanup.
    //

    exitCode = 0;

    fail:

    if (exitCode)
    {
      Error("ERROR: Cannot read remote directory.\n"
                " Server status is: [%d][%s].", serverStatus,
                    TranslateSftpStatus(serverStatus));

    }

    DBG_LEAVE3("SftpClient::readdir");

    return exitCode;
  }

  //
  // Retrieve info about remote disk.
  //
  // WARNING: Server MUST support "statvfs@openssh.com" extension.
  //
  // stvfs - buffer to store info about disk (see Sftp.h) (OUT)
  // path  - remote path to check (IN)
  //
  // RETURNS: 0 if OK.
  //

  int SftpClient::statvfs(Statvfs_t *stvfs, const char *path)
  {
    DBG_ENTER3("SftpClient::statvfs");

    int exitCode = -1;

    uint32_t id           = GenerateUniqueId();
    uint32_t pathLen      = 0;
    uint32_t extNameLen   = 0;
    uint32_t idRet        = 0;
    uint32_t serverStatus = 0;
    uint32_t size         = 0;

    uint8_t type;

    const char *extName = NULL;

    string packet;

    DEBUG1("SFTP #%d: statvfs path [%s]", id, path);

    FAILEX(dead_, "SFTP #%d: rejected because session dead.\n", id);

    //
    // Prepare SSH2_FXP_EXTENDED packet for
    // 'statvfs@openssh.com' command.
    //
    // size       4
    // type       1
    // id         4
    // extNameLen 4
    // extName    strlen(statvfs@openssh.com)
    // pathLen    4
    // path       strlen(path)
    // --------------------------------------
    //     total: 13 + extLen + pathLen + 4
    //

    DEBUG3("SFTP #%d: preparing SSH2_FXP_EXTENDED...", id);

    extName    = "statvfs@openssh.com";
    pathLen    = strlen(path);
    extNameLen = strlen(extName);
    size       = 13 + extNameLen + pathLen;

    StrPushDword(packet, size, STR_BIG_ENDIAN);       // size       4
    StrPushByte(packet, SSH2_FXP_EXTENDED);           // type       1
    StrPushDword(packet, id, STR_BIG_ENDIAN);         // id         4
    StrPushDword(packet, extNameLen, STR_BIG_ENDIAN); // extNameLen 4
    StrPushRaw(packet, extName, extNameLen);          // extName    extNameLen
    StrPushDword(packet, pathLen, STR_BIG_ENDIAN);    // pathLen    4
    StrPushRaw(packet, path, pathLen);                // path       pathLen

    //
    // Send packet, wait for answer.
    //

    FAIL(processPacket(packet, packet));

    DEBUG2("SFTP #%d: sent [SSH2_FXP_ENTENDED].\n", id, path);

    //
    // Parse answer.
    //
    // <size> 4
    // <type> 1
    // <id>   4
    // ...
    //

    FAIL(StrPopDword(&size, packet, STR_BIG_ENDIAN));  // size 4
    FAIL(StrPopByte(&type, packet));                   // type 1
    FAIL(StrPopDword(&idRet, packet, STR_BIG_ENDIAN)); // id   4

    //
    // Check packet ID.
    //

    if (idRet != id)
    {
      Error("ERROR: Packet ID mismatch.\n");

      shutdown();

      goto fail;
    }

    //
    // Check packet type.
    //

    if (type != SSH2_FXP_EXTENDED_REPLY)
    {
      Error("ERROR: Unexpected packet type [%d].\n", type);

      shutdown();

      goto fail;
    }

    DEBUG2("SFTP #%d: received [SSH2_FXP_EXTENDED_REPLY].", id);

    FAIL(StrPopQword(&stvfs -> bsize_, packet, STR_BIG_ENDIAN));
    FAIL(StrPopQword(&stvfs -> frsize_, packet, STR_BIG_ENDIAN));
    FAIL(StrPopQword(&stvfs -> blocks_, packet, STR_BIG_ENDIAN));
    FAIL(StrPopQword(&stvfs -> bfree_, packet, STR_BIG_ENDIAN));
    FAIL(StrPopQword(&stvfs -> bavail_, packet, STR_BIG_ENDIAN));
    FAIL(StrPopQword(&stvfs -> files_, packet, STR_BIG_ENDIAN));
    FAIL(StrPopQword(&stvfs -> ffree_, packet, STR_BIG_ENDIAN));
    FAIL(StrPopQword(&stvfs -> favail_, packet, STR_BIG_ENDIAN));
    FAIL(StrPopQword(&stvfs -> fsid_, packet, STR_BIG_ENDIAN));
    FAIL(StrPopQword(&stvfs -> flags_, packet, STR_BIG_ENDIAN));
    FAIL(StrPopQword(&stvfs -> namemax_, packet, STR_BIG_ENDIAN));

    DEBUG2("bsize   : [%"PRIu64"]", stvfs -> bsize_);
    DEBUG2("frsize  : [%"PRIu64"]", stvfs -> frsize_);
    DEBUG3("blocks  : [%"PRIu64"]", stvfs -> blocks_);
    DEBUG2("bfree   : [%"PRIu64"]", stvfs -> bfree_);
    DEBUG2("bavail  : [%"PRIu64"]", stvfs -> bavail_);
    DEBUG2("files   : [%"PRIu64"]", stvfs -> files_);
    DEBUG2("ffree   : [%"PRIu64"]", stvfs -> ffree_);
    DEBUG3("favail  : [%"PRIu64"]", stvfs -> favail_);
    DEBUG2("fsid    : [%"PRIu64"]", stvfs -> fsid_);
    DEBUG2("flags   : [%"PRIu64"]", stvfs -> flags_);
    DEBUG2("namemax : [%"PRIu64"]", stvfs -> namemax_);

    //
    // Error handler.
    //

    exitCode = 0;

    fail:

    DBG_LEAVE3("SftpClient::statvfs");

    return exitCode;
  };

  //
  // Create new directory on server.
  //
  // Sends   : SSH2_FXP_MKDIR packet.
  // Expects : SSH2_FXP_STATUS packet.
  //
  // path - path to craete (IN).
  //
  // RETURNS: 0 if OK.
  //

  int SftpClient::mkdir(const char *path)
  {
    DBG_ENTER3("SftpClient::mkdir");

    uint32_t serverStatus = SSH2_FX_FAILURE;

    uint32_t id      = GenerateUniqueId();
    uint32_t size    = 0;
    uint32_t idRet   = 0;
    uint32_t pathLen = 0;

    uint8_t type = 0;

    string packet;

    DEBUG1("SFTP #%d BEGIN: mkdir path [%s].", id, path);

    FAILEX(dead_, "SFTP #%d: rejected because session dead.\n", id);

    //
    // Prepare SSH2_FXP_MKDIR packet:
    //
    // size           4
    // SSH2_FXP_MKDIR 1
    // id             4
    // pathLen        4
    // path           pathLen
    // attrib         4
    // ----------------------------------
    //        total: 13 + pathLen + 4
    //

    DEBUG2("SFTP #%d: Preparing SSH2_FXP_MKDIR packet...", id);

    pathLen = strlen(path);

    StrPushDword(packet, 13 + pathLen, STR_BIG_ENDIAN); // size    4
    StrPushByte(packet, SSH2_FXP_MKDIR);                // type    1
    StrPushDword(packet, id, STR_BIG_ENDIAN);           // id      4
    StrPushDword(packet, pathLen, STR_BIG_ENDIAN);      // pathLen 4
    StrPushRaw(packet, path, pathLen);                  // path    pathLen
    StrPushDword(packet, 0, STR_BIG_ENDIAN);            // attr    4

    //
    // Send packet and wait for answer.
    //

    FAIL(processPacket(packet, packet));

    //
    // Receive status packet.
    //

    serverStatus = popStatusPacket(packet, id);

    //
    // Error handler.
    //

    fail:

    DEBUG1("SFTP #%d END  : mkdir path [%s] finished with code [%d].", id, path, serverStatus);

    DBG_LEAVE3("SftpClient::mkdir");

    return serverStatus;
  }

  //
  // Remove file on server.
  //
  // Sends   : SSH2_FXP_REMOVE packet.
  // Expects : SSH2_FXP_STATUS packet.
  //
  // path  - path to craete (IN).
  //
  // RETURNS: 0 if OK.
  //

  int SftpClient::remove(const char *path)
  {
    DBG_ENTER3("SftpClient::remove");

    uint32_t serverStatus = SSH2_FX_FAILURE;

    uint32_t id      = GenerateUniqueId();
    uint32_t size    = 0;
    uint32_t idRet   = 0;
    uint32_t pathLen = 0;

    uint8_t type = 0;

    string packet;

    DEBUG1("SFTP #%d BEGIN: remove path [%s].", id, path);

    FAILEX(dead_, "SFTP #%d: rejected because session dead.\n", id);

    //
    // Prepare SSH2_FXP_RMDIR packet:
    //
    // size           4
    // SSH2_FXP_RMDIR 1
    // id             4
    // pathLen        4
    // path           pathLen
    // ----------------------------------
    //         total: 9 + pathLen + 4
    //

    DEBUG2("SFTP #%d: Preparing SSH2_FXP_REMOVE packet...", id);

    pathLen = strlen(path);

    StrPushDword(packet, 9 + pathLen, STR_BIG_ENDIAN); // size    4
    StrPushByte(packet, SSH2_FXP_REMOVE);               // type    1
    StrPushDword(packet, id, STR_BIG_ENDIAN);           // id      4
    StrPushDword(packet, pathLen, STR_BIG_ENDIAN);      // pathLen 4
    StrPushRaw(packet, path, pathLen);                  // path    pathLen

    //
    // Send packet and wait for answer.
    //

    FAIL(processPacket(packet, packet));

    //
    // Receive status packet.
    //

    serverStatus = popStatusPacket(packet, id);

    //
    // Error handler.
    //

    fail:

    DEBUG1("SFTP #%d END  : remove path [%s] finished with code [%d].", id, path, serverStatus);

    return serverStatus;
  }

  //
  // Remove empty directory on server.
  //
  // Sends   : SSH2_FXP_RMDIR packet.
  // Expects : SSH2_FXP_STATUS packet.
  //
  // path - path to craete (IN).
  //
  // RETURNS: 0 if OK.
  //

  int SftpClient::rmdir(const char *path)
  {
    DBG_ENTER3("SftpClient::rmdir");

    uint32_t serverStatus = SSH2_FX_FAILURE;

    uint32_t id      = GenerateUniqueId();
    uint32_t size    = 0;
    uint32_t idRet   = 0;
    uint32_t pathLen = 0;

    uint8_t type = 0;

    string packet;

    DEBUG1("SFTP #%d BEGIN: rmdir path [%s].", id, path);

    FAILEX(dead_, "SFTP #%d: rejected because session dead.\n", id);

    //
    // Prepare SSH2_FXP_RMDIR packet:
    //
    // size           4
    // SSH2_FXP_RMDIR 1
    // id             4
    // pathLen        4
    // path           pathLen
    // ----------------------------------
    //         total: 9 + pathLen + 4
    //

    DEBUG2("SFTP #%d: Preparing SSH2_FXP_RMDIR packet...", id);

    pathLen = strlen(path);

    StrPushDword(packet, 9 + pathLen, STR_BIG_ENDIAN);  // size    4
    StrPushByte(packet, SSH2_FXP_RMDIR);                // type    1
    StrPushDword(packet, id, STR_BIG_ENDIAN);           // id      4
    StrPushDword(packet, pathLen, STR_BIG_ENDIAN);      // pathLen 4
    StrPushRaw(packet, path, pathLen);                  // path    pathLen

    //
    // Send packet and wait for answer.
    //

    FAIL(processPacket(packet, packet));

    //
    // Receive status packet.
    //

    serverStatus = popStatusPacket(packet, id);

    //
    // Error handler.
    //

    fail:

    DEBUG1("SFTP #%d END  : rmdir path [%s] finished with code [%d].", id, path, serverStatus);

    return serverStatus;
  }

  //
  // Rename remote file or directory.
  //
  // Sends  : SSH2_FXP_RENAME packet.
  // Expects: SSH2_FXP_STATUS packet.
  //
  // path1 - existing, old path to rename (IN).
  // path2 - new name to set (IN).
  //
  // RETURNS: 0 if OK.
  //

  int SftpClient::rename(const char *path1, const char *path2)
  {
    DBG_ENTER3("SftpClient::rmdir");

    uint32_t serverStatus = SSH2_FX_FAILURE;

    uint32_t id       = GenerateUniqueId();
    uint32_t size     = 0;
    uint32_t idRet    = 0;
    uint32_t path1Len = 0;
    uint32_t path2Len = 0;

    uint8_t type = 0;

    string packet;

    DEBUG1("SFTP #%d BEGIN: rename path [%s] to path [%s].", id, path1, path2);

    FAILEX(dead_, "SFTP #%d: rejected because session dead.\n", id);

    //
    // Prepare SSH2_FXP_RENAME packet:
    //
    // size            4
    // SSH2_FXP_RENAME 1
    // id              4
    // path1Len        4
    // path1           path1Len
    // path2Len        4
    // path2           path2Len
    // ----------------------------------
    //          total: 13 + path1Len + path2Len + 4
    //

    DEBUG2("SFTP #%d: Preparing SSH2_FXP_RENAME packet...", id);

    path1Len = strlen(path1);
    path2Len = strlen(path2);

    size = 13 + path1Len + path2Len;

    StrPushDword(packet, size, STR_BIG_ENDIAN);     // size     4
    StrPushByte(packet, SSH2_FXP_RENAME);           // type     1
    StrPushDword(packet, id, STR_BIG_ENDIAN);       // id       4
    StrPushDword(packet, path1Len, STR_BIG_ENDIAN); // path1Len 4
    StrPushRaw(packet, path1, path1Len);            // path1    path1Len
    StrPushDword(packet, path2Len, STR_BIG_ENDIAN); // path2Len 4
    StrPushRaw(packet, path2, path2Len);            // path2    path2Len

    //
    // Send packet and wait for answer.
    //

    FAIL(processPacket(packet, packet));

    //
    // Receive status packet.
    //

    serverStatus = popStatusPacket(packet, id);

    //
    // Error handler.
    //

    fail:

    DEBUG1("SFTP #%d END  : rename path [%s] to path [%s]"
               " finished with code [%d].", id, path1, path2, serverStatus);

    return serverStatus;
  }

  //
  // ----------------------------------------------------------------------------
  //
  //    SSH2_FXP_DIRLIGO_XXX commands to fit SFTP protocol to WINAPI better.
  //
  // ----------------------------------------------------------------------------
  //

  //
  // Open remote file using {access, shared, create, flags} masks
  // used with CreateFile() on Windows.
  //
  // Sends   : SSH2_FXP_DIRLIGO_CREATEFILE packet.
  // Expects : SSH2_FXP_HANDLE packet.
  //
  // path       - path to remote file (IN).
  // accesMode  - the same as dwSharedAccess in CreateFile (IN).
  // sharedMode - the same as dwShareShared in CreateFile (IN).
  // create     - the same as dwCreateDisposition in CreateFile (IN).
  // flags      - the same as dwFlags in CreateFile (IN).
  // isDIr      - on output set to 1 if opened file is a directory (OUT/OPT).
  //
  // RETURNS: remote file handle,
  //          or -winapi error if error (e.g. -ERROR_FILE_NOT_FOUND).
  //

  int64_t SftpClient::createfile(const char *path, uint32_t access,
                                     uint32_t shared, uint32_t create,
                                         uint32_t flags, int *isDir)
  {
    DBG_ENTER3("SftpClient::createfile");

    int exitCode = -1;

    string packet;

    uint32_t pathLen = 0;
    uint32_t id      = GenerateUniqueId();
    uint32_t idRet   = 0;
    uint32_t size    = 0;
    uint32_t tmp     = 0;

    int64_t handle = 0;

    uint8_t type = 0;

    DEBUG1("SFTP #%d: createfile '%s' access=%x shared=%x create=%x flags=%x",
               id, path, access, shared, create, flags);

    FAILEX(dead_, "SFTP #%d: rejected because session dead.\n", id);

    //
    // Prepare SSH2_FXP_DIRLIGO_CREATEFILE packet.
    //
    // size     4
    // type     1
    // id       4
    // pathLen  4
    // path     pathLen
    // access   4
    // shared   4
    // create   4
    // flags    4
    // ----------------------------------
    //          total: 25 + pathLen + 4
    //

    DEBUG2("SftpClient::createfile : Preparing SSH2_FXP_DIRLIGO_CREATEFILE packet...");

    pathLen = strlen(path);

    StrPushDword(packet, 25 + pathLen, STR_BIG_ENDIAN); // size    4
    StrPushByte(packet, SSH2_FXP_DIRLIGO_CREATEFILE);   // type    1
    StrPushDword(packet, id, STR_BIG_ENDIAN);           // id      4
    StrPushDword(packet, pathLen, STR_BIG_ENDIAN);      // pathLen 4
    StrPushRaw(packet, path, pathLen);                  // path    pathLen
    StrPushDword(packet, access, STR_BIG_ENDIAN);       // access  4
    StrPushDword(packet, shared, STR_BIG_ENDIAN);       // shared  4
    StrPushDword(packet, create, STR_BIG_ENDIAN);       // create  4
    StrPushDword(packet, flags, STR_BIG_ENDIAN);        // flags   4

    //
    // Send packet and wait for answer.
    //

    FAIL(processPacket(packet, packet));

    //
    // Parse answer.
    //
    // size   4
    // type   1
    // id     4
    // handle 4
    //

    FAIL(StrPopDword(&size, packet, STR_BIG_ENDIAN));  // size 4
    FAIL(StrPopByte(&type, packet));                   // type 1
    FAIL(StrPopDword(&idRet, packet, STR_BIG_ENDIAN)); // id   4

    //
    // Check packet ID.
    //

    if (idRet != id)
    {
      Error("ERROR: Packet ID mismatch.\n");

      shutdown();

      goto fail;
    }

    //
    // Check packet type.
    //

    if (type != SSH2_FXP_HANDLE)
    {
      Error("ERROR: Unexpected packet type [%d].\n", type);

      shutdown();

      goto fail;
    }

    DEBUG2("SFTP #%d: Received [SSH2_FXP_HANDLE].", id);

    FAIL(StrPopDword(&tmp, packet, STR_BIG_ENDIAN));
    FAIL(StrPopDword(&tmp, packet, STR_BIG_ENDIAN));

    handle = int64_t(int32_t(tmp));

    if (handle >= 0)
    {
      if (handle & SSH2_FXP_DIRLIGO_DIR_FLAG)
      {
        if (isDir)
        {
          *isDir = 1;
        }
        else
        {
          *isDir = 0;
        }
      }

      handle &= ~SSH2_FXP_DIRLIGO_DIR_FLAG;

      DEBUG1("SFTP #%d: createfile finished with handle [%"PRId64"].\n", id, handle);

      DBG_SET_ADD("sftp handle", handle, "[%s]", path);
    }
    else
    {
      DEBUG1("SFTP #%d: createfile finished with error [%"PRId64"].\n", id, -handle);
    }

    //
    // Error handler.
    //

    fail:

    DBG_LEAVE3("SftpClient::createfile");

    return handle;
  }

  //
  // ----------------------------------------------------------------------------
  //
  //                     Widechar functions for DOKAN.
  //             Only wrappers for existing UTF8 functions above.
  //
  // ----------------------------------------------------------------------------
  //

  #ifdef WIN32

  //
  // Widechar wrapper for open().
  // See utf8 version of SftpClient::open().
  //

  int64_t SftpClient::open(const wchar_t *path16, int mode, int isDir)
  {
    char path[MAX_PATH] = {0};

    WideCharToMultiByte(CP_UTF8, 0, path16, -1, path, sizeof(path), NULL, NULL);

    return open(path, mode, isDir);
  }

  //
  // Widechar wrapper for opendir().
  // See utf8 version of SftpClient::opendir().
  //

  int64_t SftpClient::opendir(const wchar_t *path16)
  {
    return open(path16, 0, 1);
  }

  //
  // Widechar wrapper for stat().
  // See utf8 version of SftpClient::stat().
  //

  int SftpClient::stat(const wchar_t *path16, BY_HANDLE_FILE_INFORMATION *info)
  {
    char path[MAX_PATH] = {0};

    WideCharToMultiByte(CP_UTF8, 0, path16, -1, path, sizeof(path), NULL, NULL);

    return stat(path, info);
  }

  //
  // Widechar wrapper for mkdir().
  // See utf8 version of SftpClient::mkdir().
  //

  int SftpClient::mkdir(const wchar_t *path16)
  {
    char path[MAX_PATH] = {0};

    WideCharToMultiByte(CP_UTF8, 0, path16, -1, path, sizeof(path), NULL, NULL);

    return mkdir(path);
  }

  //
  // Widechar wrapper for remove().
  // See utf8 version of SftpClient::remove().
  //

  int SftpClient::remove(const wchar_t *path16)
  {
    char path[MAX_PATH] = {0};

    WideCharToMultiByte(CP_UTF8, 0, path16, -1, path, sizeof(path), NULL, NULL);

    return remove(path);
  }

  //
  // Widechar wrapper for rmdir().
  // See utf8 version of SftpClient::rmdir().
  //

  int SftpClient::rmdir(const wchar_t *path16)
  {
    char path[MAX_PATH] = {0};

    WideCharToMultiByte(CP_UTF8, 0, path16, -1, path, sizeof(path), NULL, NULL);

    return rmdir(path);
  }

  //
  // Widechar wrapper for rename().
  // See utf8 version of SftpClient::rename().
  //

  int SftpClient::rename(const wchar_t *path1_16, const wchar_t *path2_16)
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

  int64_t SftpClient::createfile(const wchar_t *path16, uint32_t access,
                                     uint32_t shared, uint32_t create,
                                         uint32_t flags, int *isDir)
  {
    char path[MAX_PATH] = {0};

    WideCharToMultiByte(CP_UTF8, 0, path16, -1, path, sizeof(path), NULL, NULL);

    return createfile(path, access, shared, create, flags, isDir);
  }

  //
  // Widechar wrapper for statvfs().
  // See utf8 version of SftpClient::statvfs().
  //

  int SftpClient::statvfs(Statvfs_t *stvfs, const wchar_t *path16)
  {
    char path[MAX_PATH] = {0};

    WideCharToMultiByte(CP_UTF8, 0, path16, -1, path, sizeof(path), NULL, NULL);

    return statvfs(stvfs, path);
  }

  #endif /* WIN32 */

  //
  // ----------------------------------------------------------------------------
  //
  //                              Helper functions.
  //
  // ----------------------------------------------------------------------------
  //

  #ifdef WIN32

  int SftpClient::popAttribs(BY_HANDLE_FILE_INFORMATION *info, string &packet)
  {
    int exitCode = -1;

    uint32_t flags   = 0;
    uint32_t uid     = 0;
    uint32_t gid     = 0;
    uint32_t perm    = 0;
    uint32_t atime32 = 0;
    uint32_t mtime32 = 0;

    uint64_t atime = 0;
    uint64_t mtime = 0;

    //
    // Check args.
    //

    FAILEX(info == NULL, "ERROR: Null 'info' passed to SftpClient::popAttribs.\n");

    //
    //
    //

    DEBUG3("SftpClient::popAttribs : Decoding attributes...");

    //
    // Clear output struct.
    //

    memset(info, 0, sizeof(BY_HANDLE_FILE_INFORMATION));

    info -> nNumberOfLinks = 1;
    info -> nFileIndexHigh = 0;
    info -> nFileIndexLow  = 0;

    //
    // Size.
    //

    FAIL(StrPopDword(&flags, packet, STR_BIG_ENDIAN));

    if (flags & SSH2_FILEXFER_ATTR_SIZE)
    {
      FAIL(StrPopDword((uint32_t *) &info -> nFileSizeHigh, packet, STR_BIG_ENDIAN));
      FAIL(StrPopDword((uint32_t *) &info -> nFileSizeLow, packet, STR_BIG_ENDIAN));
    }

    //
    // Guid/Uid flags. We ignore it on Windows.
    //

    if (flags & SSH2_FILEXFER_ATTR_UIDGID)
    {
      FAIL(StrPopDword(&uid, packet, STR_BIG_ENDIAN));
      FAIL(StrPopDword(&gid, packet, STR_BIG_ENDIAN));
    }

    //
    // Permissions.
    //

    if (flags & SSH2_FILEXFER_ATTR_PERMISSIONS)
    {
      FAIL(StrPopDword(&perm, packet, STR_BIG_ENDIAN));

      if (SFTP_ISDIR(perm))
      {
        info -> dwFileAttributes |= FILE_ATTRIBUTE_DIRECTORY;
      }
    }

    //
    // Last acces and last modify times.
    // We need translate time to Winapi comatible.
    //

    if (flags & SSH2_FILEXFER_ATTR_ACMODTIME)
    {
      //
      // Pop acces time and modify times.
      //

      FAIL(StrPopDword(&atime32, packet, STR_BIG_ENDIAN));
      FAIL(StrPopDword(&mtime32, packet, STR_BIG_ENDIAN));

      atime = atime32;
      mtime = mtime32;

      //
      // Convert time from UNIX UTC (from 1970)
      // to FILETIME UTC (from 1601).
      //

      LONGLONG atimell = Int32x32To64(atime, 10000000) + 116444736000000000LL;
      LONGLONG mtimell = Int32x32To64(mtime, 10000000) + 116444736000000000LL;

      info -> ftLastAccessTime.dwLowDateTime  = (DWORD) atimell;
      info -> ftLastAccessTime.dwHighDateTime = atimell >> 32;

      info -> ftLastWriteTime.dwLowDateTime   = (DWORD) mtimell;
      info -> ftLastWriteTime.dwHighDateTime  = mtimell >> 32;

      //
      // We don't get creation time from server.
      // We assume creation = last write here.
      //

      info -> ftCreationTime.dwLowDateTime  = (DWORD) mtimell;
      info -> ftCreationTime.dwHighDateTime = mtimell >> 32;
    }

    info -> dwFileAttributes |= FILE_ATTRIBUTE_NOT_CONTENT_INDEXED;

    //
    // Error handler.
    //

    exitCode = 0;

    fail:

    if (exitCode)
    {
      Error("ERROR: Cannot pop attributes data.\n");

      shutdown();
    }

    return exitCode;
  }

  #endif /* WIN32 */

  //
  // Decode attributes sent by SFTP server.
  //
  // info   - struct, where to write decoded atributes (OUT).
  // packet - raw packet received from SFTP server (IN).
  //

  int SftpClient::popAttribs(SftpFileAttr *info, string &packet)
  {
    int exitCode = -1;

    uint32_t sizeHigh32;
    uint32_t sizeLow32;

    //
    // Check args.
    //

    FAILEX(info == NULL, "ERROR: Null 'info' passed to SftpClient::popAttribs.\n");

    //
    //
    //

    DEBUG3("SftpClient::popAttribs : Decoding attributes...");

    //
    // Clear output struct.
    //

    memset(info, 0, sizeof(*info));

    //
    // Size.
    //

    FAIL(StrPopDword(&info -> flags_, packet, STR_BIG_ENDIAN));

    if (info -> flags_ & SSH2_FILEXFER_ATTR_SIZE)
    {
      FAIL(StrPopDword(&sizeHigh32, packet, STR_BIG_ENDIAN));
      FAIL(StrPopDword(&sizeLow32, packet, STR_BIG_ENDIAN));

      info -> size_ = (uint64_t(sizeHigh32) << 32) | uint64_t(sizeLow32);
    }

    //
    // Guid/Uid flags. We ignore it on Windows.
    //

    if (info -> flags_ & SSH2_FILEXFER_ATTR_UIDGID)
    {
      FAIL(StrPopDword(&info -> uid_, packet, STR_BIG_ENDIAN));
      FAIL(StrPopDword(&info -> guid_, packet, STR_BIG_ENDIAN));
    }

    //
    // Permissions.
    //

    if (info -> flags_ & SSH2_FILEXFER_ATTR_PERMISSIONS)
    {
      FAIL(StrPopDword(&info -> perm_, packet, STR_BIG_ENDIAN));
    }

    //
    // Last acces and last modify times.
    // We need translate time to Winapi comatible.
    //

    if (info -> flags_ & SSH2_FILEXFER_ATTR_ACMODTIME)
    {
      //
      // Pop acces time and modify times.
      //

      FAIL(StrPopDword(&info -> atime_, packet, STR_BIG_ENDIAN));
      FAIL(StrPopDword(&info -> mtime_, packet, STR_BIG_ENDIAN));
    }

    //
    // Error handler.
    //

    exitCode = 0;

    fail:

    if (exitCode)
    {
      Error("ERROR: Cannot pop attributes data.\n");

      shutdown();
    }

    return exitCode;
  }

  //
  // ----------------------------------------------------------------------------
  //
  //                              Packet processing
  //
  // ----------------------------------------------------------------------------
  //

  //
  // Send <packet> to server and receive <answer> packet.
  //
  // TIP: You can use the same buffers for answer and packet to send.
  //
  // answer - buffer, where to store received answer paceket (OUT).
  // packet - buffer with packet to send (IN).
  //
  // RETURNS: 0 if OK.
  //

  int SftpClient::processPacketSimple(string &answer, string &packet)
  {
    DBG_ENTER3("SftpClient::processPacket");

    int exitCode = -1;

    int readed       = -1;
    int written      = -1;
    int totalReaded  = 0;
    int totalWritten = 0;

    int bytesProcessed = 0;

    double startTime = 0.0;
    double endTime   = 0.0;
    double elapsed   = 0.0;

    mutex_.lock();

    //
    // Send packet.
    //

    DEBUG3("SFTP: Sending [%d] bytes...\n", packet.size());

    DBG_DUMP(&packet[0], packet.size());

    startTime = GetTimeMs();

    switch(fdType_)
    {
      //
      // CRT FD.
      //

      case SFTP_CLIENT_FD:
      {
        while(totalWritten < packet.size())
        {
          written = ::write(fdout_, &packet[totalWritten],
                                packet.size() - totalWritten);

          if (written > 0)
          {
            DEBUG3("SFTP: Sent [%d] bytes.\n", written);

            totalWritten += written;
          }
          else
          {
            Error("ERROR: Write failed.\n");

            shutdown();

            goto fail;
          }
        }

        break;
      }

      //
      // Socket.
      //

      case SFTP_CLIENT_SOCKET:
      {
        while(totalWritten < packet.size())
        {
          written = ::send(fdout_, &packet[totalWritten],
                               packet.size() - totalWritten, 0);

          if (written > 0)
          {
            DEBUG3("SFTP: Sent [%d] bytes.\n", written);

            totalWritten += written;
          }
          else
          {
            Error("ERROR: Write failed.\n");

            shutdown();

            goto fail;
          }
        }

        break;
      }
    }

    FAILEX(totalWritten != packet.size(), "ERROR: Cannot send packet.\n");

    //
    // Receive answer.
    //

    answer.resize(1024 * 64);

    while(packetComplete(answer, totalReaded) == 0)
    {
      //
      // Check free space in answer[] buffer.
      //

      int freeSpace = answer.size() - totalReaded;

      if (freeSpace <= 0)
      {
        Error("ERROR: Received packet > 64KB.\n");

        shutdown();

        goto fail;
      }

      //
      // Read next piece.
      //

      DEBUG3("SFTP: Reading...\n");

      switch(fdType_)
      {
        //
        // CRT FD.
        //

        case SFTP_CLIENT_FD:
        {
          readed = ::read(fdin_, &answer[totalReaded], freeSpace);

          break;
        }

        //
        // Socket.
        //

        case SFTP_CLIENT_SOCKET:
        {
          readed = ::recv(fdin_, &answer[totalReaded], freeSpace, 0);

          break;
        }
      }

      DEBUG2("SFTP: Received [%d] bytes.\n", readed);

      FAILEX(readed <= 0, "ERROR: Cannot read data. System code is : %d.\n", GetLastError());

      //
      // Next piece.
      //

      totalReaded += readed;
    }

    answer.resize(totalReaded);

    endTime = GetTimeMs();

    DBG_DUMP(&answer[0], answer.size());

    //
    // Update statistics.
    //

    bytesProcessed = written + totalReaded;

    elapsed = endTime - startTime;

    netstat_.insertRequest(bytesProcessed, elapsed);
    netstat_.insertOutcomingPacket(written);
    netstat_.insertIncomingPacket(totalReaded);

    //
    // Send statistics to caller if needed.
    //

    if (netstat_.getRequestCount() % netStatTick_ == 0)
    {
      if (netStatCallback_)
      {
        netStatCallback_(&netstat_, netStatCallbackCtx_);
      }
    }

    //
    // Error handler.
    //

    exitCode = 0;

    fail:

    if (exitCode)
    {
      Error("ERROR: Cannot process packet.\n"
                "Error code is : %d.\n", GetLastError())

      shutdown();
    }

    mutex_.unlock();

    DBG_LEAVE3("SftpClient::processPacket");

    return exitCode;
  }

  //
  // Send <packet> to server and receive <answer> packet.
  //
  // TIP: You can use the same buffers for answer and packet to send.
  //
  // answer - buffer, where to store received answer paceket (OUT).
  // packet - buffer with packet to send (IN).
  //
  // RETURNS: 0 if OK.
  //

  int SftpClient::processPacket(string &answer, string &packet)
  {
    DBG_ENTER3("SftpClient::processPacket");

    int exitCode = -1;

    int readed       = -1;
    int written      = -1;
    int totalReaded  = 0;
    int totalWritten = 0;
    int packetSize   = 0;

    int bytesProcessed = 0;

    double startTime = 0.0;
    double endTime   = 0.0;
    double elapsed   = 0.0;

    uint32_t size;
    uint32_t id;
    uint8_t type;

    //
    // Push pening request.
    //

    packetSize = packet.size();

    decodePacketHead(&size, &id, &type, packet, packetSize);

    rpool_ -> push(id, NULL, &answer);

    //
    // Send packet.
    //

    DEBUG3("SFTP: Sending [%d] bytes...\n", packetSize);

    DBG_DUMP(&packet[0], packetSize);

    startTime = GetTimeMs();

    switch(fdType_)
    {
      //
      // CRT FD.
      //

      case SFTP_CLIENT_FD:
      {
        mutex_.lock();

        while(totalWritten < packetSize)
        {
          written = ::write(fdout_, &packet[totalWritten],
                                packetSize - totalWritten);

          DEBUG2("SFTP: Sent [%d] bytes, ptr [%p], packet size [%d], total written [%d].\n",
                     written, &packet[totalWritten], packetSize, totalWritten);

          if (written > 0)
          {
            totalWritten += written;
          }
          else
          {
            Error("ERROR: Write failed. System code is : %d.\n", GetLastError());

            shutdown();

            goto fail;
          }
        }

        mutex_.unlock();

        break;
      }

      //
      // Socket.
      //

      case SFTP_CLIENT_SOCKET:
      {
        mutex_.lock();

        while(totalWritten < packetSize)
        {
          written = ::send(fdout_, &packet[totalWritten],
                                packetSize - totalWritten, 0);

          DEBUG2("SFTP: Sent [%d] bytes, ptr [%p], packet size [%d], total written [%d].\n",
                     written, &packet[totalWritten], packetSize, totalWritten);

          if (written > 0)
          {
            totalWritten += written;
          }
          else
          {
            Error("ERROR: Write failed. System code is : %d.\n", GetLastError());

            shutdown();

            goto fail;
          }
        }

        mutex_.unlock();

        break;
      }
    }

    FAILEX(totalWritten != packetSize,
               "ERROR: Cannot send packet (sent %d, but %d needed).\n",
                   totalWritten, packetSize);

    //
    // Wait until request finished.
    // Request will be finished when
    //

    rpool_ -> wait(id);

    //
    // Update statistics.
    //

    endTime = GetTimeMs();

    DBG_DUMP(&packet[0], answer.size());

    //
    // Update statistics.
    //

    bytesProcessed = written + packet.size();

    elapsed = endTime - startTime;

    netstat_.insertRequest(bytesProcessed, elapsed);
    netstat_.insertOutcomingPacket(written);
    netstat_.insertIncomingPacket(totalReaded);

    //
    // Send statistics to caller if needed.
    //

    if (netstat_.getRequestCount() % netStatTick_ == 0)
    {
      if (netStatCallback_)
      {
        netStatCallback_(&netstat_, netStatCallbackCtx_);
      }
    }

    //
    // Error handler.
    //

    exitCode = 0;

    fail:

    if (exitCode)
    {
      Error("ERROR: Cannot process packet.\n"
                "Error code is : %d.\n", GetLastError())

      shutdown();
    }

    mutex_.unlock();

    DBG_LEAVE3("SftpClient::processPacket");

    return exitCode;
  }

  //
  // Thread reading incoming packets.
  //
  // data - pointer to related SftpClient object (this pointer) (IN)
  //

  int SftpClient::readLoop(void *data)
  {
    DBG_ENTER3("SftpClient::readLoop");

    string buffer;

    SftpClient *this_ = (SftpClient *) data;

    int readed      = 0;
    int totalReaded = 0;

    Request *r = NULL;

    uint32_t size;
    uint32_t id;
    uint8_t type;

    int packetSize = 0;

    //
    // Maximum allowed packet.
    //

    buffer.resize(1024 * 64);

    //
    // Receive packets.
    //

    while(1)
    {
      while(this_ -> packetComplete(buffer, totalReaded) == 0)
      {
        //
        // Check free space in answer[] buffer.
        //

        int freeSpace = buffer.size() - totalReaded;

        if (freeSpace <= 0)
        {
          Error("ERROR: Received packet > 64KB.\n");

          this_ -> shutdown();

          goto fail;
        }

        //
        // Read next piece.
        //

        DEBUG3("SFTP: Reading...\n");

        switch(this_ -> fdType_)
        {
          //
          // CRT FD.
          //

          case SFTP_CLIENT_FD:
          {
            readed = ::read(this_ -> fdin_, &buffer[totalReaded], freeSpace);

            break;
          }

          //
          // Socket.
          //

          case SFTP_CLIENT_SOCKET:
          {
            readed = ::recv(this_ -> fdin_, &buffer[totalReaded], freeSpace, 0);

            break;
          }
        }

        DEBUG2("SFTP: Received [%d] bytes.\n", readed);

        FAILEX(readed <= 0, "ERROR: Cannot read data. System code is : %d.\n", GetLastError());

        //
        // Next piece.
        //

        totalReaded += readed;
      }

      //
      // Complete packet readed.
      // Decode its header.
      //

      this_ -> decodePacketHead(&size, &id, &type, buffer, totalReaded);

      DEBUG3("SftpClient::readLoop : Received packet"
                 " size=[%d], id=[%d], type=[%d], totalReaded=[%d].\n",
                     size, id, type, totalReaded);
      //
      // Serve request.
      //

      packetSize = min(int(size + 4), totalReaded);

      DBG_DUMP(&buffer[0], packetSize);

      r = this_ -> rpool_ -> find(id);

      if (r)
      {
        string *output = NULL;

        r -> lockData();

        output = (string *) r -> outputData_;

        if (output)
        {
          output -> resize(packetSize);

          memcpy(&(*output)[0], &buffer[0], packetSize);
        }

        r -> unlockData();

        r -> serve();
      }
      else
      {
        Error("WARNING: SFTP request ID#%d does not exist in pool.\n");
      }

      //
      // Pop packet from read buffer.
      //
      // FIXME: Avoid memcpy.
      //

      if (totalReaded > packetSize)
      {
        memcpy(&buffer[0], &buffer[packetSize], totalReaded - packetSize);
      }

      totalReaded -= packetSize;
    }

    //
    // Error handler.
    //

    fail:

    this_ -> shutdown();

    Error("ERROR: Read loop failed.\n");

    DBG_LEAVE3("SftpClient::readLoop");
  }

  //
  // Pop and decode SSH2_FXP_STATUS packet from buffer.
  //
  // packet - buffer, where status packet is stored (IN/OUT).
  //
  // id     - expected packet's id. Status packet is a server's response
  //          for one of packet sent by client. Id for send and received
  //          packet must be the same (IN).
  //
  // RETURNS: Decoded serverStatus code.
  //

  uint32_t SftpClient::popStatusPacket(string &packet, uint32_t id)
  {
    DBG_ENTER3("SftpClient::popStatusPacket");

    uint32_t serverStatus = SSH2_FX_FAILURE;

    uint32_t size  = 0;
    uint32_t idRet = 0;

    uint8_t type = 0;

    //
    // Parse answer.
    //
    // size   4
    // type   1
    // id     4
    // status 4
    //

    FAIL(StrPopDword(&size, packet, STR_BIG_ENDIAN));          // size   4
    FAIL(StrPopByte(&type, packet));                           // type   1
    FAIL(StrPopDword(&idRet, packet, STR_BIG_ENDIAN));         // id     4
    FAIL(StrPopDword(&serverStatus, packet, STR_BIG_ENDIAN));  // status 4

    if (idRet != id)
    {
      Error("ERROR: Packet ID mismatch.\n");

      shutdown();

      goto fail;
    }

    if (type != SSH2_FXP_STATUS)
    {
      Error("ERROR: Packet type [%d] received,"
                " but [%d] expected.\n", type, SSH2_FXP_STATUS);

      shutdown();

      goto fail;
    }

    //
    // Error handler.
    //

    fail:

    DBG_LEAVE3("SftpClient::popStatusPacket");

    return serverStatus;
  }

  //
  // Change used sector size.
  //
  // size - sector size in bytes (IN).
  //

  void SftpClient::setSectorSize(int size)
  {

    if (size <= 256)
    {
      Error("Sector size %d is too small.\n", size);
    }
    else
    {
      sectorSize_ = size;
    }

    DEBUG1("Using %d byte sectors.\n", sectorSize_);
  }

  //
  // Check is given SFTP packet completem.
  // Needed to handle partial read.
  //
  // packet   - SFTP packet data to check (IN).
  // realSize - real packet size i.e. number of useful bytes in packet[] buffer (IN).
  //
  // RETURNS: 1 if given packet is a full, complete SFTP packet,
  //          0 otherwise (partial packet).
  //

  int SftpClient::packetComplete(string &packet, int realSize)
  {
    DBG_ENTER3("SftpClient::packetComplete");

    int complete = 0;

    DEBUG5("SftpClient::packetComplete :"
               " packetSize=[%d], realSize=[%d].\n", packet.size(), realSize);

    DBG_DUMP(&packet[0], realSize);

    if (packet.size() > 4 && realSize > 4)
    {
      int expectedSize = 0;

      char *src = (char *) &packet[0];
      char *dst = (char *) &expectedSize;

      dst[0] = src[3];
      dst[1] = src[2];
      dst[2] = src[1];
      dst[3] = src[0];

      DEBUG5("SftpClient::packetComplete : Expected size [%d]\n", expectedSize);

      if (realSize >= (expectedSize + 4))
      {
        complete = 1;
      }
    }

    DEBUG5("SftpClient::packetComplete : Result [%d]\n", complete);

    DBG_LEAVE3("SftpClient::packetComplete");

    return complete;
  }

  //
  // Decode packet {size, type, ID} head from packet buffer without
  // destroying it.
  //
  // size     - decoded packet size (OUT).
  // id       - decoded packet ID (OUT).
  // type     - decoded packet type (OUT).
  // packet   - SFTP packet data to check (IN).
  // realSize - real packet size i.e. number of useful bytes in packet[] buffer (IN).
  //
  // RETURNS: 0 if OK.


  int SftpClient::decodePacketHead(uint32_t *size, uint32_t *id, uint8_t *type,
                                       string &packet, int realSize)
  {
    DBG_ENTER3("SftpClient::decodePacketHead");

    int exitCode = -1;

    if (realSize >= 9)
    {
      string tmp;

      tmp.resize(9);

      memcpy(&tmp[0], &packet[0], 9);

      StrPopDword(size, tmp, STR_BIG_ENDIAN);
      StrPopByte(type, tmp);
      StrPopDword(id, tmp, STR_BIG_ENDIAN);

      exitCode = 0;
    }

    if (exitCode)
    {
      Error("ERROR: Cannot decode packet header.\n");
    }

    DBG_LEAVE3("SftpClient::decodePacketHead");

    return exitCode;
  }

  //
  // ---------------------------------------------------------------------------
  //
  //                                High level API
  //
  // ---------------------------------------------------------------------------
  //

  //
  // Download file from sftp server in background thread.
  // Internal use only. See downloadFile() method.
  //
  // data - pointer to related SftpJob object (this pointer) (IN/OUT).
  //

  int SftpClient::DownloadFileWorker(void *data)
  {
    DBG_ENTER2("SftpClient::DownloadFileWorker");

    int exitCode = -1;

    SftpFileAttr attr;

    int64_t sftpHandle = -1;

    int pieceSize   = 0;

    int64_t bytesToCopy  = 0;
    int64_t totalWritten = 0;
    int64_t written      = 0;
    int64_t readed       = 0;

    FILE *f = NULL;

    char buf[1024 * 32];

    double rate = 0.0;

    SftpJob *job = (SftpJob *) data;

    SftpClient *sftp = job -> getSftpClient();

    const char *remotePath = job -> getRemoteName();
    const char *localPath  = job -> getLocalName();

    //
    // Add refference couner to related SFTP job to avoid
    // delete it while this thread works.
    //

    job -> addRef();

    //
    // Wait until job state is initializing.
    //

    while(job -> getState() == SFTP_JOB_STATE_INITIALIZING)
    {
      ThreadSleepMs(10);
    }

    //
    // Stat file on server.
    // We get total size of file here.
    //

    if (sftp -> stat(remotePath, &attr) != 0)
    {
      Error("ERROR: Cannot stat file '%s' on server side.\n", remotePath);

      goto fail;
    }

    //
    // Open remote file on sftp server.
    //

    sftpHandle = sftp -> open(remotePath);

    if (sftpHandle == -1)
    {
      Error("ERROR: Cannot open '%s' file on server.\n", remotePath);

      goto fail;
    }

    //
    // Open file on local.
    //

    f = fopen(localPath, "wb+");

    FAILEX(f == NULL, "ERROR: Cannot create '%s' file.\n", localPath);

    //
    // Download file from server to local by 32kB blocks.
    //

    bytesToCopy = attr.size_;

    while(bytesToCopy > 0)
    {
      //
      // Check for stopped state.
      //

      if (job -> getState() == SFTP_JOB_STATE_STOPPED)
      {
        DEBUG1("SftpJob PTR#%p: Downlading [%s] stopped.\n", job, localPath);

        exitCode = 0;

        goto fail;
      }

      //
      // Compute next piece size.
      //


      pieceSize = min(int64_t(sizeof(buf)), bytesToCopy);

      //
      // Read piece from sftp server.
      //

      readed = sftp -> read(sftpHandle, buf, totalWritten, pieceSize);

      FAILEX(readed != pieceSize,
                 "ERROR: Cannot read from remote file.\n");

      //
      // Write readed part to local file.
      //

      fwrite(buf, pieceSize, 1, f);

      //
      // Go to next part.
      //

      bytesToCopy  -= pieceSize;
      totalWritten += pieceSize;

      //
      // Compute time statistics.
      //

      job -> updateStatistics(totalWritten, attr.size_);
    }

    //
    // Set job state as finished.
    //

    job -> setState(SFTP_JOB_STATE_FINISHED);

    //
    // Clean up.
    //

    exitCode = 0;

    fail:

    if (exitCode)
    {
      Error("ERROR: Cannot download file from '%s' to '%s'.\n", remotePath, localPath);

      job -> setState(SFTP_JOB_STATE_ERROR);
    }

    if (f)
    {
      fclose(f);
    }

    if (sftp && sftpHandle != -1)
    {
      sftp -> close(sftpHandle);
    }

    job -> release();

    DBG_LEAVE2("SftpClient::DownloadFile");

    return exitCode;
  }

  //
  // Download file from sftp server.
  //
  // WARNING: Returned SftpJob object MUST be free by calling job -> release()
  //          method.
  //
  // TIP#1 : Use job -> wait() method to wait until job finished.
  // TIP#2 : Use job -> stop() method to stop job before finished.
  //
  // localPath      - local path, where to write downloaded file (IN).
  // remotePath     - full remote path on server side pointing to file to download (IN).

  // notifyCallback - function to be called when new transfer statistics arrives
  //                  or job's state changed. Optional, can be NULL (IN/OPT).
  //
  //
  // RETURNS: Pointer to new allocated SftpJob object,
  //          or NULL if error.
  //

  SftpJob *SftpClient::downloadFile(const char *localPath,
                                        const char *remotePath,
                                            SftpJobNotifyCallbackProto notifyCallback)
  {
    DBG_ENTER2("SftpClient::downloadFile");

    SftpJob *job = NULL;

    ThreadHandle_t *thread = NULL;

    //
    // Check args.
    //

    FAILEX(localPath == NULL, "ERROR: 'localPath' cannot be NULL in DownloadFile().\n");
    FAILEX(remotePath == NULL, "ERROR: 'remotePath' cannot be NULL in DownloadFile().\n");
    FAILEX(dead_, "ERROR: download job rejected because session dead.\n");

    //
    // Create new SFTP job.
    //

    job = new SftpJob(SFTP_JOB_TYPE_DOWNLOAD, this,
                          localPath, remotePath, notifyCallback);

    //
    // Create worker thread.
    //

    thread = ThreadCreate(DownloadFileWorker, job);

    job -> setThread(thread);

    //
    // Set state to pending.
    // It's signal for worker thread to start work.
    //

    job -> setState(SFTP_JOB_STATE_PENDING);

    //
    // Error handler.
    //

    fail:

    DBG_LEAVE2("SftpClient::downloadFile");

    return job;
  }

  //
  // Upload file to sftp server in background thread.
  // Internal use only. See uploadFile() method.
  //
  // data - pointer to related SftpJob object (this pointer) (IN/OUT).
  //

  int SftpClient::UploadFileWorker(void *data)
  {
    DBG_ENTER2("SftpClient::UploadFileWorker");

    int exitCode = -1;

    struct stat info = {0};

    int64_t sftpHandle = -1;

    int pieceSize   = 0;

    int64_t bytesToCopy  = 0;
    int64_t totalWritten = 0;
    int64_t written      = 0;
    int64_t readed       = 0;

    FILE *f = NULL;

    char buf[1024 * 32];

    SftpJob *job = (SftpJob *) data;

    SftpClient *sftp = job -> getSftpClient();

    const char *remotePath = job -> getRemoteName();
    const char *localPath  = job -> getLocalName();

    //
    // Add refference couner to related SFTP job to avoid
    // delete it while this thread works.
    //

    job -> addRef();

    //
    // Wait until job state is initializing.
    //

    while(job -> getState() == SFTP_JOB_STATE_INITIALIZING)
    {
      ThreadSleepMs(10);
    }

    //
    // Stat local file.
    // We get total size of file here.
    //

    if (::stat(localPath, &info) != 0)
    {
      Error("ERROR: Cannot stat local file '%s'.\n", localPath);

      goto fail;
    }

    //
    // Open local file for reading.
    //

    f = fopen(localPath, "rb");

    FAILEX(f == NULL, "ERROR: Cannot open local '%s' file.\n", localPath);

    //
    // Open remote file on sftp server for writing.
    //

    sftpHandle = sftp -> open(remotePath, SSH2_FXF_WRITE | SSH2_FXF_CREAT | SSH2_FXF_TRUNC);

    if (sftpHandle == -1)
    {
      Error("ERROR: Cannot open '%s' file on server.\n", remotePath);

      goto fail;
    }

    //
    // Upload file from local to server 32kB blocks.
    //

    bytesToCopy = info.st_size;

    while(bytesToCopy > 0)
    {
      //
      // Check for stopped state.
      //

      if (job -> getState() == SFTP_JOB_STATE_STOPPED)
      {
        DEBUG1("SftpJob PTR#%p: Uploading [%s] stopped.\n", job, localPath);

        exitCode = 0;

        goto fail;
      }

      pieceSize = min(int64_t(sizeof(buf)), bytesToCopy);

      //
      // Read piece from local.
      //

      readed = fread(buf, 1, pieceSize, f);

      FAILEX(readed != pieceSize,
                 "ERROR: Cannot read from local file.\n");

      //
      // Write piece to remote file on sftp server.
      //

      written = sftp -> write(sftpHandle, buf, totalWritten, pieceSize);

      FAILEX(written != pieceSize,
                 "ERROR: Cannot write to remote file.\n");

      //
      // Go to next part.
      //

      bytesToCopy  -= pieceSize;
      totalWritten += pieceSize;

      //
      // Compute time statistics.
      //

      job -> updateStatistics(totalWritten, info.st_size);
    }

    //
    // Set job state as finished.
    //

    job -> setState(SFTP_JOB_STATE_FINISHED);

    //
    // Clean up.
    //

    exitCode = 0;

    fail:

    if (exitCode)
    {
      Error("ERROR: Cannot upload file from '%s' to '%s'.\n", remotePath, localPath);

      job -> setState(SFTP_JOB_STATE_ERROR);
    }

    if (f)
    {
      fclose(f);
    }

    if (sftp && sftpHandle != -1)
    {
      sftp -> close(sftpHandle);
    }

    job -> release();

    DBG_LEAVE2("SftpClient::UploadFileWorker");

    return exitCode;
  }

  //
  // Upload file to sftp server.
  //
  // WARNING: Returned SftpJob object MUST be free by calling job -> release()
  //          method.
  //
  // TIP#1 : Use job -> wait() method to wait until job finished.
  // TIP#2 : Use job -> stop() method to stop job before finished.
  //
  // remotePath     - full remote path on server side, where to put file
  //                  (containing file name too) (IN).
  //
  // localPath      - local path, with file to upload (IN).
  //
  // notifyCallback - function to be called when new transfer statistics arrives
  //                  or job's state changed. Optional, can be NULL (IN/OPT).
  //
  //
  // RETURNS: 0 if OK.
  //

  SftpJob *SftpClient::uploadFile(const char *remotePath,
                                      const char *localPath,
                                          SftpJobNotifyCallbackProto notifyCallback)
  {
    DBG_ENTER2("SftpClient::uploadFile");

    SftpJob *job = NULL;

    ThreadHandle_t *thread = NULL;

    //
    // Check args.
    //

    FAILEX(localPath == NULL, "ERROR: 'localPath' cannot be NULL in UploadFile().\n");
    FAILEX(remotePath == NULL, "ERROR: 'remotePath' cannot be NULL in UploadFile().\n");
    FAILEX(dead_, "ERROR: upload job rejected because session dead.\n");

    //
    // Create new SFTP job.
    //

    job = new SftpJob(SFTP_JOB_TYPE_UPLOAD, this,
                          localPath, remotePath, notifyCallback);

    //
    // Create worker thread.
    //

    thread = ThreadCreate(UploadFileWorker, job);

    job -> setThread(thread);

    //
    // Set state to pending.
    // It's signal for worker thread to start work.
    //

    job -> setState(SFTP_JOB_STATE_PENDING);

    //
    // Error handler.
    //

    fail:

    DBG_LEAVE2("SftpClient::uploadFile");

    return job;
  }

  //
  // List content of remote directory in background thread.
  // Internal use only. See listFiles() method.
  //
  // data - pointer to related SftpJob object (this pointer) (IN/OUT).
  //

  int SftpClient::ListFilesWorker(void *data)
  {
    DBG_ENTER2("SftpClient::ListFilesWorker");

    int exitCode = -1;

    int64_t handle = -1;

    int goOn = 1;
    int iter = 0;

    string packet;

    uint32_t serverStatus = 0;

    SftpJob *job = (SftpJob *) data;

    SftpClient *sftp = job -> getSftpClient();

    const char *remotePath = job -> getRemoteName();

    //
    // Add refference couner to related SFTP job to avoid
    // delete it while this thread works.
    //

    job -> addRef();

    //
    // Wait until job state is initializing.
    //

    while(job -> getState() == SFTP_JOB_STATE_INITIALIZING)
    {
      ThreadSleepMs(10);
    }

    //
    // Open remote directory on sftp server.
    //

    handle = sftp -> opendir(remotePath);

    if (handle < 0)
    {
      Error("Cannot open remote directory [%s].\n", remotePath);

      goto fail;
    }

    //
    // Send requests for data until EOF or error.
    //

    while(goOn)
    {
      uint32_t id    = GenerateUniqueId();
      uint32_t idRet = 0;
      uint32_t size  = 0;
      uint32_t count = 0;

      uint8_t type = 0;

      iter ++;

      //
      // Check for stopped state.
      //

      if (job -> getState() == SFTP_JOB_STATE_STOPPED)
      {
        DEBUG1("SftpJob PTR#%p: Listing [%s] stopped.\n", job, remotePath);

        exitCode = 0;

        goto fail;
      }

      //
      // Prepare SSH2_FXP_DIRLIGO_READDIR_SHORT message.
      //
      //
      // size      4
      // type      1
      // id        4
      // handleLen 4
      // handle    4
      // ----------------------------------
      //    total: 13 + 4
      //

      packet.clear();

      StrPushDword(packet, 13, STR_BIG_ENDIAN);               // size      4
      StrPushByte(packet, SSH2_FXP_DIRLIGO_READDIR_SHORT);    // type      1
      StrPushDword(packet, id, STR_BIG_ENDIAN);               // id        4
      StrPushDword(packet, 4, STR_BIG_ENDIAN);                // handleLen 4
      StrPushDword(packet, uint32_t(handle), STR_BIG_ENDIAN); // handle    4

      //
      // Send packet and wait for answer.
      //

      FAIL(sftp -> processPacket(packet, packet));

      DEBUG2("SFTP #%d: Sent [SSH2_FXP_DIRLIGO_READDIR_SHORT][%"PRId64"].\n", id, handle);

      //
      // Pop header:
      //
      // size 4
      // type 1
      // id   4
      //

      FAIL(StrPopDword(&size, packet, STR_BIG_ENDIAN));  // size 4
      FAIL(StrPopByte(&type, packet));                   // type 1
      FAIL(StrPopDword(&idRet, packet, STR_BIG_ENDIAN)); // id   4

      if (idRet != id)
      {
        Error("ERROR: Packet ID mismatch.\n");

        sftp -> shutdown();

        goto fail;
      }

      //
      // Interprete received message.
      //

      switch(type)
      {
        //
        // Status message. This means EOF or error.
        //

        case SSH2_FXP_STATUS:
        {
          DEBUG2("SFTP #%d: received [SSH2_FXP_STATUS].", id);

          goOn = 0;

          FAIL(StrPopDword(&serverStatus, packet, STR_BIG_ENDIAN));

          FAIL(serverStatus != SSH2_FX_EOF);

          DEBUG1("SFTP #%d: Readdir handle [%"PRId64"] finished with code [%d][%s].\n",
                     id, handle, serverStatus, TranslateSftpStatus(serverStatus));

          break;
        }

        //
        // SSH2_FXP_NAME. Next portion of data.
        //

        case SSH2_FXP_NAME:
        {
          DEBUG2("SFTP #%d: received [SSH2_FXP_NAME].", id);

          //
          // Pop number of elements.
          //

          FAIL(StrPopDword(&count, packet, STR_BIG_ENDIAN));

          DEBUG2("SFTP #%d: Popped elements count [%d].\n", id, count);

          //
          // Pop <count> elements from packet.
          //

          for (int i = 0; i < count; i++)
          {
            SftpFileInfo info;

            memset(&info.attr_, 0, sizeof(info.attr_));

            char name[MAX_PATH]     = {0};
            char longName[MAX_PATH] = {0};

            uint32_t nameLen     = 0;
            uint32_t longNameLen = 0;

            //
            // Pop element's name.
            //

            FAIL(StrPopDword(&nameLen, packet, STR_BIG_ENDIAN));
            FAIL(StrPopRaw(name, nameLen, packet));

            FAIL(StrPopDword(&longNameLen, packet, STR_BIG_ENDIAN));
            FAIL(StrPopRaw(longName, longNameLen, packet));

            DEBUG2("SFTP #%d: Popped element [%s] / [%s].\n", id, name, longName);

            info.name_ = name;

            //
            // Pop attributes.
            //

            FAIL(sftp -> popAttribs(&info.attr_, packet));

            //
            // Last entry marker. Don't go on longer.
            //

            if (strcmp(name, "...") == 0)
            {
              goOn = 0;
            }

            //
            // Put another element to list.
            //

            else
            {
              job -> addFile(info);
            }
          }

          DEBUG3("SFTP #%d: Listed next [%d] elements from [%s].\n", id, count, remotePath);

          job -> triggerNotifyCallback(SFTP_JOB_NOTIFY_FILES_LIST_ARRIVED);

          break;
        }

        //
        // Unexpected packet type. Break.
        //

        default:
        {
          Error("SFTP #%d: ERROR: Unexpected packet type [%d].", id, type);

          sftp -> shutdown();

          goto fail;
        }
      }
    }

    //
    // Set job state as finished.
    //

    job -> setState(SFTP_JOB_STATE_FINISHED);

    //
    // Clean up.
    //

    exitCode = 0;

    fail:

    if (exitCode)
    {
      Error("ERROR: Cannot list files from '%s'.\n", remotePath);

      job -> setState(SFTP_JOB_STATE_ERROR);
    }

    if (sftp && handle != -1)
    {
      sftp -> close(handle);
    }

    job -> release();

    DBG_LEAVE2("SftpClient::ListFiles");

    return exitCode;
  }

  //
  // Asynchronous list content of remote directory.
  //
  // WARNING: Returned SftpJob object MUST be free by calling job -> release()
  //          method.
  //
  // TIP#1 : Use job -> wait() method to wait until job finished.
  // TIP#2 : Use job -> stop() method to stop job before finished.
  //
  // TIP#3 : Use job -> getFiles() method inside notifyCallback to receive next
  //         part of files, wchich was listed.
  //
  // Parameters:
  //
  // remotePath     - full remote directory to list e.g. "/home" (IN).
  //
  // notifyCallback - function to be called when new part of file list arrived
  //                  from network (IN).
  //
  // RETURNS: 0 if OK.
  //

  SftpJob *SftpClient::listFiles(const char *remotePath,
                                     SftpJobNotifyCallbackProto notifyCallback)
  {
    DBG_ENTER2("SftpClient::listFiles");

    SftpJob *job = NULL;

    ThreadHandle_t *thread = NULL;

    //
    // Check args.
    //

    FAILEX(remotePath == NULL, "ERROR: 'remotePath' cannot be NULL in ListFiles().\n");
    FAILEX(dead_, "ERROR: list job rejected because session dead.\n");

    //
    // Create new SFTP job.
    //

    job = new SftpJob(SFTP_JOB_TYPE_LIST, this,
                          NULL, remotePath, notifyCallback);

    //
    // Create worker thread.
    //

    thread = ThreadCreate(ListFilesWorker, job);

    job -> setThread(thread);

    //
    // Set state to pending.
    // It's signal for worker thread to start work.
    //

    job -> setState(SFTP_JOB_STATE_PENDING);

    //
    // Error handler.
    //

    fail:

    DBG_LEAVE2("SftpClient::listFiles");

    return job;
  }

  //
  // Change interval telling how often network statistics are computed.
  //
  // tick - statistics tick in number of processed packets (IN).
  //

  void SftpClient::setNetStatTick(int tick)
  {
    netStatTick_ = tick;
  }

  //
  // Register callback function called when number of packets touch
  // net statistics tick set by setNetStatTick() function.
  //
  // callback - function to be called when new statistics arrived (IN).
  // ctx      - caller context passed to callback function directly (IN/OPT).
  //

  void SftpClient::registerNetStatCallback(SftpNetStatCallbackProto callback, void *ctx)
  {
    netStatCallback_    = callback;
    netStatCallbackCtx_ = ctx;
  }

  //
  // Set timeout in seconds after, which read/write operation is stopped
  // and only partial result is returned (e.g. readed 1024 bytes, when
  // caller wanted 2048).
  //
  // readThreshold - read threshold in seconds (IN).
  // writeThreshold - write threshold in seconds (IN).
  //

  void SftpClient::setPartialThreshold(int readThreshold, int writeThreshold)
  {
    partialReadThreshold_  = readThreshold * 1000;
    partialWriteThreshold_ = writeThreshold * 1000;
  }

  //
  // Set callback function called when underlying connection dead.
  //

  void SftpClient::registerConnectionDroppedCallback(SftpConnectionDroppedProto callback,
                                                    void *ctx)
  {
    connectionDroppedCallback_    = callback;
    connectionDroppedCallbackCtx_ = ctx;

    DEBUG1("Registered connection dropped callback"
               " ptr [%p], ctx [%p].\n", callback, ctx);
  }

  void SftpClient::shutdown()
  {
    DBG_INFO("SFTP: Shutted down.\n");

    dead_ = 1;

    if (connectionDroppedCallback_)
    {
      connectionDroppedCallback_(this, connectionDroppedCallbackCtx_);
    }
  }
} /* namespace Tegenaria */
