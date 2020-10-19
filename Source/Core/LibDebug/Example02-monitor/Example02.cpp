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
// Enable monitoring process resources in state.<pid> file
// if not in qcbuild.src.
//

#ifndef DEBUG_STATE
#define DEBUG_STATE
#endif

#include <Tegenaria/Debug.h>
#include <cstdlib>
#include <ctime>

using namespace Tegenaria;

int main()
{
  DBG_INIT_EX("example.log", DBG_LEVEL_DEBUG3, DBG_STATE_ENABLE);

  DBG_HEAD("LibDebug - Monitoring resources example");

  DBG_ENTER("main");

  //
  // Example sets of objects and example human readable names
  // to use instead of raw pointers/IDs.
  // These names are arbitral.
  //
  // Object name is optional, can be NULL or skipped.
  // If object name not specified raw ID is showed in state file.
  //

  const char *setNames[]    = {"socket", "thread", "session", "process"};
  const char *objectNames[] = {"IN", "OUT", "Listen", "Master", "Slave", NULL};

  const int setNamesSize    = 4;
  const int objectNamesSize = 6;

  srand(time(0));

  //
  // Simulate creating/deleting random objects.
  //

  for (int i = 0; i < 32; i++)
  {
    //
    // Randomize object type socket, thread, etc.
    //

    const char *setName = setNames[rand() % setNamesSize];

    //
    // Randomize optional object name.
    //

    const char *objectName = objectNames[rand() % objectNamesSize];

    //
    // Randomize object ID. For object it can be this pointer.
    //

    int id = rand() % 10;

    //
    // Create fake object.
    //

    if (rand() % 2)
    {
      DBG_SET_ADD(setName, id, objectName);
    }

    //
    // Delete fake object.
    //

    else
    {
      DBG_SET_DEL(setName, id);
    }
  }

  DBG_SET_DUMP();

  DBG_LEAVE("main");

  return 0;
}
