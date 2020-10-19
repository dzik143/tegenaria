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

#include <Tegenaria/Debug.h>
#include <Tegenaria/Sftp.h>
#include <Tegenaria/SftpClient.h>
#include <Tegenaria/Net.h>
#include <Tegenaria/Thread.h>
#include <Tegenaria/SftpJob.h>
#include <Tegenaria/Str.h>

#include <cstdlib>
#include <ctime>
#include <sys/time.h>
#include <sys/stat.h>

using namespace Tegenaria;

//
// Callback function called when upload/download/list job:
// - changed state
// - new transfer statistics arrived (upload/download jobs)
// - new portion of files list arrived (list job).
//

void JobNotifyCallback(int type, SftpJob *job)
{
  DBG_ENTER3("JobNotifyCallback");

  switch(type)
  {
    //
    // State changed.
    //

    case SFTP_JOB_NOTIFY_STATE_CHANGED:
    {
      switch(job -> getState())
      {
        case SFTP_JOB_STATE_ERROR:        printf("\nSFTP job failed.\n"); break;
        case SFTP_JOB_STATE_INITIALIZING: printf("\nSFTP job initializing.\n"); break;
        case SFTP_JOB_STATE_PENDING:      printf("\nSFTP job started.\n"); break;
        case SFTP_JOB_STATE_FINISHED:     printf("\nSFTP job finished.\n"); break;
        case SFTP_JOB_STATE_STOPPED:      printf("\nSFTP job stopped.\n"); break;
      }

      break;
    }

    //
    // Transfer statistics.
    //

    case SFTP_JOB_NOTIFY_TRANSFER_STATISTICS:
    {
      double rateMBs   = job -> getAvgRate() / 1024.0 / 1024.0;
      double processed = double(job -> getProcessedBytes());
      double total     = double(job -> getTotalBytes());

      printf("\rProcessed %.0lf / %.0lf, rate is %8.3lf MB/s.", processed, total, rateMBs);

      break;
    }

    //
    // Next part of files list arrived while performing SFTP_JOB_TYPE_LIST
    // job.
    //

    case SFTP_JOB_NOTIFY_FILES_LIST_ARRIVED:
    {
      vector<SftpFileInfo> &files = job -> getFiles();

      for (int i = 0; i < files.size(); i++)
      {
        //
        // Directory.
        //

        if (SFTP_ISDIR(files[i].attr_.perm_))
        {
          printf("[%s]\n", files[i].name_.c_str());
        }

        //
        // File.
        //

        else
        {
          printf("%s %"PRId64"\n", files[i].name_.c_str(), files[i].attr_.size_);
        }
      }

      break;
    }
  }

  DBG_LEAVE3("JobNotifyCallback");
}

//
// Get filename part from full path.
//
// Example:
//   path    = c:/tmp/file.dat
//   returns = file.dat
//
// path - full, absolute path to split (IN).
//
// RETURNS: File part of given input path.
//

const char *GetFileName(const char *path)
{
  int lastSlashPos = -1;

  const char *fname = NULL;

  if (path)
  {
    for (int i = 0; path[i]; i++)
    {
      if (path[i] == '/' || path[i] == '\\')
      {
        lastSlashPos = i;
      }
    }

    if (lastSlashPos == -1)
    {
      fname = path;
    }
    else
    {
      fname = path + lastSlashPos + 1;
    }
  }

  return fname;
}

//
// On success pwd is changed to 'pwd/subDir'.
//
// sftp    - pointer to connected sftp client object (IN).
// pwd     - buffer, where current remote directory is stored (IN/OUT).
// pwdSize - size of pwd[] buffer in bytes (IN).
// subDir  - subdirectory, where to enter (IN).
//
// RETURNS: 0 if OK.
//

int EnterDirectory(SftpClient *sftp, char *pwd, int pwdSize, const char *subDir)
{
  DBG_ENTER("EnterDirectory");

  int exitCode = -1;

  char newPwd[512];

  SftpFileAttr attr;

  //
  // Create newPwd = pwd/subDir.
  //

  strncpy(newPwd, pwd, sizeof(newPwd) - 1);

  if (pwd[0] && pwd[1])
  {
    strncat(newPwd, "/", sizeof(newPwd) - 1);
  }

  strncat(newPwd, subDir, sizeof(newPwd) - 1);

  //
  // Check is newPwd exists on server.
  //

  if (sftp -> stat(newPwd, &attr) != 0)
  {
    Error("ERROR: Directory '%s' does not exist on server.\n", newPwd);

    goto fail;
  }

  //
  // Save new pwd.
  //

  strncpy(pwd, newPwd, pwdSize);

  DEBUG1("Entered to '%s'.\n", pwd);

  //
  // Error handler.
  //

  exitCode = 0;

  fail:

  if (exitCode)
  {
    Error("ERROR: Cannot enter '%s' subdirectory.\n", subDir);
  }

  DBG_LEAVE("EnterDirectory");

  return exitCode;
}

//
// Leave current directory (i.e. go one directory up).
// On success part after last slash (/) removed from pwd[] buffer.
//
// Example:
//  input pwd[]  = /home/john
//  output pwd[] = /home
//
// sftp - pointer to connected sftp client object (IN).
// pwd  - buffer, where current remote directory is stored (IN/OUT).
//
// RETURNS: 0 if OK.
//

int LeaveDirectory(SftpClient *sftp, char *pwd)
{
  DBG_ENTER("LeaveDirectory");

  int exitCode = -1;

  //
  // Root directory. No leave possible.
  //

  if (pwd[0] == '/' && pwd[1] == 0)
  {
    Error("ERROR: Cannot leave root directory.\n");

    goto fail;
  }

  //
  // Non-root directory. Remove part after last slash.
  //

  else
  {
    int lastSlashPos = 0;

    for (int i = 0; pwd[i]; i++)
    {
      if (pwd[i] == '/' || pwd[i] == '\\')
      {
        lastSlashPos = i;
      }
    }

    if (lastSlashPos == 0)
    {
      lastSlashPos = 1;
    }

    pwd[lastSlashPos] = 0;

    DEBUG1("Leaved to '%s'.\n", pwd);
  }

  //
  // Error handler.
  //

  exitCode = 0;

  fail:

  DBG_LEAVE("LeaveDirectory");

  return exitCode;
}

//
// Entry point.
//

int main(int argc, char **argv)
{
  NetConnection *nc = NULL;

  SftpClient *sftp = NULL;

  const char *ip = NULL;

  int port = -1;

  int sock = -1;
  int fd   = -1;

  const char *localPath  = NULL;
  const char *remotePath = NULL;

  char cmd[128] = {0};
  char pwd[512] = "/";

  char *ret = NULL;

  vector<char *> tokens;

  DBG_INIT_EX("libsftp-example-client.log", "debug1", DBG_STATE_ENABLE);

  DBG_HEAD("LibSFTP console client example");

  //
  // Parse command line parameters.
  //

  if (argc < 3)
  {
    fprintf(stderr, "Usage is: %s <ip> <port>\n", argv[0]);

    exit(-1);
  }

  ip = argv[1];

  port = atoi(argv[2]);

  //
  // Connect to server.
  //

  nc = NetConnect(ip, port);

  //
  // Get raw socket from net connection.
  //

  sock = nc -> getSocket();

  //
  // Init SFTP client over raw socket with 30s timeout.
  //

  sftp = new SftpClient(sock, sock, 30, SFTP_CLIENT_SOCKET);

  sftp -> connect();

  //
  // Fall into client loop.
  //

  while(1)
  {
    //
    // Read command from stdin.
    //

    printf("%s> ", pwd);

    ret = fgets(cmd, sizeof(cmd), stdin);

    //
    // Remove end of line character.
    //

    StrRemoveChar(cmd, 10);
    StrRemoveChar(cmd, 13);

    //
    // Tokenize command.
    //

    StrTokenize(tokens, cmd, " \t", "\"");

    //
    // Dispatch command.
    //

    if (tokens.size() > 0)
    {
      //
      // ls
      //

      if (strcmp(tokens[0], "ls") == 0)
      {
        SftpJob *job = NULL;

        //
        // Start listing content of remote directory in background
        // thread.
        //

        job = sftp -> listFiles(pwd, JobNotifyCallback);

        //
        // Wait until list job finished.
        // TIP#1: Use job -> stop() method to stop listing before finished.
        //

        job -> wait();

        job -> release();
      }

      //
      // cd <remotedir>
      //

      else if (strcmp(tokens[0], "cd") == 0)
      {
        //
        // argument missing.
        //

        if (tokens.size() < 2)
        {
          Error("ERROR: argument missing.\n");
        }

        //
        // Change current directory.
        //

        else
        {
          remotePath = tokens[1];

          //
          // Go one directory up.
          //

          if (strcmp(remotePath, "..") == 0)
          {
            LeaveDirectory(sftp, pwd);
          }

          //
          // Go to deeper subdirectory.
          //

          else
          {
            EnterDirectory(sftp, pwd, sizeof(pwd), remotePath);
          }
        }
      }

      //
      // download <remotefile>
      //

      else if (strcmp(tokens[0], "download") == 0)
      {
        //
        // argument missing.
        //

        if (tokens.size() < 2)
        {
          Error("ERROR: argument missing.\n");
        }

        //
        // Download file.
        //

        else
        {
          string remote;
          string local;

          SftpJob *job = NULL;

          //
          // Remote source path = pwd/file.
          // Download file from current working directory on server.
          //

          remote = string(pwd);

          if (remote[remote.size() - 1] != '/')
          {
            remote += '/';
          }

          remote += tokens[1];

          //
          // Local destination path = file.
          // Download to current directory.
          //

          local = tokens[1];

          //
          // Download from <remote> to <local>.
          //

          job = sftp -> downloadFile(local.c_str(), remote.c_str(), JobNotifyCallback);

          //
          // Wait until download job finished.
          // TIP#1: Use job -> stop() method to stop downlading before finished.
          //

          job -> wait();

          job -> release();
        }
      }

      //
      // upload <localfile>
      //

      else if (strcmp(tokens[0], "upload") == 0)
      {
        //
        // argument missing.
        //

        if (tokens.size() < 2)
        {
          Error("ERROR: argument missing.\n");
        }

        //
        // Download file.
        //

        else
        {
          string remote;
          string local;

          SftpJob *job = NULL;

          //
          // Remote source path = pwd/file.
          // Download file from current working directory on server.
          //

          remote = string(pwd);

          if (remote[remote.size() - 1] != '/')
          {
            remote += '/';
          }

          remote += GetFileName(tokens[1]);

          //
          // Local destination path = file.
          // Download to current directory.
          //

          local = tokens[1];

          //
          // Download from <remote> to <local>.
          //

          job = sftp -> uploadFile(remote.c_str(), local.c_str(), JobNotifyCallback);

          //
          // Wait until download job finished.
          // TIP#1: Use job -> stop() method to stop downlading before finished.
          //

          job -> wait();

          job -> release();
        }
      }

      //
      // Unknown command.
      //

      else
      {
        Error("ERROR: Unknown command '%s'.\n", tokens[0]);
      }
    }
  }

  return 0;
}
