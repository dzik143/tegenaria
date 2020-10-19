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

#include <cstdio>
#include <cstring>
#include <zlib.h>
#include <Tegenaria/Debug.h>

#define THUNK_SIZE (1024 * 64)

using namespace Tegenaria;

int main(int argc, char **argv)
{
  char source[THUNK_SIZE] = {0};
  char dest[THUNK_SIZE]   = {0};

  int sourceSize = 0;
  int destSize   = 0;
  int readed     = 1;

  FILE *finp = NULL;
  FILE *fout = NULL;

  //
  // Check args.
  //

  if (argc < 4)
  {
    fprintf(stderr, "Usage is:\n"
                    " Compress:   %s -c <input file> <output file>\n"
                    " Decompress: %s -d <input file> <output file>\n",
                    argv[0], argv[0]);

    return -1;
  }

  //
  // Open input and ouput files.
  //

  finp = fopen(argv[2], "rb");
  fout = fopen(argv[3], "wb");

  FAILEX(finp == NULL, "ERROR: Cannot open [%s] file.\n", argv[2]);
  FAILEX(fout == NULL, "ERROR: Cannot create [%s] file.\n", argv[3]);

  //
  // Compress mode.
  //

  if (strcmp(argv[1], "-c") == 0)
  {
    //
    // Compress file by thunks.
    //

    DBG_INFO("Compressing file %s into %s...\n", argv[2], argv[3]);

    while(readed > 0)
    {
      //
      // Read up to THUNK_SIZE from uncompressed file.
      //

      readed = fread(source, 1, sizeof(source), finp);

      //
      // Compress thunk.
      //

      sourceSize = readed;
      destSize   = sizeof(dest);

      compress((Bytef *) dest, (uLongf *) &destSize,
                   (Bytef *) source, (uLong) sourceSize);

      //
      // Write <size><compressed data> thunk to file.
      //

      fwrite(&destSize, 1, sizeof(destSize), fout);
      fwrite(dest, 1, destSize, fout);
    }
  }

  //
  // Decompress mode.
  //

  else
  {
    while(readed > 0)
    {
      //
      // Read <thunkSize><data> from compressed file.
      //

      readed = fread(&sourceSize, sizeof(sourceSize), 1, finp);

      if (readed > 0)
      {
        readed = fread(source, sourceSize, 1, finp);

        //
        // Decompress thunk.
        //

        destSize   = sizeof(dest);

        uncompress((Bytef *) dest, (uLongf *) &destSize,
                       (Bytef *) source, (uLong) sourceSize);

        //
        // Write uncompressed data thunk to file.
        //

        fwrite(dest, 1, destSize, fout);
      }
    }
  }

  //
  // Clean up.
  //

  fail:

  if (finp)
  {
    fclose(finp);
  }

  if (fout)
  {
    fclose(fout);
  }

  return 0;
}
