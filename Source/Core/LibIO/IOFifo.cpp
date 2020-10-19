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
// Purpose: Cyclic buffer to store I/O buffer.
//
//          Incoming data is appended to the end.
//          Outcoming data is popped from the begin.
//
//          xx xx xx xx xx xx xx ... yy yy yy yy yy
//
//          ^^^                                 ^^^
//      Read position.                     Write position.
//      Pop data from here.                Push data here.
//

#include <cstring>
#include <cstdlib>

#include "IOFifo.h"
#include <Tegenaria/Debug.h>

namespace Tegenaria
{
  // ----------------------------------------------------------------------------
  //
  //                         Constructors and destructors
  //
  // ----------------------------------------------------------------------------

  //
  // Create new IOFifo with given capacity.
  //
  // capacity - size of fifo in bytes (IN).
  //

  IOFifo::IOFifo(unsigned int capacity)
  {
    DBG_SET_ADD("IOFifo", this);

    buffer_    = (char *) malloc(capacity);
    capacity_  = capacity;
    bytesLeft_ = capacity;
    writePos_  = 0;
    readPos_   = 0;

    mutex_ = new Mutex("IOFifo");
  }

  //
  // Free buffers allocated in constructor.
  //

  IOFifo::~IOFifo()
  {
    DBG_SET_DEL("IOFifo", this);

    if (buffer_)
    {
      free(buffer_);

      buffer_ = NULL;
    }

    if (mutex_)
    {
      delete mutex_;

      mutex_ = NULL;
    }
  }

  // ----------------------------------------------------------------------------
  //
  //                       Destructive functions (push/pop)
  //
  // ----------------------------------------------------------------------------

  //
  // Add data to the end of FIFO.
  //
  // Buffer before: xx xx xx xx xx
  // Buffer after : xx xx xx xx xx yy yy yy yy ...
  //
  // source - source buffer with data to append (IN).
  // len    - number of bytes to append (IN).
  //
  // RETURNS: 0 if all bytes appended,
  //          -1 otherwise.
  //

  int IOFifo::push(void *source, int len)
  {
    DBG_ENTER3("IOFifo::push");

    int exitCode = -1;

    char *src = (char *) source;

    //
    // Check is it sufficient space left.
    //

    FAILEX(len > bytesLeft_,
               "ERROR: Going to append [%d] bytes to IOFifo PTR#%p,"
                   " but only [%d] bytes left.\n", len, this, bytesLeft_);

    //
    // Write data to buffer.
    //

    if ((writePos_ + len) < capacity_)
    {
      memcpy(buffer_ + writePos_, src, len);
    }
    else
    {
      unsigned int bytesToEnd = capacity_ - writePos_;

      unsigned int overflow = (len - bytesToEnd) % capacity_;

      memcpy(buffer_ + writePos_, src, bytesToEnd);

      memcpy(buffer_, src + bytesToEnd, overflow);
    }

    writePos_ = (writePos_ + len) % capacity_;

    bytesLeft_ -= len;

    DEBUG2("Appended [%d] bytes to IOFifo PTR#%p,"
               " [%d] bytes left.\n", len, this, bytesLeft_);

    //
    // Error handler.
    //

    exitCode = 0;

    fail:

    DBG_LEAVE3("IOFifo::push");

    return exitCode;
  }

  //
  // Pop data from the begin of FIFO.
  //
  // Buffer before: xx xx xx xx yy yy yy yy ...
  // Buffer after :             yy yy yy yy ...
  //
  //
  // TIP#1: If dest buffer is NULL, data are popped from FIFO,
  //        but don't written anywhere.
  //
  // TIP#2: Skip len parameter or set to -1 if you want to pop up
  //        all data stored in queue.
  //
  // TIP#3: Use peekOnly flag to get data WITHOUT removing it from FIFO.
  //
  //
  // dest     - buffer where to write popped data (OUT/OPT).
  //
  // len      - number of bytes to pop, if set to -1 all available data
  //            will be popped (IN/OPT).
  //
  // peekOnly - set to 1 if you want copy data to dest buffer WITHOUT
  //            remove it from buffer (IN/OPT).
  //
  // RETURNS: 0 if all bytes popped,
  //          -1 otherwise..
  //

  int IOFifo::pop(void *dest, int len, int peekOnly)
  {
    DBG_ENTER3("IOFifo::pop");

    int exitCode = -1;

    char *dst = (char *) dest;

    //
    // Check is it sufficient space left.
    //

    FAILEX(capacity_ - bytesLeft_ < len,
               "ERROR: Going to pop [%d] bytes from IOFifo PTR#%p,"
                   " but only [%d] bytes available.\n",
                       len, this, capacity_ - bytesLeft_);

    //
    // Read data from buffer if needed.
    //

    if (dst)
    {
      if ((readPos_ + len) < capacity_)
      {
        memcpy(dst, buffer_ + readPos_, len);
      }
      else
      {
        unsigned int bytesToEnd = capacity_ - readPos_;

        unsigned int overflow = (len - bytesToEnd) % capacity_;

        memcpy(dst, buffer_ + readPos_, bytesToEnd);

        memcpy(dst + bytesToEnd, buffer_, overflow);
      }
    }

    //
    // Move read position.
    //

    if (peekOnly == 0)
    {
      readPos_ = (readPos_ + len) % capacity_;

      bytesLeft_ += len;

      DEBUG2("Popped [%d] bytes from IOFifo PTR#%p,"
                 " [%d] bytes left.\n", len, this, bytesLeft_);
    }

    //
    // Error handler.
    //

    exitCode = 0;

    fail:

    DBG_LEAVE3("IOFifo::pop");

    return exitCode;
  }

  //
  // Eat data from the begin of FIFO.
  //
  // Buffer before: xx xx xx xx yy yy yy yy ...
  // Buffer after :             yy yy yy yy ...
  //
  //
  // Works as pop() method with destination set to NULL.
  //
  // len - number of bytes to eat (IN).
  //
  // RETURNS: 0 if all bytes eated,
  //         -1 otherwise..
  //

  int IOFifo::eat(int len)
  {
    return pop(NULL, len);
  }

  // ----------------------------------------------------------------------------
  //
  //                       Non destructive read (peek)
  //
  // ----------------------------------------------------------------------------

  //
  // Copy <len> bytes from begin of FIFO to <dest>, but
  // do NOT remove them from FIFO.
  //
  // Works as pop() with peekOnly flag set to 1.
  //
  //
  // dest - buffer where to store readed data (OUT).
  //
  // len  - number of bytes to read. If set to -1 all
  //        available data will be copied (IN/OPT).
  //
  //
  // RETURNS: 0 if all bytes copied,
  //         -1 otherwise.
  //
  //

  int IOFifo::peek(void *dest, int len)
  {
    return pop(dest, len, 1);
  }

  //
  // Peek QWORD from fifo begin, but do NOT remove it from buffer.
  //
  // WARNING: If there is less than 8 bytes in buffer, return value
  //          is always zero.
  //
  // endian - set to IO_BIG_ENDIAN or IO_LITTLE_ENDIAN (IN).
  //
  // RETURNS: First QWORD in queue.
  //

  uint64_t IOFifo::peekQword(int endian)
  {
    uint64_t ret = 0;

    peek(&ret, 8);

    //
    // Inverse bytes if needed.
    //

    if (endian == IO_BIG_ENDIAN)
    {
      uint64_t tmp = ret;

      uint8_t *src = (uint8_t *) &tmp;
      uint8_t *dst = (uint8_t *) &ret;

      dst[0] = src[7];
      dst[1] = src[6];
      dst[2] = src[5];
      dst[3] = src[4];
      dst[4] = src[3];
      dst[5] = src[2];
      dst[6] = src[1];
      dst[7] = src[0];
    }

    return ret;
  }

  //
  // Peek DWORD from fifo begin, but do NOT remove it from buffer.
  //
  // WARNING: If there is less than 4 bytes in buffer, return value
  //          is always zero.
  //
  // endian - set to IO_BIG_ENDIAN or IO_LITTLE_ENDIAN (IN).
  //
  // RETURNS: First DWORD in queue.
  //

  uint32_t IOFifo::peekDword(int endian)
  {
    uint32_t ret = 0;

    peek(&ret, 4);

    //
    // Inverse bytes if needed.
    //

    if (endian == IO_BIG_ENDIAN)
    {
      uint32_t tmp = ret;

      uint8_t *src = (uint8_t *) &tmp;
      uint8_t *dst = (uint8_t *) &ret;

      dst[0] = src[7];
      dst[1] = src[6];
      dst[2] = src[5];
      dst[3] = src[4];
    }

    return ret;
  }

  //
  // Peek byte from fifo begin, but do NOT remove it from buffer.
  //
  // WARNING: If there is less than 1 bytes in buffer, return value
  //          is always zero.
  //
  // RETURNS: First byte in queue.
  //

  uint8_t IOFifo::peekByte()
  {
    uint8_t ret = 0;

    if (capacity_ - bytesLeft_ >= 1)
    {
      ret = buffer_[writePos_];
    }

    return ret;
  }

  // ----------------------------------------------------------------------------
  //
  //                           Multithread synchronization
  //
  // ----------------------------------------------------------------------------

  //
  // Lock access to fifo.
  //

  void IOFifo::lock()
  {
    mutex_ -> lock();
  }

  //
  // Unlock access to fifo blocked by lock() before.
  //

  void IOFifo::unlock()
  {
    mutex_ -> unlock();
  }

  // ----------------------------------------------------------------------------
  //
  //                              Getters and setters
  //
  // ----------------------------------------------------------------------------

  //
  // Return number of free bytes, which can be append to fifo.
  //

  unsigned int IOFifo::bytesLeft()
  {
    return bytesLeft_;
  }

  //
  // Return number of bytes already stored inside fifo.
  //

  unsigned int IOFifo::size()
  {
    return capacity_ - bytesLeft_;
  }

  //
  // Return total fifo capacity in bytes.
  //

  unsigned int IOFifo::capacity()
  {
    return capacity_;
  }
} /* namespace Tegenaria */
