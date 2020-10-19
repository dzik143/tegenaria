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

#ifndef Tegenaria_Core_IOFifo_H
#define Tegenaria_Core_IOFifo_H

#include <Tegenaria/Mutex.h>
#include <stdint.h>

namespace Tegenaria
{
  #define IOFIFO_DEFAULT_BUFFER_SIZE (1024 * 32)

  #define IO_BIG_ENDIAN    1
  #define IO_LITTLE_ENDIAN 0

  class IOFifo
  {
    private:

    char *buffer_;

    unsigned int bytesLeft_;
    unsigned int capacity_;
    unsigned int writePos_;
    unsigned int readPos_;

    Mutex *mutex_;

    public:

    //
    // Constructors and destructors.
    //

    IOFifo(unsigned int capacity = IOFIFO_DEFAULT_BUFFER_SIZE);

    ~IOFifo();

    //
    // Destructive methods (changes fifo).
    //

    int push(void *src, int len);
    int pop(void *dst, int len = -1, int peekOnly = 0);
    int eat(int len);

    //
    // Non-destructive read (read without changing fifo).
    //

    int peek(void *dst, int len = -1);

    uint64_t peekQword(int endian);
    uint32_t peekDword(int endian);
    uint8_t peekByte();

    //
    // Multithread synchronization.
    //

    void lock();
    void unlock();

    //
    // Getters and setters.
    //

    unsigned int bytesLeft();
    unsigned int size();
    unsigned int capacity();
  };

} /* namespace Tegenaria */

#endif /* Tegenaria_Core_IOFifo_H */
