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
// Purpose: Client to send mail via SMTP server.
//

#pragma qcbuild_set_file_title("SMTP client (email)")

#include "Net.h"
#include "NetInternal.h"

namespace Tegenaria
{
  #define SMTP_EHLO        "EHLO Dirligo\r\n"
  #define SMTP_AUTH_LOGIN  "AUTH LOGIN\r\n"
  #define SMTP_DATA        "DATA\r\n"
  #define SMTP_END         "\r\n.\r\n"
  #define SMTP_RSET        "RSET\r\n"
  #define SMTP_QUIT        "QUIT\r\n"

  #define SMTP_HEAD_FMT    "From: %s <%s>\r\n"                       \
                           "To: <%s>\r\n"                            \
                           "Subject: %s\r\n"                         \
                           "Content-Type: text/plain\r\n"            \
                           "Mime-Version: 1.0\r\n"                   \
                           "X-Mailer: Dirligo\r\n"                   \
                           "Content-Transfer-Encoding: 7bit\r\n\r\n"

  #define SMTP_BASE64_USER "334 VXNlcm5hbWU6"
  #define SMTP_BASE64_PASS "334 UGFzc3dvcmQ6"

  //
  // Read SMTP server response in 'XXX message' format.
  //
  // smtpCode - SMTP response code (OUT).
  // msg      - buffer, where to put server message (OUT).
  // msgSize  - size of msg[] buffer in bytes (IN).
  // nc       - pointer NetConnection object connected to SMTP server (IN).
  //
  // RETURNS: 0 if OK.
  //

  int NetSmtpReadServerAnswer(int *smtpCode, char *msg,
                                  int msgSize, NetConnection *nc)
  {
    DBG_ENTER3("NetSmtpReadServerAnswer");

    int exitCode = -1;

    int smtpCodeGroup = 0;

    msg[0] = 0;

    //
    // Read message.
    //

    int readed = nc -> read(msg, msgSize);

    FAIL(readed <= 0);

    msg[readed - 1] = 0;

    //
    // Decode SMTP code.
    //

    *smtpCode = atoi(msg);

    DEBUG2("Received SMTP message [%s].\n", msg);

    //
    // Get first digit from 3-digits SMTP code.
    // 2xx and 3xx means success.
    // 4xx and 5xx means error.
    //

    smtpCodeGroup = (*smtpCode) / 100;

    FAIL(smtpCodeGroup != 2 && smtpCodeGroup != 3);

    //
    // Error handler.
    //

    exitCode = 0;

    fail:

    if (exitCode)
    {
      Error("ERROR: Cannot read answer from SMTP server or server fail.\n"
                "SMTP message is : '%s'.\n", msg);
    }

    DBG_LEAVE3("NetSmtpReadServerAnswer");

    return exitCode;
  }

  //
  // Send email using existing SMTP server.
  //
  // smtpHost  - hostname of smtp server to use e.g. "smtp.wp.pl" (IN).
  // smtpPort  - port, where smtp server listening. Default is 25. (IN).
  // from      - from field in email e.g. login@wp.pl (IN).
  // subject   - subject field in email (IN).
  // receivers - list of destination email addresses (IN).
  // message   - message field in email (IN).
  // login     - login to authenticate on smtp server (IN).
  // password  - password to authenticate on smtp server (IN).
  //
  // RETURNS: 0 if OK,
  //         -1 otherwise.
  //

  int NetSmtpSendMail(const char *smtpHost, int smtpPort, const char *from,
                              const char *fromFull, const char *subject,
                                  vector<string> receivers, const char *message,
                                      const char *login, const char *password)
  {
    DBG_ENTER2("NetSmtpSendMail");

    int exitCode = -1;

    NetConnection *nc = NULL;

    string body;
    string hash;

    char buf[512];

    int smtpCode = -1;
    int written  = -1;
    int readed   = -1;
    int len      = -1;

    //
    // Check args.
    //

    FAILEX(smtpHost == NULL,  "ERROR: 'smtpHost' cannot be NULL in NetSmtpSendMail()");
    FAILEX(from == NULL,      "ERROR: 'from' cannot be NULL in NetSmtpSendMail()");
    FAILEX(fromFull == NULL,  "ERROR: 'fromFull' cannot be NULL in NetSmtpSendMail()");
    FAILEX(subject == NULL,   "ERROR: 'subject' cannot be NULL in NetSmtpSendMail()");
    FAILEX(message == NULL,   "ERROR: 'message' cannot be NULL in NetSmtpSendMail()");
    FAILEX(login == NULL,     "ERROR: 'login' cannot be NULL in NetSmtpSendMail()");
    FAILEX(password == NULL,  "ERROR: 'password' cannot be NULL in NetSmtpSendMail()");
    FAILEX(receivers.empty(), "ERROR: 'receivers' vector cannot be empty in NetSmtpSendMail()");

    //
    // Connect to SMTP server.
    //

    nc = NetConnect(smtpHost, smtpPort);

    FAIL(nc == NULL);

    //
    // Get server hello.
    //

    FAIL(NetSmtpReadServerAnswer(&smtpCode, buf, sizeof(buf), nc));

    //
    // Send client hello message.
    //

    written = nc -> write(SMTP_EHLO, sizeof(SMTP_EHLO) - 1);

    DEBUG2("NetSmtpSendMail: Sent [%s].\n", SMTP_EHLO);

    FAIL(written < 0);

    //
    // Server should reply with list of known auth methods.
    //

    FAIL(NetSmtpReadServerAnswer(&smtpCode, buf, sizeof(buf), nc));

    //
    // Authenticate.
    //   Client: AUTH LOGIN
    //   Server: VXNlcm5hbWU6  <- ask for base64(username)
    //   Client: base64(login)
    //   Server: UGFzc3dvcmQ6  <- ask for base64(pass)
    //   Client: base64(pass)
    //

    written = nc -> write(SMTP_AUTH_LOGIN, sizeof(SMTP_AUTH_LOGIN) - 1);

    DEBUG2("NetSmtpSendMail: Sent AUTH LOGIN.\n");

    for (int i = 0; i < 2; i++)
    {
      FAIL(NetSmtpReadServerAnswer(&smtpCode, buf, sizeof(buf), nc));

      //
      // Username request.
      //

      if (strncmp(buf, SMTP_BASE64_USER, sizeof(SMTP_BASE64_USER) - 1) == 0)
      {
        hash = NetBase64(login, strlen(login));

        len = snprintf(buf, sizeof(buf), "%s\r\n", hash.c_str());

        nc -> write(buf, len);

        DEBUG2("NetSmtpSendMail: Sent username.\n");
      }

      //
      // Password request.
      //

      else if (strncmp(buf, SMTP_BASE64_PASS, sizeof(SMTP_BASE64_PASS) - 1) == 0)
      {
        hash = NetBase64(password, strlen(password));

        len = snprintf(buf, sizeof(buf), "%s\r\n", hash.c_str());

        nc -> write(buf, len);

        DEBUG2("NetSmtpSendMail: Sent password.\n");
      }

      //
      // Unexpected message request.
      //

      else
      {
        Error("Unexpected SMTP message in AUTH LOGIN protocol [%s]\n", buf);

        goto fail;
      }
    }

    FAIL(NetSmtpReadServerAnswer(&smtpCode, buf, sizeof(buf), nc));

    //
    // Send 'MAIL FROM: <from>'.
    //

    len = snprintf(buf, sizeof(buf) - 1, "MAIL FROM: <%s>\r\n", from);

    written = nc -> write(buf, len);

    FAIL(written <= 0);

    DEBUG2("NetSmtpSendMail: Sent [%s]\n", buf);

    FAIL(NetSmtpReadServerAnswer(&smtpCode, buf, sizeof(buf), nc));

    //
    // Send 'RCPT TO: <address>\r\n' for each receiver address.
    //

    for (int i = 0; i < receivers.size(); i++)
    {
      len = snprintf(buf, sizeof(buf) - 1, "RCPT TO: <%s>\r\n", receivers[i].c_str());

      written = nc -> write(buf, len);

      FAIL(written <= 0);

      DEBUG2("NetSmtpSendMail: Sent [%s]\n", buf);

      FAIL(NetSmtpReadServerAnswer(&smtpCode, buf, sizeof(buf), nc));
    }

    //
    // Send 'DATA' to begin email body.
    //

    written = nc -> write(SMTP_DATA, sizeof(SMTP_DATA) - 1);

    FAIL(written <= 0);

    DEBUG2("NetSmtpSendMail: Sent DATA.\n");

    FAIL(NetSmtpReadServerAnswer(&smtpCode, buf, sizeof(buf), nc));

    //
    // Send body header.
    //

    len = snprintf(buf, sizeof(buf), SMTP_HEAD_FMT,
                       fromFull, from, receivers[0].c_str(), subject);

    written = nc -> write(buf, len);

    FAIL(written <= 0);

    DEBUG2("NetSmtpSendMail: Sent header [%s].\n", buf);

    //
    // Send message part.
    //

    written = nc -> write(message, strlen(message));

    FAIL(written <= 0);

    DEBUG2("NetSmtpSendMail: Sent message body.\n");

    //
    // Send '\r\n.\r\n' to finish DATA part.
    //

    nc -> write(SMTP_END, sizeof(SMTP_END) - 1);

    DEBUG2("NetSmtpSendMail: Sent END tag.\n");

    FAIL(NetSmtpReadServerAnswer(&smtpCode, buf, sizeof(buf), nc));

    //
    // Send RSET command.
    //

    nc -> write(SMTP_RSET, sizeof(SMTP_RSET) - 1);

    DEBUG2("NetSmtpSendMail: Sent RSET.\n");

    FAIL(NetSmtpReadServerAnswer(&smtpCode, buf, sizeof(buf), nc));

    //
    // Send QUIT command.
    //

    nc -> write(SMTP_QUIT, sizeof(SMTP_QUIT) - 1);

    DEBUG2("NetSmtpSendMail: Sent QUIT.\n");

    FAIL(NetSmtpReadServerAnswer(&smtpCode, buf, sizeof(buf), nc));

    //
    // Clean up.
    //

    exitCode = 0;

    fail:

    if (exitCode)
    {
      Error("ERROR: Cannot send email.\n"
                "Error code is : %d.\nSMTP code is : %d.\n",
                    GetLastError(), smtpCode);
    }

    if (nc)
    {
      nc -> release();
    }

    DBG_LEAVE2("NetSmtpSendMail");

    return exitCode;
  }
} /* namespace Tegenaria */
