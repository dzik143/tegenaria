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

#ifndef Tegenaria_Core_File_H
#define Tegenaria_Core_File_H

#include <string>
#include <vector>

namespace Tegenaria
{
  using std::vector;
  using std::string;

  //
  // Defines.
  //

  #define FILE_SKIP_HIDDEN (1 << 0) // Skip hidden files in ListFiles().

  //
  // General file manegement.
  //

  char *FileLoad(const char *fname, int *readed = NULL);

  int FileSize(const char *fname);

  int FileSave(const char *fname, const char *buf, int size);

  int ListFiles(string path, string mask, vector<string>& files,
                    bool recursive, unsigned int flags = 0);

  int ListDirs(string path, string mask, vector<string>& dirs,
                   bool recursive, unsigned int flags = 0);

  int FileCompare(int &stat, const string &fname1, const string &fname2);

  int FileExists(const string &fname);

  int FileWait(const char *fname, int timeout = -1);

  int FileDelete(const char *fname);

  //
  // Paths manegement.
  //

  int FileGetDir(char *path, int pathSize);

  int FileSetDir(const char *path);

  int FileCreateRecursiveDir(const string &path);

  int FileCreateDir(const string &path, int quiet = 0);

  string FileCreateTempName(const char *baseDir = NULL, const char *prefix = NULL);

  string FileGetRootName(const string &path);

  const string FileGetTempDir();

  int CanonizePath(string &path);

  string FilePathGetTopDir(const string &path);
  string FilePathGetDirAtDeep(const string &path, int deep);

  void FileNormalizeSlash(char *path, char slash = '/');
  void FileNormalizeSlash(wchar_t *path, wchar_t slash = L'/');

  //
  // Time functions.
  //

  int FileGetLastModificationTimestamp(const char *path);

  //
  // Load/store binary data.
  //

  int FileWriteString(FILE *f, const string &str);
  int FileSkipBytes(FILE *f, int numberOfBytesToSkip);
  int FileReadString(FILE *f, string &str);
  int FileReadInt16(FILE *f, int16_t *rv);
  int FileReadInt32(FILE *f, int32_t *rv);
  int FileReadInt64(FILE *f, int64_t *rv);
  int FileReadFloat(FILE *f, float *rv);
  int FileReadDouble(FILE *f, double *rv);

  //
  // High level IO for text files.
  //

  char *FileGetLine(char *buf, int bufSize, FILE *f, int *readed, int trim);

  //
  // Transacted I/O.
  //

  struct TFILE;

  //
  // Transacted versions of stdio functions.
  //
  // FILE*   -> TFILE *
  // fopen   -> tfopen
  // fclose  -> tfclose
  // fprintf -> tfprintf
  // fscanf  -> tfscanf
  // fwrite  -> tfwrite
  // fread   -> tfread
  //

  TFILE *tfopen(const char *fname, const char *mode);

  void tfclose(TFILE *f);

  int FileRecoverFailedIO(const char *directory);

  #define tfprintf(F, ...) fprintf(F -> ftemp_, __VA_ARGS__)
  #define tfscanf(F, ...) fscanf(F -> ftemp_, __VA_ARGS__)
  #define tfwrite(BUF, SIZE, COUNT, F) fwrite(BUF, SIZE, COUNT, F -> ftemp_)
  #define tfread(BUF, SIZE, COUNT, F) fread(BUF, SIZE, COUNT, F -> ftemp_)

  //
  // Windows only.
  //

  const char FileGetFirstFreeLetter();

} /* namespace Tegenaria */

#endif /* Tegenaria_Core_File_H */
