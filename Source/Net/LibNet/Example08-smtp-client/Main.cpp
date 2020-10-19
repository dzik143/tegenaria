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

#include <Tegenaria/Debug.h>
#include <Tegenaria/Net.h>
#include <Tegenaria/Secure.h>

using namespace Tegenaria;

void ReadField(char *dst, int dstSize, const char *prompt)
{
  printf("%s: ", prompt);

  fgets(dst, dstSize, stdin);

  for (int i = 0; dst[i]; i++)
  {
    if (dst[i] == 10 || dst[i] == 13)
    {
      dst[i] = 0;
    }
  }
}

//
// Entry point.
//

int main(int argc, char *argv[])
{
  DBG_INIT_EX(NULL, "debug2", -1);

  char smtpHost[128];
  char smtpPort[32];
  char smtpLogin[64];
  char smtpPass[64];
  char message[128];
  char subject[128];
  char fromFull[128];
  char from[128];
  char to[128];

  int ret = -1;

  vector<string> receivers;

  //
  // Read fields from stdin.
  //

  ReadField(smtpHost, sizeof(smtpHost), "SMTP Host (e.g. smtp.wp.pl)");
  ReadField(smtpPort, sizeof(smtpPort), "SMTP Port (default is 25)");
  ReadField(smtpLogin, sizeof(smtpLogin), "SMTP Login");

  SecureReadPassword(smtpPass, sizeof(smtpPass), "SMTP Password: ");

  ReadField(fromFull, sizeof(fromFull), "From full name (e.g. John Smith)");
  ReadField(from, sizeof(from), "From address");
  ReadField(to, sizeof(to), "To address");
  ReadField(subject, sizeof(subject), "Subject");
  ReadField(message, sizeof(message), "Message");

  //
  // Put one receiver to list.
  //

  receivers.push_back(to);

  //
  // Send mail.
  //

  ret = NetSmtpSendMail(smtpHost, atoi(smtpPort), from, fromFull,
                            subject, receivers, message, smtpLogin, smtpPass);

  if (ret == 0)
  {
    printf("Success.\n");
  }

  return ret;
}
