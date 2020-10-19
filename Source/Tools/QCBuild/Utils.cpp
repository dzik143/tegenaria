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

#ifdef WIN32
# define WINVER 0x501
# include <windows.h>
#endif

#include "Utils.h"

using namespace Tegenaria;

using std::string;

int MakefileMkdir(FILE *f, const char *path)
{
  fprintf(f, "\tmkdir -p %s\n", path);
}

int MakefileRemove(FILE *f, const char *path)
{
  return fprintf(f, "\trm -f %s\n", path);
}

int MakefileCopy(FILE *f, const char *src, const char *dst)
{
  return fprintf(f, "\tcp -rp %s %s\n", src, dst);
}

//
// Put comment into script file.
//
// f       - file, where to write comment (IN).
// str     - message to put inside comment (IN).
// comment - tag used for one line comment (e.g. '#' in bash) (IN).
//

void FilePutComment(FILE *f, const char *str, const char *comment)
{
  fprintf(f, "\n");
  fprintf(f, "%s\n", comment);
  fprintf(f, "%s %s\n", comment, str);
  fprintf(f, "%s\n", comment);
  fprintf(f, "\n");
}


//
// Skip white chars.
//
// buf[] - pointer to ASCIZ string (IN).
//
// RETURNS: pointer to first non-white char in string.
//

char *SkipWhites(char *buf)
{
  while(isspace(*buf))
  {
    buf ++;
  }

  return buf;
}

//
// Remove white chars from left and right sides of string.
// White chars on right hand are filled by zeros.
//
// p[] - pointer to ASCIZ string (IN/OUT).
//
// RETURNS: pointer to first non-white char in string.
//

char *CleanToken(char *p)
{
  p = SkipWhites(p);

  int len = strlen(p);

  while(len > 0 && isspace(p[len - 1]))
  {
    len --;

    p[len] = 0;
  }

  return p;
}

//
// Go to next line at buffer.
//
// p[] - Pointer to ASCIZ string (IN/OUT).
//
// RETURNS: Pointer to begin of next line.
//

char *SkipLine(char *p)
{
  while((*p) && (*p != 13) && (*p != 10))
  {
    p ++;
  }

  while((*p) && ((*p == 13) || (*p == 10)))
  {
    p ++;
  }

  if (p[0])
  {
    p[-1] = 0;
  }

  return p;
}

//
// Convert absolute component path to relative to root.
//
// path    - abosolute component's path e.g. 'C:/proj/comp' (IN).
// relRoot - relative root path e.g. './../' (IN).
//
// RETURNS: pointer to relative part of component's path.
//

char *CreateRelativePath(char *path, char *relRoot)
{
  int count = 0;

  char *p = relRoot;

  int len = strlen(path);

  while(p = strstr(p, ".."))
  {
    count ++;

    p += 2;
  }

  p = path + len;

  if (len > 0) p --;

  while(count > 0 && len > 0)
  {
    p --;

    while(p > path && p[0] != '\\' && p[0] != '/')
    {
      p --;
    }

    count --;
  }

  //
  // Normalize path.
  //

  for (int i = 0; p[i]; i++)
  {
    if (p[i] == '\\')
    {
      p[i] = '/';
    }
  }

  p ++;

  return p;
}

//
// Retrieve QT property by 'qmake -query <property>'.
//
// lvalue - name of property to retrieve e.g. QT_INSTALL_LIBS (IN).
//
// RERUTNS: Retrieved proeprty
//          or empty string if error.
//

string QtQueryProperty(const char *lvalue)
{
  DBG_ENTER("QtQueryProperty");

  int exitCode = -1;

  char cmd[1024];

  char *rvalueBuf = NULL;

  string rvalue;

  //
  // Run qmake -query <lvalue>
  // and redirect stdout to temp file.
  //

  snprintf(cmd, sizeof(cmd), "qmake -query %s > .qcbuild.tmp", lvalue);

  FAIL(system(cmd));

  //
  // Retrieve returned <rvalue> from temp file.
  //

  rvalueBuf = FileLoad(".qcbuild.tmp");

  FAIL(rvalueBuf == NULL);

  //
  // Remove EOL.
  // Change \\ to / for cygwin compatible.
  //

  for (int i = 0; rvalueBuf[i]; i++)
  {
    if (rvalueBuf[i] == 10 || rvalueBuf[i] == 13)
    {
      rvalueBuf[i] = 0;
    }
    else if (rvalueBuf[i] == '\\')
    {
      rvalueBuf[i] = '/';
    }
  }

  rvalue = rvalueBuf;

  //
  // Clean up.
  //

  exitCode = 0;

  fail:

  if (exitCode)
  {
    Error("ERROR: Cannot run qmake tool.\n"
          "ERROR: Please ensure does QT is properly installed.\n");
  }

  if (rvalueBuf)
  {
    free(rvalueBuf);
  }

  #ifdef WIN32
  DeleteFile(".qcbuild.tmp");
  #else
  unlink(".qcbuild.tmp");
  #endif

  DBG_LEAVE("QtQueryProperty");

  return rvalue;
}

//
// Detect current running architecture.
//
// RETURNS: Returns i386, amd64 or Unknown.
//

const char *DetectArch()
{
  DBG_ENTER("DetectArch");

  const char *arch = "unknown";

  //
  // Windows.
  //

  #ifdef WIN32
  {
    //
    // FIXME!
    //

    arch = "i386";

  /*
    SYSTEM_INFO si = {0};

    GetNativeSystemInfo(&si);

    switch(si.wProcessorArchitecture)
    {
      case PROCESSOR_ARCHITECTURE_INTEL: arch = "i386"; break;
      case PROCESSOR_ARCHITECTURE_AMD64: arch = "amd64"; break;
    }
  */
  }

  //
  // Linux, MacOS.
  //

  #else
  {
    char cmd[1024] = {0};

    char *unameResult = NULL;

    //
    // Run 'uname -m'.
    // and redirect stdout to temp file.
    //

    snprintf(cmd, sizeof(cmd), "uname -m > .qcbuild.uname.tmp");

    FAIL(system(cmd));

    //
    // Check for '64' string in uname result.
    // If not found assume i386.
    //

    unameResult = FileLoad(".qcbuild.uname.tmp");

    FAIL(unameResult == NULL);

    if (strstr(unameResult, "64"))
    {
      arch = "amd64";
    }
    else
    {
      arch = "i386";
    }

    //
    // Clean up.
    //

    fail:

    unlink(".qcbuild.uname.tmp");

    if (unameResult)
    {
      free(unameResult);
    }
  }
  #endif

  DBG_MSG("Detected architecture is [%s].\n", arch);

  DBG_LEAVE("DetectArch");

  return arch;
}
