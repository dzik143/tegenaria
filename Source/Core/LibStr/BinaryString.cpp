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
// Purpose: Edit raw byte buffers stored in string containter.
//

#include "Str.h"

namespace Tegenaria
{
  //
  // ----------------------------------------------------------------------------
  //
  //                   Pop data from std::string buffer
  //
  // ----------------------------------------------------------------------------
  //

  //
  // Pop <rawSize> bytes from begin of string.
  //
  // Buffer before: xx xx xx xx xx xx ... yy yy yy yy
  // Buffer after : ... yy yy yy yy
  //
  // raw     - buffer, where to write popped data (OUT).
  // rawSize - how much bytes to pop (IN).
  // string  - buffer from pop data (IN/OUT).
  //
  // RETURNS: 0 if all bytes popped,
  //          -1 if error.
  //

  int StrPopRaw(void *raw, int rawSize, string &buf)
  {
    DBG_ENTER3("StrPopRaw");

    int exitCode = -1;

    int bufSize = buf.size();

    //
    // Check args.
    //

    FAILEX(raw == NULL, "ERROR: Null pointer passed to StrPopRaw.");
    FAILEX(rawSize > bufSize, "ERROR: Buffer too small in StrPopRaw.");

    //
    // Pop raw bytes from buffer.
    //

    memcpy(raw, &buf[0], rawSize);

    DEBUG3("StrPopRaw : Popped [%d] bytes from buffer.\n", rawSize);

    //
    // FIXME: Don't copy.
    //

    memcpy(&buf[0], &buf[rawSize], bufSize - rawSize);

    buf.resize(bufSize - rawSize);

    //
    // Error handler.
    //

    exitCode = 0;

    fail:

    DBG_LEAVE3("StrPopRaw");

    return exitCode;
  }

  //
  // Pop QWORD from begin of string.
  //
  // Buffer before: 01 02 03 04 05 06 07 08 xx xx xx xx
  // Buffer after : xx xx xx xx
  //
  // value   - buffer, where to write popped QWORD (OUT).
  // string  - buffer from pop data (IN/OUT).
  // flags   - set to STR_BIG_ENDIAN or STR_LITTLE_ENDIAN (IN).
  //
  // RETURNS: 0 if all 8 bytes popped,
  //          -1 if error.
  //

  int StrPopQword(uint64_t *value, string &buf, int flags)
  {
    DBG_ENTER3("StrPopQword");

    int exitCode = -1;

    int bufSize = buf.size();

    //
    // Check args.
    //

    FAILEX(value == NULL, "ERROR: Null pointer passed to StrPopQword.");
    FAILEX(bufSize < 8,   "ERROR: Buffer too small in StrPopQword.");

    //
    // Pop big endian qword (motorola 68k like).
    //

    if (flags & STR_BIG_ENDIAN)
    {
      uint8_t *dst = (uint8_t *) value;
      uint8_t *src = (uint8_t *) &buf[0];

      dst[0] = src[7];
      dst[1] = src[6];
      dst[2] = src[5];
      dst[3] = src[4];
      dst[4] = src[3];
      dst[5] = src[2];
      dst[6] = src[1];
      dst[7] = src[0];

      DEBUG3("StrPopQword : Popped big endian [0x%"PRIx64"].\n", *value);
    }

    //
    // Pop little endian dword (intel 86 like).
    //

    else
    {
      *value = *(uint64_t *)(&buf[0]);

      DEBUG3("StrPopQword : Popped little endian [0x%"PRIx64"].\n", *value);
    }

    //
    // FIXME: Don't copy.
    //

    bufSize -= 8;

    if (bufSize > 0)
    {
      memcpy(&buf[0], &buf[8], bufSize);
    }

    buf.resize(bufSize);

    //
    // Error handler.
    //

    exitCode = 0;

    fail:

    DBG_LEAVE3("StrPopQword");

    return exitCode;
  }

  //
  // Pop DWORD from begin of string.
  //
  // Buffer before: 01 02 03 04 xx xx xx xx
  // Buffer after : xx xx xx xx
  //
  // value   - buffer, where to write popped DWORD (OUT).
  // string  - buffer from pop data (IN/OUT).
  // flags   - set to STR_BIG_ENDIAN or STR_LITTLE_ENDIAN (IN).
  //
  // RETURNS: 0 if all 4 bytes popped,
  //          -1 if error.
  //

  int StrPopDword(uint32_t *value, string &buf, int flags)
  {
    DBG_ENTER3("StrPopDword");

    int exitCode = -1;

    int bufSize = buf.size();

    //
    // Check args.
    //

    FAILEX(value == NULL, "ERROR: Null pointer passed to StrPopDword.");
    FAILEX(bufSize < 4,   "ERROR: Buffer too small in StrPopDword.");

    //
    // Pop big endian dword (motorola 68k like).
    //

    if (flags & STR_BIG_ENDIAN)
    {
      uint8_t *dst = (uint8_t *) value;
      uint8_t *src = (uint8_t *) &buf[0];

      dst[0] = src[3];
      dst[1] = src[2];
      dst[2] = src[1];
      dst[3] = src[0];

      DEBUG3("StrPopDword : Popped big endian [0x%x].\n", *value);
    }

    //
    // Pop little endian dword (intel 86 like).
    //

    else
    {
      *value = *(uint32_t *)(&buf[0]);

      DEBUG3("StrPopDword : Popped little endian [0x%x].\n", *value);
    }

    //
    // FIXME: Don't copy.
    //

    bufSize -= 4;

    if (bufSize > 0)
    {
      memcpy(&buf[0], &buf[4], bufSize);
    }

    buf.resize(bufSize);

    //
    // Error handler.
    //

    exitCode = 0;

    fail:

    DBG_LEAVE3("StrPopDword");

    return exitCode;
  }

  //
  // Pop one byte from begin of string.
  //
  // Buffer before: 01 xx xx xx xx
  // Buffer after : xx xx xx xx
  //
  // value   - buffer, where to write popped byte (OUT).
  // string  - buffer from pop data (IN/OUT).
  //
  // RETURNS: 0 if byte popped,
  //          -1 if error.
  //

  int StrPopByte(uint8_t *value, string &buf)
  {
    DBG_ENTER3("StrPopByte");

    int exitCode = -1;

    int bufSize = buf.size();

    //
    // Check args.
    //

    FAILEX(value == NULL, "ERROR: Null pointer passed to StrPopDword.");
    FAILEX(bufSize < 1,   "ERROR: Buffer too small in StrPopDword.");

    //
    // Pop byte from buffer.
    //

    *value = buf[0];

    DEBUG3("StrPopByte : Popped byte [0x%x].\n", *value);

    //
    // FIXME: Don't copy.
    //

    bufSize --;

    if (bufSize > 0)
    {
      memcpy(&buf[0], &buf[1], bufSize);
    }

    buf.resize(bufSize);

    //
    // Error handler.
    //

    exitCode = 0;

    fail:

    DBG_LEAVE3("StrPopByte");

    return exitCode;
  }

  //
  // ----------------------------------------------------------------------------
  //
  //                      Push data to std::string buffer
  //
  // ----------------------------------------------------------------------------
  //

  //
  // Append <rawSize> buffer to the end of string.
  //
  // Buffer before: xx xx xx xx
  // Buffer after : xx xx xx xx yy yy yy yy yy ...
  //
  // buf     - buffer, where to append data (IN/OUT).
  // rawSize - how much bytes to append (IN).
  // raw     - source buffer with data to append (IN).
  //
  // RETURNS: 0 if all data appended,
  //          -1 if error.
  //

  int StrPushRaw(string &buf, const void *raw, int rawSize)
  {
    DBG_ENTER3("StrPushRaw");

    int exitCode = -1;

    int bufSize = buf.size();

    if (rawSize > 0)
    {
      //
      // Check args.
      //

      FAILEX(raw == NULL, "ERROR: Null pointer passed to StrPushRaw.");

      //
      // Pop raw bytes from buffer.
      //

      buf.resize(bufSize + rawSize);

      memcpy(&buf[bufSize], raw, rawSize);

      DEBUG3("StrPushRaw : Pushed [%d] bytes from buffer.\n", rawSize);
    }

    //
    // Error handler.
    //

    exitCode = 0;

    fail:

    DBG_LEAVE3("StrPushRaw");

    return exitCode;
  }

  //
  // Append QWORD value to the end of string.
  //
  // Buffer before: xx xx xx xx
  // Buffer after : xx xx xx xx 01 02 03 04 05 06 07 08
  //
  // buf   - buffer, where to append data (IN/OUT).
  // value - QWORD value to append (IN).
  // flags - set to STR_BIG_ENDIAN or STR_LITTLE_ENDIAN (IN).
  //
  // RETURNS: 0 if all data appended,
  //          -1 if error.
  //

  int StrPushQword(string &buf, uint64_t value, int flags)
  {
    DBG_ENTER3("StrPushQword");

    int bufSize = buf.size();

    //
    // Push big endian qword (motorola 68k like).
    //

    buf.resize(bufSize + 8);

    if (flags & STR_BIG_ENDIAN)
    {
      uint8_t *dst = (uint8_t *) &buf[bufSize];
      uint8_t *src = (uint8_t *) &value;

      dst[0] = src[7];
      dst[1] = src[6];
      dst[2] = src[5];
      dst[3] = src[4];
      dst[4] = src[3];
      dst[5] = src[2];
      dst[6] = src[1];
      dst[7] = src[0];

      DEBUG3("StrPushQword : Pushed big endian [0x%"PRIx64"].\n", value);
    }

    //
    // Pop little endian qword (intel 86 like).
    //

    else
    {
      *(uint64_t *)(&buf[bufSize]) = value;

      DEBUG3("StrPushQword : Pushed little endian [0x%"PRIx64"].\n", value);
    }

    DBG_LEAVE3("StrPushQword");

    return 0;
  }

  //
  // Append DWORD value to the end of string.
  //
  // Buffer before: xx xx xx xx
  // Buffer after : xx xx xx xx 01 02 03 04
  //
  // buf   - buffer, where to append data (IN/OUT).
  // value - DWORD value to append (IN).
  // flags - set to STR_BIG_ENDIAN or STR_LITTLE_ENDIAN (IN).
  //
  // RETURNS: 0 if all data appended,
  //          -1 if error.
  //

  int StrPushDword(string &buf, uint32_t value, int flags)
  {
    DBG_ENTER3("StrPushDword");

    int bufSize = buf.size();

    //
    // Push big endian dword (motorola 68k like).
    //

    buf.resize(bufSize + 4);

    if (flags & STR_BIG_ENDIAN)
    {
      uint8_t *dst = (uint8_t *) &buf[bufSize];
      uint8_t *src = (uint8_t *) &value;

      dst[0] = src[3];
      dst[1] = src[2];
      dst[2] = src[1];
      dst[3] = src[0];

      DEBUG3("StrPushDword : Pushed big endian [0x%x].\n", value);
    }

    //
    // Pop little endian dword (intel 86 like).
    //

    else
    {
      *(uint32_t *)(&buf[bufSize]) = value;

      DEBUG3("StrPushDword : Pushed little endian [0x%x].\n", value);
    }

    DBG_LEAVE3("StrPushDword");

    return 0;
  }

  //
  // Append one byte to the end of string.
  //
  // Buffer before: xx xx xx xx
  // Buffer after : xx xx xx xx 01
  //
  // buf   - buffer, where to append data (IN/OUT).
  // value - byte to append (IN).
  //
  // RETURNS: 0 if byte appended,
  //          -1 if error.
  //

  int StrPushByte(string &buf, uint8_t value)
  {
    DBG_ENTER3("StrPushByte");

    buf.push_back(value);

    DEBUG3("StrPushByte : Pushed byte [0x%x].\n", value);

    DBG_LEAVE3("StrPushByte");

    return 0;
  }

  //
  // Append string to the end of string buffer.
  //
  // Buffer before: xx xx xx xx
  // Buffer after : xx xx xx xx ll ll ll ll ss ss ss ss ss ... 00
  //
  // WHERE:
  //
  // ll - little endian size of string including zero terminator
  // ss - string data
  // 00 - zero terminator
  //
  // buf - buffer, where to append data (IN/OUT).
  // str - C-style string to append (IN).
  //
  // RETURNS: 0 if byte appended,
  //          -1 if error.
  //

  int StrPushString(string &buf, const char *str)
  {
    DBG_ENTER3("StrPushString");

    int len = strlen(str) + 1;

    StrPushDword(buf, len);
    StrPushRaw(buf, str, len);

    DBG_LEAVE3("StrPushString");

    return 0;
  }
} /* namespace Tegenaria */
