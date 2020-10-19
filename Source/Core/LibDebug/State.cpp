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
// Purpose : Monitor objects created inside process.
//

//
// Inlcudes.
//

#undef DEBUG

#include "Config.h"
#include "Debug.h"

namespace Tegenaria
{
  using std::map;
  using std::set;
  using std::multiset;
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
  // Global variables.
  //

  extern DebugConfig_t DebugConfig;

  struct DbgObject
  {
    int count_;

    char name_[64];
  };

  static map<string, map<uint64_t, DbgObject> > DbgSet;

  #ifdef WIN32
  static HANDLE DbgSetMutex = NULL;
  #else
  static sem_t DbgSetMutex;
  #endif

  //
  // Return current thread ID on linux.
  // Shipped natively on Windows.
  //

  #ifndef WIN32
  inline int GetCurrentThreadId()
  {
    return int(syscall (SYS_gettid));
  }
  #endif

  //
  // Called once to init state monitor.
  // Used internally only.
  //
  // - Init mutex to protect DbgSet variable, where list
  //   of created objects is stored here.
  //
  // - Open 'state-history.<pid>' file to write create/destroy history.
  //

  void DbgSetInit(const char *fname)
  {
    #pragma qcbuild_set_private(1)

    DBG_ENTER("DbgSetInit");

    DbgInit(NULL, -1, -1);

    static int firstCall = 1;

    if (firstCall == 1 && DebugConfig.flags_ & DBG_STATE_ENABLE)
    {
      DBG_MSG("DbgSetInit : Going to init...\n");

      firstCall = 0;

      char path[256] = {0};

      int flags = 0;
      int pmode = 0;

      //
      // Init Mutex and get curent process PID.
      //

      //
      // Windows.
      //

      #ifdef WIN32
      {
        DbgSetMutex = CreateMutex(NULL, FALSE, NULL);
      }
      //
      // Linux, MacOS.
      //

      #else
      {
        sem_init(&DbgSetMutex, 0, 1);
      }
      #endif

      DBG_MSG("DbgSetInit : flags are [0x%x].\n", DebugConfig.flags_);

      if (DebugConfig.flags_ & DBG_STATE_ENABLE)
      {
        //
        // Open state-history.<pid> file.
        // We store history of create/delete operation.
        //

        if (fname)
        {
          snprintf(path, sizeof(path), "%s.state-history.%d.log", fname, DebugConfig.pid_);
        }
        else
        {
          snprintf(path, sizeof(path), "state-history.%d.log", DebugConfig.pid_);
        }

        DBG_MSG("DbgSetInit : Opening file [%s]...\n", DebugConfig.stateHisFd_);

        DebugConfig.stateHisFd_ = open(path, O_CREAT | O_TRUNC | O_WRONLY,
                                           S_IREAD | S_IWRITE);

        //
        // Open state.<pid> file.
        // We store most actual snaphot of all monitored objects
        // inside current running process.
        //

        if (fname)
        {
          snprintf(path, sizeof(path), "%s.state.%d.log", fname, DebugConfig.pid_);
        }
        else
        {
          snprintf(path, sizeof(path), "state.%d.log", DebugConfig.pid_);
        }

        DBG_MSG("DbgSetInit : Opening file [%s]...\n", DebugConfig.stateFd_);

        DebugConfig.stateFd_ = open(path, O_CREAT | O_TRUNC | O_WRONLY,
                                        S_IREAD | S_IWRITE);
      }
      else
      {
        DebugConfig.stateHisFd_ = -1;
        DebugConfig.stateFd_    = -1;
      }
    }

    DBG_LEAVE("DbgSetInit");
  }

  //
  // Lock DbgSet variable.
  // We can't use LibLock, because LibLock depends on LibDebug.
  //

  static void DbgSetLock()
  {
    #pragma qcbuild_set_private(1)

    DBG_ENTER("DbgSetLock");

    #ifdef WIN32
    WaitForSingleObject(DbgSetMutex, INFINITE);
    #else
    sem_wait(&DbgSetMutex);
    #endif

    DBG_LEAVE("DbgSetLock");
  }

  //
  // Unlock DbgSet variable.
  // We can't use LibLock, because LibLock depends on LibDebug.
  //

  static void DbgSetUnlock()
  {
    #pragma qcbuild_set_private(1)

    DBG_ENTER("DbgSetUnlock");

    #ifdef WIN32
    ReleaseMutex(DbgSetMutex);
    #else
    sem_post(&DbgSetMutex);
    #endif

    DBG_LEAVE("DbgSetUnlock");
  }

  //
  // Dump state of all monitored objects to state.<pid> file.
  //

  void DbgSetDump()
  {
    if ((DebugConfig.flags_ & DBG_STATE_ENABLE) == 0)
    {
      DBG_MSG("Rejected DbgSetDump.\n");

      return;
    }

    char fname[1024];

    char head[64];

    int written = 0;

    map<string, map<uint64_t, DbgObject> >::iterator it;

    string buf;

    DbgSetInit();

    //
    // Dump all objects set to file.
    //

    if (DebugConfig.stateFd_ != -1)
    {
      DbgSetLock();

      //
      // Rewind to file begin.
      // We want only one most actual state in file.
      //

      lseek(DebugConfig.stateFd_, 0, SEEK_SET);

      //
      // Put header on top.
      //

      buf = "------------- BEGIN OF SNAPSHOT ------------\n";

      written = write(DebugConfig.stateFd_, buf.c_str(), buf.size());

      //
      // For all object sets...
      //

      for (it = DbgSet.begin(); it != DbgSet.end(); it++)
      {
        map<uint64_t, DbgObject> &currentSet = it -> second;

        map<uint64_t, DbgObject>::iterator object;

        //
        // Form '<set name> : {' head.
        //

        snprintf(head, sizeof(head), "%20s : {", it -> first.c_str());

        buf = head;

        //
        // List all objects in set.
        //

        for (object = currentSet.begin();
                 object != currentSet.end(); object++)
        {
          for (int i = 0; i < object -> second.count_; i++)
          {
            buf += object -> second.name_;
            buf += ", ";
          }
        }

        buf += "}\n";

        //
        // Flush buffer to file.
        //

        written = write(DebugConfig.stateFd_, buf.c_str(), buf.size());

        buf.clear();
      }

      //
      // Overwrite possible old content.
      //

      buf = "-------------- END OF SNAPSHOT -------------\n";

      buf.resize(buf.size() + 1024);

      written = write(DebugConfig.stateFd_, buf.c_str(), buf.size());

      DbgSetUnlock();
    }
  }

  //
  // Add object with given ID to objects set.
  //
  // setName - name of target set, eg. 'socket' (IN).
  // id      - object id (IN).
  // fmtName - optional, human readable name e.g. NETAPI#2/Mutex,
  //           or printf like list to format name in runtime (IN/OPT).
  //
  // Examples:
  //  DbgSetAdd("socket", 1234);
  //  DbgSetAdd("mutex", 0x1000, "LogFile");
  //  DbgSetAdd("session", this, "NETAPI #%d/%d, id, ownerId);
  //

  void DbgSetAdd(const char *setName, uint64_t id, const char *fmtName, ...)
  {
    if ((DebugConfig.flags_ & DBG_STATE_ENABLE) == 0)
    {
      DBG_MSG("Rejected DbgSetAdd.\n");

      return;
    }

    DbgSetInit();

    if (setName)
    {
      DbgSetLock();

      DbgObject &object = DbgSet[setName][id];

      object.count_ ++;

      //
      // Name specified,
      // use it as printf like list.
      //

      if (fmtName)
      {
        char name[64];

        int written = 0;

        va_list ap;
        va_start(ap, fmtName);
        vsnprintf(name, sizeof(name), fmtName, ap);
        va_end(ap);

        if (id < uint64_t(64000))
        {
          snprintf(object.name_, sizeof(object.name_), "[#%"PRId64"!%s]", id, name);
        }
        else
        {
          snprintf(object.name_, sizeof(object.name_), "[0x%"PRIx64"!%s]", id, name);
        }
      }

      //
      // Name not specified.
      // Use ID as name.
      //

      else
      {
        if (id < 64000)
        {
          snprintf(object.name_, sizeof(object.name_), "#%"PRId64, id);
        }
        else
        {
          snprintf(object.name_, sizeof(object.name_), "0x%"PRIx64, id);
        }
      }

      //
      // Log create event into history file.
      //

      if (DebugConfig.stateHisFd_ != -1)
      {
        char buf[256];

        int written = 0;

        snprintf(buf, sizeof(buf), "Created %s %s by thread ID#%d.\n",
                     setName, object.name_, GetCurrentThreadId());

        written = write(DebugConfig.stateHisFd_, buf, strlen(buf));

        DbgMsg(DBG_LEVEL_DEBUG4, "%s", buf);
      }

      DbgSetUnlock();

      DbgSetDump();
    }
  }

  void DbgSetAdd(const char *setName, void *ptr, const char *fmt, ...)
  {
    if ((DebugConfig.flags_ & DBG_STATE_ENABLE) == 0)
    {
      DBG_MSG("Rejected DbgSetAdd.\n");

      return;
    }

    if (fmt)
    {
      char name[64] = {0};

      va_list ap;
      va_start(ap, fmt);
      vsnprintf(name, sizeof(name), fmt, ap);
      va_end(ap);

      DbgSetAdd(setName, uint64_t(ptr), name);
    }
    else
    {
      DbgSetAdd(setName, uint64_t(ptr));
    }
  }

  //
  // Remove object with given ID from objects set.
  //
  // setName - name of target set, eg. 'socket' (IN).
  // id      - object id (IN).
  //
  // Examples:
  //  DbgSetDel("socket", 1234);
  //  DbgSetDel("session", this);
  //

  void DbgSetDel(const char *setName, uint64_t id)
  {
    if ((DebugConfig.flags_ & DBG_STATE_ENABLE) == 0)
    {
      DBG_MSG("Rejected DbgSetDel.\n");

      return;
    }

    DbgSetInit();

    if (setName)
    {
      int written = 0;

      DbgSetLock();

      //
      // Find object set.
      //

      map<uint64_t, DbgObject> &currentSet = DbgSet[setName];

      //
      // Find object in given set.
      //

      map<uint64_t, DbgObject>::iterator it = currentSet.find(id);

      //
      // If object exist decrease its counter.
      //

      if (it != currentSet.end())
      {
        it -> second.count_ --;

        //
        // Log delete event into history file and main log.
        //

        if (DebugConfig.stateHisFd_ != -1)
        {
          char buf[256];

          snprintf(buf, sizeof(buf), "Deleted %s %s by thread ID#%d.\n",
                       setName, it -> second.name_, GetCurrentThreadId());

          written = write(DebugConfig.stateHisFd_, buf, strlen(buf));

          DbgMsg(DBG_LEVEL_DEBUG4, "%s", buf);
        }

        //
        // If counter decreased to 0 remove object from set.
        //

        if (it -> second.count_ <= 0)
        {
          currentSet.erase(it);
        }
      }

      DbgSetUnlock();

      DbgSetDump();
    }
  }

  //
  // Move object with given ID from source set to destination set.
  //
  // dstSet - name of destination set (IN).
  // srcSet - name of source set (IN).
  // id     - object id (IN).
  //

  void DbgSetMove(const char *dstSet, const char *srcSet, uint64_t id)
  {
    if ((DebugConfig.flags_ & DBG_STATE_ENABLE) == 0)
    {
      DBG_MSG("Rejected DbgSetMove.\n");

      return;
    }

    DbgSetInit();

    if (srcSet && dstSet)
    {
      DbgSetLock();

      //
      // Find source and destination sets.
      //

      map<uint64_t, DbgObject> &src = DbgSet[srcSet];
      map<uint64_t, DbgObject> &dst = DbgSet[dstSet];

      //
      // Find object in source.
      //

      map<uint64_t, DbgObject>::iterator it = src.find(id);

      //
      // If object exist decrease its counter in source
      // and increase in destination set.
      //

      if (it != src.end())
      {
        it -> second.count_ --;

        dst[id].count_ ++;

        strncpy(dst[id].name_, it -> second.name_, sizeof(dst[id].name_));

        //
        // Log move event into history file and main log.
        //

        if (DebugConfig.stateHisFd_ != -1)
        {
          char buf[256];

          int written = 0;

          snprintf(buf, sizeof(buf), "Moved   %s from %s to %s by thread ID#%d.\n",
                       it -> second.name_, srcSet, dstSet, GetCurrentThreadId());

          written = write(DebugConfig.stateHisFd_, buf, strlen(buf));

          DbgMsg(DBG_LEVEL_DEBUG4, "%s", buf);
        }

        //
        // If counter decreased to 0 remove object from source set.
        //

        if (it -> second.count_ <= 0)
        {
          src.erase(it);
        }
      }

      DbgSetUnlock();

      DbgSetDump();
    }
  }

  //
  // Assign human readable string to object.
  //
  // setName - name of object set used in DbgSetAdd() before (IN).
  // id      - object id (IN).
  //
  // fmtName - new object's name to assign e.g. 'Session ID#5' or
  //           printf like list to format new name. See DbgSetAdd()
  //           for examples (IN).
  //

  void DbgSetRename(const char *setName, uint64_t id, const char *fmtName, ...)
  {
    if ((DebugConfig.flags_ & DBG_STATE_ENABLE) == 0)
    {
      DBG_MSG("Rejected DbgSetRename.\n");

      return;
    }

    DbgSetInit();

    if (setName && fmtName && DebugConfig.flags_ & DBG_STATE_ENABLE)
    {
      DbgSetLock();

      map<uint64_t, DbgObject>::iterator it = DbgSet[setName].find(id);

      if (it != DbgSet[setName].end())
      {
        DbgObject &object = it -> second;

        char name[64];
        char oldName[64];

        va_list ap;
        va_start(ap, fmtName);
        vsnprintf(name, sizeof(name), fmtName, ap);
        va_end(ap);

        strcpy(oldName, object.name_);

        if (id < 64000)
        {
          snprintf(object.name_, sizeof(object.name_), "[#%"PRId64"!%s]", id, name);
        }
        else
        {
          snprintf(object.name_, sizeof(object.name_), "[0x%"PRIx64"!%s]", id, name);
        }

        //
        // Log rename event into history file and main log.
        //

        if (DebugConfig.stateHisFd_ != -1)
        {
          char buf[256];

          int written = 0;

          snprintf(buf, sizeof(buf), "Renamed %s from %s to %s by thread ID#%d.\n",
                       setName, oldName, object.name_, GetCurrentThreadId());

          written = write(DebugConfig.stateHisFd_, buf, strlen(buf));

          DbgMsg(DBG_LEVEL_DEBUG3, "%s", buf);
        }
      }

      DbgSetUnlock();

      DbgSetDump();
    }
  }

  void DbgSetRename(const char *setName, void *ptr, const char *fmt, ...)
  {
    if ((DebugConfig.flags_ & DBG_STATE_ENABLE) == 0)
    {
      DBG_MSG("Rejected DbgSetRename.\n");

      return;
    }

    if (fmt && DebugConfig.flags_ & DBG_STATE_ENABLE)
    {
      char name[64] = {0};

      va_list ap;
      va_start(ap, fmt);
      vsnprintf(name, sizeof(name), fmt, ap);
      va_end(ap);

      DbgSetRename(setName, uint64_t(ptr), "%s", name);
    }
  }

} /* namespace Tegenaria */
