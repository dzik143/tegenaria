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
// Purpose: One-direction hash functions.
//

#include "Secure.h"

#ifdef WIN64
  static int Win64NotImportedError()
  {
    fprintf(stderr, "OpenSSL is not imported on Win64");
    exit(-1);
  }

  #define SHA256_Init(x)         Win64NotImportedError()
  #define SHA256_Update(x, y, z) Win64NotImportedError()
  #define SHA256_Final(x, y)     Win64NotImportedError()
#endif

namespace Tegenaria
{
  //
  // Compute sha256 hash. Function compute hash = SHA256(data + salt).
  // Salt is optional, set to NULL if not needed.
  //
  // WARNING! Hash[] buffer MUST have at least 65 bytes length.
  //
  //
  // hash     - buffer, where to store computed hash (OUT).
  // hashSize - size of hash[] buffer in bytes (IN).
  // data     - data to hash (IN).
  // dataSize - size of data[] buffer in bytes (IN).
  // salt     - optional salt data to add. Can be NULL (IN/OPT).
  // saltSize - size of salt[] buffer in bytes. Can be 0. (IN/OPT).
  //
  // RETURNS: 0 if OK.
  //

  int SecureHashSha256(char *hash, int hashSize,
                           const char *data, int dataSize,
                               const char *salt, int saltSize)
  {
    DBG_ENTER3("SecureHashSha256");

    int exitCode = -1;

    SHA256_CTX ctx;

    unsigned char hashRaw[SHA256_DIGEST_LENGTH];

    char *dst = NULL;

    char hex[] = "0123456789abcdef";

    //
    // Check args.
    //

    FAILEX(hash == NULL , "ERROR: 'hash' cannot be NULL in SecureHashSha256().\n");
    FAILEX(hashSize < 65, "ERROR: 'hashSize' MUST be at least 65 in SecureHashSha256().\n");
    FAILEX(data == NULL,  "ERROR: 'data' cannot be NULL in SecureHashSha256().\n");
    FAILEX(dataSize < 1,  "ERROR: 'dataSize' cannot be < 1 in SecureHashSha256().\n");

    //
    // Init SHA256 context.
    //

    FAILEX(SHA256_Init(&ctx) == 0,
               "ERROR: Cannot init SHA256 context.\n");

    //
    // Comptue SHA256(data + salt).
    //

    FAILEX(SHA256_Update(&ctx, (unsigned char *) data, dataSize) == 0,
               "ERROR: Cannot compute SHA256 hash.\n");

    if (salt && saltSize > 0)
    {
      FAILEX(SHA256_Update(&ctx, (unsigned char *) salt, saltSize) == 0,
                 "ERROR: Cannot compute SHA256 hash.\n");
    }

    //
    // Pop raw binary hash from SHA256 context.
    //

    FAILEX(SHA256_Final(hashRaw, &ctx) == 0,
               "ERROR: Cannot pop hash from SHA256 context.\n");

    //
    // Convert raw, binary hash into asciz string.
    //

    dst = hash;

    for (int i = 0; i < 32; i++)
    {
      dst[0] = hex[(hashRaw[i] & 0x0f)];
      dst[1] = hex[(hashRaw[i] & 0xf0) >> 4];

      dst += 2;
    }

    hash[64] = 0;

    //
    // Error handler.
    //

    exitCode = 0;

    fail:

    DBG_LEAVE3("SecureHashSha256");

    return exitCode;
  }
} /* namespace Tegenaria */
