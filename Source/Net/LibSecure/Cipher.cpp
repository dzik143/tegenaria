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
// Purpose: General encryption/decryption routines working on raw buffers.
//

#include "Secure.h"
#include "Internal.h"

#ifdef WIN64
  static int Win64NotImportedError()
  {
    fprintf(stderr, "OpenSSL is not imported on Win64");
    exit(-1);
  }

  #define BF_cfb64_encrypt(a, b, c, d, e, f, g) Win64NotImportedError()
  #define BF_set_key(a, b, c)                   Win64NotImportedError()
#endif

namespace Tegenaria
{
  //
  // Encrypt data.
  //
  // ctx    - secure context containing cipher state created by
  //          SecureCipherCreate before (IN/OUT).
  //
  // buffer - buffer to encrypt (IN/OUT).
  //
  // size   - size of buffer[] in bytes (IN).
  //

  void SecureEncrypt(SecureCipher *ctx, void *buffer, int size)
  {
    DBG_ENTER3("SecureEncrypt");

    unsigned char iv[16];

    int num = 0;

    int *piv = (int *) iv;

    memcpy(iv, ctx -> iv_, 16);

    //
    // Apply counter number for CTR mode.
    //

    if (ctx -> cipherMode_ == SECURE_CIPHER_MODE_CTR)
    {
      (*piv) ^= ctx -> counterEncrypt_;

      ctx -> counterEncrypt_ ++;
    }

    //
    // Encrypt data.
    //

    BF_cfb64_encrypt((unsigned char *) buffer, (unsigned char *) buffer,
                         size, &(ctx -> key_), (unsigned char *) iv, &num, BF_ENCRYPT);

    DBG_LEAVE3("SecureEncrypt");
  }

  //
  // Decrypt data.
  //
  // ctx    - secure context containing cipher state created by
  //          SecureCipherCreate before (IN/OUT).
  //
  // buffer - buffer to decrypt (IN/OUT).
  //
  // size   - size of buffer[] in bytes (IN).
  //

  void SecureDecrypt(SecureCipher *ctx, void *buffer, int size)
  {
    DBG_ENTER("SecureDecrypt");

    unsigned char iv[16];

    int num = 0;

    int *piv = (int *) iv;

    memcpy(iv, ctx -> iv_, 16);

    //
    // Apply counter number for CTR mode.
    //

    if (ctx -> cipherMode_ == SECURE_CIPHER_MODE_CTR)
    {
      (*piv) ^= ctx -> counterDecrypt_;

      ctx -> counterDecrypt_ ++;
    }

    //
    // Decrypt data.
    //

    BF_cfb64_encrypt((unsigned char *) buffer, (unsigned char *) buffer,
                         size, &(ctx -> key_), (unsigned char *) iv, &num, BF_DECRYPT);

    DBG_LEAVE3("SecureDecrypt");
  }

  //
  // Create secure context object to track state of encrypt/decrypt process.
  //
  // TIP#1: Use SecureEncrypt to encrypt data using created context.
  // TIP#2: Use SecureDecrypt to decrypt data using created context.
  //
  // cipher     - cipher to use, see SECURE_CIPHER_XXX defines in Secure.h (IN).
  //
  // cipherMode - the way how encrpted blocks are joined into stream,
  //              see SECURE_CIPHER_MODE_XXX defines in Secure.h (IN).
  //
  // key        - symmetric key to use (IN).
  // keySize    - size of key[] buffer in bytes (IN).
  // iv         - init vector, can be treated as second part of key (IN).
  // ivSize     - size of iv[] buffer in bytes (IN).
  //

  SecureCipher *SecureCipherCreate(int cipher, int cipherMode,
                                         const char *key, int keySize,
                                             const char *iv, int ivSize)
  {
    DBG_ENTER("SecureCipherCreate");

    int exitCode = -1;

    //
    // Allocate context buffer.
    //

    SecureCipher *ctx = (SecureCipher *) calloc(sizeof(SecureCipher), 1);

    ctx -> cipher_     = cipher;
    ctx -> cipherMode_ = cipherMode;

    //
    // Init selected cipher.
    //

    switch(cipher)
    {
      //
      // Blowfish.
      //
      // Save iv vector and set key.
      //

      case SECURE_CIPHER_BLOWFISH:
      {
        //
        // Check iv[] length.
        //

        FAILEX(ivSize != SECURE_BLOWFISH_KEY_SIZE,
                   "ERROR: iv[] musts have %d bytes long for blowfish cipher.\n",
                       SECURE_BLOWFISH_KEY_SIZE);

        //
        // Check key[] length.
        //

        FAILEX(keySize != SECURE_BLOWFISH_KEY_SIZE,
                   "ERROR: key[] musts have %d bytes long for blowfish cipher.\n",
                       SECURE_BLOWFISH_KEY_SIZE);

        //
        // Save iv[] vector.
        //

        memcpy(ctx -> iv_, iv, ivSize);

        //
        // Set blowfish key.
        //

        BF_set_key(&(ctx -> key_), keySize, (unsigned char *) key);

        break;
      }

      //
      // Unknown cipher.
      //

      default:
      {
        Error("ERROR: Unsupported cipher '%d'.\n", cipher);

        goto fail;
      }
    }

    //
    // Error handler.
    //

    exitCode = 0;

    if (exitCode)
    {
      Error("Cannot init cipher context '%d' mode '%d'.\n", cipher, cipherMode);

      SecureCipherDestroy(ctx);

      ctx = NULL;
    }

    fail:

    DBG_LEAVE("SecureCipherCreate");

    return ctx;
  }

  //
  // Free secure context created by SecureCipherCreate() before.
  //

  void SecureCipherDestroy(SecureCipher *ctx)
  {
    DBG_ENTER("SecureCipherDestroy");

    if (ctx)
    {
      free(ctx);
    }

    DBG_LEAVE("SecureCipherDestroy");
  }
} /* namespace Tegenaria */
