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

#include "Math.h"
#include <Tegenaria/Debug.h>
#include <cstdio>

namespace Tegenaria
{
  //
  // Create empty weighted average object with current value set to 0.0.
  //
  //                1          2          3
  //         x1*alfa  + x2*alfa  + x3*alfa  + ...
  // w_avg = ------------------------------------
  //             1          2          3
  //         alfa     + alfa     + alfa     + ...
  //

  MathWeightAvg::MathWeightAvg(double alfa)
  {
    alfa_ = alfa;

    clear();
  }

  //
  // Insert value to average.
  //

  void MathWeightAvg::insert(double x)
  {
    accTop_    = accTop_ * alfa_ + x * alfa_;
    accBottom_ = accBottom_ * alfa_ + alfa_;
  }

  //
  // Get current average value.
  //

  double MathWeightAvg::getValue()
  {
    return accTop_ / accBottom_;
  }

  //
  // Zero stored average.
  //

  void MathWeightAvg::clear()
  {
    accTop_    = 0.0;
    accBottom_ = 1.0;
  }
} /* namespace Tegenaria */
