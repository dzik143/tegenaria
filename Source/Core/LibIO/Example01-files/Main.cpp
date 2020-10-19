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

#include <Tegenaria/IOMixer.h>
#include <Tegenaria/Debug.h>
#include <fcntl.h>

using namespace Tegenaria;

//
// Usage:
//
// 'libio-example'    : dump four inputs mixed into one 'master' file.
// 'libio-example -r' : reconstruct original four inputs from 'master'.

int main(int argc, char **argv)
{
  char buf[32] = {0};

  DBG_INIT("iomixer.log");

  IOMixer *iomixer = NULL;

  //
  // No args.
  // Mix outputs from 4 slaves into one master:
  //
  //
  // slave1 \
  // slave2 -> master ->
  // slave3 /
  //
  // For more details see IOMixer.cpp.
  //

  if (argc < 2)
  {
    DBG_HEAD
    (
      "iomixer.log\n\n"
      "Pack 'N to 1' example.\n"
    );


    //
    // Redirect 'master-output' to file.
    // No master input.
    //

    int masterOut = open("master.out", O_WRONLY | O_CREAT | O_TRUNC, 666);
    int masterIn  = -1;

    //
    // Wrap IOMixer over master FDs.
    //

    iomixer = new IOMixer(masterOut, masterIn);

    //
    // Declare four slaves.
    //

    int slave1[2] = {-1, -1};
    int slave2[2] = {-1, -1};
    int slave3[2] = {-1, -1};
    int slave4[2] = {-1, -1};

    int id1 = iomixer -> addSlave(slave1);
    int id2 = iomixer -> addSlave(slave2);
    int id3 = iomixer -> addSlave(slave3);
    int id4 = iomixer -> addSlave(slave4);

    //
    // Start master thread.
    // From this time IOMixer start splitting master data
    // into four slaves.
    //

    iomixer -> start();

    //
    // Write test data on every slaves input.
    // All messages will be mixed in output 'master' file.
    //

    printf("Mixing four 'slave' channels into 'master.out' file...\n");

    write(slave1[1], "Hello from slave0", 17);
    write(slave2[1], "Hello from slave1", 17);
    write(slave3[1], "Hello from slave2", 17);
    write(slave4[1], "Hello from slave3", 17);

    //
    // Wait until all threads finished work.
    //

    iomixer -> join();
  }

  //
  // '-r' specified.
  // Rconstructs original four inputs from 'master' file.
  //
  //
  //           / slave1
  // -> master - slave2
  //           \ slave3
  //

  else
  {
    DBG_HEAD
    (
      "iomixer.log\n\n"
      "Split '1 to N' example.\n"
    );

    //
    // Redirect 'master-input' to file.
    // No master output.
    //

    int masterOut = -1;
    int masterIn  = open("master.out", O_RDONLY);

    //
    // Wrap IOMixer over master FDs.
    //

    iomixer = new IOMixer(masterOut, masterIn);

    //
    // Declare four slaevs.
    //

    int slave1[2] = {-1, -1};
    int slave2[2] = {-1, -1};
    int slave3[2] = {-1, -1};
    int slave4[2] = {-1, -1};

    int id1 = iomixer -> addSlave(slave1);
    int id2 = iomixer -> addSlave(slave2);
    int id3 = iomixer -> addSlave(slave3);
    int id4 = iomixer -> addSlave(slave4);

    //
    // Start master thread.
    // From this time IOMixer start splitting master data
    // into four slaves.
    //

    iomixer -> start();

    //
    // Read data from four slaves.
    //

    char buf1[32] = {0};
    char buf2[32] = {0};
    char buf3[32] = {0};
    char buf4[32] = {0};

    printf("Splitting 'master.out' input into four channels...\n");

    read(slave1[0], buf1, sizeof(buf1));
    read(slave2[0], buf2, sizeof(buf2));
    read(slave3[0], buf3, sizeof(buf3));
    read(slave4[0], buf4, sizeof(buf4));

    printf("Readed [%s] from slave1.\n", buf1);
    printf("Readed [%s] from slave2.\n", buf2);
    printf("Readed [%s] from slave3.\n", buf3);
    printf("Readed [%s] from slave4.\n", buf4);

    //
    // Wait until all threads finished work.
    //

    iomixer -> join();
  }

  //
  // Release IOMixer object.
  //

  iomixer -> release();

  return 0;
}
