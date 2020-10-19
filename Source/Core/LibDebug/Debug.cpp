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

#undef DEBUG

#include "Debug.h"
#include "Config.h"

namespace Tegenaria
{
  #ifdef WIN32
  # define vsnprintf _vsnprintf
  #else
  # undef stricmp
  # define stricmp strcasecmp
  #endif

  extern DebugConfig_t DebugConfig;

  using std::min;
  using std::string;

  //
  // Used to debug itself.
  // Needed for LibDebug developing only.
  //

  #undef  SELF_DEBUG

  #ifdef SELF_DEBUG
  # undef DBG_ENTER
  # undef DBG_LEAVE
  # undef DBG_MSG

  # define DBG_ENTER(X) fprintf(stderr, "-> " X "()...\n")
  # define DBG_LEAVE(X) fprintf(stderr, "<- " X "()...\n")
  # define DBG_MSG(...) fprintf(stderr, __VA_ARGS__)
  #endif

  //
  // Defines.
  //

  #define DBG_MSG_BUF_SIZE (1024 * 64)

  //
  // Initialize:
  //
  // - main log file
  // - state.<pid> file.
  // - state-history.<pid> file.
  // - save pid of current process.
  //
  // fname - file path, where to store main log file.
  //         Defaulted to stderr if NULL or file cannot be created (IN/OPT).
  //
  //
  // level - one of DBG_LEVEL_XXX (see Debug.h) level to set.
  //         Defaulted to DBG_LEVEL_DEFAULT if -1. (IN/OPT).
  //
  // flags - combination of DBG_XXX flags degined in Debug.h.
  //         Defaulted to DBG_FLAGS_DEFAULT if -1. (IN/OPT).
  //

  void DbgInit(const char *fname, int logLevel, int flags)
  {
    DBG_ENTER("DbgInit");

    //
    // Flags changed.
    //

    int reinit = 0;

    if (flags != -1)
    {
      DBG_MSG("DbgInit : Flags changed from [0x%x] to [0x%x].\n", DebugConfig.flags_, flags);

      reinit = 1;
    }

    //
    // Do reinit if log file changed.
    //

    else if (fname)
    {
      DBG_MSG("DbgInit : Log file changed to [%s], going to reinit...\n", fname);

      reinit = 1;
    }

    //
    // No reinit needed, but log level changed.
    // Change log level only
    //

    else if (logLevel != -1)
    {
      DBG_MSG("DbgInit : Log level changed to [%d].\n", logLevel);

      DbgSetLevel(logLevel);
    }

    //
    // Avoid calling DbgInit twice.
    // Do real work only if first time or reinit needed.
    //

    if (DebugConfig.init_ == 0 || reinit)
    {
      DBG_MSG("DbgInit : Not inited yet.\n");

      //
      // Retrieve current running pid.
      //

      #ifdef WIN32
      DebugConfig.pid_ = GetCurrentProcessId();
      #else
      DebugConfig.pid_ = getpid();
      #endif

      //
      // Try load config from .debugConfig.<pid>.
      // If .debugConfig.<pid> not found init and save own one.
      //

      if (reinit || DbgLoadConfig(&DebugConfig) != 0)
      {
        char path[260] = {0};

        DBG_MSG("DbgInit : Config not found.\n");

        //
        // Default flags and logLevel if not specified.
        //

        if (logLevel == -1)
        {
          DBG_MSG("DbgInit : Defaulted log level to [%d].\n", DBG_LEVEL_DEFAULT);

          logLevel = DBG_LEVEL_DEFAULT;
        }

        if (flags == -1)
        {
          DBG_MSG("DbgInit : Defaulted flags to [%d].\n", DBG_FLAGS_DEFAULT);

          flags = DBG_FLAGS_DEFAULT;
        }

        //
        // Clear DBG_REINIT flag.
        //

        flags = flags & (~DBG_REINIT);

        DebugConfig.init_     = 1;
        DebugConfig.flags_    = flags;
        DebugConfig.logLevel_ = logLevel;

        DBG_MSG("DbgInit : Set flags to [0x%x].\n", DebugConfig.flags_);
        DBG_MSG("DbgInit : Set loglevel to [%d].\n", DebugConfig.logLevel_);

        //
        // Init main log over selected file if specified.
        //

        DBG_MSG("DbgInit : Log file is set to [%s].\n", fname);

        if (fname)
        {
          strncpy(path, fname, sizeof(path));

          //
          // DBG_SEPARATE_PROCESSES flag.
          // Convert 'fname' to 'fname.<pid>'.
          //

          if (flags & DBG_SEPARATE_PROCESSES)
          {
            snprintf(path, sizeof(path), "%s.%d", fname, DebugConfig.pid_);
          }

          //
          // DBG_SEPARATE_THREADS flag.
          // Convert 'path' to 'path.<threadid>'.
          //

          if (flags & DBG_SEPARATE_THREADS)
          {
            fprintf(stderr, "WARNING: DBG_SEPARATE_THREADS not implemented.");
          }

          //
          // Open main log file.
          //

          DBG_MSG("DbgInit : Opening file [%s]...\n", path);

          DebugConfig.logFd_ = open(path, O_APPEND | O_CREAT | O_WRONLY,
                                        S_IREAD | S_IWRITE);

          DBG_MSG("DbgInit : Retrieved FD : [%d].\n", DebugConfig.logFd_);
        }

        //
        // Cannot create main log file of filename not specified.
        // Use stderr.
        //

        if (DebugConfig.logFd_ < 0 || fname == NULL)
        {
          DebugConfig.logFd_ = 2;
        }

        //
        // Init io.<pid> file if needed.
        //

        if (flags & DBG_IO_ENABLE)
        {
          if (fname)
          {
            snprintf(path, sizeof(path), "%s.io.%d.log", fname, DebugConfig.pid_);
          }
          else
          {
            snprintf(path, sizeof(path), "io.%d.log", DebugConfig.pid_);
          }

          DBG_MSG("DbgInit : Opening io file [%s]...\n", path);

          DebugConfig.ioFd_ = open(path, O_APPEND | O_CREAT | O_WRONLY,
                                       S_IREAD | S_IWRITE);

          DBG_MSG("DbgInit : Retrieved FD : [%d].\n", DebugConfig.ioFd_);
        }
        else
        {
          DebugConfig.ioFd_ = -1;
        }

        //
        // Save current config for other modules.
        //

        DbgSaveConfig(&DebugConfig);

        //
        // Init state.<pid> and state-history.<pid> file if needed.
        // See State.cpp for more.

        if (flags & DBG_STATE_ENABLE)
        {
          DBG_MSG("DbgInit : Going to init state log with fname = [%s]...\n", fname);

          DbgSetInit(fname);
        }
      }
    }

    DBG_LEAVE("DbgInit");
  }

  //
  // Wrapper for DbgInit() working with human readable string
  // as log level name.
  // See DbgInit() and TranslateLogName() for more.
  //

  void DbgInit(const char *fname, const char *levelName, int flags)
  {
    int level = DbgTranslateLevelName(levelName);

    DbgInit(fname, level, flags);
  }

  //
  // Generate '[pid][threadID] time' prefix.
  //
  // prefix     - buffer, where to store created prefix (OUT).
  // prefixSize - size of buffer[] in bytes (IN).
  // level      - log level to generate prefix if DBG_LOGLEVEL_PREFIX
  //              flag set (IN).
  //

  void DbgGetPrefix(char *prefix, int prefixSize, int level)
  {
    //
    // Windows.
    //

    #ifdef WIN32
    {
      SYSTEMTIME st;

      GetSystemTime(&st);

      snprintf(prefix, prefixSize, "[%05d][%05d] %02d:%02d:%02d %03d",
                   GetCurrentProcessId(), GetCurrentThreadId(),
                       (int) st.wHour, (int) st.wMinute,
                           (int) st.wSecond, (int) st.wMilliseconds);
    }

    //
    // Linux, Mac.
    //

    #else
    {
      struct timeval nowMs;

      gettimeofday(&nowMs, NULL);

      int ms = nowMs.tv_usec / 1000;

      int threadId = (int) syscall (SYS_gettid);

      char buf[80];

      struct tm localTimeBuffer;

      time_t now = time(0);

      struct tm tstruct = *localtime_r(&now, &localTimeBuffer);

      strftime(buf, sizeof(buf), "%X", &tstruct);

      snprintf(prefix, prefixSize, "[%05d][%05d] %s %03d ",
                   getpid(), threadId, buf, ms);
    }
    #endif

    //
    // Add log level name if needed.
    //

    if ((DebugConfig.flags_ & DBG_LOGLEVEL_PREFIX) &&
            level >= 0 && level <= DBG_LEVEL_MAX)
    {
      const char *knownNames[] =
      {
        "", " ERROR:", " Info:", " Debug1:", " Debug2:", " Debug3:"
      };

      strncat(prefix, knownNames[level], prefixSize);
    }
  }

  //
  // Put formatted message into main log.
  //
  // TIP#1: Default log file is stderr. To redirect it to file,
  //        use DBG_INIT(logfile).
  //
  // TIP#2: To change log level in current process use
  //        DBG_INIT_EX(logfile, level, flags) instead of DBG_INIT.
  //
  // level    - requested log level, if less than log level for current
  //            process nothing happen. See DBG_LEVEL_XXX in Debug.h (IN).
  //
  // fmt, ... - printf like arguments to format message (IN).
  //

  void DbgMsg(int level, const char *fmt, ...)
  {
    char timestamp[128];

    char *msg = NULL;

    char *timestampMsg = NULL;

    char *src = NULL;
    char *dst = NULL;

    int len = 0;
    int spaceLeft = 0;

    int timeStampLen = 0;

    size_t msgSize = DBG_MSG_BUF_SIZE - 1 - sizeof("\n");

    va_list ap;

    //
    // Check log level.
    //

    DbgInit(NULL, -1, -1);

    if (level > DebugConfig.logLevel_)
    {
      DBG_MSG("DbgMsg : Rejected message level %d (current %d).\n",
                  level, DebugConfig.logLevel_);

      return;
    }

    if (level < 0 || level > DBG_LEVEL_MAX)
    {
      DBG_MSG("DbgMsg : Rejected message with invalid level %d.\n", level);

      return;
    }

    //
    // Allocate buffers.
    //

    msg          = (char *) calloc(1, DBG_MSG_BUF_SIZE + 1);
    timestampMsg = (char *) calloc(1, DBG_MSG_BUF_SIZE + 1);

    //
    // Get time prefix.
    //

    DbgGetPrefix(timestamp, sizeof(timestamp), level);

    //
    // Create message.
    //

    va_start(ap, fmt);
    len = vsnprintf(msg, msgSize, fmt, ap);
    va_end(ap);

    len = strlen(msg);

    //
    // Remove end of line.
    //

    while (len > 0 && (msg[len - 1] == 13 || msg[len - 1] == 10))
    {
      msg[len - 1] = 0;

      len --;
    }

    //
    // Write error on stderr too, but only
    // if logfile is not stder itself.
    //

    if (DebugConfig.logFd_ != 2 && level == DBG_LEVEL_ERROR)
    {
      int written = -1;

      strcpy(msg + len, "\n");

      written = write(2, msg, len + sizeof("\n") - 1);

      msg[len] = 0;
    }

    //
    // Don't write timestamp if logs passed to stderr and
    // ERROR or INFO level set.
    //

    if (DebugConfig.logFd_ == 2 && DebugConfig.logLevel_ <= DBG_LEVEL_INFO)
    {
      if (msg[len - 1] != 10 && msg[len - 1] != 13)
      {
        msg[len]     = 10;
        msg[len + 1] = 0;
      }

      len = write(DebugConfig.logFd_, msg, strlen(msg));
    }
    else
    {
      snprintf(timestampMsg, DBG_MSG_BUF_SIZE - 1, "%s %s", timestamp, msg);

      //
      // Change "\n" to SPC(<length of timestamp>).
      //

      src = timestampMsg;
      dst = msg;

      timeStampLen = strlen(timestamp) + 2;

      spaceLeft = DBG_MSG_BUF_SIZE - 1;

      for (; *src; src ++)
      {
        switch(*src)
        {
          case 13:
          {
            break;
          }

          case 10:
          {
            if (spaceLeft >= timeStampLen + 1)
            {
              dst[0] = 10;

              memset(dst + 1, ' ', timeStampLen);

              dst += timeStampLen;

              spaceLeft -= timeStampLen;
            }

            break;
          }

          default:
          {
            if (spaceLeft >= 1)
            {
              *dst = *src;

              dst ++;

              spaceLeft --;
            }
          }
        }
      }

      if (dst > msg && dst[-1] != 10 && dst[-1] != 13)
      {
        dst[0] = 10;
        dst[1] = 0;
      }

      len = write(DebugConfig.logFd_, msg, strlen(msg));
    }

    //
    // Free buffers.
    //

    if (msg)
    {
      free(msg);
    }

    if (timestampMsg)
    {
      free(timestampMsg);
    }
  }

  //
  // Put formatted message into I/O log.
  //
  // TIP#1: Disabled as default, use DBG_IO_ENABLE flag in DBG_INIT_EX.
  //
  // TIP#2: IO log is written to io.<pid> file.
  //
  // TIP#3: Use DBG_IO_XXX() macros.
  //
  // level    - requested log level, if less than log level for current
  //            process nothing happen. See DBG_LEVEL_XXX in Debug.h (IN).
  //
  // fmt, ... - printf like arguments to format message (IN).
  //

  void DbgIoMsg(int level, const char *fmt, ...)
  {
    char timestamp[128];

    char *msg = NULL;

    char *timestampMsg = NULL;

    char *src = NULL;
    char *dst = NULL;

    int len = 0;

    int timeStampLen = 0;

    size_t msgSize = DBG_MSG_BUF_SIZE - 1 - sizeof("\n");

    va_list ap;

    //
    // Check log level.
    //

    DbgInit(NULL, -1, -1);

    if (level > DebugConfig.logLevel_)
    {
      DBG_MSG("DbgIoMsg : Rejected message level %d (current %d).\n",
                  level, DebugConfig.logLevel_);

      return;
    }

    if (DebugConfig.ioFd_ == -1)
    {
      DBG_MSG("DbgIoMsg : Rejected message - IO log not opened.");

      return;
    }

    if (level < 0 || level > DBG_LEVEL_MAX)
    {
      DBG_MSG("DbgIoMsg : Rejected message with invalid level %d.\n", level);

      return;
    }

    //
    // Allocate buffers.
    //

    msg          = (char *) calloc(1, DBG_MSG_BUF_SIZE + 1);
    timestampMsg = (char *) calloc(1, DBG_MSG_BUF_SIZE + 1);

    //
    // Get time prefix.
    //

    DbgGetPrefix(timestamp, sizeof(timestamp), level);

    //
    // Create message.
    //

    va_start(ap, fmt);
    len = vsnprintf(msg, msgSize, fmt, ap);
    va_end(ap);

    len = strlen(msg);

    //
    // Remove end of line.
    //

    while (len > 0 && (msg[len - 1] == 13 || msg[len - 1] == 10))
    {
      msg[len - 1] = 0;

      len --;
    }

    //
    // Change "\n" to SPC(<length of timestamp>).
    //

    snprintf(timestampMsg, DBG_MSG_BUF_SIZE - 1, "%s %s", timestamp, msg);

    src = timestampMsg;
    dst = msg;

    timeStampLen = strlen(timestamp) + 2;

    for (; *src; src ++)
    {
      switch(*src)
      {
        case 13:
        {
          break;
        }

        case 10:
        {
          dst[0] = 10;

          memset(dst + 1, ' ', timeStampLen);

          dst += timeStampLen;

          break;
        }

        default:
        {
          *dst = *src;

          dst ++;
        }
      }
    }

    if (dst > msg && dst[-1] != 10 && dst[-1] != 13)
    {
      dst[0] = 10;
      dst[1] = 0;
    }

    len = write(DebugConfig.ioFd_, msg, strlen(msg));

    //
    // Free buffers.
    //

    if (msg)
    {
      free(msg);
    }

    if (timestampMsg)
    {
      free(timestampMsg);
    }
  }

  //
  // Put formatted header into log file in form:
  //
  // ------------------------------------------------
  // -    some printf like formatted message here   -
  // ------------------------------------------------
  //
  // level - requested log level, if current level is less nothing happen (IN).
  // fmt   - printf like args to fomat message (IN).
  //

  void DbgHead(int level, const char *fmt, ...)
  {
    int maxLen = 0;

    const int wide = 60;

    char *msg = NULL;

    char *line = NULL;

    int len = 0;

    char buf[wide + 1] = {0};

    va_list ap;

    //
    // Check log level.
    //

    DbgInit(NULL, -1, -1);

    //
    // Allocate buffer.
    //

    msg = (char *) calloc(1, DBG_MSG_BUF_SIZE + 1);

    //
    // Format message.
    //

    va_start(ap, fmt);

    vsnprintf(msg, DBG_MSG_BUF_SIZE - 1, fmt, ap);

    va_end(ap);

    //
    // Top.
    //

    memset(buf, '-', wide);

    DbgMsg(level, "\n%s\n", buf);

    //
    // Split message to lines.
    //

    line = strtok((char *) msg, "\n");

    while (line)
    {
      int len  = strlen(line);
      int skip = (wide - 2 - len) / 2;

      memset(buf + 1,              ' ', skip);
      memset(buf + 1 + skip + len, ' ', wide - skip - len - 2);

      memcpy(buf + 1 + skip, line, len);

      DbgMsg(level, "%s\n", buf);

      line = strtok(NULL, "\n");
    }

    //
    // Bottom.
    //

    memset(buf, '-', wide);

    DbgMsg(level, "%s\n \n", buf);

    //
    // Free buffers.
    //

    if (msg)
    {
      free(msg);
    }
  }

  //
  // Dump raw buffer to log as hex values.
  //
  // buf  - buffer to dump (IN).
  // size - size of buffer in bytes (IN).
  //

  void DbgDump(void *buf, int size)
  {
    if (DebugConfig.logLevel_ < DBG_LEVEL_DEBUG5)
    {
      return;
    }

    string s;

    unsigned char *src = (unsigned char *) buf;

    //
    // FIXME: Handle bigger buffers.
    //

    size = min(size, 1024 * 32);

    for (int i = 0; i < size; i++)
    {
      char tmp[8];

      if (i > 0)
      {
        if (i % 4 == 0)
        {
          s += "  ";
        }

        if (i % 16 == 0)
        {
          s += "\n";
        }
      }

      snprintf(tmp, sizeof(tmp), "%02x ", (unsigned char) src[i]);

      s += tmp;
    }

    DbgMsg(DBG_LEVEL_DEBUG5, s.c_str());
  }

  //
  //
  //

  void DbgEnterEx(int level, const char *fname, const char *fmt, ...)
  {
    char *msg = NULL;

    va_list ap;

    //
    // Allocate buffer.
    //

    msg = (char *) calloc(DBG_MSG_BUF_SIZE + 1, 1);

    //
    // Format args.
    //

    va_start(ap, fmt);

    vsnprintf(msg, DBG_MSG_BUF_SIZE - 1, fmt, ap);

    va_end(ap);

    //
    // Print '-> fname(MSG)...' log.
    //

    DbgMsg(level, "-> %s(%s)...\n", fname, msg);

    //
    // Free buffer.
    //

    if (msg)
    {
      free(msg);
    }
  }

  //
  // Change log level for current running process.
  //
  // level - one of DBG_LEVEL_XXX values from Debug.h. (IN).
  //
  // RETURNS: 0 if OK.
  //

  int DbgSetLevel(int level)
  {
    DbgInit(NULL, -1, -1);

    if (level < 0 || level > DBG_LEVEL_MAX)
    {
      DbgMsg(DBG_LEVEL_ERROR, "ERROR: Wrong log level '%d'.\n", level);

      return -1;
    }
    else
    {
      DBG_MSG("DbgSetLevel : Changed level to [%d].\n", level);

      DebugConfig.logLevel_ = level;

      //
      // Save current config for other modules.
      //

      DbgSaveConfig(&DebugConfig);

      return 0;
    }
  }

  //
  // Translate human readable log level value.
  //
  // levelName - Name of log level to set. Supported values are:
  //             none, error, info, debug1, debug2 or debug3 or
  //             number. Names are case insensitve - INFO and info are
  //             the same (IN).
  //
  // Examples:
  //   DbgTranslateLevelName("info") gives 2
  //   DbgTranslateLevelName("2")    gives 2
  //   DbgTranslateLevelName("bla")  gives -1
  //
  // RETURNS: One of DBG_LEVEL_XXX values from Debug.h,
  //          or -1 if unknown level name.
  //

  int DbgTranslateLevelName(const char *levelName)
  {
    //
    // Log level in decimal form.
    //

    if (levelName && levelName[0] >= '0' && levelName[0] <= '9')
    {
      int level = atoi(levelName);

      if (level >= 0 && level <= DBG_LEVEL_MAX)
      {
        return level;
      }
    }

    //
    // Log level in name form.
    //

    else if (levelName)
    {
      const char *knownNames[] = {"none", "error", "info", "debug1", "debug2", "debug3", "debug4", "debug5", NULL};

      for (int i = 0; knownNames[i]; i++)
      {
        if (stricmp(levelName, knownNames[i]) == 0)
        {
          return i;
        }
      }
    }

    DbgMsg(DBG_LEVEL_ERROR, "ERROR: Unknown log level '%s'.\n", levelName);

    return -1;
  }

  //
  // Wrapper for DbgSetLevel(int) working with human readable names.
  // It can be used with name readed from text config file directly.
  //
  // levelName - Name of log level to set. Supported values are:
  //             none, error, info, debug1, debug2 or debug3.
  //             Names are case insensitve - INFO and info are the same (IN).
  //
  // RETURNS: 0 if OK.
  //

  int DbgSetLevel(const char *levelName)
  {
    int level = DbgTranslateLevelName(levelName);

    return DbgSetLevel(level);
  }

  //
  // Dump read/write operation into IO log.
  //
  // level      - requested log level, if less than log level for current
  //              process nothing happen. See DBG_LEVEL_XXX in Debug.h (IN).
  //
  // operation  - one of DBG_IO_OPERATION_XXX code defined in Debug.h (IN)
  //
  // fdType     - type of underlying IO object eg. "FD", "Socket" (IN).
  //
  // fdId64     - 64-bit ID of related IO object eg. socket number or pointer
  //              to NetConnection object (IN).
  //
  // buf        - buffer related with operation (IN).
  //
  // count      - size of operation in bytes (IN).
  //
  // sourceFile - source file name, where operation is called (IN).
  //
  // sourceLine - number of code line in source file (IN).
  //

  void DbgIoDumpOperation(int level, int operationCode,
                              const char *fdType, uint64_t fdId64,
                                  const void *buf, int count,
                                      const char *sourceFile, int sourceLine)
  {
    char textBuf[16 + 1];

    int bufSize = std::max(0, std::min(16, count));

    char *inputBuf = (char *) buf;

    const char *textBufPostfix = "";
    const char *operationText = "Unknown";

    //
    // Convert binary buffer into ASCIZ text ready to write
    // with printf like function.
    //

    if (operationCode == DBG_IO_OPERATION_READING)
    {
      bufSize = 0;

      textBuf[0] = 0;
    }
    else
    {
      for (int i = 0; i < bufSize; i++)
      {
        switch(inputBuf[i])
        {
          case 0:
          case 10:
          case 13:
          {
            textBuf[i] = '.';

           break;
          }

          default:
          {
            textBuf[i] = inputBuf[i];
          }
        }
      }

      textBuf[bufSize] = 0;
    }

    //
    // Adjust operation name human readable string.
    //

    switch(operationCode)
    {
      case DBG_IO_OPERATION_READING: operationText = "Reading"; break;
      case DBG_IO_OPERATION_READED:  operationText = "Readed "; break;
      case DBG_IO_OPERATION_WRITING: operationText = "Writing"; break;
      case DBG_IO_OPERATION_WRITTEN: operationText = "Written"; break;
    }

    //
    // Append '...' postfix if we print only part of buffer.
    //

    if (count > 16 && operationCode != DBG_IO_OPERATION_READING)
    {
      textBufPostfix = "...";
    }

    //
    // Dump operation into IO log.
    //

    if (fdId64 > 65535)
    {
      DbgIoMsg(level, "%s: (%s 0x%"PRIx64", %d bytes, buf=[%s%s]) (file '%s', line %d)...\n",
                   operationText, fdType, fdId64, count, textBuf, textBufPostfix, sourceFile, sourceLine);
    }
    else
    {
      DbgIoMsg(level, "%s: (%s #%"PRIu64", %d bytes, buf=[%s%s]) (file '%s', line %d)...\n",
                   operationText, fdType, fdId64, count, textBuf, textBufPostfix, sourceFile, sourceLine);
    }
  }

} /* namespace Tegeneria */
