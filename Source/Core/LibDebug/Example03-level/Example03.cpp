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
#include <cstdlib>

using namespace Tegenaria;

//
// Entry point.
//

int main(int argc, char **argv)
{
  const int level     = -1;   // log level to set.
  const int flags     = -1;   // combination of DBG_XXX flags from Debug.h.
  const char *logFile = NULL; // write log on stderr.

  char buffer[] = {1,   2,  3,  4,  5,  6,  7,  8,
                   9,  10, 11, 12, 13, 14, 15, 16,
                   17, 18, 19, 20, 21, 22, 23, 24};

  //
  // Get log level from argv[1].
  //

  if (argc < 2)
  {
    fprintf(stderr, "Usage2: %s<log level number from 0 to 7>\n", argv[0]);
    fprintf(stderr, "Usage1: %s{none|error|info|debug1|debug2|debug3|debug4|debug5}\n", argv[0]);

    goto fail;
  }

  //
  // Init log with given level and default flags.
  //

  DBG_INIT_EX(logFile, argv[1], -1);

  DBG_ENTER("main");

  //
  // DBG_LEVEL_NONE
  //
  // Enabled: Fatal.
  //
  // Comment out to test.
  // Write error message and exit whole application.
  //

  /*
  Fatal("FATAL ERROR: You're too big sucker to use this program.\n");
  */

  //
  // DBG_LEVEL_ERROR
  //
  // Enabled: Error().
  //
  // Error() macro writes message twice:
  // - to log file if set in DBG_INIT().
  // - to stderr.
  //

  Error("ERROR: Something wrong is here.\n");

  //
  // DBG_LEVEL_INFO.
  //
  // Enabled: DBG_HEAD, DBG_INFO
  //

  DBG_HEAD("LibDebug - log level example");
  DBG_INFO("Info: New sucker arrived.");

  //
  // DBG_LEVEL_DEBUG1.
  //
  // Enable: DEBUG1, DBG_MSG, DBG_MSG1,
  //         DBG_HEAD1,
  //         DBG_ENTER, DBG_ENTER1,
  //         DBG_LEAVE, DBG_LEAVE1
  //

  DEBUG1("Debug1: Some debug1 message.");
  DBG_MSG("Debug1: Another debug1 message.");
  DBG_MSG1("Debug1: Another debug1 message.");

  //
  // DBG_LEVEL_DEBUG2
  //
  // Enable: DEBUG2, DBG_MSG2,
  //         DBG_HEAD2,
  //         DBG_ENTER2,
  //         DBG_LEAVE2
  //

  DEBUG2("Debug2: Some level debug2 message.");
  DBG_MSG2("Debug2: Some level debug2 message.");

  //
  // DBG_LEVEL_DEBUG3
  //
  // Enable:
  //   DEBUG3, DBG_MSG3,
  //   DBG_HEAD3
  //   DBG_ENTER3,
  //   DBG_LEAVE3
  //

  DEBUG3("Debug3: Some level debug3 message.");
  DBG_MSG3("Debug3: Some level debug3 message.");
  DBG_DUMP(buffer, sizeof(buffer));

  //
  // DBG_LEVEL_DEBUG4
  //
  // Enable:
  //   DEBUG4, DBG_MSG4,
  //   DBG_HEAD4
  //   DBG_ENTER4,
  //   DBG_LEAVE4
  //

  DEBUG4("Debug4: Some level debug4 message.");
  DBG_MSG4("Debug4: Some level debug4 message.");

  //
  // DBG_LEVEL_DEBUG5
  //
  // Enable:
  //   DEBUG5, DBG_MSG5,
  //   DBG_HEAD5
  //   DBG_ENTER5,
  //   DBG_LEAVE5,
  //   DBG_DUMP
  //

  DEBUG5("Debug5: Some level debug5 message.");
  DBG_MSG5("Debug5: Some level debug5 message.");

  DBG_DUMP(buffer, sizeof(buffer));

  //
  // Error handler.
  // We get here from FAIL and FAILEX macros.
  //

  fail:

  DBG_LEAVE("main");

  return 0;
}
