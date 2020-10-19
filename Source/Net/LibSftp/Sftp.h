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

#ifndef Tegenaria_Core_Sftp_H
#define Tegenaria_Core_Sftp_H

#ifdef WIN32
# include <windows.h>
#endif

#include <stdint.h>
#include <string>

#include <Tegenaria/Net.h>

namespace Tegenaria
{
  using std::string;

  struct Statvfs_t
  {
    uint64_t bsize_;
    uint64_t frsize_;
    uint64_t blocks_;
    uint64_t bfree_;
    uint64_t bavail_;
    uint64_t files_;
    uint64_t ffree_;
    uint64_t favail_;
    uint64_t fsid_;
    uint64_t flags_;
    uint64_t namemax_;
  };

  struct SftpFileAttr
  {
    int64_t size_;

    uint32_t flags_;
    uint32_t uid_;
    uint32_t guid_;
    uint32_t perm_;
    uint32_t atime_;
    uint32_t mtime_;
  };

  struct SftpFileInfo
  {
    string name_;

    SftpFileAttr attr_;
  };

  //
  // Our Sftp version.
  //

  #define SSH2_FILEXFER_VERSION 3

  //
  // Default sector size.
  //

  #define SFTP_DEFAULT_SECTOR_SIZE (1024*32)

  //
  // Timeouts to trigger partial read/write in seconds.
  //

  #define SFTP_PARTIAL_READ_TIMEOUT  10
  #define SFTP_PARTIAL_WRITE_TIMEOUT 10

  //
  // Check network statistics every N packets.
  //

  #define SFTP_NETSTAT_DEFAULT_TICK 128

  //
  // Client to server messages.
  //

  #define SSH2_FXP_INIT               1
  #define SSH2_FXP_OPEN               3
  #define SSH2_FXP_CLOSE              4
  #define SSH2_FXP_READ               5
  #define SSH2_FXP_WRITE              6
  #define SSH2_FXP_LSTAT              7
  #define SSH2_FXP_STAT_VERSION_0     7
  #define SSH2_FXP_FSTAT              8
  #define SSH2_FXP_SETSTAT            9
  #define SSH2_FXP_FSETSTAT           10
  #define SSH2_FXP_OPENDIR            11
  #define SSH2_FXP_READDIR            12
  #define SSH2_FXP_REMOVE             13
  #define SSH2_FXP_MKDIR              14
  #define SSH2_FXP_RMDIR              15
  #define SSH2_FXP_REALPATH           16
  #define SSH2_FXP_STAT               17
  #define SSH2_FXP_RENAME             18
  #define SSH2_FXP_READLINK           19
  #define SSH2_FXP_SYMLINK            20
  #define SSH2_FXP_EXTENDED           200

  //
  // Server to client messages.
  //

  #define SSH2_FXP_VERSION 2
  #define SSH2_FXP_STATUS  101
  #define SSH2_FXP_HANDLE  102
  #define SSH2_FXP_DATA    103
  #define SSH2_FXP_NAME    104
  #define SSH2_FXP_ATTRS   105

  #define SSH2_FXP_EXTENDED_REPLY 201

  //
  // Dirligo packets.
  //

  #define SSH2_FXP_DIRLIGO_CREATEFILE    220
  #define SSH2_FXP_DIRLIGO_MULTICLOSE    221
  #define SSH2_FXP_DIRLIGO_RESETDIR      222
  #define SSH2_FXP_DIRLIGO_APPEND        223
  #define SSH2_FXP_DIRLIGO_READDIR_SHORT 224

  #define SSH2_FXP_DIRLIGO_DIR_FLAG   (1 << 30)

  //
  // Attributes.
  //

  #define SSH2_FILEXFER_ATTR_SIZE        0x00000001
  #define SSH2_FILEXFER_ATTR_UIDGID      0x00000002
  #define SSH2_FILEXFER_ATTR_PERMISSIONS 0x00000004
  #define SSH2_FILEXFER_ATTR_ACMODTIME   0x00000008
  #define SSH2_FILEXFER_ATTR_EXTENDED    0x80000000

  //
  // Portable open modes.
  //

  #define SSH2_FXF_READ   0x00000001
  #define SSH2_FXF_WRITE  0x00000002
  #define SSH2_FXF_APPEND 0x00000004
  #define SSH2_FXF_CREAT  0x00000008
  #define SSH2_FXF_TRUNC  0x00000010
  #define SSH2_FXF_EXCL   0x00000020

  //
  // statvfs@openssh.com f_flag flags.
  //

  #define SSH2_FXE_STATVFS_ST_RDONLY 0x00000001
  #define SSH2_FXE_STATVFS_ST_NOSUID 0x00000002

  //
  // Status codes.
  //

  #define SSH2_FX_OK                0
  #define SSH2_FX_EOF               1
  #define SSH2_FX_NO_SUCH_FILE      2
  #define SSH2_FX_PERMISSION_DENIED 3
  #define SSH2_FX_FAILURE           4
  #define SSH2_FX_BAD_MESSAGE       5
  #define SSH2_FX_NO_CONNECTION     6
  #define SSH2_FX_CONNECTION_LOST   7
  #define SSH2_FX_OP_UNSUPPORTED    8
  #define SSH2_FX_MAX               8

} /* namespace Tegenaria */

#endif /* Tegenaria_Core_Sftp_H */
