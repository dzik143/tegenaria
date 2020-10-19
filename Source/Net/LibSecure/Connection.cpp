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
// WARNING!
// To create fully secure connection we need two steps:
//
// 1. Create secure TLS session - after that transfered data are encrypted
//   and *NOT READABLE* by third parties,
//
// 2. validate *WHO is ON THE OTHER SIDE* basing on data from certificate,
//   e.g. by check domain name.
//
// Below code performs (1) but does *NOT* perform (2).
// Caller *SHOULD* deliver own mechanism to check does data delivered
// within certificate are match to what he's expecting.
//

//
// Purpose: Wrap existing unsecure connection into secure one.
//
// Source unsecure connection can be defined in follow ways:
//
// - {read, write} callbacks
// - {FDin, FDout} pair
// - socket
//
// - custom, caller must use SecureEncrypt before every write operation and
//   SecureDecrypt afrer every read operation manually
//

#include "Secure.h"

#ifdef WIN64
  static int Win64NotImportedError()
  {
    fprintf(stderr, "OpenSSL is not imported on Win64");
    exit(-1);
  }

  #define SSL_write(x, y, z)                           Win64NotImportedError()
  #define SSL_read(x, y, z)                            Win64NotImportedError()
  #define SSL_free(x)                                  Win64NotImportedError()
  #define SSL_CTX_free(x)                              Win64NotImportedError()
  #define SSL_is_init_finished(x)                      Win64NotImportedError()
  #define SSL_do_handshake(x)                          Win64NotImportedError()
  #define BIO_read(x, y, z)                            Win64NotImportedError()
  #define BIO_write(x, y, z)                           Win64NotImportedError()
  #define SSL_library_init()                           Win64NotImportedError()
  #define SSL_CTX_new(x)                               (SSL_CTX *) Win64NotImportedError()
  #define SSL_CTX_use_certificate_chain_file(x, y)     Win64NotImportedError()
  #define SSL_CTX_set_default_passwd_cb(x, y)          Win64NotImportedError()
  #define SSL_CTX_set_default_passwd_cb_userdata(x, y) Win64NotImportedError()
  #define SSL_CTX_use_PrivateKey_file(x, y, z)         Win64NotImportedError()
  #define SSL_CTX_use_certificate_file(x, y, z)        Win64NotImportedError()
  #define SSL_CTX_set_options(x, y)                    Win64NotImportedError()
  #define SSL_CTX_set_session_id_context(x, y, z)      Win64NotImportedError()
  #define SSL_new(x)                                   (SSL *) Win64NotImportedError()
  #define SSL_set_accept_state(x)                      Win64NotImportedError()
  #define SSL_set_verify(x, y, z)                      Win64NotImportedError()
  #define SSL_set_session_id_context(x, y, z)          Win64NotImportedError()
  #define BIO_s_mem()                                  Win64NotImportedError()
  #define SSL_set_connect_state(x)                     Win64NotImportedError()
  #define BIO_new(x)                                   (BIO *) Win64NotImportedError()
  #define BIO_set_nbio(x, y)                           Win64NotImportedError()
  #define SSL_set_bio(x, y, z)                         Win64NotImportedError()
#endif

namespace Tegenaria
{
  //
  // Write <len> bytes directly to underlying IO skipping SSL object beetwen.
  //
  // Used internally only.
  //
  // buf     - source buffer with data, which we want to write (IN).
  // len     - number of bytes to write (IN).
  // timeout - timeout in ms, set to -1 for infinite (IN).
  //
  // RETURNS: Number of bytes written or
  //          -1 if error.
  //

  int SecureConnection::writeRaw(const void *buf, int len, int timeout)
  {
    DBG_ENTER3("SecureConnection::writeRaw");

    int written = -1;

    //
    // Write data to underlying IO.
    //

    switch(ioMode_)
    {
      //
      // Underlying IO is defined by {read, write, cancel} callbacks.
      //

      case SECURE_IOMODE_CALLBACKS:
      {
        written = writeCallback_(buf, len, -1, ioCtx_);

        break;
      }

      //
      // Underlying IO is defined as FD pair.
      //

      case SECURE_IOMODE_FDS:
      {
        if (timeout > 0)
        {
          Error("ERROR: Timeout not implemented for FDs yet in SecureConnection::writeRaw().\n");
        }

        written = ::write(fdOut_, buf, len);

        break;
      }

      //
      // Underlying IO is a socket.
      //

      case SECURE_IOMODE_SOCKET:
      {
        if (timeout > 0)
        {
          Error("ERROR: Timeout not implemented for SOCKET yet in SecureConnection::writeRaw().\n");
        }

        written = send(sock_, (char *) buf, len, 0);

        break;
      }
    }

    DBG_LEAVE3("SecureConnection::writeRaw");

    return written;
  }

  //
  // Read data directly from underlying IO (without parsing it via SSL
  // object).
  //
  // Used internally only.
  //
  // buf     - destination buffer, where to write readed data (OUT).
  // len     - number of bytes to read (IN).
  // timeout - timeout in ms, set -1 to infinity (IN).
  //
  // RETURNS: Number of bytes readed or
  //          -1 if error.
  //

  int SecureConnection::readRaw(void *buf, int len, int timeout)
  {
    DBG_ENTER3("SecureConnection::readRaw");

    int written = -1;
    int readed  = -1;

    //
    // Read data from underlying IO.
    //

    switch(ioMode_)
    {
      //
      // Underlying IO is defined as {read, write, cancel} callbacks.
      //

      case SECURE_IOMODE_CALLBACKS:
      {
        readed = readCallback_(buf, len, timeout, ioCtx_);

        break;
      }

      //
      // Underlying IO is defined as FD pair.
      //

      case SECURE_IOMODE_FDS:
      {
        if (timeout > 0)
        {
          Error("WARNING: Timeout is not available for FDs yet in SecureConnection::readRaw().\n");
        }

        readed = ::read(fdIn_, buf, len);

        break;
      }

      //
      // Underlying IO is a socket.
      //

      case SECURE_IOMODE_SOCKET:
      {
        if (timeout > 0)
        {
          Error("WARNING: Timeout is not available for SOCKETs yet in SecureConnection::readRaw().\n");
        }

        readed = recv(sock_, (char *) buf, len, 0);

        break;
      }
    }

    DBG_ENTER3("SecureConnection::readRaw");

    return readed;
  }

  //
  // Encrypt message.
  //
  // encrypted     - buffer, where to store encrypted message (OUT).
  // encryptedSize - size of encrypted[] buffer in bytes (IN).
  // buffer        - source buffer with data to encrypt (IN).
  // bufferSize    - number of bytes to be encrypted (IN).
  //
  // RETURNS: Length of encrypted data written to encrypted[] in bytes or
  //          -1 if error.
  //

  int SecureConnection::encrypt(void *encrypted, int encryptedSize,
                                    const void *buffer, int bufferSize)
  {
    DBG_ENTER3("SecureConnection::encrypt");

    int readed = 0;

    //
    // Pass unecrypted message to SSL BIO.
    //

    SSL_write(ssl_, buffer, bufferSize);

    //
    // Read back encrypted message from SSL.
    //

    readed = BIO_read(writeBio_, encrypted, encryptedSize);

    DBG_LEAVE3("SecureConnection::encrypt");

    return readed;
  }

  //
  // Decrypt message.
  //
  // decrypted     - buffer, where to store decrypted message (OUT).
  // decryptedSize - size of decrypted[] buffer in bytes (IN).
  // buffer        - source buffer with data to be decrypt (IN).
  // bufferSize    - number of bytes to be decrypted (IN).
  //
  // RETURNS: Length of decrypted data written to decrypted[] in bytes or
  //          -1 if error.
  //

  int SecureConnection::decrypt(void *decrypted, int decryptedSize,
                                    const void *buffer, int bufferSize)
  {
    DBG_ENTER3("SecureConnection::decrypt");

    int readed = 0;

    //
    // Pass readed enrypted data to SSL BIO.
    //

    BIO_write(readBio_, buffer, bufferSize);

    //
    // Read back decrypted data from SSL.
    //

    readed = SSL_read(ssl_, decrypted, decryptedSize);

    DBG_LEAVE3("SecureConnection::ecrypt");

    return readed;
  }

  //
  // Write <len> bytes throught secure connection.
  //
  // buf     - source buffer with data, which we want to write (IN).
  // len     - number of bytes to write (IN).
  // timeout - timeout in ms, set to -1 for infinite (IN).
  //
  // RETURNS: Number of bytes written or
  //          -1 if error.
  //

  int SecureConnection::write(const void *buf, int len, int timeout)
  {
    DBG_ENTER3("SecureConnection::write");

    int written = -1;
    int readed  = -1;

    char encrypted[1024];

    int encryptedSize = 0;

    //
    // Encrypt message.
    //

    encryptedSize = this -> encrypt(encrypted, sizeof(encrypted), buf, len);

    //
    // Write encrypted data to underlying IO.
    //

    if (encryptedSize > 0)
    {
      written = this -> writeRaw(encrypted, encryptedSize, timeout);
    }

    DBG_LEAVE3("SecureConnection::write");

    return written;
  }

  //
  // Read data from secure connection.
  //
  // buf     - destination buffer, where to write readed data (OUT).
  // len     - number of bytes to read (IN).
  // timeout - timeout in ms, set -1 to infinite (IN)
  //
  // RETURNS: Number of bytes readed or
  //          -1 if error.
  //

  int SecureConnection::read(void *buf, int len, int timeout)
  {
    DBG_ENTER3("SecureConnection::read");

    int written = -1;
    int readed  = -1;

    char encrypted[1024];

    int encryptedSize = 0;

    //
    // Read encrypted data from underlying IO.
    //

    encryptedSize = this -> readRaw(encrypted, sizeof(encrypted), timeout);

    //
    // Decrypt message into caller buffer.
    //

    if (encryptedSize > 0)
    {
      readed = this -> decrypt(buf, len, encrypted, encryptedSize);
    }

    DBG_LEAVE3("SecureConnection::read");

    return readed;
  }

  //
  // - Send single, printf like formatted request to server
  // - read answer in format 'XYZ > message'
  // - split answer to <XYZ> code and <message> parts.
  //
  // Example usage:
  //
  // request(&serverCode, serverMsg, sizeof(serverMsg),
  //             "share --alias %s --path %s", alias, path);
  //
  // TIP: If only exit code is needed <answer> can be set to NULL.
  //
  // sc            - pointer to SecureConnection object connected to server (IN).
  // serverCode    - exit code returned by server (OUT).
  // serverMsg     - ASCIZ message returned by server (OUT/OPT).
  // serverMsgSize - size of answer buffer in bytes (IN).
  // timeout       - timeout in ms, defaulted to infinite if -1 (IN/OPT).
  // fmt           - printf like parameters to format command to send (IN).
  //
  // RETURNS: 0 if request sucessfuly sent and asnwer from server received.
  //         -1 otherwise.
  //
  // WARNING!: Request could still failed on server side.
  //           To get server's side exit code use 'answerCode' parameter.
  //

  int SecureConnection::request(int *serverCode, char *serverMsg,
                                    int serverMsgSize, int timeout,
                                        const char *fmt, ...)
  {
    DBG_ENTER("SecureConnection::request");

    int exitCode = -1;

    char buf[1024];

    int cmdLen = 0;

    char *dst = NULL;

    int readed = 0;
    int total  = 0;

    int eofReceived = 0;

    int len = 0;

    va_list ap;

    //
    // Check args.
    //

    FAILEX(serverCode == NULL, "ERROR: 'serverCode' cannot be NULL in SecureRequest.");
    FAILEX(fmt == NULL, "ERROR: 'fmt' cannot be NULL in SecureRequest.");

    //
    // Format printf like message.
    //

    va_start(ap, fmt);

    len = vsnprintf(buf, sizeof(buf) - 1, fmt, ap);

    va_end(ap);

    //
    // Send command to server INCLUDING zero terminator byte.
    //

    FAILEX(this -> write(buf, len + 1, timeout) < 0,
               "ERROR: Cannot send request.\n");

    //
    // Read answer from server in below format:
    // 'XYZ> <message>'
    //
    // Where <XYZ> is 3 decimal server side code e.g. "871".
    //

    //
    // Read 'XYZ> ' prefix first.
    // where XYZ is 3 decimal exit code returned by server.
    //

    FAILEX(this -> read(buf, 5, timeout) != 5,
               "ERROR: Cannot read 'XYZ> ' prefix.");

    buf[4] = 0;

    *serverCode = atoi(buf);

    //
    // Read ASCIZ message part if needed.
    //

    if (serverMsg && serverMsgSize > 0)
    {
      dst = serverMsg;

      total = 0;

      //
      // FIXME: Avoid reading byte by byte.
      //

      while(this -> read(dst, 1, timeout) == 1)
      {
        //
        // Caller buffer too short.
        //

        if (total == serverMsgSize)
        {
          break;
        }

        //
        // End of message, it's ordinal end.
        //

        if (dst[0] == 0)
        {
          eofReceived = 1;

          break;
        }

        total ++;

        dst ++;
      }

      serverMsg[total] = 0;
    }

    //
    // Flush remaining message from server if any.
    // This is scenario when caller message[] buffer is shorter
    // than message sent by server.
    //

    while(eofReceived == 0)
    {
      if (this -> read(buf, 1, timeout) <= 0 || buf[0] == 0)
      {
        eofReceived = 1;
      }
    }

    exitCode = 0;

    //
    // Clean up.
    //

    fail:

    if (exitCode)
    {
      Error("Cannot execute secure request.\n"
                "Error code is : %d.\n", GetLastError());
    }

    DBG_LEAVE("SecureConnection::request");

    return exitCode;
  }

  //
  // Desctroy secure connection created by SecureConnectionCreate() before.
  //

  SecureConnection::~SecureConnection()
  {
    DBG_ENTER("SecureConnection::~SecureConnection");

    if (ssl_)
    {
      SSL_free(ssl_);
    }

    if (sslCtx_)
    {
      SSL_CTX_free(sslCtx_);
    }

    DBG_LEAVE("SecureConnection::~SecureConnection");
  }

  //
  // Perform underlying SSL handshake to init encrypted secure connection.
  //
  // Internal use only.
  //
  // TIP#1: For custom connection use 5-parameter SecureHandshakeStep.
  //
  // TIP#2: For non-custom connections (underlying IO is set to callbacks,
  //        socket or FDs pair} handshake is performed automatically in
  //        SecureConnectionCreate. No manually work needed.
  //
  //
  // WARNING! Handshake must be performed before any data would be send
  //          via SecureWrite() or read via SecureRead() functions.
  //
  // Parameters:
  //
  // customBuffer  - on input data treated as readed from underlying IO if IOMODE
  //                 set to NONE. On output data needed to be written to
  //                 underlying IO. (IN/OUT/OPT).
  //
  // customSize    - on input size of customBuffer[] in bytes if IOMODE set to
  //                 NONE. On output number of bytes returned in customBuffer[]
  //                 and needed to be written back to underlying IO (IN/OUT/OPT).
  //
  // RETURNS: 0 if OK.
  //

  int SecureConnection::handshakeStep(void *customBuffer, int *customSize)
  {
    DBG_ENTER("SecureConnection::handshakeStep");

    int exitCode = -1;

    char buffer[1024];

    int readed  = 0;
    int written = 0;

    //
    // Handshake finished.
    //

    if (SSL_is_init_finished(ssl_))
    {
      state_ = SECURE_STATE_ESTABLISHED;

      //
      // Server should sent "OK" message at the end of handshake.
      //

      if (intent_ == SECURE_INTENT_CLIENT)
      {
        readed = SSL_read(ssl_, buffer, 2);

        FAIL(readed != 2);
        FAIL(buffer[0] != 'O');
        FAIL(buffer[1] != 'K');
      }
    }

    //
    // Handshake still pending.
    //

    else
    {
      switch(state_)
      {
        //
        // Write turn.
        // Client begins here.
        //
        // -> SSL -> IO -> ...
        //

        case SECURE_STATE_HANDSHAKE_WRITE:
        {
          //
          // Read handshake data from SSL BIO.
          //

          SSL_do_handshake(ssl_);

          readed = BIO_read(writeBio_, buffer, 1024);

          DEBUG3("Readed [%d] bytes from SSL BIO.\n", readed);

          FAIL(readed <= 0);

          //
          // Underlying IO is set to NONE.
          // Pass data needed to be written back to caller.
          //

          if (ioMode_ == SECURE_IOMODE_NONE)
          {
            FAILEX(*customSize < readed,
                       "ERROR: 'customSize' too small in SecureHandshakeStep.\n");

            memcpy(customBuffer, buffer, readed);

            *customSize = readed;

            written = readed;
          }

          // Underlying IO is set.
          // Write back handshake data to underlying IO.
          //

          else
          {
            written = this -> writeRaw(buffer, readed, SECURE_TLS_HANDSHAKE_TIMEOUT);
          }

          DEBUG3("Written [%d] bytes to underlying IO.\n", written);

          FAIL(written <= 0);

          //
          // Handshake finished.
          //

          if (SSL_is_init_finished(ssl_))
          {
            state_ = SECURE_STATE_ESTABLISHED;
          }

          //
          // Handshake pending.
          // Swap turn into read.
          //

          else
          {
            state_ = SECURE_STATE_HANDSHAKE_READ;
          }

          break;
        }

        //
        // Read turn.
        // Server begins here.
        //
        // ... -> IO -> SSL
        //

        case SECURE_STATE_HANDSHAKE_READ:
        {
          //
          // Underlying IO is not set. Use input customBuffer as data
          // readed from underlying IO.
          //

          if (ioMode_ == SECURE_IOMODE_NONE)
          {
            memcpy(buffer, customBuffer, *customSize);

            readed = *customSize;
          }

          //
          // Underlying IO is set.
          // Read handshake data from underlying IO.
          //

          else
          {
            readed = this -> readRaw(buffer, 1024, SECURE_TLS_HANDSHAKE_TIMEOUT);
          }

          DEBUG3("Readed [%d] bytes from underlying IO.\n", readed);

          FAIL(readed <= 0);

          //
          // Redirect readed data to SSL BIO.
          //

          written = BIO_write(readBio_, buffer, readed);

          DEBUG3("Written [%d] bytes to SSL BIO.\n", written);

          FAIL(written <= 0);

          //
          // Handshake finished.
          //

          if (SSL_is_init_finished(ssl_))
          {
            state_ = SECURE_STATE_ESTABLISHED;

            if (intent_ == SECURE_INTENT_CLIENT)
            {
              readed = SSL_read(ssl_, buffer, 2);

              FAIL(readed != 2);
              FAIL(buffer[0] != 'O');
              FAIL(buffer[1] != 'K');
            }
          }

          //
          // Handshake pending.
          // Swap turn into write.
          //

          else
          {
            SSL_do_handshake(ssl_);

            state_ = SECURE_STATE_HANDSHAKE_WRITE;
          }

          break;
        }
      }
    }

    //
    // Server send ecrypted "OK" message if handshake finished.
    //

    if (state_ == SECURE_STATE_ESTABLISHED && intent_ == SECURE_INTENT_SERVER)
    {
      if (ioMode_ == SECURE_IOMODE_NONE)
      {
        *customSize = this -> encrypt(customBuffer, *customSize, "OK", 2);

        DEBUG1("SSL Handshake finished.\n");
      }
      else
      {
        this -> write("OK", 2, SECURE_TLS_HANDSHAKE_TIMEOUT);
      }
    }

    exitCode = 0;

    fail:

    if (exitCode)
    {
      Error("SSL handshake failed.\n");
    }

    DBG_LEAVE("SecureConnection::handshakeStep");

    return exitCode;
  }

  //
  // Handshake step for custom connection (iomode set to none).
  //
  // TIP#1: This function should be used to perform manually handshake when
  //        underlying IO is set to NONE (i.e. custom secure connection).
  //
  // TIP#2: Before call, caller should read data from underlying IO manually
  //        and pass it in {inputBuffer, inputSize} parameters.
  //
  // TIP#3: After call, caller should write data returned in
  //        {outputBuffer, outputSize} to underlying IO manually.
  //
  // Caller algorithm to do handshake manually:
  //
  // while(sc -> state != SECURE_STATE_ESTABLISHED)
  // {
  //   Read data from underlying IO to inputBuffer[].
  //   Call SecureHandshakeStep(inputBuffer, ..., outputBuffer, ...)
  //   Write data from outputBuffer[] to underlying IO.
  // }
  //
  //
  // sc           - secure connection object returned from SecureConnectionCreate() (IN).
  // outputBuffer - data needed to be written to underlying IO by caller (OUT).
  //
  // outputSize   - on input size of outputBuffer[] in bytes, on output number
  //                of bytes written to outputBuffer[] (IN/OUT).
  //
  // inputBuffer  - data readed from underlying IO (IN).
  // inputSize    - size of inputBuffer[] in bytes (IN).
  //
  // RETURNS: 0 if OK.
  //

  int SecureConnection::handshakeStep(void *outputBuffer, int *outputSize,
                                          void *inputBuffer, int inputSize)
  {
    DBG_ENTER("SecureConnection::handshakeStep");

    int exitCode = -1;

    FAIL(this -> handshakeStep(inputBuffer, &inputSize));

    if (this -> state_ != SECURE_STATE_ESTABLISHED)
    {
      FAIL(this -> handshakeStep(outputBuffer, outputSize));
    }

    //
    // Error handler.
    //

    exitCode = 0;

    fail:

    DBG_LEAVE("SecureConnection::handshakeStep");

    return exitCode;
  }

  //
  // Create empty secure connection object.
  // Used internally only.
  //
  // TIP#1: Don't create SecureConnection object directly.
  //        Use one of SecureConnectionCreate() function instead.
  //

  SecureConnection::SecureConnection()
  {
    readBio_  = NULL;
    writeBio_ = NULL;
    ssl_      = NULL;
    sslCtx_   = NULL;

    int intent_ = -1;
    int state_  = -1;
    int ioMode_ = -1;

    readCallback_  = NULL;
    writeCallback_ = NULL;

    ioCtx_ = NULL;

    int fdIn_  = -1;
    int fdOut_ = -1;
    int sock_  = -1;

    refCount_ = 1;

    refCountMutex_.setName("SecureConnection::refCountMutex_");
  }

  //
  // Initialize SSL DTLS connection inside secure connection object.
  //
  // Internal use only by SecureConnectionCreate().
  //
  // RETURNS: 0 if OK.
  //

  int SecureConnection::initSSL(int intent, const char *cert,
                                    const char *privKey, const char *privKeyPass)
  {
    DBG_ENTER("SecureConnection::initSSL");

    int exitCode = -1;

    unsigned char sid[SSL_MAX_SSL_SESSION_ID_LENGTH] ;

    //
    // Init SSL library.
    //

    SSL_library_init();

    //
    // Create SSL context.
    //

    DEBUG3("Creating SSL context...\n");

    sslCtx_ = SSL_CTX_new(DTLSv1_method());

    FAILEX(sslCtx_ == NULL, "ERROR: Cannot create SSL context.\n");

    //
    // Server.
    //

    if (intent == SECURE_INTENT_SERVER)
    {
      //
      // Assign certificate and private key to context.
      //

      DEBUG3("Assigning certificate to SSL context...\n");

      SSL_CTX_use_certificate_chain_file(sslCtx_, cert);

      //
      // Set pass to decode private key if needed.
      //

      if (privKeyPass)
      {
        FAILEX(true, "ERROR: 'privKeyPass' param is obsolete.")
      }

      //
      // Set input files with prviate key and server certificate.
      //

      SSL_CTX_use_PrivateKey_file(sslCtx_, privKey, SSL_FILETYPE_PEM);
      SSL_CTX_use_certificate_file(sslCtx_, cert, SSL_FILETYPE_PEM);

      //
      // Set SINGLE_DH_USE option.
      //

      DEBUG3("Setting SINGLE_DH_USE...\n");

      SSL_CTX_set_options(sslCtx_, SSL_OP_SINGLE_DH_USE);

      //
      // Assign session ID.
      //

      DEBUG3("Setting session id...\n");

      FAIL(SecureRandom(sid, sizeof(sid)));

      SSL_CTX_set_session_id_context(sslCtx_, sid, 4);
    }

    //
    // Allocate SSL object.
    //

    DEBUG3("Allocating SSL object...\n");

    ssl_ = SSL_new(sslCtx_);

    //
    // Server.
    //

    if (intent == SECURE_INTENT_SERVER)
    {
      SSL_set_accept_state(ssl_);
      SSL_set_verify(ssl_, SSL_VERIFY_NONE, NULL);
      SSL_set_session_id_context(ssl_, sid, 4);

      state_ = SECURE_STATE_HANDSHAKE_READ;
    }

    //
    // Client.
    //

    else
    {
      SSL_set_connect_state(ssl_);

      SSL_set_verify(ssl_, SSL_VERIFY_NONE, NULL);

      state_ = SECURE_STATE_HANDSHAKE_WRITE;
    }

    readBio_ = BIO_new(BIO_s_mem());

    BIO_set_nbio(readBio_, 1);

    writeBio_ = BIO_new(BIO_s_mem());

    BIO_set_nbio(writeBio_, 1);

    SSL_set_bio(ssl_, readBio_, writeBio_);

    //
    // Do SSL handshake, if underlying IO available.
    // Otherwise must be handled manually.
    //

    if (ioMode_ != SECURE_IOMODE_NONE)
    {
      while(state_ != SECURE_STATE_ESTABLISHED)
      {
        FAIL(handshakeStep());
      }

      DBG_INFO("SSL Handshake finished.\n");
    }

    //
    // Error handler.
    //

    exitCode = 0;

    fail:

    if (exitCode)
    {
      Error("ERROR: Cannot init SSL connection.\n");
    }

    DBG_LEAVE("SecureConnection::initSSL");

    return exitCode;
  }

  //
  // Increase refference counter.
  //
  // WARNING! Every call to addRef() MUSTS be followed by one release() call.
  //
  // TIP #1: Object will not be destroyed until refference counter is greater
  //         than 0.
  //
  // TIP #2: Don't call destructor directly, use release() instead. If
  //         refference counter achieve 0, object will be destroyed
  //         automatically.
  //

  void SecureConnection::addRef()
  {
    refCountMutex_.lock();

    refCount_ ++;

    DEBUG2("Increased refference counter to %d for SecureConnection PTR#%p.\n",
               refCount_, this);

    refCountMutex_.unlock();
  }

  //
  // Decrease refference counter increased by addRef() before and
  // desroy object when it's refference counter reach 0.
  //

  void SecureConnection::release()
  {
    int deleteNeeded = 0;

    //
    // Decrease refference counter by 1.
    //

    refCountMutex_.lock();

    refCount_ --;

    DEBUG2("Decreased refference counter to %d for SecureConnection PTR#%p.\n",
               refCount_, this);

    if (refCount_ == 0)
    {
      deleteNeeded = 1;
    }

    refCountMutex_.unlock();

    //
    // Delete object if refference counter goes down to 0.
    //

    if (deleteNeeded)
    {
      delete this;
    }
  }

  int SecureConnection::getState()
  {
    return state_;
  }

  //
  // Wrap abstract read/write callbacks into secure connection.
  //
  // intent         - set to SECURE_INTENT_CLIENT or SECURE_INTENT_SERVER (IN).
  // readCallback   - callback used to read from underlying unsecure IO (IN).
  // writeCallback  - callback used to write to underlying unsecure IO (IN).
  // cancelCallback - callback used to cancel pending read on underlying IO (IN).
  // ioCtx          - caller specified context passed to IO callbacks directly (IN).
  // cert           - filename, where server certificate is stored (IN/OPT).
  // privKey        - filename, where server private key is stored (server side
  //                  only) (IN/OPT).
  // privKeyPass    - passphrase to decode private key. Readed from keyboard
  //                  if skipped (IN/OPT).
  //
  // WARNING! Returned pointer MUSTS be freed by SecureConnectionDestroy() if not
  //          needed longer.
  //
  // TIP#1: Use SecureHandshake() to init encrypted SSL connection before
  //        attemp to read or write via created connection.
  //
  // TIP#2: Use SecureWrite to send data via secure connection.
  //
  // TIP#3: Use SecureRead to read data from secure connection.
  //
  //
  // RETURNS: Pointer to new allocated SecureConnectionCreate object
  //          or NULL if error.
  //

  SecureConnection *SecureConnectionCreate(int intent,
                                               SecureReadProto readCallback,
                                                   SecureWriteProto writeCallback,
                                                       void *ioCtx,
                                                           const char *cert,
                                                              const char *privKey,
                                                                  const char *privKeyPass)
  {
    DBG_ENTER("SecureConnectionCreate");

    int exitCode = -1;

    SecureConnection *sc = new SecureConnection;

    sc -> readCallback_   = readCallback;
    sc -> writeCallback_  = writeCallback;
    sc -> ioCtx_          = ioCtx;
    sc -> intent_         = intent;
    sc -> ioMode_         = SECURE_IOMODE_CALLBACKS;

    //
    // Init SSL.
    //

    FAIL(sc -> initSSL(intent, cert, privKey, privKeyPass));

    //
    // Error handler.
    //

    exitCode = 0;

    fail:

    if (exitCode)
    {
      Error("ERROR: Cannot create secure connection.\n");

      sc -> release();

      sc = NULL;
    }

    DBG_LEAVE("SecureConnectionCreate");

    return sc;
  }

  //
  // Wrap FD pair into secure connection.
  //
  // intent      - set to SECURE_INTENT_CLIENT or SECURE_INTENT_SERVER (IN).
  // fdin        - FD used to read data from underlying unsecure IO (IN).
  // fdout       - FD used to write data into underlying unsecure IO (IN).
  // cert        - filename, where server certificate is stored (IN/OPT).
  //
  // privKey     - filename, where server private key is stored (server side
  //               only) (IN/OPT).
  //
  // privKeyPass - passphrase to decode private key, readed from keyboard if
  //               skipped (IN/OPT).
  //
  // WARNING! Returned pointer MUSTS be released release() method when not
  //          needed longer.
  //
  // TIP#1: Use SecureHandshake() to init encrypted SSL connection before
  //        attemp to read or write via created connection.
  //
  // TIP#2: Use SecureWrite to send data via secure connection.
  //
  // TIP#3: Use SecureRead to read data from secure connection.
  //
  //
  // RETURNS: Pointer to new allocated SecureConnectionCreate object
  //          or NULL if error.
  //

  SecureConnection *SecureConnectionCreate(int intent, int fdin, int fdout,
                                               const char *cert, const char *privKey,
                                                   const char *privKeyPass)
  {
    DBG_ENTER("SecureConnectionCreate");

    int exitCode = -1;

    SecureConnection *sc = new SecureConnection;

    sc -> fdIn_   = fdin;
    sc -> fdOut_  = fdout;
    sc -> intent_ = intent;
    sc -> ioMode_ = SECURE_IOMODE_FDS;

    //
    // Init SSL.
    //

    FAIL(sc -> initSSL(intent, cert, privKey, privKeyPass));

    //
    // Error handler.
    //

    exitCode = 0;

    fail:

    if (exitCode)
    {
      Error("ERROR: Cannot create secure connection over FDs #%d/%d.\n", fdin, fdout);

      sc -> release();

      sc = NULL;
    }

    DBG_LEAVE("SecureConnectionCreate");

    return sc;
  }

  //
  // Wrap socket into secure connection.
  //
  // intent      - set to SECURE_INTENT_CLIENT or SECURE_INTENT_SERVER (IN).
  // sock        - socket connected to remote machine (IN)
  // cert        - filename, where server certificate is stored (IN/OPT).
  //
  // privKey     - filename, where server private key is stored (server
  //               side only) (IN/OPT).
  //
  // privKeyPass - passphrase to decode private key, readed from keyboard if
  //               skipped (IN/OPT).
  //
  // WARNING! Returned pointer MUSTS be released release() method when not
  //          needed longer.
  //
  // TIP#1: Use SecureHandshake() to init encrypted SSL connection before
  //        attemp to read or write via created connection.
  //
  // TIP#2: Use SecureWrite to send data via secure connection.
  //
  // TIP#3: Use SecureRead to read data from secure connection.
  //
  //
  // RETURNS: Pointer to new allocated SecureConnectionCreate object
  //          or NULL if error.
  //

  SecureConnection *SecureConnectionCreate(int intent, int sock,
                                               const char *cert, const char *privKey,
                                                   const char *privKeyPass)
  {
    DBG_ENTER("SecureConnectionCreate");

    int exitCode = -1;

    SecureConnection *sc = new SecureConnection;

    sc -> sock_   = sock;
    sc -> intent_ = intent;
    sc -> ioMode_ = SECURE_IOMODE_SOCKET;

    //
    // Init SSL.
    //

    FAIL(sc -> initSSL(intent, cert, privKey, privKeyPass));

    //
    // Error handler.
    //

    exitCode = 0;

    fail:

    if (exitCode)
    {
      Error("ERROR: Cannot create secure connection over SOCKET #%d.\n", sock);

      sc -> release();

      sc = NULL;
    }

    DBG_LEAVE("SecureConnectionCreate");

    return sc;
  }

  //
  // Custom secure connection.
  //
  // intent      - set to SECURE_INTENT_CLIENT or SECURE_INTENT_SERVER (IN).
  //
  // cert        - filename, where server certificate is stored (IN/OPT).
  //
  // privKey     - filename, where server private key is stored (server
  //               side only) (IN/OPT).
  //
  // privKeyPass - passphrase to decode private key, readed from keyboard if
  //               skipped (IN/OPT).
  //
  // WARNING! Returned pointer MUSTS be released release() method when not
  //          needed longer.
  //
  // TIP#1: Use SecureHandshake() to init encrypted SSL connection before
  //        attemp to read or write via created connection.
  //
  // TIP#2: Use SecureWrite to send data via secure connection.
  //
  // TIP#3: Use SecureRead to read data from secure connection.
  //
  //
  // RETURNS: Pointer to new allocated SecureConnectionCreate object
  //          or NULL if error.
  //

  SecureConnection *SecureConnectionCreate(int intent, const char *cert,
                                               const char *privKey,
                                                   const char *privKeyPass)
  {
    DBG_ENTER("SecureConnectionCreate");

    int exitCode = -1;

    SecureConnection *sc = new SecureConnection;

    sc -> intent_ = intent;
    sc -> ioMode_ = SECURE_IOMODE_NONE;

    //
    // Init SSL.
    //

    FAIL(sc -> initSSL(intent, cert, privKey, privKeyPass));

    //
    // Error handler.
    //

    exitCode = 0;

    fail:

    if (exitCode)
    {
      Error("ERROR: Cannot create custom secure connection.\n");

      sc -> release();

      sc = NULL;
    }

    DBG_LEAVE("SecureConnectionCreate");

    return sc;
  }
} /* namespace Tegenaria */
