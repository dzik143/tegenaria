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

#include <cstdlib>
#include <Tegenaria/Reg.h>

using namespace Tegenaria;

int main()
{
  DWORD fileSystem = 0;

  char biosDate[1024] = {0};

  list<string> securityPackages;
  list<string> exampleList;

  //
  // Read some example keys from registry.
  //

  printf("Read test\n");
  printf("---------\n\n");

  RegGetDword(&fileSystem, HKEY_LOCAL_MACHINE,
                  "SYSTEM\\CurrentControlSet\\Control\\FileSystem",
                      "Win95TruncatedExtensions");

  RegGetString(biosDate, sizeof(biosDate), HKEY_LOCAL_MACHINE,
                   "SYSTEM\\CurrentControlSet\\Control\\BiosInfo",
                       "SystemBiosDate");

  RegGetStringList(securityPackages, HKEY_LOCAL_MACHINE,
                       "SYSTEM\\CurrentControlSet\\Control\\Lsa",
                           "Security Packages");

  //
  // Print readed keys.
  //

  printf("FileSystem : [%d]\n", fileSystem);
  printf("BiosDate   : [%s]\n\n", biosDate);

  printf("Security packages:\n");

  for (list<string>::iterator it = securityPackages.begin();
           it != securityPackages.end(); it++)
  {
    printf("  '%s'\n", it -> c_str());
  }

  //
  // Write example keys:
  //
  // HKEY_LOCAL_MACHINE\Software\Dirligo\dwordExample
  // HKEY_LOCAL_MACHINE\Software\Dirligo\stringExample
  // HKEY_LOCAL_MACHINE\Software\Dirligo\stringListExample
  //

  printf("\nWrite test\n");
  printf("----------\n\n");

  exampleList.push_back("Some example string");
  exampleList.push_back("Some another example string");
  exampleList.push_back("Bla bla bla");

  RegSetDword(HKEY_LOCAL_MACHINE, "Software\\Dirligo", "dwordExample", 1234);
  RegSetString(HKEY_LOCAL_MACHINE, "Software\\Dirligo", "stringExample", "example");
  RegSetStringList(HKEY_LOCAL_MACHINE, "Software\\Dirligo", "stringListExample", exampleList);

  printf("Open regedit and go to HKEY_LOCAL_MACHINE\\Software\\Dirligo.\n");

  //
  // Remove test.
  //

  getchar();

  printf("Press enter to remove HKEY_LOCAL_MACHINE\\Software\\Dirligo.\n");

  RegRemove(HKEY_LOCAL_MACHINE, "Software\\Dirligo");

  return 0;
}
