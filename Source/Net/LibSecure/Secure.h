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

#ifndef Tegenaria_Core_Secure_H
#define Tegenaria_Core_Secure_H

//
// Includes.
//

#include <cstdio>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <cstring>
#include <unistd.h>
#include <string>
#include <list>
#include <map>

#ifdef WIN32
# include <io.h>
#else
# include <sys/socket.h>
# include <termios.h>
#endif

#include <openssl/ssl.h>
#include <openssl/rand.h>
#include <openssl/blowfish.h>

#include <Tegenaria/Debug.h>
#include <Tegenaria/Mutex.h>

namespace Tegenaria
{
  using std::string;
  using std::list;
  using std::map;

  //
  // Defines.
  //

  #define SECURE_INTENT_SERVER 0
  #define SECURE_INTENT_CLIENT 1

  #define SECURE_STATE_HANDSHAKE_WRITE 0
  #define SECURE_STATE_HANDSHAKE_READ  1
  #define SECURE_STATE_ESTABLISHED     2

  #define SECURE_IOMODE_NONE      0
  #define SECURE_IOMODE_CALLBACKS 1
  #define SECURE_IOMODE_FDS       2
  #define SECURE_IOMODE_SOCKET    3

  #define SECURE_TLS_HANDSHAKE_TIMEOUT 30000 // Timeout while performing SSL handshake

  #define SECURE_BLOWFISH_KEY_SIZE 16

  #define SECURE_CIPHER_BLOWFISH 0

  #define SECURE_CIPHER_MODE_ECB 0
  #define SECURE_CIPHER_MODE_CTR 1

  #define SECURE_MAX_KEYPASS_LEN 64

  //
  // Rights for ACL.
  //

  #define SECURE_ACL_DENY  (1 << 0)
  #define SECURE_ACL_FULL  (1 << 1)
  #define SECURE_ACL_READ  (1 << 2)
  #define SECURE_ACL_WRITE (1 << 3)
  #define SECURE_ACL_ERASE (1 << 4)

  #define SECURE_ACL_SYMBOL_DENY  'D'
  #define SECURE_ACL_SYMBOL_FULL  'F'
  #define SECURE_ACL_SYMBOL_READ  'R'
  #define SECURE_ACL_SYMBOL_WRITE 'W'
  #define SECURE_ACL_SYMBOL_ERASE 'E'

  //
  // Typedef.
  //

  typedef int (*SecureReadProto)(void *buf, int count, int timeout, void *ctx);
  typedef int (*SecureWriteProto)(const void *buf, int count, int timeout, void *ctx);

  //
  // Class to wrap FD/SOCKET/Callbacks into secure one.
  //

  class SecureConnection
  {
    //
    // Friends.
    //

    friend SecureConnection
               *SecureConnectionCreate(int, SecureReadProto, SecureWriteProto,
                                           void *, const char *, const char *,
                                               const char *);
    friend SecureConnection
               *SecureConnectionCreate(int, int, int, const char *,
                                           const char *, const char *);
    friend SecureConnection
               *SecureConnectionCreate(int, int, const char *,
                                           const char *, const char *);

    friend SecureConnection
               *SecureConnectionCreate(int, const char *,
                                           const char *, const char *);

    //
    // Private fields.
    //

    private:

    //
    // Underlying SSL data.
    //

    BIO *readBio_;
    BIO *writeBio_;

    SSL *ssl_;

    SSL_CTX *sslCtx_;

    //
    // SECURE_INTENT_CLIENT or SECURE_INTENT_SERVER.
    //

    int intent_;

    //
    // Current connection state.
    // See SECURE_STATE_XXX defines.
    //

    int state_;

    //
    // Specify type of underlying unecrypted IO.
    // See SECURE_IOMODE_XXX defines.
    //

    int ioMode_;

    //
    // Used when underlying IO is defined by {read, write} callbacks.
    //

    SecureReadProto readCallback_;
    SecureWriteProto writeCallback_;

    void *ioCtx_;

    //
    // Used when underlying IO is defined by FD pair.
    //

    int fdIn_;
    int fdOut_;

    //
    // Used when underlying IO is a socket.
    //

    int sock_;

    //
    // Reffrence counter to track number of third objects
    // using current secure connection object.
    //

    int refCount_;

    Mutex refCountMutex_;

    //
    // Private init functions.
    //

    SecureConnection();

    ~SecureConnection();

    int initSSL(int intent, const char *cert,
                    const char *privKey, const char *privKeyPass);

    //
    // Callbacks called by OpenSSL.
    //

    static int readPassCallback(char *buf, int size,
                                    int rwflag, void *userdata);

    //
    // Public interface.
    //

    public:

    //
    // Refference counter.
    //

    void addRef();
    void release();

    //
    // Functions for read/write/request over secure connection.
    //

    int write(const void *buf, int len, int timeout);

    int read(void *buf, int len, int timeout);

    int request(int *serverCode, char *serverMsg,
                    int serverMsgSize, int timeout, const char *fmt, ...);

    //
    // Direct encrypt/decrypt using underlying SSL object.
    //

    int encrypt(void *encrypted, int encryptedSize,
                    const void *buffer, int bufferSize);

    int decrypt(void *decrypted,int decryptedSize,
                    const void *buffer, int bufferSize);

    //
    // Functions for read/write bypassing encryption system.
    // These function passes data to underlying IO directly.
    //

    int writeRaw(const void *buf, int len, int timeout);

    int readRaw(void *buf, int len, int timeout);

    //
    // Handshake.
    //

    int handshakeStep(void *customBuffer = NULL, int *customSize = NULL);

    int handshakeStep(void *outputBuffer, int *outputSize,
                          void *inputBuffer, int inputSize);

    //
    // Getters.
    //

    int getState();
  };

  //
  // Structure to store cipher context while encrypting/decrypting
  // streams.
  //

  struct SecureCipher;

  //
  // Class to implement generic access list.
  //

  class SecureAcl
  {
    private:

    map<string, int> rights_;

    public:

    //
    // Init methods.
    //

    SecureAcl();

    int initFromString(const char *acl);

    //
    // Get rights from ACL.
    //

    int getRights(const char *user);

    string getRightsString(const char *user);

    //
    // Grant/revoke rights to ACL.
    //

    int setRights(const char *user, int rights);

    int setRights(const char *user, const char *rights);

    int setOthersRights(int rights);

    int setOthersRights(const char *rights);

    int revokeAll(const char *user);

    void clear();

    //
    // Conversion methods.
    //

    int encodeRights(const char *str);

    string decodeRights(int rights);

    string toString();
  };

  //
  // Exported functions.
  //

  //
  // Random numbers.
  //

  int SecureRandom(void *buf, int len);

  int SecureRandomText(char *buf, int len);

  int SecureRandomHexText(char *buf, int len);

  uint32_t SecureRandomInt32();
  uint64_t SecureRandomInt64();

  unsigned char SecureRandomByte();

  //
  // Generic encrypt/decrypt for raw buffers.
  //

  void SecureEncrypt(SecureCipher *sc, void *buffer, int size);
  void SecureDecrypt(SecureCipher *sc, void *buffer, int size);

  SecureCipher *SecureCipherCreate(int cipher, int cipherMode,
                                         const char *key, int keySize,
                                             const char *iv, int ivSize);

  void SecureCipherDestroy(SecureCipher *ctx);

  //
  // Wrap {read, write} callback into secure connection.
  //

  SecureConnection *SecureConnectionCreate(int intent,
                                               SecureReadProto readCallback,
                                                   SecureWriteProto writeCallback,
                                                       void *ioCtx,
                                                           const char *cert,
                                                               const char *privKey,
                                                                   const char *privKeyPass);

  //
  // Wrap fdin/fdout pair into secure connection.
  //

  SecureConnection *SecureConnectionCreate(int intent, int fdin, int fdout,
                                               const char *cert,
                                                   const char *privKey,
                                                       const char *privKeyPass);

  //
  // Wrap socket into secure connection.
  //

  SecureConnection *SecureConnectionCreate(int intent, int sock,
                                               const char *cert,
                                                   const char *privKey,
                                                       const char *privKeyPass);

  //
  // Custom secure connection.
  //

  SecureConnection *SecureConnectionCreate(int intent,
                                               const char *cert,
                                                   const char *privKey,
                                                       const char *privKeyPass);

  //
  // One-direction hash functions.
  //

  int SecureHashSha256(char *hash, int hashSize,
                           const char *data, int dataSize,
                               const char *salt, int saltSize);

  //
  // Password related functions.
  //

  int SecureDisableEcho();

  int SecureEnableEcho();

  int SecureReadPassword(char *pass, int passSize, const char *prompt);

  int SecurePassAuthorize(const char *expectedHash,
                              const char *password, const char *salt);

} /* namespace Tegenaria */

#endif /* Tegenaria_Core_Secure_H */
