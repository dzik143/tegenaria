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

#include <sys/stat.h>
#include <dirent.h>
#include <cstring>
#include <string>
#include <vector>
#include <stack>
#include <fstream>

#ifdef WIN32
#include <windows.h>
#include <io.h>
#endif

#include <Tegenaria/Debug.h>
#include "File.h"
#include "Internal.h"

namespace Tegenaria
{
  using namespace std;

  //
  // Load file to new allocate buffer.
  //
  // fname  - name of file to load (IN).
  // readed - number of bytes allocated and readed (OUT/OPT).
  //
  // RETURNS: New allocated buffer or NULL if fail.
  //

  char *FileLoad(const char *fname, int *readed)
  {
    char *buf = NULL;

    int fsize = 0;

    FILE *f = NULL;

    //DBG_MSG("LoadFile(\"%s\")...\n", fname);

    //
    // Open file.
    //

    f = fopen(fname, "rb");

    FAIL(f == NULL);

    //
    // Get file size.
    //

    FAIL(fseek(f, 0, 2) < 0);

    fsize = ftell(f);

    //
    // Allocate buffer.
    //

    buf = (char *) malloc(sizeof(char) * (fsize + 2));

    FAIL(buf == NULL);

    //
    // Read file.
    //

    fseek(f, 0, 0);

    if (fsize > 0)
    {
      FAIL(fread(buf, fsize, 1, f) != 1);

      buf[fsize] = 0;
      buf[fsize + 1] = 0;
    }
    else
    {
      free(buf);

      buf = NULL;
    }

    fclose(f);

    if (readed)
    {
      *readed = fsize;
    }

    return buf;

  fail:

    return NULL;
  }

  //
  // Make file list in given directory.
  //
  // path      - directory where to search (IN)
  // mask      - file mask (e.g. '*.dat') (IN)
  // files     - output file list (OUT)
  // recursive - search also in subdirs, if set to true (IN)
  // flags     - combination of FILE_XXX flags defined in File.h (IN/OPT).
  //
  // RETURNS: 0 if OK
  //

  int ListFiles(string path, string mask, vector<string>& files,
                    bool recursive, unsigned int flags)
  {
    //
    // Windows.
    //

    #ifdef WIN32

    HANDLE hFind = INVALID_HANDLE_VALUE;

    WIN32_FIND_DATA ffd;

    string spec;

    stack<string> directories;

    directories.push(path);

    files.clear();

    while (!directories.empty())
    {
      path = directories.top();

      spec = path + "\\" + mask;

      directories.pop();

      hFind = FindFirstFile(spec.c_str(), &ffd);

      if (hFind == INVALID_HANDLE_VALUE)
      {
        return -1;
      }

      do
      {
        if (strcmp(ffd.cFileName, ".") != 0 && strcmp(ffd.cFileName, "..") != 0)
        {
          if (recursive && ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
          {
            directories.push(path + "\\" + ffd.cFileName);
          }
          else if ((ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
          {
            files.push_back(ffd.cFileName);
          }
        }
      } while (FindNextFile(hFind, &ffd) != 0);

      if (GetLastError() != ERROR_NO_MORE_FILES)
      {
        FindClose(hFind);

        return -1;
      }

      FindClose(hFind);

      hFind = INVALID_HANDLE_VALUE;
    }

    return 0;

    //
    // Linux, Mac.
    //

    #else

    fprintf(stderr, "ERROR: ListFiles() is not implemented yet.\n");

    return -1;

    #endif
  }

  //
  // Make directories list in given directory.
  //
  // path      - directory where to search (IN)
  // mask      - file mask (IN)
  // files     - output file list (OUT)
  // recursive - list subdirs if true (IN)
  // flags     - combination of FILE_XXX flags defined in File.h (IN/OPT).
  //
  // RETURNS: 0 if OK
  //

  int ListDirs(string path, string mask, vector<string>& dirs,
                   bool recursive, unsigned int flags)
  {
    //
    // Windows.
    //

    #ifdef WIN32

    HANDLE hFind = INVALID_HANDLE_VALUE;

    WIN32_FIND_DATA ffd;

    string spec;

    int skip = path.size();

    stack<string> directories;

    directories.push(path);

    dirs.clear();

    while (!directories.empty())
    {
      path = directories.top();

      spec = path + "\\" + mask;

      directories.pop();

      hFind = FindFirstFile(spec.c_str(), &ffd);

      if (hFind == INVALID_HANDLE_VALUE)
      {
        return -1;
      }

      do
      {
        //
        // Skip hidden if needed.
        //

        if (flags & FILE_SKIP_HIDDEN)
        {
          if (ffd.dwFileAttributes == FILE_ATTRIBUTE_HIDDEN)
          {
            continue;
          }

          if (ffd.cFileName[0] == '.')
          {
            continue;
          }
        }

        //
        // Parse dir.
        //

        if (strcmp(ffd.cFileName, ".") != 0 && strcmp(ffd.cFileName, "..") != 0)
        {
          if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
          {
            if (path.size() == skip)
            {
              dirs.push_back(ffd.cFileName);
            }
            else
            {
              dirs.push_back(path.substr(skip + 1) + "\\" + ffd.cFileName);
            }

            if (recursive)
            {
              directories.push(path + "\\" + ffd.cFileName);
            }
          }
        }
      } while (FindNextFile(hFind, &ffd) != 0);

      if (GetLastError() != ERROR_NO_MORE_FILES)
      {
        FindClose(hFind);

        return -1;
      }

      FindClose(hFind);

      hFind = INVALID_HANDLE_VALUE;
    }

    return 0;

    //
    // Linux, MacOS.
    //

    #else

    string spec;

    int skip = path.size();

    stack<string> directories;

    directories.push(path);

    dirs.clear();

    while (!directories.empty())
    {
      path = directories.top();

      spec = path + "/";

      directories.pop();

      DIR *dir = opendir(spec.c_str());

      if (dir == NULL)
      {
        fprintf(stderr, "ERROR: Cannot open directory '%s'.\n", spec.c_str());

        return -1;
      }

      //
      //
      //

      struct dirent *direntp = NULL;

      while ((direntp = readdir(dir)) != NULL)
      {
        //
        // Skip hidden directories if needed.
        //

        if (flags & FILE_SKIP_HIDDEN && direntp -> d_name[0] == '.')
        {
          continue;
        }

        //
        // Parse directory.
        //

        if (strcmp(direntp -> d_name, ".") != 0 && strcmp(direntp -> d_name, "..") != 0)
        {
          struct stat stat_buf = {0};

          int ret = stat((path + "/" + direntp -> d_name).c_str(), &stat_buf);

          if (S_ISDIR(stat_buf.st_mode))
          {
            if (path.size() == skip)
            {
              dirs.push_back(direntp -> d_name);
            }
            else
            {
              dirs.push_back(path.substr(skip + 1) + "/" + direntp -> d_name);
            }

            if (recursive)
            {
              directories.push(path + "/" + direntp -> d_name);
            }
          }
        }
      }

      closedir(dir);
    }

    return 0;

    #endif
  }

  //
  // Compare two files.
  //
  // stat   - result of comparison (1 if equal, 0 if not) (OUT).
  // fname1 - first file to compare (IN).
  // fname2 - second file to compare (IN).
  //
  // RETURNS: 0 if OK.
  //

  int FileCompare(int &stat, const string &fname1, const string &fname2)
  {
    int exitCode = -1;

    ifstream f1(fname1.c_str());
    ifstream f2(fname2.c_str());

    FAIL(f1.fail());
    FAIL(f2.fail());

    stat = 1;

    while(f1.good() || f2.good())
    {
      char c1;

      char c2;

      f1.get(c1);
      f2.get(c2);

      if (c1 != c2)
      {
        stat = 0;

        break;
      }
    }

    exitCode = 0;

  fail:

    f1.close();
    f2.close();

    return exitCode;
  }

  //
  // Check does file or directory exists on disk.
  //
  // path - path to check (IN).
  //
  // RETURNS: 1 if file exists.
  //

  int FileExists(const string &path)
  {
    int exist = 0;

    struct stat buf;

    if (stat(path.c_str(), &buf) == 0)
    {
      exist = 1;

      DEBUG3("FileExists: Path [%s] exists.\n", path.c_str());
    }
    else
    {
      DEBUG3("FileExists: Path [%s] does not exists.\n", path.c_str());
    }

    return exist;
  }

  //
  // Wait until given file exists on disk.
  //
  // fname   - file name to wait (IN).
  //
  // timeout - maximum allowed wait time in ms. Defaulted to
  //           infinite if skipped (IN/OPT).
  //
  // RETURNS: 0 if file found within timeout,
  //         -1 if timeout reached.
  //

  int FileWait(const char *fname, int timeout)
  {
    DBG_ENTER("FileWait");

    int exitCode = -1;

    //
    // Infinite loop.
    //

    if (timeout < 0)
    {
      while(FileExists(fname) == 0)
      {
        #ifdef WIN32
        Sleep(10);
        #else
        usleep(10000);
        #endif
      }
    }

    //
    // Timeouted loop.
    //

    else
    {
      while(timeout > 0 && FileExists(fname) == 0)
      {
        #ifdef WIN32
        Sleep(10);
        #else
        usleep(10000);
        #endif

        timeout -= 10;
      }

      FAILEX(timeout <= 0, "ERROR: Timeout while waiting for [%s] file.\n", fname);
    }

    //
    // Error handler.
    //

    exitCode = 0;

    fail:

    DBG_LEAVE("FileWait");

    return exitCode;
  }

  //
  // Remove file from disk.
  //
  // fname - path to file which we want to delete (IN).
  //
  // RETURNS: 0 if OK.
  //

  int FileDelete(const char *fname)
  {
    int exitCode = -1;

    #ifdef WIN32
    {
      FAIL(DeleteFile(fname) == FALSE);
    }
    #else
    {
      FAIL(unlink(fname) != 0);
    }
    #endif

    DEBUG2("FileDelete: File [%s] deleted.\n", fname);

    //
    // Error handler.
    //

    exitCode = 0;

    fail:

    if (exitCode)
    {
      Error("ERROR: Cannot delete file [%s]. Error is %d.\n", fname, GetLastError());
    }

    return exitCode;
  }

  //
  // Create directory recursively at one call.
  //
  // target - path to create (e.g. 'A/B/C/D') (IN).
  //
  // RETURNS: 0 if OK.
  //

  int FileCreateRecursiveDir(const string &target)
  {
    int exitCode = -1;

    string path = target + "/";

    for (int i = 0; i < path.size(); i++)
    {
      if (path[i] == '\\' || path[i] == '/')
      {
        path[i] = 0;

        //
        // Windows.
        //

        #ifdef WIN32
        {
          if (path[0] && FileExists(path.c_str()) == 0)
          {
            FAILEX(CreateDirectory(path.c_str(), NULL) == FALSE,
                       "ERROR: Cannot create [%s] dir.\n", path.c_str());
          }

          path[i] = '\\';
        }

        //
        // Linux, MacOS.
        //

        #else
        {
          if (path[0] && FileExists(path.c_str()) == 0)
          {
            FAILEX(mkdir(path.c_str(), 0755),
                       "ERROR: Cannot create [%s] dir.\n", path.c_str());
          }

          path[i] = '/';
        }
        #endif


      }
    }

    //
    // Error handler.
    //

    exitCode = 0;

    fail:

    if (exitCode)
    {
      Error("ERROR: Cannot create [%s] path. Error code is %d.\n",
                target.c_str(), GetLastError());
    }

    return exitCode;
  }

  //
  // Create single directory.
  // TIP#1: Use FileCreateRecursiveDir() to create whole path recursively.
  //
  // path  - target path to create (IN).
  // quiet - do not write error message if set to 1 (IN).
  //
  // RETURNS: 0 if OK.
  //


  int FileCreateDir(const string &path, int quiet)
  {
    int exitCode = -1;

    #ifdef WIN32
    {
      FAIL(CreateDirectory(path.c_str(), NULL) == FALSE);
    }
    #else
    {
      FAIL(mkdir(path.c_str(), 0755));
    }
    #endif

    //
    // Error handler.
    //

    exitCode = 0;

    fail:

    if (exitCode && quiet == 0)
    {
      Error("ERROR: Cannot create [%s] dir."
                "Error code is : %d.\n", path.c_str(), GetLastError());
    }

    return exitCode;
  }

  //
  // Save buffer to file. If file already exists will be overwritten.
  //
  // fname - path to output file (IN).
  // buf   - buffer to dump (IN).
  // size  - size of buf[] buffer in bytes (IN).
  //
  // RETURNS: 0 if OK.
  //

  int FileSave(const char *fname, const char *buf, int size)
  {
    int exitCode = -1;

    FILE *f = fopen(fname, "wb+");

    FAIL(f == NULL);

    fwrite(buf, size, 1, f);

    exitCode = 0;

    fail:

    if (f)
    {
      fclose(f);
    }

    return exitCode;
  }

  //
  // Get current working directory.
  //
  // path     - buffer where to store retrieved directory (OUT).
  // pathSize - size of path[] buffer in bytes (IN).
  //
  // RETURNS: 0 if OK.
  //

  int FileGetDir(char *path, int pathSize)
  {
    int exitCode = -1;

    #ifdef WIN32
    FAIL(GetCurrentDirectory(pathSize - 1, path) == FALSE);
    #else
    FAIL(getcwd(path, pathSize - 1) == NULL);
    #endif

    //
    // Error handler.
    //

    fail:

    return exitCode;
  }

  //
  // Change current working directory.
  //
  // path - new working directory to set (e.g. 'c:/tmp') (IN).
  //
  // RETURN: 0 if OK.
  //

  int FileSetDir(const char *path)
  {
    int exitCode = -1;

    FAIL(path == NULL);

    if (path[0])
    {
      #ifdef WIN32
      FAIL(SetCurrentDirectory(path) == FALSE);
      #else
      FAIL(chdir(path));
      #endif
    }

    //
    // Error handler.
    //

    exitCode = 0;

    fail:

    if (exitCode)
    {
      Error("ERROR: Cannot set working directory to '%s'.\n", path);
    }

    return exitCode;
  }

  //
  // Get root filename from path, eg. it retrieve
  // 'file' from 'c:/tmp/file.dat'.
  //
  // path - path to split (IN).
  //
  // RETURNS: Root filename part or empty string if error.
  //

  string FileGetRootName(const string &path)
  {
    #ifdef WIN32

    char root[MAX_PATH + 1] = "";

    _splitpath(path.c_str(), NULL, NULL, root, NULL);

    return string(root);

    #else

    fprintf(stderr, "ERROR: Not implemented.\n");

    return "";

    #endif
  }

  //
  // Genearete temporary filename (WITHOUT creating the file).
  // Ouput path has format : '<BaseDir>/<prefix>XXX.YYY',
  // where XXX.YYY are generated unique name and extension.
  //
  // baseDir - directory, where to store file. System temp will be
  //           used if skipped (IN/OPT).
  //
  // prefix  - prefix to add before filename (IN/OPT).
  //
  // RETURNS: Abosolute path to temporary file or
  //          empty string if error.
  //
  //

  string FileCreateTempName(const char *baseDir, const char *prefix)
  {
    #ifdef WIN32

    int exitCode = -1;

    char base[MAX_PATH + 1] = {0};
    char path[MAX_PATH + 1] = {0};
    char fname[MAX_PATH + 1] = {0};

    if (baseDir == NULL)
    {
      GetTempPath(MAX_PATH, base);
    }
    else
    {
      strncpy(base, baseDir, MAX_PATH);
    }

    FAIL(GetTempFileName(base, prefix, 0, fname) == 0);

    //
    // Error handler.
    //

    exitCode = 0;

    fail:

    if (exitCode)
    {
      fprintf(stderr, "ERROR: Cannot crate temp name.\n"
                  "Error code is : %d.\n", GetLastError());
    }

    return string(fname);

    #else

    fprintf(stderr, "ERROR: Not implemented.\n");

    return "";

    #endif
  }

  int CanonizePath(string &path)
  {
    #ifdef WIN32

    char fullPath[MAX_PATH + 1];

    if (GetFullPathName(path.c_str(), MAX_PATH, fullPath, NULL) == FALSE)
    {
      return -1;
    }

    path = fullPath;

    return 0;

    #else

    return -1;

    #endif
  };

  //
  // Retrieve size of given file.
  //
  // fname - path to check (e.g. 'c:/tmp/test.dat') (IN).
  //
  // RETURNS: Size of file in bytes or -1 if error.
  //

  int FileSize(const char *fname)
  {
    int fsize = -1;

    FILE *f = NULL;

    //
    // Open file.
    //

    f = fopen(fname, "rb");

    FAIL(f == NULL);

    //
    // Get file size.
    //

    FAIL(fseek(f, 0, 2) < 0);

    fsize = ftell(f);

    fail:

    if (fsize < 0)
    {
      fprintf(stderr, "ERROR: Cannot get size of [%s] file.\n", fname);
    }

    if (f)
    {
      fclose(f);
    }

    return fsize;
  }

  //
  // Normalize slashes in path to '/' or '\\'.
  //
  // path  - path to normalize (IN/OUT).
  // slash - new slash character to use ('/' or '\\') (IN).
  //

  void FileNormalizeSlash(char *path, char slash)
  {
    //
    // Remove extra slash at the end.
    //

    int pathLen = strlen(path);

    while((pathLen > 0) && ((path[pathLen - 1] == '\\') || (path[pathLen - 1] == '/')))
    {
      path[pathLen - 1] = 0;

      pathLen --;
    }

    //
    // Normalize slashes.
    //

    for (int i = 0; path[i]; i++)
    {
      if (path[i] == '/' || path[i] == '\\')
      {
        path[i] = slash;
      }
    }
  }

  //
  // Normalize slashes in widechar path to L'/' or L'\\'.
  //
  // path  - path to normalize (IN/OUT).
  // slash - new slash character to use (L'/' or L'\\') (IN).
  //

  void FileNormalizeSlash(wchar_t *path, wchar_t slash)
  {
    DBG_ENTER3("FileNormalizeSlash");

    //
    // Remove extra slash at the end.
    //

    int pathLen = wcslen(path);

    while((pathLen > 1) && ((path[pathLen - 1] == L'\\') || (path[pathLen - 1] == L'/')))
    {
      path[pathLen - 1] = 0;

      pathLen --;
    }

    //
    // Normalize slashes.
    //

    for (int i = 0; path[i]; i++)
    {
      if (path[i] == L'/' || path[i] == L'\\')
      {
        path[i] = slash;
      }
    }

    DBG_LEAVE3("FileNormalizeSlash");
  }

  #ifdef WIN32

  //
  // Retrieve first free drive letter.
  //
  // RETURNS: First available letter (e.g. 'J'),
  //          or 0 if error.
  //

  const char FileGetFirstFreeLetter()
  {
    DWORD drives = GetLogicalDrives();

    for (int i = 3; i < 26; i++)
    {
      if ((drives & (1 << i)) == 0)
      {
        DEBUG1("First free drive letter is [%c].\n", 'A' + i);

        return 'A' + i;
      }
    }

    Error("ERROR: Can't find free drive letter.\n");

    return 0;
  }

  #endif /* WIN32 */

  //
  // Dump C++ string to binary file.
  // It writes to file xx xx xx xx ss ss ss ss ss ....
  // where:
  //
  // xx - little endian dword (4 bytes) length of string without 0
  // ss - variable string data
  //
  // f   - C stream, where to write data (IN).
  // str - C++ string to dump (IN).
  //
  // RETURNS: 0 if OK.
  //

  int FileWriteString(FILE *f, const string &str)
  {
    int exitCode = -1;

    int len = str.size();

    FAIL(fwrite(&len, sizeof(len), 1, f) != 1);

    if (len > 0)
    {
      FAIL(fwrite(&str[0], len, 1, f) != 1);
    }

    //
    // Error handler.
    //

    exitCode = 0;

    fail:

    if (exitCode)
    {
      Error("Cannot write '%s' string to file.\n", str.c_str());
    }

    return exitCode;
  }

  //
  // Load C++ string from binary file, stored by FileWriteString() before.
  //
  // f   - C stream, where to read data from (IN).
  // str - C++ string where to load data (IN).
  //
  // RETURNS: 0 if OK.
  //

  int FileReadString(FILE *f, string &str)
  {
    int exitCode = -1;

    int len = 0;;

    FAIL(fread(&len, sizeof(len), 1, f) != 1);

    if (len > 0)
    {
      str.resize(len);

      FAIL(fread(&str[0], len, 1, f) != 1);
    }
    else
    {
      str.clear();
    }

    //
    // Error handler.
    //

    exitCode = 0;

    fail:

    if (exitCode)
    {
      Error("Cannot read string from file.\n");
    }

    return exitCode;
  }

  //
  // Return system TEMP directory.
  //

  const string FileGetTempDir()
  {
    string tmp;

    //
    // Windows.
    //

    #ifdef WIN32
    {
      char path[MAX_PATH + 1] = {0};

      GetTempPath(MAX_PATH, path);

      tmp = path;
    }

    //
    // Linux.
    //

    #else
    {
      tmp = "/tmp";
    }
    #endif

    return tmp.c_str();
  }

  //
  //
  //

  int FileReadInt16(FILE *f, int16_t *rv)
  {
    int exitCode = -1;

    FAILEX(f  == NULL, "ERROR: Null file pointer at FileReadInt16().\n");
    FAILEX(rv == NULL, "ERROR: Null rv pointer at FileReadInt16().\n");

    FAIL(fread(rv, 2, 1, f) != 1);

    exitCode = 0;

    fail:

    if (exitCode)
    {
      Error("ERROR: Can't read 16-bit integer from file.");
    }

    return exitCode;
  }

  int FileReadInt32(FILE *f, int32_t *rv)
  {
    int exitCode = -1;

    FAILEX(f  == NULL, "ERROR: Null file pointer at FileReadInt32().\n");
    FAILEX(rv == NULL, "ERROR: Null rv pointer at FileReadInt32().\n");

    FAIL(fread(rv, 4, 1, f) != 1);

    exitCode = 0;

    fail:

    if (exitCode)
    {
      Error("ERROR: Can't read 32-bit integer from file.");
    }

    return exitCode;
  }

  int FileReadInt64(FILE *f, int64_t *rv)
  {
    int exitCode = -1;

    FAILEX(f  == NULL, "ERROR: Null file pointer at FileReadInt64().\n");
    FAILEX(rv == NULL, "ERROR: Null rv pointer at FileReadInt64().\n");

    FAIL(fread(rv, 8, 1, f) != 1);

    exitCode = 0;

    fail:

    if (exitCode)
    {
      Error("ERROR: Can't read 64-bit integer from file.");
    }

    return exitCode;
  }

  int FileReadFloat(FILE *f, float *rv)
  {
    int exitCode = -1;

    FAILEX(f  == NULL, "ERROR: Null file pointer at FileReadInt32().\n");
    FAILEX(rv == NULL, "ERROR: Null rv pointer at FileReadInt32().\n");

    FAIL(fread(rv, sizeof(float), 1, f) != 1);

    exitCode = 0;

    fail:

    if (exitCode)
    {
      Error("ERROR: Can't read single-precision float from file.");
    }

    return exitCode;
  }

  int FileSkipBytes(FILE *f, int numberOfBytesToSkip)
  {
    int exitCode = -1;

    FAILEX(f  == NULL, "ERROR: Null file pointer at FileSkipBytes().\n");

    FAIL(fseek(f, numberOfBytesToSkip, SEEK_CUR));

    exitCode = 0;

    fail:

    if (exitCode)
    {
      Error("ERROR: Can't skip '%d' bytes from file.", numberOfBytesToSkip);
    }

    return exitCode;
  }

  int FileReadDouble(FILE *f, double *rv)
  {
    int exitCode = -1;

    FAILEX(f  == NULL, "ERROR: Null file pointer at FileReadInt32().\n");
    FAILEX(rv == NULL, "ERROR: Null rv pointer at FileReadInt32().\n");

    FAIL(fread(rv, sizeof(double), 1, f) != 1);

    exitCode = 0;

    fail:

    if (exitCode)
    {
      Error("ERROR: Can't read double-precision float from file.");
    }

    return exitCode;
  }

  //
  // Read line from C stream WITHOUT end of line character.
  //
  // buf     - destination buffer, where to put readed data (OUT).
  // bufSize - buf[] size in bytes (IN).
  // f       - C stream retrievead from fopen before (IN).
  // readed  - number of bytes readed (without end of line) (OUT/OPT).
  // trim    - remove white space from left and right side if set to 1 (IN/OPT).
  //
  // RETURNS: pointer to first byte of line
  //          or NULL if error.
  //

  char *FileGetLine(char *buf, int bufSize, FILE *f, int *readed, int trim)
  {
    char *ret = NULL;

    int len = 0;

    //
    // Read line.
    //

    FAIL(fgets(buf, bufSize - 1, f) == NULL);

    //
    // Remove end of line character.
    //

    len = strlen(buf);

    while(len > 0 && ((buf[len - 1] == 10) || (buf[len - 1] == 13)))
    {
      buf[len - 1] = 0;
      len --;
    }

    //
    // Trim buffer if needed.
    //

    if (trim)
    {
      //
      // Left side.
      //

      while(isspace(*buf) && (*buf))
      {
        buf ++;
        len --;
      }

      //
      // Right side.
      //

      while(len > 0 && isspace(buf[len - 1]))
      {
        buf[len - 1] = 0;
        len --;
      }
    }

    //
    // Put line length to caller if needed.
    //

    if (readed)
    {
      *readed = len;
    }

    ret = buf;

    //
    // Error handler.
    //

    fail:

    return ret;
  }

  //
  // Get root directory from path.
  //
  // Example input  : "Lib/LibFile"
  // Example output : "Lib"
  //

  string FilePathGetTopDir(const string &path)
  {
    string rv;

    const int SPLIT_IDX_INVALID = 1000000;

    int firstSlashIdx     = path.find('/');
    int firstBackSlashIdx = path.find('\\');
    int splitIdx          = SPLIT_IDX_INVALID;

    if (firstSlashIdx != string::npos && firstSlashIdx < splitIdx)
    {
      splitIdx = firstSlashIdx;
    }

    if (firstBackSlashIdx != string::npos && firstBackSlashIdx < splitIdx)
    {
      splitIdx = firstBackSlashIdx;
    }

    if (splitIdx != SPLIT_IDX_INVALID)
    {
      rv = path.substr(0, splitIdx);
    }
    else
    {
      rv = path;
    }

    return rv;
  }

  //
  // Get directory name at given deep level inside path.
  //
  // Example path    : "Source/Lib/LibFile"
  // Example deep    : 1
  // Example returns : Lib
  //

  string FilePathGetDirAtDeep(const string &path, int deep)
  {
    string rv;

    int beginIdx = -1;
    int endIdx   = -1;
    int slashCnt = 0;

    for (int i = 0; i < path.size(); i++)
    {
      if (path[i] == '\\' || path[i] == '/')
      {
        slashCnt++;

        if (slashCnt == deep)
        {
          beginIdx = i + 1;
        }
        else if (slashCnt == deep + 1)
        {
          endIdx = i;

          break;
        }
      }
    }

    if (beginIdx != -1 && endIdx != -1)
    {
      rv = path.substr(beginIdx, endIdx - beginIdx);
    }

    return rv;
  }

  //
  // Returns UNIX timestamp of last file modification or -1 if error.
  //

  int FileGetLastModificationTimestamp(const char *path)
  {
    int rv = -1;

    struct stat attr;

    if (path && (stat(path, &attr) == 0))
    {
      rv = attr.st_mtime;
    }

    return rv;
  }

} /* namespace Tegenaria */
