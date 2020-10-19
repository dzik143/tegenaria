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

#include <Tegenaria/Str.h>
#include <Tegenaria/Error.h>
#include <cstdio>

using namespace Tegenaria;

void ReadField(char *dst, int dstSize, const char *prompt)
{
  printf("%s", prompt);

  fgets(dst, dstSize, stdin);

  for (int i = 0; dst[i]; i++)
  {
    if (dst[i] == 10 || dst[i] == 13)
    {
      dst[i] = 0;
    }
  }
}

int main(int argc, char **argv)
{
  char login[32];
  char email[32];
  char pass[32];

  //
  // Read fields from stdin.
  //

  ReadField(login, sizeof(login), "Enter login: ");
  ReadField(email, sizeof(email), "Enter email: ");
  ReadField(pass, sizeof(pass), "Enter password: ");

  //
  // Verify email.
  //

  switch(StrEmailVerify(email))
  {
    case ERR_OK:                       printf("Email OK.\n"); break;
    case ERR_VERIFY_WRONG_FORMAT:      printf("Email has wrong format.\n"); break;
    case ERR_VERIFY_ALIAS_FORBIDDEN:   printf("Email can't contain + aliases.\n"); break;
    case ERR_VERIFY_COMMENT_FORBIDDEN: printf("Email can't contain () comment(s).\n"); break;
  }

  //
  // Verify login.
  //

  switch(StrLoginVerify(login))
  {
    case ERR_OK:                  printf("Login OK.\n"); break;
    case ERR_VERIFY_WRONG_FORMAT: printf("Login has wrong format.\n"); break;
    case ERR_VERIFY_WRONG_CHAR:   printf("Login contains not allowed characters.\n"); break;
    case ERR_VERIFY_TOO_SHORT:    printf("Login too short.\n"); break;
    case ERR_VERIFY_TOO_LONG:     printf("Login too long.\n"); break;
  }

  //
  // Verify password.
  //

  switch(StrPassVerify(pass))
  {
    case ERR_OK:                  printf("Password OK.\n"); break;
    case ERR_VERIFY_WRONG_FORMAT: printf("Password has wrong format.\n"); break;
    case ERR_VERIFY_WRONG_CHAR:   printf("Password contains not allowed characters.\n"); break;
    case ERR_VERIFY_TOO_SHORT:    printf("Password too short.\n"); break;
    case ERR_VERIFY_TOO_LONG:     printf("Password too long.\n"); break;
  }

  //
  // Compute password strength.
  //

  printf("Password strength is %d.\n", StrPassStrength(pass));

  return 0;
}
