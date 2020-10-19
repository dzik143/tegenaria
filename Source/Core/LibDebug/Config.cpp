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
// Purpose: Store debug configuration for current running process
//          inside shared, temporary file ".debugConfig.<pid>".
//          We need it to, force the same configuration even inside
//          external DLLs linked statically with -debug-static.
//

#pragma qcbuild_set_private(1)

#undef DEBUG

#include "Debug.h"
#include "Config.h"

namespace Tegenaria
{
  //
  // Used to debug LibDebug itself.
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

  using namespace Tegenaria;

  //
  // Global variable where config for current process stored.
  //

  DebugConfig_t DebugConfig = {0};

  //
  // Helper function to retrieve temp directory.
  //

  const char *DbgGetTempDir()
  {
    static char temp[1024] = {0};

    if (temp[0] == 0)
    {
      #ifdef WIN32
      GetTempPath(MAX_PATH, temp);
      #else
      strcpy(temp, "/tmp");
      #endif
    }

    return temp;
  }

  //
  // FIXME: Port O_TEMPORARY flag to Linux,
  //
  // Delete temporary .debugConfig.<pid> files on Linux.
  //

  #ifndef WIN32

  #define O_TEMPORARY 0

  void DbgConfigCleanUp()
  {
    char path[260] = {0};

    snprintf(path, sizeof(path), DEBUG_CONFIG_PATTERN, DbgGetTempDir(), DebugConfig.pid_);

    DBG_MSG("Deleting file [%s]...\n", path);

    unlink(path);
  }

  void DbgConfigCleanUpHandler(int unused)
  {
    DbgConfigCleanUp();
  }

  #endif /* !WIN32 */

  //
  // Load debug configuratin for current running process, saved
  // before by DbgSaveConfig. See DbgSaveConfig() for more .
  //
  // Used to share the same debug config within process even inside
  // laoded DLLs linked static with -ldebug-static.
  //
  // debugCofig - struct, where is stored current config to save (IN).
  //
  // RETURNS: 0 if OK.
  //

  int DbgLoadConfig(DebugConfig_t *debugConfig)
  {
    DBG_ENTER("DbgLoadConfig");

    int exitCode = -1;

    int fd = -1;

    //
    // Avoid loading config twice.
    //

    if (debugConfig -> init_ == 0)
    {
      int readed = 0;

      char path[260] = {0};

      //
      // Try open .debugConfig.<pid> file.
      // If exist it means there was already one DBG_INIT() sucessfull
      // call inside current running process.
      //

      snprintf(path, sizeof(path), DEBUG_CONFIG_PATTERN, DbgGetTempDir(), DebugConfig.pid_);

      fd = open(path, O_RDONLY | O_TEMPORARY, S_IREAD);

      FAIL(fd < 0);

      //
      // Set hidden attribute on Windows.
      //

      #ifdef WIN32
      SetFileAttributes(path, FILE_ATTRIBUTE_HIDDEN);
      #endif

      //
      // Load configuration from file.
      //

      readed = read(fd, debugConfig, sizeof(DebugConfig_t));

      FAIL(readed != sizeof(DebugConfig_t));

      DBG_MSG("DbgLoadConfig : Loaded config:\n");
      DBG_MSG("  level                  : [%d]\n", debugConfig -> logLevel_);
      DBG_MSG("  logFd                  : [%d]\n", debugConfig -> logFd_);
      DBG_MSG("  pid                    : [%d]\n", debugConfig -> pid_);
      DBG_MSG("  DBG_STATE              : [%d]\n", debugConfig -> flags_ & DBG_STATE);
      DBG_MSG("  DBG_TRACE              : [%d]\n", debugConfig -> flags_ & DBG_TRACE);
      DBG_MSG("  DBG_SEPARATE_PROCESSES : [%d]\n", debugConfig -> flags_ & DBG_SEPARATE_PROCESSES);
      DBG_MSG("  DBG_SEPARATE_THREADS   : [%d]\n", debugConfig -> flags_ & DBG_SEPARATE_THREADS);
    }

    exitCode = 0;

    //
    // Clean up.
    //

    fail:

    if (exitCode)
    {
      DBG_MSG("DbgLoadConfig : Failed with code [%d].\n", GetLastError());
    }

    if (fd > 0)
    {
      close(fd);
    }

    DBG_LEAVE("DbgLoadConfig");

    return exitCode;
  }

  // Save debug configuratin for current running process into
  // temporary ".debugConfig.<pid>" file.
  //
  // File is created as temporary and will be deleted just after
  // process dead.
  //
  // Used to share the same debug config within whole process,
  // even inside laoded DLLs linked static with -ldebug-static.
  //
  // It avoids scenario, when Foo.dll (linked with -ldebug-static) loaded:
  //
  // - Process main() begin.
  // - Process init log DBG_INIT("file.log", flags);
  // - Process calls Foo() from Foo.dll,
  // - Foo() try use one of DBG_XXX() macro.
  //
  // PROBLEM:  Foo.dll has own DebugConfig from static linked.
  //           DBG_INIT() called in main() has no efect on DLL.
  //
  // SOLUTION: First call to DBG_INIT() saves config to shared, temporary file.
  //           Another DBG_INIT() calls tries load config from file first.
  //
  // ARGS:
  //
  // debugConfig - pointer to global DebugConfig struct, where to store
  //               loaded options (OUT).
  //
  // RETURNS: 0 if OK.
  //

  int DbgSaveConfig(DebugConfig_t *debugConfig)
  {
    int exitCode = -1;

    char path[260] = {0};

    int written = 0;

    #ifdef WIN32
    snprintf(path, sizeof(path), DEBUG_CONFIG_PATTERN, DbgGetTempDir(), int(GetCurrentProcessId()));
    #else
    snprintf(path, sizeof(path), DEBUG_CONFIG_PATTERN, DbgGetTempDir(), int(getpid()));
    #endif

    int fd = open(path, O_CREAT | O_TEMPORARY | O_RDWR, S_IREAD | S_IWRITE);

    if (fd > 0)
    {
      written = write(fd, debugConfig, sizeof(DebugConfig_t));

      exitCode = 0;

      DBG_MSG("DbgSaveConfig : Saved config:\n");
      DBG_MSG("  level                  : [%d]\n", debugConfig -> logLevel_);
      DBG_MSG("  logFd                  : [%d]\n", debugConfig -> logFd_);
      DBG_MSG("  pid                    : [%d]\n", debugConfig -> pid_);
      DBG_MSG("  DBG_STATE              : [%d]\n", debugConfig -> flags_ & DBG_STATE_ENABLE);
      DBG_MSG("  DBG_TRACE              : [%d]\n", debugConfig -> flags_ & DBG_TRACE);
      DBG_MSG("  DBG_SEPARATE_PROCESSES : [%d]\n", debugConfig -> flags_ & DBG_SEPARATE_PROCESSES);
      DBG_MSG("  DBG_SEPARATE_THREADS   : [%d]\n", debugConfig -> flags_ & DBG_SEPARATE_THREADS);

      //
      // Set SIGTERM handler to remove .debugConfig.<pid> file
      // manualy on linux due to missing O_TEMPORARY flag.
      //

      #ifndef WIN32
      {
        struct sigaction action = {0};

        action.sa_handler = DbgConfigCleanUpHandler;

        sigaction(SIGTERM, &action, NULL);

        atexit(DbgConfigCleanUp);
      }
      #endif
    }

    return exitCode;
  }

} /* namespace Tegenaria */
