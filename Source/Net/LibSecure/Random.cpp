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
// Purpose: Generate cryptogically strong random numbers or buffers.
//

#include "Secure.h"

#ifdef WIN64
  static int Win64NotImportedError()
  {
    fprintf(stderr, "OpenSSL is not imported on Win64");
    exit(-1);
  }

  #define RAND_bytes(x, y) Win64NotImportedError()
#endif

namespace Tegenaria
{
  //
  // Generate random buffer.
  //
  // buf - buffer, where to store generated data (OUT).
  // len - number of butes to generate (IN).
  //
  // RETURNS: 0 if OK.
  //

  int SecureRandom(void *buf, int len)
  {
    if (RAND_bytes((unsigned char *) buf, len) == 1)
    {
      return 0;
    }
    else
    {
      Error("Cannot generate random buffer.\n");

      return -1;
    }
  }

  //
  // Generate random 32-bit integer.
  //

  uint32_t SecureRandomInt32()
  {
    uint32_t ret = -1;

    SecureRandom(&ret, sizeof(ret));

    return ret;
  }

  //
  // Generate random 64-bit integer.
  //

  uint64_t SecureRandomInt64()
  {
    uint64_t ret = -1;

    SecureRandom(&ret, sizeof(ret));

    return ret;
  }

  //
  // Generate random byte.
  //

  uint8_t SecureRandomByte()
  {
    uint8_t ret = -1;

    SecureRandom(&ret, sizeof(ret));

    return ret;
  }

  //
  // Generate random text.
  // Example: K1aCK1bC0=38]8GM9ggk5=K836@yee5M
  //
  // buf - buffer, where to store generated text (OUT).
  // len - size of buf[] buffer in bytes (IN).
  //
  // RETURNS: 0 if OK.
  //

  int SecureRandomText(char *buf, int len)
  {
    int exitCode = -1;

    FAIL(SecureRandom(buf, len));

    for (int i = 0; i < len; i++)
    {
      buf[i] = '0' + ((unsigned int) buf[i]) % ('z' - '0');
    }

    buf[len - 1] = 0;

    //
    // Error handler.
    //

    exitCode = 0;

    fail:

    return exitCode;
  }

  //
  // Generate random hex asciz text.
  // Example: 23c02a8b7c7
  //
  // buf - buffer, where to store generated hex text (OUT).
  // len - size of buf[] buffer in bytes (IN).
  //
  // RETURNS: 0 if OK.
  //

  int SecureRandomHexText(char *buf, int len)
  {
    int exitCode = -1;

    char *dst = buf;

    char hex[] = "0123456789abcdef";

    //
    // Generate raw random buffer.
    //

    FAIL(SecureRandom(buf + len / 2, len / 2));

    //
    // Convert raw buffer to hex.
    //

    dst = buf;

    for (int i = len / 2; i < len; i++)
    {
      dst[0] = hex[(buf[i] & 0x0f)];
      dst[1] = hex[(buf[i] & 0xf0) >> 4];

      dst += 2;
    }

    buf[len - 1] = 0;

    //
    // Error handler.
    //

    exitCode = 0;

    fail:

    return exitCode;
  }
} /* namespace Tegenaria */
