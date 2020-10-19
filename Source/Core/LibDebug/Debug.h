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

#ifndef Tegenaria_Core_Debug_H
#define Tegenaria_Core_Debug_H

//
// Includes.
//

#include <cstdio>
#include <stdint.h>
#include <algorithm>

using std::min;

#ifdef WIN32
# include <windows.h>
#else
# include <errno.h>

# define GetLastError() errno
# define SetLastError(X) ((errno) = (X))
#endif

namespace Tegenaria
{
  //
  // Macros to get printf format to print int64_t type correctly.
  //

  #ifndef PRId64
  #  ifdef WIN32
  #    define PRId64 "I64d" // Windows32
  #  else
  #    ifdef __x86_64__
  #      define PRId64 "ld"  // Linux64
  #    else
  #      define PRId64 "lld" // Linux32
  #    endif
  #  endif
  #endif

  #ifndef PRIu64
  #  ifdef WIN32
  #    define PRIu64 "I64u" // Windows32
  #  else
  #    ifdef __x86_64__
  #      define PRIu64 "lu"  // Linux64
  #    else
  #      define PRIu64 "llu" // Linux32
  #    endif
  #  endif
  #endif

  #ifndef PRIx64
  #  ifdef WIN32
  #    define PRIx64 "I64x" // Windows32
  #  else
  #    ifdef __x86_64__
  #      define PRIx64 "lx"  // Linux64
  #    else
  #      define PRIx64 "llx" // Linux32
  #    endif
  #  endif
  #endif

  //
  // Defines.
  //

  //
  // Log levels passed to DBG_INIT_EX or to DBG_SET_LEVEL.
  //

  #define DBG_LEVEL_NONE   0  // All messages disabled.
  #define DBG_LEVEL_ERROR  1  // Fatal(), Error()
  #define DBG_LEVEL_INFO   2  // DBG_INFO, DBG_HEAD
  #define DBG_LEVEL_DEBUG1 3  // DEBUG1, DBG_MSG1, DBG_HEAD1, DBG_MSG
  #define DBG_LEVEL_DEBUG2 4  // DEBUG2, DBG_MSG2, DBG_HEAD2
  #define DBG_LEVEL_DEBUG3 5  // DEBUG3, DBG_MSG3, DBG_HEAD3
  #define DBG_LEVEL_DEBUG4 6  // DEBUG4, DBG_MSG4, DBG_HEAD4
  #define DBG_LEVEL_DEBUG5 7  // DEBUG5, DBG_MSG5, DBG_HEAD5, DBG_DUMP

  #define DBG_LEVEL_MAX     DBG_LEVEL_DEBUG5
  #define DBG_LEVEL_DEFAULT DBG_LEVEL_INFO

  //
  // Flags passed to DBG_INIT_EX.
  //

  #define DBG_SEPARATE_PROCESSES (1 << 0) // Split logs into <filename>.<pid> files.
  #define DBG_SEPARATE_THREADS   (1 << 1) // Split logs into <filename>.<threadid> files.
  #define DBG_TRACE              (1 << 2) // Use DBG_ENTER() and DBG_LEAVE().
  #define DBG_STATE_ENABLE       (1 << 3) // Use DBG_SET_XXX() macros.
  #define DBG_REINIT             (1 << 4) // Force init once again even if inited before.
  #define DBG_LOGLEVEL_PREFIX    (1 << 5) // Prepend messages by 'level name:' prefix.
  #define DBG_IO_ENABLE          (1 << 6) // Use DBG_IO_XXX macros.

  #define DBG_FLAGS_DEFAULT 0

  //
  // Known IO operations.
  //

  #define DBG_IO_OPERATION_READING 0
  #define DBG_IO_OPERATION_READED  1
  #define DBG_IO_OPERATION_WRITING 2
  #define DBG_IO_OPERATION_WRITTEN 3

  //
  // Exported functions.
  //

  void DbgInit(const char *fname, int level, int flags);
  void DbgInit(const char *fname, const char *levelName, int flags);
  void DbgUninit();

  void DbgMsg(int level, const char *fmt, ...);
  void DbgEnterEx(int level, const char *fname, const char *fmt, ...);
  void DbgHead(int level, const char *fmt, ...);
  void DbgDump(void *buf, int size);
  void DbgGetPrefix(char *prefix, int prefixSize);

  int DbgSetLevel(int level);
  int DbgSetLevel(const char *level);
  int DbgTranslateLevelName(const char *levelName);

  void DbgIoMsg(int level, const char *fmt, ...);

  void DbgIoDumpOperation(int level, int operationCode,
                              const char *fdType, uint64_t fdId64,
                                  const void *buf, int count,
                                      const char *sourceFile, int sourceLine);

  //
  // General logs.
  //

  #ifdef DEBUG

    #define DBG_INIT_EX(FNAME, LEVEL, FLAGS) DbgInit(FNAME, LEVEL, FLAGS)

    #define DBG_INIT(FNAME) DbgInit(FNAME, DBG_LEVEL_DEFAULT, DBG_FLAGS_DEFAULT)

    #define DBG_DUMP(BUF, SIZE) DbgDump(BUF, SIZE)

    #define DBG_SET_LEVEL(LEVEL) DbgSetLevel(LEVEL)

    //
    // Wrap DbgMsg(Level, ...) into macros for different levels.
    //

    #define DBG_INFO(...)  DbgMsg(DBG_LEVEL_INFO, __VA_ARGS__)

    #define DEBUG1(...)    DbgMsg(DBG_LEVEL_DEBUG1, __VA_ARGS__)
    #define DEBUG2(...)    DbgMsg(DBG_LEVEL_DEBUG2, __VA_ARGS__)
    #define DEBUG3(...)    DbgMsg(DBG_LEVEL_DEBUG3, __VA_ARGS__)
    #define DEBUG4(...)    DbgMsg(DBG_LEVEL_DEBUG4, __VA_ARGS__)
    #define DEBUG5(...)    DbgMsg(DBG_LEVEL_DEBUG5, __VA_ARGS__)

    #define DBG_MSG(...)   DbgMsg(DBG_LEVEL_DEBUG1, __VA_ARGS__)
    #define DBG_MSG1(...)  DbgMsg(DBG_LEVEL_DEBUG1, __VA_ARGS__)
    #define DBG_MSG2(...)  DbgMsg(DBG_LEVEL_DEBUG2, __VA_ARGS__)
    #define DBG_MSG3(...)  DbgMsg(DBG_LEVEL_DEBUG3, __VA_ARGS__)
    #define DBG_MSG4(...)  DbgMsg(DBG_LEVEL_DEBUG4, __VA_ARGS__)
    #define DBG_MSG5(...)  DbgMsg(DBG_LEVEL_DEBUG5, __VA_ARGS__)

    #define DBG_ENTER(X)   DEBUG1("-> " X "()...");
    #define DBG_ENTER1(X)  DEBUG1("-> " X "()...");
    #define DBG_ENTER2(X)  DEBUG2("-> " X "()...");
    #define DBG_ENTER3(X)  DEBUG3("-> " X "()...");
    #define DBG_ENTER4(X)  DEBUG4("-> " X "()...");
    #define DBG_ENTER5(X)  DEBUG5("-> " X "()...");

    #define DBG_LEAVE(X)   DEBUG1("<- " X "()...");
    #define DBG_LEAVE1(X)  DEBUG1("<- " X "()...");
    #define DBG_LEAVE2(X)  DEBUG2("<- " X "()...");
    #define DBG_LEAVE3(X)  DEBUG3("<- " X "()...");
    #define DBG_LEAVE4(X)  DEBUG4("<- " X "()...");
    #define DBG_LEAVE5(X)  DEBUG5("<- " X "()...");

    #define DBG_ENTER_EX(X, ...)  Tegenaria::DbgEnterEx(DBG_LEVEL_DEBUG1, X, __VA_ARGS__);
    #define DBG_ENTER_EX1(X, ...) Tegenaria::DbgEnterEx(DBG_LEVEL_DEBUG1, X, __VA_ARGS__);
    #define DBG_ENTER_EX2(X, ...) Tegenaria::DbgEnterEx(DBG_LEVEL_DEBUG2, X, __VA_ARGS__);
    #define DBG_ENTER_EX3(X, ...) Tegenaria::DbgEnterEx(DBG_LEVEL_DEBUG3, X, __VA_ARGS__);
    #define DBG_ENTER_EX4(X, ...) Tegenaria::DbgEnterEx(DBG_LEVEL_DEBUG4, X, __VA_ARGS__);
    #define DBG_ENTER_EX5(X, ...) Tegenaria::DbgEnterEx(DBG_LEVEL_DEBUG5, X, __VA_ARGS__);


    #define DBG_HEAD(...)  Tegenaria::DbgHead(DBG_LEVEL_INFO, __VA_ARGS__);
    #define DBG_HEAD1(...) Tegenaria::DbgHead(DBG_LEVEL_DEBUG1, __VA_ARGS__);
    #define DBG_HEAD2(...) Tegenaria::DbgHead(DBG_LEVEL_DEBUG2, __VA_ARGS__);
    #define DBG_HEAD3(...) Tegenaria::DbgHead(DBG_LEVEL_DEBUG3, __VA_ARGS__);
    #define DBG_HEAD4(...) Tegenaria::DbgHead(DBG_LEVEL_DEBUG4, __VA_ARGS__);
    #define DBG_HEAD5(...) Tegenaria::DbgHead(DBG_LEVEL_DEBUG5, __VA_ARGS__);

    //
    // IO related logs.
    //

    #define DBG_IO_MSG(...)  Tegenaria::DbgIoMsg(DBG_LEVEL_DEBUG1, __VA_ARGS__)
    #define DBG_IO_MSG1(...) Tegenaria::DbgIoMsg(DBG_LEVEL_DEBUG1, __VA_ARGS__)
    #define DBG_IO_MSG2(...) Tegenaria::DbgIoMsg(DBG_LEVEL_DEBUG2, __VA_ARGS__)
    #define DBG_IO_MSG3(...) Tegenaria::DbgIoMsg(DBG_LEVEL_DEBUG3, __VA_ARGS__)

    //
    // FIXME: Handle buffer dump at level3.
    //

    #define DBG_IO_WRITE_BEGIN(TYPE, ID, BUF, COUNT) Tegenaria::DbgIoDumpOperation(2, DBG_IO_OPERATION_WRITING, TYPE, int64_t(ID), BUF, COUNT, __FILE__, __LINE__)
    #define DBG_IO_WRITE_END(TYPE, ID, BUF, COUNT)   Tegenaria::DbgIoDumpOperation(1, DBG_IO_OPERATION_WRITTEN, TYPE, int64_t(ID), BUF, COUNT, __FILE__, __LINE__)

    #define DBG_IO_READ_BEGIN(TYPE, ID, BUF, COUNT)  Tegenaria::DbgIoDumpOperation(1, DBG_IO_OPERATION_READING, TYPE, int64_t(ID), BUF, COUNT, __FILE__, __LINE__)
    #define DBG_IO_READ_END(TYPE, ID, BUF, COUNT)    Tegenaria::DbgIoDumpOperation(1, DBG_IO_OPERATION_READED, TYPE, int64_t(ID), BUF, COUNT, __FILE__, __LINE__)

    #define DBG_IO_CLOSE_BEGIN(TYPE, ID)             DBG_IO_MSG2("Closing : (%s #%d) (file '%s', line %d)...\n", TYPE, ID, __FILE__, __LINE__)
    #define DBG_IO_CLOSE_END(TYPE, ID)               DBG_IO_MSG1("Closed  : (%s #%d) (file '%s', line %d)...\n", TYPE, ID, __FILE__, __LINE__)

    #define DBG_IO_CANCEL(TYPE, ID)                  DBG_IO_MSG1("Canceled: (%s #%d) (file '%s', line %d)...\n", TYPE, ID, __FILE__, __LINE__)

  //
  // No DEBUG enabled.
  // Declare only ghost macros to avoid compilation debug code
  // in release version.
  //

  #else /* DEBUG */

    #define DBG_INIT(FNAME)
    #define DBG_INIT_EX(FNAME, LEVEL, FLAGS)
    #define DBG_MSG(...)
    #define DBG_HEAD(...)
    #define DBG_ENTER(X)
    #define DBG_ENTER_EX(X, ...)

    #define DBG_ENTER1(X)
    #define DBG_ENTER2(X)
    #define DBG_ENTER3(X)
    #define DBG_ENTER4(X)
    #define DBG_ENTER5(X)

    #define DBG_LEAVE(X)
    #define DBG_LEAVE1(X)
    #define DBG_LEAVE2(X)
    #define DBG_LEAVE3(X)
    #define DBG_LEAVE4(X)
    #define DBG_LEAVE5(X)

    #define DBG_DUMP(BUF, SIZE)

    #define DBG_SET_LEVEL(LEVEL)

    #define DEBUG1(...)
    #define DEBUG2(...)
    #define DEBUG3(...)
    #define DEBUG4(...)
    #define DEBUG5(...)

    #define DBG_MSG1(...)
    #define DBG_MSG2(...)
    #define DBG_MSG3(...)
    #define DBG_MSG4(...)
    #define DBG_MSG5(...)

    #define DBG_INFO(...)

    //
    // IO related logs.
    //

    #define DBG_IO_WRITE_BEGIN(TYPE, ID, BUF, COUNT)
    #define DBG_IO_WRITE_END(TYPE, ID, BUF, COUNT)

    #define DBG_IO_READ_BEGIN(TYPE, ID, BUF, COUNT)
    #define DBG_IO_READ_END(TYPE, ID, BUF, COUNT)

    #define DBG_IO_CLOSE_BEGIN(TYPE, ID)
    #define DBG_IO_CLOSE_END(TYPE, ID)

    #define DBG_IO_CANCEL(TYPE, ID)

  #endif /* DEBUG */

  //
  // Macros to catch errors.
  //

  #define Error(...) {DbgMsg(DBG_LEVEL_ERROR, __VA_ARGS__);}
  #define Fatal(...) {DbgMsg(DBG_LEVEL_NONE,  __VA_ARGS__); exit(-1);}

  #define FAIL(X) if (X) goto fail

  #define FAILEX(X, ...) if (X) \
  {                             \
    int _tmp_ = GetLastError(); \
    Error(__VA_ARGS__);         \
    SetLastError(_tmp_);        \
    goto fail;                  \
  }

  //
  // Monitor objects created inside process.
  // See State.cpp and example02.
  //

  void DbgSetAdd(const char *setName, uint64_t id, const char *name = NULL, ...);
  void DbgSetAdd(const char *setName, void *ptr, const char *name = NULL, ...);
  void DbgSetDel(const char *setName, uint64_t id);
  void DbgSetMove(const char *dstName, const char *srcName, uint64_t id);

  void DbgSetRename(const char *dsetName, uint64_t id, const char *name = NULL, ...);
  void DbgSetRename(const char *dsetName, void *ptr, const char *name = NULL, ...);

  void DbgSetDump();
  void DbgSetInit(const char *fname = NULL);

  #ifdef DEBUG_STATE

    #define DBG_SET_ADD                Tegenaria::DbgSetAdd
    #define DBG_SET_RENAME             Tegenaria::DbgSetRename
    #define DBG_SET_DEL(SET_NAME, ID)  Tegenaria::DbgSetDel(SET_NAME, uint64_t(ID))
    #define DBG_SET_MOVE(DST, SRC, ID) Tegenaria::DbgSetMove(DST, SRC, uint64_t(ID));
    #define DBG_SET_INIT()             Tegenaria::DbgSetInit()
    #define DBG_SET_DUMP()             Tegenaria::DbgSetDump()

  #else /* DEBUG_STATE */

    #define DBG_SET_ADD(SETNAME, ID, ...)
    #define DBG_SET_MOVE(DST, SRC, ID)
    #define DBG_SET_DEL(SETNAME, ID)
    #define DBG_SET_RENAME(SETNAME, ID, ...)
    #define DBG_SET_MOVE(DST, SRC, ID)
    #define DBG_SET_INIT()
    #define DBG_SET_DUMP()

  #endif /* DEBUG_STATE */

} /* Namespace Tegenaria */

#endif /* Tegenaria_Core_Debug_H */
