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

#include <vector>

#include <Tegenaria/Debug.h>
#include <Tegenaria/Args.h>
#include <Tegenaria/File.h>

#include "Main.h"
#include "Utils.h"
#include "SourceInfo.h"
#include "Diagram.h"
#include <sys/stat.h>

using namespace std;
using namespace Tegenaria;

//
// Global variables.
//

const char *LogLevel = NULL;
const char *LogFile  = NULL;

//
// Target specific variables.
//

const char *TargetMachine    = HOST_MACHINE;
const char *ExePostfix       = EXE_POSTFIX;
const char *DllPostfix       = DLL_POSTFIX;
const char *DllPrefix        = DLL_PREFIX;

//
// Host specific variables.
//

const char *HostMachine       = HOST_MACHINE;
const char *ScriptPostfix     = SCRIPT_POSTFIX;
const char *ScriptDelCmd      = SCRIPT_DEL;
const char *ScriptRmdirCmd    = SCRIPT_RMDIR;
const char *ScriptMkdirCmd    = SCRIPT_MKDIR;
const char *ScriptCopyCmd     = SCRIPT_COPY;
const char *ScriptComment     = SCRIPT_COMMENT;
const char *ScriptIntroBlock  = SCRIPT_INTRO;
const char *ScriptFailBlock   = SCRIPT_FAIL;
const char *ScriptMakeInstall = SCRIPT_MAKE_INSTALL;
const char *CxxCompiler       = CXX_COMPILER;
const char *CCompiler         = C_COMPILER;

//
// Detect current running machine, where qcbuild is runned.
//
// RETURNS: Host machine name or NULL if unknown.
//

const char *DetectHost()
{
  DBG_ENTER("DetectHost");

  const char *host = NULL;

  //
  // On Windows check is it Cygwin or MinGW.
  //

  #ifdef WIN32
  {
    //
    // FIXME: Review plain old native Cygwin (where --mno-cygwin still needed)
    //

    /*
    char buf[1024] = {0};

    //
    // Run 'uname' command.
    //

    FILE *f = _popen("uname", "rt");

    FAIL(f == NULL);

    fgets(buf, sizeof(buf), f);

    fclose(f);

    //
    // Check 'uname' result.
    //

    if (strstr(buf, "CYGWIN"))
    {
      host = "Cygwin";
    }
    else
    {
      host = "MinGW";
    }
    */

    host = "MinGW";
  }

  //
  // Linux, MacOS. Use default based on compiler used to build
  // current qcbuild binary.
  //

  #else
  {
    host = HostMachine;
  }
  #endif

  fail:

  DBG_LEAVE("DetectHost");

  return host;
}

//
//
//

int LoadConfig(const char *targetMachine, const char *hostMachine)
{
  int exitCode = -1;

  if (strcmp(hostMachine, "Cygwin") == 0)
  {
    CxxCompiler = "g++ -mwin32";
  }

  //
  // Error handler.
  //

  exitCode = 0;

  fail:

  return exitCode;
}

//
// Generate makefle in current directory from given
// component's description.
//
// comp - structure describing component (IN).
//
// RETURNS: 0 if OK.
//

int GenerateMakefile(SourceInfo *comp)
{
  DBG_ENTER("GenerateMakefile");

  int exitCode = -1;

  string qtLibs;
  string qtInclude;

  vector<string> qtModules;

  int useQt = 0;

  char buildInc[MAX_PATH + 1] = {0};
  char buildLib[MAX_PATH + 1] = {0};

  string titleStr = comp -> get("TITLE");

  char *title = &titleStr[0];

  int type = comp -> getType();

  int library = 0;

  int anythingToCompile = 0;
  int anythingToLink    = 0;


  FILE *f = fopen("Makefile", "wt+");

  FAILEX(f == NULL, "Cannot create 'Makefile' file.\n");

  //
  // Default declared MacOS version if not set.
  //

  if (stricmp(HostMachine, "MacOS") == 0)
  {
    if (stristr(comp -> get("LFLAGS"), "-mmacosx-version-min") == NULL)
    {
      comp -> cat("LFLAGS", "-mmacosx-version-min=10.7");

      DBG_INFO("Defaulted mmacosx-version-min to 10.7 for [%s].\n", comp -> get("TITLE"));
    }
  }

  //
  // Warning message.
  //

  FilePutComment(f, "DO NOT EDIT! This file was generated automatically from 'qcbuild.src' file.");

  //
  // Lowerize title for linux/mac.
  //

  if (stricmp(HostMachine, "Linux") == 0
        || stricmp(HostMachine, "MacOS") == 0)
  {
    for (int i = 0; title[i]; i++)
    {
      if (title[i] >= 'A' && title[i] <= 'Z')
      {
        title[i] += 'a' - 'A';
      }
    }
  }

  //
  // Check is there any source files to compile.
  // If not we don't build anything, but it may needed to copy
  // prebuild files on 'make install'.
  //

  if (comp -> isset("CXXSRC") || comp -> isset("CSRC"))
  {
    if (comp -> getType() != TYPE_MULTI_LIBRARY)
    {
      anythingToCompile = 1;
    }

    anythingToLink = 1;
  }

  //
  // Check for QT dependency.
  //

  if (comp -> isset("QT_MODULES"))
  {
    useQt = 1;

    qtInclude = QtQueryProperty("QT_INSTALL_HEADERS");
    qtLibs    = QtQueryProperty("QT_INSTALL_LIBS");

    FAIL(qtInclude.empty());
    FAIL(qtLibs.empty());

    StrTokenize(qtModules, comp -> get("QT_MODULES"));
  }

  //
  // Paths.
  //

  if (type == TYPE_LIBRARY || type == TYPE_SIMPLE_LIBRARY)
  {
    library = 1;

    if (strncmp(title, "Lib", 3) == 0 || strncmp(title, "lib", 3) == 0)
    {
      title += 3;
    }
  }

  comp -> set("REAL_TITLE", title);

  fprintf(f, "TITLE = %s\n", title);
  fprintf(f, "ROOT  = %s\n", comp -> get("ROOT"));
  fprintf(f, "\n");

  fprintf(f, "BIN_DIR = %s\n", comp -> get("BIN_DIR"));
  fprintf(f, "LIB_DIR = %s\n", comp -> get("LIB_DIR"));
  fprintf(f, "INC_DIR = %s\n", comp -> get("INC_DIR"));
  fprintf(f, "\n");

  if (type != TYPE_SIMPLE_PROGRAM && type != TYPE_SIMPLE_LIBRARY)
  {
    fprintf(f, "BUILD_ROOT     = $(ROOT)/Build\n");
    fprintf(f, "BUILD_BIN_ROOT = $(ROOT)/Build/Bin\n");
    fprintf(f, "BUILD_LIB_ROOT = $(ROOT)/Build/Lib\n");
    fprintf(f, "BUILD_INC_ROOT = $(ROOT)/Build/Include\n");
    fprintf(f, "\n");

    //
    // Custom 'Build/Bin/$(BIN_DIR)' directory.
    //

    if (comp -> isset("BIN_DIR"))
    {
      fprintf(f, "BUILD_BIN = $(BUILD_BIN_ROOT)/$(BIN_DIR)\n");
    }
    else
    {
      fprintf(f, "BUILD_BIN = $(BUILD_BIN_ROOT)\n");
    }

    //
    // Custom 'Build/Lib/$(LIB_DIR)' directory.
    //

    if (comp -> isset("LIB_DIR"))
    {
      fprintf(f, "BUILD_LIB = $(BUILD_LIB_ROOT)/$(LIB_DIR)\n");
    }
    else
    {
      fprintf(f, "BUILD_LIB = $(BUILD_LIB_ROOT)\n");
    }

    //
    // Custom 'Build/Inc/$(INC_DIR)' directory.
    //

    if (comp -> isset("INC_DIR"))
    {
      fprintf(f, "BUILD_INC = $(BUILD_INC_ROOT)/$(INC_DIR)\n");
    }
    else
    {
      fprintf(f, "BUILD_INC = $(BUILD_INC_ROOT)\n");
    }

    strncpy(buildInc, "-I $(BUILD_INC_ROOT)", MAX_PATH);
    strncpy(buildLib, "-L $(BUILD_LIB_ROOT)", MAX_PATH);
  }

  //
  // QT sources list if specified.
  //

  if (useQt)
  {
    FilePutComment(f, "QT: headers and UI sources.");

    //
    // Declare QT modules, forms and headers.
    //

    fprintf(f, "QT_MODULES  = %s\n", comp -> get("QT_MODULES"));
    fprintf(f, "QT_HEADERS  = %s\n", comp -> get("QT_HEADERS"));
    fprintf(f, "QT_FORMS    = %s\n", comp -> get("QT_FORMS"));
    fprintf(f, "QT_LIBS     = %s\n", qtLibs.c_str());
    fprintf(f, "QT_INCLUDE  = %s\n", qtInclude.c_str());

    //
    // Add <QT_LIBS> to libraries dirs.
    // Add <QT_INCLUDE> to includes dirs.
    //

    comp -> cat("LIBS",    "-L$(QT_LIBS)");
    comp -> cat("INCLUDE", "-I$(QT_INCLUDE)");

    //
    // For every module add:
    //
    // - '-I<QT_INCLUDE>/<module>' to INCLUDE.
    // - '-l<module>' to LIBS.
    //

    for (int i = 0; i < qtModules.size(); i++)
    {
      string base;
      string inc;
      string lib;

      //
      // Add Qt prefix if not specified.
      //

      if (qtModules[i][0] == 'q' || qtModules[i][0] == 'Q')
      {
        base = qtModules[i];
      }
      else
      {
        base = "Qt" + qtModules[i];
      }

      if (base[2] > 'a' && base[2] < 'z')
      {
        base[2] -= 'a' - 'A';
      }

      DBG_MSG("Adding QT module '%s'...\n", base.c_str());

      //
      // Add entries to LIBS and INCLUDE.
      //

      inc = "-I$(QT_INCLUDE)/" + base;
      lib = "-l" + base;

      comp -> cat("INCLUDE", inc.c_str());

      //
      // FIXME: Detect real library file for given QT module
      //        e.g. it may be -lqtcore4 for QtCore module.
      //

      // comp -> cat("LIBS", lib.c_str());
    }

    //
    // Add rules to generate file names:
    //
    // <base>.cpp |-> moc_<base>.cpp
    // <base>.h   |-> ui_<base>.h
    //

    fprintf(f, "QT_UI_H     = $(QT_FORMS:%%.ui=ui_%%.h)\n"
               "QT_MOC      = $(QT_HEADERS:%%.h=moc_%%.cpp) $(QT_UI_H:%%h=moc_%%cpp)\n"
               "QT_MOC_OBJ  = $(QT_MOC:.cpp=.o)\n");
  }

  //
  // Flex and bison sources if specified.
  //

  if (comp -> isset("FLEXSRC"))
  {
    FilePutComment(f, "FLEX: sources.");

    //
    // Add rules to generate file names:
    //
    // <base>.l |-> <base>.lex.cpp
    //

    fprintf(f, "FLEXSRC_CXX = $(FLEXSRC:%%.l=%%.lex.cpp)\n");
  }

  if (comp -> isset("BISONSRC"))
  {
    FilePutComment(f, "BISON: sources.");

    //
    // Add rules to generate file names:
    //
    // <base>.y |-> <base>.tab.cpp
    // <base>.y |-> <base>.tab.hpp
    //

    fprintf(f, "BISONSRC_CXX = $(BISONSRC:%%.y=%%.tab.cpp)\n");
    fprintf(f, "BISONSRC_HPP = $(BISONSRC:%%.y=%%.tab.hpp)\n");
  }

  //
  // Source list.
  //

  FilePutComment(f, "Source lists.");

  fprintf(f, "CSRC           = %s\n", comp -> get("CSRC"));
  fprintf(f, "CXXSRC         = $(FLEXSRC_CXX) $(BISONSRC_CXX) $(QT_MOC) %s\n", comp -> get("CXXSRC"));
  fprintf(f, "ISRC           = %s\n", comp -> get("ISRC"));
  fprintf(f, "FLEXSRC        = %s\n", comp -> get("FLEXSRC"));
  fprintf(f, "BISONSRC       = %s\n", comp -> get("BISONSRC"));
  fprintf(f, "PREBUILD_LIBS  = %s\n", comp -> get("PREBUILD_LIBS"));
  fprintf(f, "PREBUILD_PROGS = %s\n", comp -> get("PREBUILD_PROGS"));
  fprintf(f, "DEFINES        = %s\n", comp -> get("DEFINES"));

  fprintf(f, "EXE_POSTFIX    = %s\n", ExePostfix);
  fprintf(f, "DLL_POSTFIX    = %s\n", DllPostfix);
  fprintf(f, "DLL_PREFIX     = %s\n", DllPrefix);

  fprintf(f, "CXXOBJ         = $(CXXSRC:.cpp=.o)\n");
  fprintf(f, "COBJ           = $(CSRC:.c=.o)\n");
  fprintf(f, "OBJ            = $(CXXOBJ) $(COBJ)\n\n");

  //
  // Compiler/linker flags.
  //

  FilePutComment(f, "Compiler/linker flags.");

  fprintf(f, "CXX     = %s\n", CxxCompiler);
  fprintf(f, "CC      = %s\n", CCompiler);

  if (stricmp(HostMachine, "Linux") == 0
        || stricmp(HostMachine, "MacOS") == 0)
  {
    fprintf(f, "CXXFLAGS= -c -g -O3 -fPIC %s %s $(DEFINES)\n", buildLib, buildInc);
    fprintf(f, "CFLAGS  = -c -g -O3 -fPIC %s %s $(DEFINES)\n", buildLib, buildInc);
  }
  else
  {
    fprintf(f, "CXXFLAGS= -c -g -O3 %s %s $(DEFINES)\n", buildLib, buildInc);
    fprintf(f, "CFLAGS  = -c -g -O3 %s %s $(DEFINES)\n", buildLib, buildInc);
  }

  fprintf(f, "LIBS    = %s %s\n", comp -> get("LIBS"), buildLib);
  fprintf(f, "INCLUDE = %s %s\n", comp -> get("INCLUDE"), buildInc);
  fprintf(f, "LFLAGS  = %s\n", comp -> get("LFLAGS"));
  fprintf(f, "ARCH    = %s\n", DetectArch());

  //
  // Build program or library from sources if any sources specified.
  //

  if (anythingToCompile)
  {
    //
    // Link objects to executable/library.
    //

    FilePutComment(f, "Link objects to executable/library.");

    if (type == TYPE_LIBRARY || type == TYPE_SIMPLE_LIBRARY)
    {
      fprintf(f, "all: $(OBJ)\n");
    }
    else
    {
      fprintf(f, "all: $(TITLE)$(EXE_POSTFIX)\n");

      fprintf(f, "$(TITLE)$(EXE_POSTFIX): $(OBJ)\n");
      fprintf(f, "\t@echo Linking %s%s...\n", comp -> get("TITLE"), ExePostfix);
      fprintf(f, "\t$(CXX) $(LFLAGS) $(OBJ) $(LIBS) -o $@\n");
    }

    //
    // Compile sources to objects.
    //

    FilePutComment(f, "Compile sources to objects.");

    fprintf(f, "$(CXXOBJ) : %%.o : %%.cpp\n");
    fprintf(f, "\t@echo Compiling $<...\n");
    fprintf(f, "\t@$(CXX) $(CXXFLAGS) $(INCLUDE) -c $< -o $@\n\n");

    fprintf(f, "$(COBJ) : %%.o : %%.c\n");
    fprintf(f, "\t@echo Compiling $<...\n");
    fprintf(f, "\t@$(CC) $(CFLAGS) $(INCLUDE) -c $< -o $@\n");

    if (useQt)
    {
      //
      // QT: Create moc files from QT headers.
      //

      FilePutComment(f, "QT: Create moc files from headers.");

      fprintf(f, "$(QT_MOC) : moc_%%.cpp : %%.h $(QT_UI_H)\n");
      fprintf(f, "\t@echo QT: Generating moc file from $<...\n");
      fprintf(f, "\t@moc $< -o $@\n");

      //
      // QT: Create headers from QT forms (UI files).
      //

      FilePutComment(f, "QT: Create headers from *.ui files.");

      fprintf(f, "$(QT_UI_H) : ui_%%.h : %%.ui\n");
      fprintf(f, "\t@echo QT: Generating header from $<...\n");
      fprintf(f, "\t@uic $< -o $@\n");
    }

    //
    // FLEX and BISON.
    //

    if (comp -> isset("FLEXSRC"))
    {
      FilePutComment(f, "FLEX: Create lex.cpp files from *.l sources.");

      fprintf(f, "$(FLEXSRC_CXX) : %%.lex.cpp : %%.l $(BISONSRC_CXX)\n");
      fprintf(f, "\t@echo FLEX: Generating cpp from $<...\n");
      fprintf(f, "\t@flex -o$@ $<\n");
    }

    if (comp -> isset("BISONSRC"))
    {
      FilePutComment(f, "BISON: Create tab.cpp files from *.y sources.");

      fprintf(f, "$(BISONSRC_CXX) : %%.tab.cpp : %%.y\n");
      fprintf(f, "\t@echo BISON: Generating cpp from $<...\n");
      fprintf(f, "\t@bison -d -y -v $< -o $@\n");
    }
  }

  //
  // No source specified. Generate empty 'Makefile'.
  // Only 'make install' does real work.
  //

  else
  {
    FilePutComment(f, "No any source to compile.");

    fprintf(f, "all:\n");
    fprintf(f, "\t@echo Nothing to compile.\n");
  }

  //
  // Make install. Copy binaries to build.
  //

  FilePutComment(f, "Make install. Copy created binaries to build dir.");

  fprintf(f, "install: all\n");

  if (type != TYPE_SIMPLE_PROGRAM && type != TYPE_SIMPLE_LIBRARY)
  {
    MakefileMkdir(f, "$(BUILD_ROOT)");
    MakefileMkdir(f, "$(BUILD_INC)");
    MakefileMkdir(f, "$(BUILD_LIB)");
    MakefileMkdir(f, "$(BUILD_BIN)");
  }

/*
  TODO: Obsolete code - remove after review.

  if (comp -> isset("INSTALL"))
  {
    if (comp -> isset("INSTALL"))     MakefileMkdir(f, "$(INSTALL)");
    if (comp -> isset("INSTALL_INC")) MakefileMkdir(f, "$(INSTALL_INC)");
    if (comp -> isset("INSTALL_LIB")) MakefileMkdir(f, "$(INSTALL_LIB)");
    if (comp -> isset("INSTALL_BIN")) MakefileMkdir(f, "$(INSTALL_BIN)");
  }
*/

  //
  // Copy header files.
  //

  if (comp -> isset("ISRC"))
  {
    if (type != TYPE_SIMPLE_PROGRAM && type != TYPE_SIMPLE_LIBRARY)
    {
      MakefileCopy(f, "$(ISRC)", "$(BUILD_INC)");
    }

    if (comp -> isset("INSTALL_INC"))
    {
      MakefileCopy(f, "$(ISRC)", "$(INSTALL_INC)");
    }
  }

  //
  // Copy prebuild if any.
  //

  if (comp -> isset("PREBUILD_LIBS"))
  {
    MakefileCopy(f, "$(PREBUILD_LIBS)", "$(BUILD_LIB)");
  }

  if (comp -> isset("PREBUILD_PROGS"))
  {
    MakefileCopy(f, "$(PREBUILD_PROGS)", "$(BUILD_BIN)");
  }

  //
  // Link program or library from objects if any sources
  // was specified.
  //

  if (anythingToLink)
  {
    //
    // Link executable program ('exe').
    //

    if (type == TYPE_PROGRAM || type == TYPE_SIMPLE_PROGRAM)
    {
      if (type != TYPE_SIMPLE_PROGRAM && type != TYPE_SIMPLE_LIBRARY)
      {
        MakefileCopy(f, "$(TITLE)$(EXE_POSTFIX)", "$(BUILD_BIN)");
      }

      if (comp -> isset("INSTALL"))
      {
        MakefileCopy(f, "$(TITLE)$(EXE_POSTFIX)", "$(INSTALL)");
      }

      if (comp -> isset("INSTALL_BIN"))
      {
        MakefileCopy(f, "$(TITLE)$(EXE_POSTFIX)", "$(INSTALL_BIN)");
      }
    }

    //
    // Link library.
    //

    else
    {
      //
      // Link static library.
      // We add '-static' postfix to library to switch beetwen
      // static/dynamic linking easly by switching name passed to -l option.
      //
      // '-l<title>-static' : will link with static library.
      // '-l<title>         : will link with dynamic library.
      //

      fprintf(f, "\t@echo Linking static lib$(TITLE)-static.a...\n");

      MakefileRemove(f, "$(TITLE)-static.a");

      fprintf(f, "\tar rcs lib$(TITLE)-static.a $(OBJ)\n");

      //
      // Link dynamic library.
      //

      fprintf(f, "\t@echo Linking $(TITLE)$(DLL_POSTFIX)...\n");

      if (stricmp(HostMachine, "MinGW") == 0)
      {
        fprintf(f, "\t$(CXX) -shared $(LFLAGS) -o $(DLL_PREFIX)$(TITLE)$(DLL_POSTFIX) $(OBJ) $(LIBS) -Wl,--out-implib,lib$(TITLE).a\n");
      }
      else
      {
        fprintf(f, "\t$(CXX) -shared $(LFLAGS) -o $(DLL_PREFIX)$(TITLE)$(DLL_POSTFIX) $(OBJ) $(LIBS)\n");
      }

      if (type != TYPE_SIMPLE_LIBRARY)
      {
        if (stricmp(HostMachine, "MinGW") == 0)
        {
          MakefileCopy(f, "lib$(TITLE).a", "$(BUILD_LIB)");
        }

        MakefileCopy(f, "$(DLL_PREFIX)$(TITLE)$(DLL_POSTFIX)", "$(BUILD_LIB)");
        MakefileCopy(f, "lib$(TITLE)-static.a", "$(BUILD_LIB)");
      }

      if (comp -> isset("INSTALL_LIB"))
      {
        if (stricmp(HostMachine, "MinGW") == 0)
        {
          MakefileCopy(f, "lib$(TITLE).a", "$(INSTALL_LIB)");
        }

        MakefileCopy(f, "$(DLL_PREFIX)$(TITLE)$(DLL_POSTFIX)", "$(INSTALL_LIB)");
        MakefileCopy(f, "lib$(TITLE)-static.a", "$(INSTALL_LIB)");
      }
    }
  }

  //
  // Make clean.
  //

  FilePutComment(f, "Make clean. Remove files created by make and make install.");

  fprintf(f, "clean:\n");

  MakefileRemove(f, "*.o");
  MakefileRemove(f, "*.log");

  if (library)
  {
    MakefileRemove(f, "*$(DLL_POSTFIX)");
  }
  else
  {
    MakefileRemove(f, "$(TITLE)$(EXE_POSTFIX)");
  }

  MakefileRemove(f, "*.a");
  MakefileRemove(f, "*.def");

  if (useQt)
  {
    MakefileRemove(f, "$(QT_MOC)");
    MakefileRemove(f, "$(QT_UI_H)");
  }

  if (comp -> isset("FLEXSRC"))
  {
    MakefileRemove(f, "$(FLEXSRC_CXX)");
  }

  if (comp -> isset("BISONSRC"))
  {
    MakefileRemove(f, "$(BISONSRC_CXX)");
    MakefileRemove(f, "$(BISONSRC_HPP)");
  }

  //
  // Make distclean.
  //

  FilePutComment(f, "Make distclean. Make clean + clean Makefile.");

  fprintf(f, "distclean: clean\n");

  MakefileRemove(f, "Makefile");

  fprintf(f, "\n");
  fprintf(f, ".PHONY: install clean distclean\n");

  //
  // Error handler.
  //

  exitCode = 0;

  fail:

  if (f)
  {
    fclose(f);
  }

  DBG_LEAVE("GenerateMakefile");

  return exitCode;
};

//
// Append section to build component into main 'Build' script.
//
// f  - File, where to write section (IN).
// ci - structure describing component to build (IN).
//
// RETURNS: 0 if OK.
//

int FilePutBuildSection(FILE *f, SourceInfo *ci)
{
  fprintf(f, "qcbuild --setcolor --color 6\n");
  fprintf(f, "echo -------------------------------------------\n");
  fprintf(f, "echo Building '%s'...\n", ci -> get("TITLE"));
  fprintf(f, "echo -------------------------------------------\n");
  fprintf(f, "qcbuild --setcolor --color 7\n");

  fprintf(f, "cd %s\n", ci -> get("COMP_DIR"));
  fprintf(f, "%s\n", ScriptMakeInstall);
  fprintf(f, "%s\n", ScriptFailBlock);
  fprintf(f, "cd %s\n", ci -> get("ROOT"));
  fprintf(f, "\n");

  return 0;
}

//
// Append section to clean component into main 'Clean' script.
//
// f  - File, where to write section (IN).
// ci - structure describing component to build (IN).
//
// RETURNS: 0 if OK.
//

int FilePutCleanSection(FILE *f, SourceInfo *ci)
{
  fprintf(f, "qcbuild --setcolor --color 6\n");
  fprintf(f, "echo -------------------------------------------\n");
  fprintf(f, "echo Cleaning '%s'...\n", ci -> get("TITLE"));
  fprintf(f, "echo -------------------------------------------\n");
  fprintf(f, "qcbuild --setcolor --color 7\n");

  fprintf(f, "cd %s\n", ci -> get("COMP_DIR"));
  fprintf(f, "%s *.o\n", ScriptDelCmd);

  fprintf(f, "%s %s%s\n", ScriptDelCmd, ci -> getLowered("TITLE").c_str(), ExePostfix);

  fprintf(f, "%s *.a\n", ScriptDelCmd);
  fprintf(f, "%s *%s\n", ScriptDelCmd, DllPostfix);
  fprintf(f, "%s *.def\n", ScriptDelCmd);

  fprintf(f, "%s Makefile\n", ScriptDelCmd);

  if (ci -> isset("QT_MODULES"))
  {
    fprintf(f, "%s moc_*.cpp\n", ScriptDelCmd);
    fprintf(f, "%s ui_*.h\n", ScriptDelCmd);
  }

  fprintf(f, "cd %s\n", ci -> get("ROOT"));
  fprintf(f, "\n");

  return 0;
}

//
// Resolve dependencies beetwen list of components
// and generate main Build and Clean scripts (for whole project).
//
// comps - list of every components inside project (IN).
//
// RETURNS: 0 if OK.
//

int GenerateBuildCleanScripts(vector<SourceInfo> comps)
{
  DBG_ENTER("GenerateBuildCleanScripts");

  int exitCode = -1;

  int iter = 0;

  char buildFileName[MAX_PATH + 1] = {0};
  char cleanFileName[MAX_PATH + 1] = {0};

  const int maxIter = comps.size();

  std::set<string> resolvedSet;

  std::set<string>::iterator ri;

  vector<SourceInfo>::iterator ci;

  //
  // Create new 'Build' and 'Clean' script files.
  //

  snprintf(buildFileName, sizeof(buildFileName), "Build%s", ScriptPostfix);
  snprintf(cleanFileName, sizeof(buildFileName), "Clean%s", ScriptPostfix);

  FILE *buildFile = fopen(buildFileName, "wt+");
  FILE *cleanFile = fopen(cleanFileName, "wt+");

  FAILEX(buildFile == NULL, "ERROR: Cannot create '%s' script.\n", buildFileName);
  FAILEX(cleanFile == NULL, "ERROR: Cannot create '%s' script.\n", cleanFileName);

  if (stricmp(HostMachine, "MinGW") != 0)
  {
    FAILEX(system("chmod a+x Build.sh"), "ERROR: Cannot grant execute right for Build.sh script.\n");
    FAILEX(system("chmod a+x Clean.sh"), "ERROR: Cannot grant execute right for Clean.sh script.\n");
  }

  //
  // Write common script header.
  //

  fprintf(buildFile, "%s\n", ScriptIntroBlock);
  fprintf(cleanFile, "%s\n", ScriptIntroBlock);

  FilePutComment(buildFile, "DO NOT EDIT! This file was generated automatically from 'qcbuild.src' file.", ScriptComment);
  FilePutComment(cleanFile, "DO NOT EDIT! This file was generated automatically from 'qcbuild.src' file.", ScriptComment);

  //
  // Go on until all dependencies resolved.
  //

  while(comps.size() > 0 && iter < maxIter)
  {
    iter ++;

    //
    // Put resolved components first.
    //

    for (ci = comps.begin(); ci != comps.end();)
    {
      //
      // Already resolved. Move to resolved set
      // and put entry in build/clean scripts.
      //

      if (ci -> resolved())
      {
        FilePutBuildSection(buildFile, &(*ci));
        FilePutCleanSection(cleanFile, &(*ci));

        FAILEX(resolvedSet.count(ci -> get("TITLE")),
                   "ERROR: Component's name '%s' is NOT unique.\n",
                       ci -> get("TITLE"));

        resolvedSet.insert(ci -> get("TITLE"));

        comps.erase(ci);
      }

      //
      // Not resolved, go on.
      //

      else
      {
        ci ++;
      }
    }

    //
    // Try resolve another one basing on already resolved.
    //

    for (ci = comps.begin(); ci != comps.end();)
    {
      for (ri = resolvedSet.begin(); ri != resolvedSet.end(); ri ++)
      {
        ci -> resolve(ri -> c_str());
      }

      if (ci -> resolved())
      {
        FilePutBuildSection(buildFile, &(*ci));
        FilePutCleanSection(cleanFile, &(*ci));

        FAILEX(resolvedSet.count(ci -> get("TITLE")),
                   "ERROR: Component's name '%s' is NOT unique.\n",
                       ci -> get("TITLE"));

        resolvedSet.insert(ci -> get("TITLE"));

        comps.erase(ci);
      }
      else
      {
        ci ++;
      }
    }
  }

  if (comps.size() > 0)
  {
    for (int i = 0; i < comps.size(); i++)
    {
      Error("ERROR: Cannot resolve [%s] dependencies for [%s] component.\n",
                comps[i].getDepends().c_str(), comps[i].get("TITLE"));
    }

    FAIL(1);
  }

  #ifdef WIN32
  fprintf(cleanFile, "rd /S /Q Build\n");
  #else
  fprintf(cleanFile, "rm -rf Build\n");
  #endif

  fprintf(cleanFile, "%s Project.info\n", ScriptDelCmd);
  fprintf(cleanFile, "%s Build%s\n", ScriptDelCmd, ScriptPostfix);
  fprintf(cleanFile, "%s Clean%s\n", ScriptDelCmd, ScriptPostfix);

  //
  // Clean up.
  //

  exitCode = 0;

  fail:

  if (exitCode)
  {
    Error("ERROR: Cannot create main Build and/or Clean scripts.\n");
  }

  if (cleanFile)
  {
    fclose(cleanFile);
  }

  if (buildFile)
  {
    fclose(buildFile);
  }

  DBG_LEAVE("GenerateBuildCleanScripts");

  return exitCode;
};


//
// Generate list of whole components inside project.
//
// pi    - Info for root project's component (OUT/OPT).
// comps - list of infos for every components inside project (OUT).
//
// RETURNS: 0 if OK.
//

int ListComponents(SourceInfo *pi, vector<SourceInfo> &comps)
{
  DBG_ENTER("ListComponents");

  int exitCode = -1;

  char currentDir[MAX_PATH + 1];

  vector<string> dirs;

  SourceInfo piLocal;

  //
  // 'TITLE' |-> number in comps[] table.
  // Internal use only while generatin USED_BY lists.
  //

  map<string, int> titleToIdMap;

  //
  // Assume: ci = Component Info.
  //         pi = whole Project Info.
  //

  SourceInfo ci;

  //
  // Adjust, which variable use to load project info.
  //

  if (pi == NULL)
  {
    pi = &piLocal;
  }

  comps.clear();

  //
  // Load project info.
  //

  FAILEX(pi -> loadComponent(), "Cannot open 'qcbuild.src' file.\n");

  //
  // Save current working dir.
  //

  FileGetDir(currentDir, MAX_PATH);

  //
  // Switch to project root dir.
  //

  FileSetDir(pi -> get("ROOT"));

  //
  // List all subdirs recursively.
  //

  ListDirs(".", "*", dirs, true, FILE_SKIP_HIDDEN);

  for (int i = 0; i < dirs.size(); i++)
  {
    if (dirs[i].find("node_modules") != string::npos)
    {
      DBG_MSG("Skipped path [%s].\n", dirs[i].c_str());

      continue;
    }

    if (dirs[i].size() >= 240)
    {
      DBG_INFO("WARNING! Path too long [%s].\n", dirs[i].c_str());

      continue;
    }

    DBG_MSG("Entering [%s]...\n", dirs[i].c_str());

    FileSetDir(currentDir);
    FileSetDir(dirs[i].c_str());

    //
    // Load component info from 'subdir/.../qcbuild.src'.
    //

    if (ci.loadComponent() == 0)
    {
      pi -> loadProject();

      if (ci.getType() != TYPE_PROJECT)
      {
        ci.set("ROOT", pi -> get("ROOT"));
        ci.set("COMP_DIR_REL", dirs[i].c_str());

        //
        // If copyright not set in component, copyright from main
        // qcbuild.src in root directory.
        //

        if (ci.isset("COPYRIGHT") == 0 && pi -> isset("COPYRIGHT"))
        {
          ci.set("COPYRIGHT", pi -> get("COPYRIGHT"));
          ci.cat("COPYRIGHT", "(inherited from project's root)");
        }

        //
        // If licenset not set in component, copyright from main
        // qcbuild.src in root directory.
        //

        if ((strcmp(ci.get("LICENSE"), "Unknown") == 0) && pi -> isset("LICENSE"))
        {
          ci.set("LICENSE", pi -> get("LICENSE"));
          ci.cat("LICENSE", "(inherited from project's root)");
        }

        //
        // Add component's info to ouput vector.
        //

        comps.push_back(ci);

        titleToIdMap[ci.get("TITLE")] = comps.size() - 1;

        DBG_MSG("Imported component '%s'.\n", ci.get("TITLE"));
      }
    }
  }

  //
  // Restore original working dir.
  //

  FileSetDir(currentDir);

  //
  // Generate "USED_BY" list from "DEPENDS" list.
  //

  for (int i = 0; i < comps.size(); i++)
  {
    //
    // Get list of dependencies for every component.
    //

    const std::set<string> &depends = comps[i].getDependsSet();

    std::set<string>::iterator it;

    //
    // If 'comps[i] depends on X', thens 'X is used by comps[i]'
    //

    for (it = depends.begin(); it != depends.end(); it++)
    {
      int targetId = titleToIdMap[*it];

      DBG_MSG("[%s] is used by [%s].\n", it -> c_str(), comps[i].get("TITLE"));

      comps[targetId].cat("USED_BY", comps[i].get("TITLE"));
    }
  }

  //
  // Clean up.
  //

  exitCode = 0;

  fail:

  if (exitCode)
  {
    Error("ERROR: Cannot list components.\n");
  }

  DBG_LEAVE("ListComponents");

  return exitCode;
}

int AttachDependsToMultiLibrary(SourceInfo *mlib)
{
  int exitCode = -1;

  //
  // Multi-library.
  // Accumulate sources from all dependent libraries.
  //

  if (mlib -> getType() == TYPE_MULTI_LIBRARY)
  {
    vector<SourceInfo>::iterator ci;

    vector<SourceInfo> comps;

    FAIL(ListComponents(NULL, comps));

    for (ci = comps.begin(); ci != comps.end(); ci++)
    {
      if (mlib -> doesDependOn(ci -> get("TITLE")))
      {
        mlib -> cat("CSRC", ci -> getPathList("CSRC").c_str());
        mlib -> cat("CXXSRC", ci -> getPathList("CXXSRC").c_str());
        // TODO: Is it really needed: mlib -> cat("LIBS", ci -> get("LIBS"));
      }
    }
  }

  //
  // Error handler.
  //

  exitCode = 0;

  fail:

  return exitCode;
}

//
// Entry point.
//

int main(int argc, char **argv)
{
  DBG_ENTER("main");

  int exitCode = -1;

  SourceInfo proj;
  SourceInfo comp;

  vector<SourceInfo> comps;

  int color = 0;

  //
  // Parse arguments.
  //

  enum Mode
  {
    DEFAULT_MODE,
    INFO_MODE,
    DIAGRAM_MODE,
    PRINT_ROOT_MODE,
    MAKEDOC_MODE,
    SETCOLOR_MODE
  };

  Mode mode = DEFAULT_MODE;

  ArgsObj ao[] =
  {
    {ARGS_MODE,    "--info",     &mode,          INFO_MODE},
    {ARGS_MODE,    "--root",     &mode,          PRINT_ROOT_MODE},
    {ARGS_MODE,    "--diagram",  &mode,          DIAGRAM_MODE},
    {ARGS_MODE,    "--makedoc",  &mode,          MAKEDOC_MODE},
    {ARGS_MODE,    "--setcolor", &mode,          SETCOLOR_MODE},

    {ARGS_STRING,  "--target",   &TargetMachine, 0},
    {ARGS_STRING,  "--loglevel", &LogLevel,      0},
    {ARGS_STRING,  "--logfile",  &LogFile,       0},
    {ARGS_INT,     "--color",    &color,         0},
    {ARGS_NULL,    NULL,         NULL,           0}
  };

  FAIL(ArgsParse(ao, argc, argv));

  //
  // Set log level if specified.
  //

  if (LogLevel || LogFile)
  {
    DBG_INIT_EX(LogFile, LogLevel, DBG_REINIT);
  }

  //
  // Detect host machine, where compilation is runned.
  //

  HostMachine = DetectHost();

  FAILEX(HostMachine == NULL, "ERROR: Can't detect host machine.\n");

  //
  // Resolve host and target machines.
  //

  if (mode != SETCOLOR_MODE)
  {
    FAIL(LoadConfig(TargetMachine, HostMachine));

    DBG_MSG("Host machine   : [%s].\n", HostMachine);
    DBG_MSG("Target machine : [%s].\n", TargetMachine);
    DBG_MSG("EXE postfix    : [%s].\n", ExePostfix);
    DBG_MSG("DLL postfix    : [%s].\n", DllPostfix);

    printf("Host         : %s\n", HostMachine);
    printf("Target       : %s\n", TargetMachine);
    printf("Architecture : %s\n", DetectArch());
    printf("C++ compiler : %s\n", CxxCompiler);
    printf("\n");
  }

  //
  // Switch control depending on specified work mode.
  //

  switch(mode)
  {
    //
    // Default mode.
    //
    // - When callet from project's root generate main build/clean
    //   scripts and makefiles for all compnonents in subdirs recursively.
    //
    // - If called from components subdir generate makefile for
    //   current component only.
    //

    case DEFAULT_MODE:
    {
      FAILEX(comp.loadComponent(), "Cannot open 'qcbuild.src' file.\n");

      //
      // Project's root.
      //

      if (comp.getType() == TYPE_PROJECT)
      {
        FAILEX(freopen("Project.info", "wt+", stdout) == NULL,
                   "ERROR: Cannot redirect stdout to Project.info file.\n");

        DBG_MSG("Found project '%s'.\n", comp.get("TITLE"));

        comp.print();

        char currentDir[MAX_PATH + 1] = {0};

        FileGetDir(currentDir, MAX_PATH);

        vector<string> dirs;

        ListDirs(".", "*", dirs, true, FILE_SKIP_HIDDEN);

        for (int i = 0; i < dirs.size(); i++)
        {
          if (dirs[i].find("node_modules") != string::npos)
          {
            DBG_MSG("Skipped path [%s].\n", dirs[i].c_str());

            continue;
          }

          if (dirs[i].size() >= 240)
          {
            DBG_INFO("WARNING! Path too long [%s].\n", dirs[i].c_str());

            continue;
          }

          DBG_MSG("Entering [%s][%d][%d]...\n", dirs[i].c_str(), dirs[i].size(), MAX_PATH);

          FileSetDir(currentDir);
          FileSetDir(dirs[i].c_str());

          if (comp.loadComponent() == 0)
          {
            proj.loadProject();

            comp.set("ROOT", proj.get("ROOT"));
            comp.cat("LIBS", proj.get("LIBS"));
            comp.cat("INCLUDE", proj.get("INCLUDE"));
            comp.cat("DEFINES", proj.get("DEFINES"));
            comp.cat("INSTALL", proj.get("INSTALL"));
            comp.cat("INSTALL_INC", proj.get("INSTALL_INC"));
            comp.cat("INSTALL_LIB", proj.get("INSTALL_LIB"));
            comp.cat("INSTALL_BIN", proj.get("INSTALL_BIN"));

            printf("\n");

            if (comp.getType() != TYPE_PROJECT)
            {
              comps.push_back(comp);

              //
              // Multi-library.
              // Attach sources from dependent libraries.
              //

              if (comp.getType() == TYPE_MULTI_LIBRARY)
              {
                FAIL(AttachDependsToMultiLibrary(&comp));
              }

              //
              // Set BIN_DIR to Examples if *example* binary name found.
              //

              if ((comp.getType() == TYPE_PROGRAM)
                      && (stristr(comp.get("TITLE"), "example"))
                          && (!comp.isset("BIN_DIR")))
              {
                comp.set("BIN_DIR", "Examples");
              }

              //
              // Generate makefile.
              //

              GenerateMakefile(&comp);

              comp.print();

              DBG_MSG("Imported component '%s'.\n", comp.get("TITLE"));
            }
          }
        }

        FileSetDir(currentDir);

        //
        // Resolve compoments dependencies and generate
        // main Build/Clean scripts for whole project.
        //

        FAIL(GenerateBuildCleanScripts(comps));
      }

      //
      // Project's Component or stand alone.
      //

      else
      {
        if (comp.getType() != TYPE_SIMPLE_LIBRARY &&
                comp.getType() != TYPE_SIMPLE_PROGRAM)
        {
          FAIL(proj.loadProject());

          comp.set("ROOT", proj.get("ROOT"));
          comp.cat("LIBS", proj.get("LIBS"));
          comp.cat("INCLUDE", proj.get("INCLUDE"));
          comp.cat("DEFINES", proj.get("DEFINES"));
          comp.cat("INSTALL", proj.get("INSTALL"));
          comp.cat("INSTALL_INC", proj.get("INSTALL_INC"));
          comp.cat("INSTALL_LIB", proj.get("INSTALL_LIB"));
          comp.cat("INSTALL_BIN", proj.get("INSTALL_BIN"));

          //
          // Multi-library.
          // Attach sources from dependent libraries.
          //

          if (comp.getType() == TYPE_MULTI_LIBRARY)
          {
            FAIL(AttachDependsToMultiLibrary(&comp));
          }

          proj.print();
        }

        comp.print();

        FAIL(GenerateMakefile(&comp));
      }

      break;
    }

    //
    // --root
    // Find project's root directory.
    //

    case PRINT_ROOT_MODE:
    {
      FAIL(proj.loadProject());

      printf("%s\n", proj.get("ROOT"));

      break;
    }

    //
    // --info
    // Print info about current component.
    //

    case INFO_MODE:
    {
      FAIL(proj.loadProject());

      proj.print();

      if (comp.loadComponent() == 0)
      {
        printf("\n");

        comp.print();
      }

      break;
    }

    //
    // --diagram
    //
    // Generate SVG diagram showing connections beetwen components
    // inside project.
    //

    case DIAGRAM_MODE:
    {
      //
      // Load all components from tree.
      //

      FAIL(ListComponents(&proj, comps));

      //
      // Generate diagram on stdout.
      //

      FAIL(CreateComponentDiagram(stdout, comps));

      break;
    }

    //
    // --makedoc
    //
    // Generate HTML documentation for whole project in './doc'
    // subdirectory.
    //

    case MAKEDOC_MODE:
    {
      printf("Generating public documentation...\n");
      printf("----------------------------------\n");
      AutoDocGenerate(0);

      printf("\n");
      printf("Generating private documentation...\n");
      printf("-----------------------------------\n");
      AutoDocGenerate(1);

      break;
    }

    //
    // --setcolor --color <colorcode>
    // Set current console color.
    //

    case SETCOLOR_MODE:
    {
      #ifdef WIN32
      SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), color);
      #endif

      break;
    }
  }

  //
  // Error handler.
  //

  exitCode = 0;

  fail:

  DBG_LEAVE("main");

  return exitCode;
}
