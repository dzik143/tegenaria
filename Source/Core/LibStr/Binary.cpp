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
// Purpose: Edit raw byte buffers stored in pure C-style buffer i.e.
// {buf, len} pair.
//

#include "Str.h"

namespace Tegenaria
{
  //
  // ----------------------------------------------------------------------------
  //
  //                  Pop data from C-style {buf, len} buffer
  //               All below functions do NOT destroy input buffer.
  //
  // ----------------------------------------------------------------------------
  //

  //
  // Pop <rawSize> bytes from begin of buffer.
  //
  // raw       - buffer, where to write popped data (OUT).
  // rawSize   - how much bytes to pop (IN).
  // it        - pointer inside raw buffer, where to read data from (IN/OUT).
  // bytesLeft - number of bytes left in buffer (IN/OUT).
  //
  // RETURNS: 0 if all bytes popped,
  //          -1 if error.
  //

  int StrPopRaw(void *raw, int rawSize, char **it, int *bytesLeft)
  {
    DBG_ENTER3("StrPopRaw");

    int exitCode = -1;

    //
    // Check args.
    //

    FAILEX(it == NULL, "ERROR: 'it' cannot be NULL in StrPopRaw.");
    FAILEX(*it == NULL, "ERROR: '*it' cannot be NULL in StrPopRaw.");
    FAILEX(raw == NULL, "ERROR: 'raw' cannot be NULL in StrPopRaw.");
    FAILEX(bytesLeft == NULL, "ERROR: 'bytesLeft' cannot be NULL in StrPopRaw.");
    FAILEX(rawSize > *bytesLeft, "ERROR: Buffer too small in StrPopRaw.");

    //
    // Pop raw bytes from buffer.
    //

    memcpy(raw, *it, rawSize);

    DEBUG3("StrPopRaw : Popped [%d] bytes from buffer.\n", rawSize);

    //
    // Move pointer by rawSize bytes.
    //

    (*it) += rawSize;

    (*bytesLeft) -= rawSize;

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
  // value     - buffer, where to write popped QWORD (OUT).
  // it        - pointer inside raw buffer, where to read data from (IN/OUT).
  // bytesLeft - number of bytes left in buffer (IN/OUT).
  // flags     - set to STR_BIG_ENDIAN or STR_LITTLE_ENDIAN (IN).
  //
  // RETURNS: 0 if all 8 bytes popped,
  //         -1 if error.
  //

  int StrPopQword(uint64_t *value, char **it, int *bytesLeft, int flags)
  {
    DBG_ENTER3("StrPopQword");

    int exitCode = -1;

    //
    // Check args.
    //

    FAILEX(it == NULL,        "ERROR: 'it' cannot be NULL in StrPopQword.");
    FAILEX(*it == NULL,       "ERROR: '*it' cannot be NULL in StrPopQword.");
    FAILEX(value == NULL,     "ERROR: Null pointer passed to StrPopQword.");
    FAILEX(bytesLeft == NULL, "ERROR: 'bytesLeft' cannot be NULL in StrPopQword.");
    FAILEX(*bytesLeft < 8,    "ERROR: Buffer too small in StrPopQword.");

    //
    // Pop big endian qword (motorola 68k like).
    //

    if (flags & STR_BIG_ENDIAN)
    {
      uint8_t *dst = (uint8_t *) value;
      uint8_t *src = (uint8_t *) *it;

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
      *value = *(uint64_t *)(*it);

      DEBUG3("StrPopQword : Popped little endian [0x%"PRIx64"].\n", *value);
    }

    //
    // Move pointer 8 bytes.
    //

    (*it) += 8;

    (*bytesLeft) -= 8;

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
  // value     - buffer, where to write popped DWORD (OUT).
  // it        - pointer inside raw buffer, where to read data from (IN/OUT).
  // bytesLeft - number of bytes left in buffer (IN/OUT).
  // flags     - set to STR_BIG_ENDIAN or STR_LITTLE_ENDIAN (IN).
  //
  // RETURNS: 0 if all 4 bytes popped,
  //          -1 if error.
  //

  int StrPopDword(uint32_t *value, char **it, int *bytesLeft, int flags)
  {
    DBG_ENTER3("StrPopDword");

    int exitCode = -1;

    //
    // Check args.
    //

    FAILEX(it    == NULL,     "ERROR: 'it' cannot be NULL in StrPopDword.");
    FAILEX(*it   == NULL,     "ERROR: '*it' cannot be NULL in StrPopDword.");
    FAILEX(bytesLeft == NULL, "ERROR: 'bytesLeft' cannot be NULL in StrPopDword.");
    FAILEX(value == NULL,     "ERROR: Null pointer passed to StrPopDword.");
    FAILEX(*bytesLeft < 4,    "ERROR: Buffer too small in StrPopDword.");

    //
    // Pop big endian dword (motorola 68k like).
    //

    if (flags & STR_BIG_ENDIAN)
    {
      uint8_t *dst = (uint8_t *) value;
      uint8_t *src = (uint8_t *) *it;

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
      *value = *(uint32_t *)(*it);

      DEBUG3("StrPopDword : Popped little endian [0x%x].\n", *value);
    }

    //
    // Move pointer 4 bytes.
    //

    (*it) += 4;

    (*bytesLeft) -= 4;

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
  // value     - buffer, where to write popped byte (OUT).
  // it        - pointer inside raw buffer, where to read data from (IN/OUT).
  // bytesLeft - number of bytes left in buffer (IN/OUT).
  //
  // RETURNS: 0 if byte popped,
  //         -1 if error.
  //

  int StrPopByte(uint8_t *value, char **it, int *bytesLeft)
  {
    DBG_ENTER3("StrPopByte");

    int exitCode = -1;

    //
    // Check args.
    //

    FAILEX(it == NULL,        "ERROR: 'it' cannot be NULL in StrPopDword.");
    FAILEX(*it == NULL,       "ERROR: '*it' cannot be NULL in StrPopDword.");
    FAILEX(bytesLeft == NULL, "ERROR: 'bytseLeft' cannot be NULL in StrPopDword.");
    FAILEX(value == NULL,     "ERROR: 'value' cannot be NULL in StrPopDword.");
    FAILEX(*bytesLeft < 1,    "ERROR: Buffer too small in StrPopDword.");

    //
    // Pop byte from buffer.
    //

    *value = **it;

    DEBUG3("StrPopByte : Popped byte [0x%x].\n", *value);

    //
    // Move pointer one byte.
    //

    (*it) ++;

    (*bytesLeft) --;

    //
    // Error handler.
    //

    exitCode = 0;

    fail:

    DBG_LEAVE3("StrPopByte");

    return exitCode;
  }

  //
  // Pop C string from buffer.
  //
  // str       - pointer to begin of string (OUT).
  // len       - length of string in bytes, can be NULL if not needed (OUT/OPT).
  // it        - pointer inside raw buffer, where to read data from (IN/OUT).
  // bytesLeft - number of bytes left in buffer (IN/OUT).
  //
  // RETURNS: 0 if byte popped,
  //         -1 if error.
  //

  int StrPopString(const char **str, int *strLen, char **it, int *bytesLeft)
  {
    DBG_ENTER3("StrPopString");

    int exitCode = -1;

    uint32_t len = 0;

    //
    // Check args.
    //

    FAILEX(str == NULL, "ERROR: 'str' cannot be NULL in StrPopString.\n");

    //
    // Pop size header.
    //

    *str = "";

    FAIL(StrPopDword(&len, it, bytesLeft));

    DEBUG3("StrPopString : Popped length [%d].\n", len);

    FAILEX(*bytesLeft < len, "ERROR: Buffer too small in StrPopString.\n");

    //
    // Move pointer by <len> bytes.
    //

    *str = *it;

    (*it) += len;

    (*bytesLeft) -= len;

    //
    // Make sure there is zero terminator at the end of string.
    //

    if (len > 0)
    {
      (*it)[-1] = 0;
    }
    else
    {
      *str = "";
    }

    //
    // Give length to user if needed.
    //

    if (strLen)
    {
      *strLen = len;
    }

    //
    // Error handler.
    //

    exitCode = 0;

    fail:

    DBG_LEAVE3("StrPopString");

    return exitCode;
  }
} /* namespace Tegenaria */
