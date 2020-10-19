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
#include <cstdio>

using namespace Tegenaria;

//
// Encode example values into binary packet:
//
// offset  length   descritpion
// 0       1        some 8-bit value
// 1       4        some 32-bit value
// 5       8        some 64-bit value
// 13      4        size of string
// 17      ...      string data with zero terminator
//

void EncodePacket(string &packet,
                      uint8_t value8,
                      uint32_t value32,
                          uint64_t value64,
                              const char *valueString)
{
  StrPushByte(packet, value8);
  StrPushDword(packet, value32);
  StrPushQword(packet, value64);
  StrPushString(packet, valueString);

  printf("Encoded packet is:\n");
  printf("  value8  : [%d].\n", value8);
  printf("  value32 : [%d].\n", value32);
  printf("  value64 : [%"PRId64"].\n", value64);
  printf("  string  : [%s].\n\n", valueString);
}

//
// Decode packet back. This function emulate decoding packet
// e.g. just after receiving it from network, where we get raw {buf, len}
// buffer.
//

int DecodePacket(const void *buf, int len)
{
  int exitCode = -1;

  uint8_t value8   = 0;
  uint32_t value32 = 0;
  uint64_t value64 = 0;

  const char *valueString = NULL;

  //
  // Pointer to track current position in buffer.
  //

  char *it = (char *) buf;

  int bytesLeft = len;

  //
  // Decode.
  //

  FAIL(StrPopByte(&value8, &it, &bytesLeft));
  FAIL(StrPopDword(&value32, &it, &bytesLeft));
  FAIL(StrPopQword(&value64, &it, &bytesLeft));
  FAIL(StrPopString(&valueString, NULL, &it, &bytesLeft));

  //
  // Print back decoded packet.
  //

  printf("Decoded packet is:\n");
  printf("  value8  : [%d].\n", value8);
  printf("  value32 : [%d].\n", value32);
  printf("  value64 : [%"PRId64"].\n", value64);
  printf("  string  : [%s].\n\n", valueString);

  //
  // Error handler.
  //

  exitCode = 0;

  fail:

  if (exitCode)
  {
    Error("ERROR: Cannot decode packet.\n");
  }
}

int main(int argc, char **argv)
{
  string packet;

  //
  // Encode some data into packet.
  //

  EncodePacket(packet, 1, 2, 3, "hello");

  //
  // Decode it back.
  //

  DecodePacket(packet.c_str(), packet.size());

  return 0;
}
