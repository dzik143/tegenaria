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

#ifndef Tegenaria_Core_Common_Object_H
#define Tegenaria_Core_Common_Object_H

#include <Tegenaria/Mutex.h>
#include <set>

namespace Tegenaria
{
  class Object
  {
    private:

    int refCount_;

    Mutex refCountMutex_;

    string className_;
    string objectName_;

    //
    // Track created instances to check is given this pointer correct or not.
    //

  #ifdef DEBUG
    static std::set<Object *> instances_;
    static Mutex instancesMutex_;
    static int instancesCreatedCount_;
    static int instancesDestroyedCount_;
    static int instancesDuplicatedCount_;
  #endif

    //
    // Private copy constructor to avoid problems with tracking
    // reference counter after copying.
    //

    private:

    Object(const Object &);

    //
    // Protected destructor.
    // Use release() method instead.
    //

    protected:

    virtual ~Object();

    Object(const char *className, const char *objectName = "anonymous");

    public:

    //
    // Refference counter.
    // Use addRef() to mark that object is needed.
    // Use release() to mark that object is no longer needed.
    //
    // At startup object has refference counter set to 1.
    // Object is destroyed when refference counter reaches 0.
    //

    void addRef();
    void release();
    int getRefCounter();

    //
    // Check is given this pointer represents correct Object.
    // Works in debug mode only.
    // In realease mode returns true always.
    //

    static bool IsPointerCorrect(Object *ptr);
    static int GetNumberOfInstances();
    static int GetCreatedCounter();
    static int GetDestroyedCounter();
    static int GetDuplicatedCounter();

    const char *getClassName();
    const char *getObjectName();
  };

} /* namespace Tegenaria */

#endif /* Tegenaria_Core_Common_Object_H */
