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
// Example shows how to use access list object.
//

#include <Tegenaria/Secure.h>

using namespace Tegenaria;

//
// Entry point.
//

int main(int argc, char **argv)
{
  SecureAcl acl;

  //
  // Test #1.
  //
  // Grant rights:
  //   - R (read) for user jozek
  //   - F (full) for user janek
  //
  // Others (not included in list explicite) has default access denied.
  //

  printf("Test #1\n");
  printf("-------\n");

  acl.setRights("jozek", "R");
  acl.setRights("janek", SECURE_ACL_FULL);

  printf("Jozek has rights  : [%s].\n", acl.getRightsString("jozek").c_str());
  printf("Janek has rights  : [%s].\n", acl.getRightsString("janek").c_str());
  printf("Stefan has rights : [%s].\n", acl.getRightsString("stefan").c_str());
  printf("ACL string is     : [%s].\n", acl.toString().c_str());

  //
  // Test #2.
  //
  // Init access list object from raw accesslist.
  // We grant below rights is acl string:
  // - R (read) to jozek
  // - F (full) to janek
  // - R (read) to others
  //

  printf("\nTest #2\n");
  printf("-------\n");

  acl.initFromString("jozek:R;janek:F;*:R;");

  printf("Jozek has rights  : [%s].\n", acl.getRightsString("jozek").c_str());
  printf("Janek has rights  : [%s].\n", acl.getRightsString("janek").c_str());
  printf("Stefan has rights : [%s].\n", acl.getRightsString("stefan").c_str());
  printf("ACL string is     : [%s].\n", acl.toString().c_str());

  return 0;
}
