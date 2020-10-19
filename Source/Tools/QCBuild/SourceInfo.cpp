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

#include "SourceInfo.h"

using namespace Tegenaria;

extern const char *TargetMachine;

SourceInfo::SourceInfo()
{
  clear();
}

//
//
//

int SourceInfo::set(const char *lvalue, const char *rvalue, int concat)
{
  DBG_ENTER3("SourceIinfo::set");

  FAIL(lvalue == NULL);
  FAIL(rvalue == NULL);

  if (stricmp(lvalue, "TYPE") == 0)
  {
    if (stricmp(rvalue, "PROJECT") == 0) type_ = TYPE_PROJECT;
    else if (stricmp(rvalue, "PROGRAM") == 0) type_ = TYPE_PROGRAM;
    else if (stricmp(rvalue, "LIBRARY") == 0) type_ = TYPE_LIBRARY;
    else if (stricmp(rvalue, "SIMPLE_LIBRARY") == 0) type_ = TYPE_SIMPLE_LIBRARY;
    else if (stricmp(rvalue, "SIMPLE_PROGRAM") == 0) type_ = TYPE_SIMPLE_PROGRAM;
    else if (stricmp(rvalue, "MULTI_LIBRARY") == 0) type_ = TYPE_MULTI_LIBRARY;
    else type_ = TYPE_UNKNOWN;

    vars_[lvalue] = rvalue;
  }
  else
  {
    if (concat)
    {
      if (isset(lvalue))
      {
        vars_[lvalue] += " ";
        vars_[lvalue] += rvalue;
      }
      else
      {
        vars_[lvalue] = rvalue;
      }
    }
    else
    {
      if (rvalue[0] == 0)
      {
        vars_.erase(rvalue);
      }
      else
      {
        vars_[lvalue] = rvalue;
      }
    }
  }

  //
  // Convert DEPENDS string to STL set<>.
  //

  if (stricmp(lvalue, "DEPENDS") == 0)
  {
    //
    // Split DEPENDS string into titles list (tokens).
    //

    std::set<string> tokens;
    std::set<string>::iterator it;

    StrTokenize(tokens, rvalue, " \t", "\"");

    //
    // Concat mode. Add more tokens to current set.
    //

    if (concat)
    {
      for (it = tokens.begin(); it != tokens.end(); it++)
      {
        dependsSet_.insert(*it);
      }
    }

    //
    // Assign mode. Simple copy tokens into current set.
    //

    else
    {
      dependsSet_ = tokens;
    }

    //
    // Review resolved state.
    //

    if (dependsSet_.empty())
    {
      dependsResolved_ = 1;
    }
    else
    {
      dependsResolved_ = 0;
    }
  }

  //
  // Convert AUTODOC_PRIVATE_DIRS string to STL set<>.
  //

  else if (stricmp(lvalue, "AUTODOC_PRIVATE_DIRS") == 0)
  {
    if (this -> getType() != TYPE_PROJECT)
    {
      Fatal("ERROR: Improper use of AUTODOC_PRIVATE_DIRS in '%s' component.\n"
            "This option can be used for project root only.\n", get("TITLE"));
    }

    //
    // Split AUTODOC_PRIVATE_DIRS string into titles list (tokens).
    //

    std::set<string> tokens;
    std::set<string>::iterator it;

    StrTokenize(tokens, rvalue, " \t", "\"");

    //
    // Concat mode. Add more tokens to current set.
    //

    if (concat)
    {
      for (it = tokens.begin(); it != tokens.end(); it++)
      {
        privateDirsSet_.insert(*it);
      }
    }

    //
    // Assign mode. Simple copy tokens into current set.
    //

    else
    {
      privateDirsSet_ = tokens;
    }
  }

  //
  // Convert absolute component path to relative to ROOT
  // for project's components.
  //

  else if ((type_ == TYPE_PROGRAM || type_ == TYPE_LIBRARY || type_ == TYPE_MULTI_LIBRARY) &&
               stricmp(lvalue, "ROOT") == 0)
  {
    string compDir = get("COMP_DIR");
    string root    = get("ROOT");

    set("COMP_DIR", CreateRelativePath(&compDir[0], &root[0]));

    //
    // Find top-subdir for component e.g. 'Lib' for 'Lib/LibFoo'.
    //

    this -> set("TOP_SUBDIR", FilePathGetTopDir(this -> get("COMP_DIR")).c_str());
  }

  fail:

  DBG_LEAVE3("SourceIinfo::set");

  return 0;
}

//
//
//

int SourceInfo::cat(const char *lvalue, const char *rvalue)
{
  set(lvalue, rvalue, 1);
}

//
//
//

const char *SourceInfo::get(const char *lvalue)
{
  map<string, string>::iterator it = vars_.find(lvalue);

  if (it != vars_.end())
  {
    return it -> second.c_str();
  }

  return "";
}

string SourceInfo::getLowered(const char *lvalue)
{
  map<string, string>::iterator it = vars_.find(lvalue);

  if (it != vars_.end())
  {
    string ret = it -> second.c_str();

    for (int i = 0; i < ret.size(); i++)
    {
      if (ret[i] >= 'A' && ret[i] <= 'Z')
      {
        ret[i] += 'z' - 'Z';
      }
    }

    return ret;
  }

  return "";
}

int SourceInfo::isset(const char *lvalue)
{
  if (get(lvalue)[0])
  {
    return 1;
  }

  return 0;
}

int SourceInfo::getInt(const char *lvalue)
{
  return atoi(get(lvalue));
}

//
//
//

void SourceInfo::print()
{
  if (type_ == TYPE_PROJECT)
  {
    printf("Project '%s':\n"
           "--------------------------\n\n", get("TITLE"));
  }
  else
  {
    printf("Component '%s':\n"
           "--------------------------\n\n", get("TITLE"));
  }

  if (isset("TITLE"))    printf("Title   : %s\n", get("TITLE"));
  if (isset("ROOT"))     printf("Root    : %s\n", get("ROOT"));
  if (isset("COMP_DIR")) printf("Path    : %s\n", get("COMP_DIR"));
  if (isset("LIBS"))     printf("LIBS    : %s\n", get("LIBS"));
  if (isset("INCLUDE"))  printf("INCLUDE : %s\n", get("INCLUDE"));
  if (isset("CSRC"))     printf("CSRC    : %s\n", get("CSRC"));
  if (isset("CXXSRC"))   printf("CXXSRC  : %s\n", get("CXXSRC"));
  if (isset("ISRC"))     printf("ISRC    : %s\n", get("ISRC"));
  if (isset("DEFINES"))  printf("DEFINES : %s\n", get("DEFINES"));
  if (isset("DEPENDS"))  printf("DEPENDS : %s\n", get("DEPENDS"));
}

//
// Check does component has resolved all dependencies.
//

int SourceInfo::resolved()
{
  if (dependsResolved_ == 1 || dependsSet_.empty())
  {
    dependsResolved_ = 1;

    return 1;
  }

  return 0;
}

//
// Resolve dependencies on given component.
//

void SourceInfo::resolve(const char *compTitle)
{
  dependsSet_.erase(compTitle);

  if (dependsSet_.empty())
  {
    dependsResolved_ = 1;
  }
}

//
//
//

void SourceInfo::clear()
{
  type_ = TYPE_UNKNOWN;

  vars_.clear();

  vars_["TITLE"]   = "Untitled";
  vars_["LICENSE"] = "Unknown";
  vars_["AUTHOR"]  = "Unknown";
  vars_["DESC"]    = "-";
  vars_["PURPOSE"] = "-";

  dependsSet_.clear();

  dependsResolved_ = 0;
}

string SourceInfo::getDepends()
{
  string ret;

  std::set<string>::iterator it;

  int delimCount = dependsSet_.size() - 1;

  for (it = dependsSet_.begin(); it != dependsSet_.end(); it++)
  {
    ret += *it;

    if (delimCount > 0)
    {
      ret += " ";

      delimCount --;
    }
  }

  return ret;
}

string SourceInfo::getPathList(const char *lvalue)
{
  string rv;

  string value = this -> get(lvalue);

  if (!value.empty())
  {
    vector<string> tokens;

    StrTokenize(tokens, value, " \t", "\"");

    for (int i = 0; i < tokens.size(); i++)
    {
      rv += this -> get("COMP_DIR_REL");
      rv += "/";
      rv += tokens[i];
      rv += " ";
    }
  }

  return rv;
}

const std::set<string> &SourceInfo::getDependsSet()
{
  return dependsSet_;
}

//
// Load data for current component from 'qcbuild.src' file.
//
// RETURNS: 0 if OK.
//

int SourceInfo::loadComponent()
{
  DBG_ENTER3("SourceInfo::loadComponent");

  char path[MAX_PATH + 1] = {0};

  FileGetDir(path, MAX_PATH);

  strncat(path, "/qcbuild.src", MAX_PATH);

  this -> set("ROOT", ".");

  DBG_LEAVE3("SourceInfo::loadComponent");

  return this -> loadFromFile(path);
}

//
// Load root project data from 'qcbuild.src' file.
//
// RETURNS: 0 if OK.
//

int SourceInfo::loadProject()
{
  DBG_ENTER("SourceInfo::loadProject");

  int exitCode = -1;

  char rootDir[MAX_PATH + 1]        = {0};
  char sourceFilePath[MAX_PATH + 1] = {0};
  char currentDir[MAX_PATH + 1]     = {0};
  char relRootDir[MAX_PATH + 1]     = ".";

  FILE *f = NULL;

  int found = 0;
  int goOn  = 1;

  FileGetDir(currentDir, MAX_PATH);

  while(goOn)
  {
    FileGetDir(rootDir, MAX_PATH);

    if (strlen(rootDir) <= 3)
    {
      goOn = 0;
    }
    else
    {
      strncpy(sourceFilePath, rootDir, MAX_PATH);
      strncat(sourceFilePath, "/qcbuild.src", MAX_PATH);

      if (loadFromFile(sourceFilePath) == 0)
      {
        if (type_ == TYPE_PROJECT)
        {
          found = 1;
          goOn  = 0;

          this -> set("ROOT", relRootDir);
        }
      }

      if (found == 0)
      {
        strncat(rootDir, "/..", MAX_PATH);
        strncat(relRootDir, "/..", MAX_PATH);

        FileSetDir(rootDir);
      }
    }
  }

  FAILEX(found == 0, "ERROR: Cannot recognise project's root directory.\n");

  this -> set("COMP_DIR", ".");

  //
  // Error handler.
  //

  exitCode = 0;

  fail:

  FileSetDir(currentDir);

  DBG_LEAVE("SourceInfo::loadProject");

  return exitCode;
}

//
// Load componend data from file.
//
// si    - buffer where to store loaded component's description (OUT).
// fname - path to .src file with component's descritpion (IN).
//
// RETURNS: 0 if OK.
//

int SourceInfo::loadFromFile(const char *fname)
{
  DBG_ENTER3("SourceInfo::loadFromFile");

  int exitCode = -1;

  char *buf    = FileLoad(fname);
  char *p      = buf;
  char *next   = NULL;
  char *lvalue = NULL;
  char *rvalue = NULL;
  char *eqPtr  = NULL;
  char *catPtr = NULL;
  char *eol    = NULL;

  char currentDir[MAX_PATH + 1] = {0};

  string currentSection = "COMMON";

  std::set<string> currentSectionsSet;

  FAIL(buf == NULL);

  clear();

  while(*p)
  {
    p = CleanToken(p);

    next = SkipLine(p);

    //
    // Remove EOL.
    //

    eol = p + strlen(p) - 1;

    while(eol > p && (eol[0] == 13 || eol[0] == 10))
    {
      eol[0] = 0;

      eol --;
    }

    DBG_MSG3("[%s] : Parsing line [%s]...\n", currentSection.c_str(), p);

    switch(p[0])
    {
      //
      // Comment.
      //

      case '#':
      {
        break;
      };

      //
      // Empty line.
      //

      case 0:
      case 13:
      case 10:
      {
        break;
      }

      //
      // Section tag. Possible tags are:
      //
      // .section
      // .sectionend
      //

      case '.':
      {
        //
        // Enter into new section.
        //

        if (strncmp(p, ".section", sizeof(".section") - 1) == 0)
        {
          std::vector<string> tokens;

          char *section = p + sizeof(".section");

          CleanToken(section);

          DBG_MSG("Entering section [%s]...\n", section);

          StrTokenize(tokens, section, ",");

          for (int i = 0; i < tokens.size(); i ++)
          {
            currentSectionsSet.insert(CleanToken(&tokens[i][0]));
          }

          currentSection = section;
        }

        //
        // Leave current section and back to 'COMMON' section.
        //

        else if (strncmp(p, ".endsection", sizeof(".endsection") - 1) == 0)
        {
          DBG_MSG("Leaving [%s] section...\n", currentSection.c_str());

          currentSection = "COMMON";

          currentSectionsSet.clear();
        }

        break;
      }

      //
      // Default. Possible syntax are:
      // <lvalue> = <rvalue>
      // <lvalue> += <rvalue>
      //

      default:
      {
        //
        // Skip line if it's not targeted for target machine.
        //

        DBG_MSG3("[%s] : [%s]\n", currentSection.c_str(), p);

        if (currentSection != "COMMON" && currentSectionsSet.count(TargetMachine) == 0)
        {
          DBG_MSG3("[%s] : Skipped [%s].\n", currentSection.c_str(), p);

          break;
        }

        //
        // Split line into three <lvalue>, <operator>, <rvalue> parts.
        //

        eqPtr  = strchr(p, '=');
        catPtr = strstr(p, "+=");

        //
        // <lvalue> += <rvalue>
        // Append another value to existing one.
        // E.g. expand LIBS defined in COMMON section by '-ldl' entry
        // only in 'Linux' section.
        //

        if (catPtr)
        {
          catPtr[0] = 0;
          lvalue    = CleanToken(p);
          rvalue    = CleanToken(catPtr + 2);

          this -> cat(lvalue, rvalue);
        }

        //
        // <lvalue> = <rvalue>
        // Assign new value to variable. If variable already set
        // old value is overwritten.
        //

        else if (eqPtr)
        {
          eqPtr[0] = 0;
          lvalue   = CleanToken(p);
          rvalue   = CleanToken(eqPtr + 1);

          this -> set(lvalue, rvalue);
        }

        //
        // Unknown operator.
        //

        else
        {
          fprintf(stderr, "ERROR: '=' or '+=' expected.\n");

          goto fail;
        }
      }
    }

    p = next;
  }

  FileGetDir(currentDir, MAX_PATH);

  this -> set("COMP_DIR", currentDir);

  //
  // Error handler.
  //

  exitCode = 0;

  fail:

  if (buf)
  {
    free(buf);
  }

  DBG_LEAVE3("SourceInfo::loadFromFile");

  return exitCode;
}

int SourceInfo::doesDependOn(const char *compTitle)
{
  return dependsSet_.count(compTitle);
}

int SourceInfo::doesMatchPrivateDir(SourceInfo *ci)
{
  for (std::set<string>::iterator it = privateDirsSet_.begin(); it != privateDirsSet_.end(); it++)
  {
    const char *privatePrefix = it -> c_str();
    const char *compDir       = ci -> get("COMP_DIR");

    const char *ptr = strstr(compDir, privatePrefix);

    if ((ptr == compDir) && (ptr[0] == '/' || ptr[0] == '\\' == ptr[0] == 0))
    {
      return 1;
    }
  }

  return 0;
}
