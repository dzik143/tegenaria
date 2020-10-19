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
// Example shows how to encrypt/decrypt raw buffers.
//

#include <Tegenaria/Secure.h>

using namespace Tegenaria;

//
// Entry point.
//

int main(int argc, char **argv)
{
  char buf[1024];

  char key[SECURE_BLOWFISH_KEY_SIZE] = "password";
  char iv[SECURE_BLOWFISH_KEY_SIZE]  = {0};

  int readed = 0;

  //
  // Initialie Blowfish cipher with given IV vector and key[].
  //

  SecureCipher *sc = SecureCipherCreate(SECURE_CIPHER_BLOWFISH,
                                              SECURE_CIPHER_MODE_CTR,
                                                key, sizeof(key), iv, sizeof(iv));

  //
  // Switch stdin/stdout to binary mode on windows.
  //

  #ifdef WIN32
  _setmode(0, _O_BINARY);
  _setmode(1, _O_BINARY);
  #endif

  //
  // Read data from stdin by 1024 blocks.
  //

  while((readed = fread(buf, 1, sizeof(buf), stdin)) > 0)
  {
    //
    // -d option specified.
    // Decrypt mode.
    //

    if (argc > 1)
    {
      SecureDecrypt(sc, buf, readed);
    }

    //
    // No option specified.
    // Encrypt mode.
    //

    else
    {
      SecureEncrypt(sc, buf, readed);
    }

    //
    // Write processed block to stdout.
    //

    fwrite(buf, readed, 1, stdout);
  }

  //
  // CLean up cipher.
  //

  SecureCipherDestroy(sc);

  return 0;
}
