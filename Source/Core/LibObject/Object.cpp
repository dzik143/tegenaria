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

#include "Object.h"
#include <typeinfo>
#include <Tegenaria/Debug.h>

namespace Tegenaria
{
  //
  // Static variables.
  //

  #ifdef DEBUG
    std::set<Object *> Object::instances_;

    Mutex Object::instancesMutex_("Object::instances_");

    int Object::instancesCreatedCount_    = 0;
    int Object::instancesDestroyedCount_  = 0;
    int Object::instancesDuplicatedCount_ = -1; // TODO
  #endif

  //
  // Private copy constructor.
  //

  Object::Object(const Object &)
  {
  }

  //
  // Constructor.
  //

  Object::Object(const char *className, const char *objectName)
  {
    if (className == NULL)
    {
      Fatal("ERROR: Class name cannot be NULL.\n");
    }

    className_  = className;
    objectName_ = objectName;
    refCount_   = 1;

    //
    // Track created instances.
    //

    DBG_SET_ADD(className, this);

    #ifdef DEBUG
    {
      instancesMutex_.lock();
      instances_.insert(this);
      instancesCreatedCount_++;
      instancesMutex_.unlock();
    }
    #endif
  }

  //
  // Virtual destructor.
  // Should be overwriten in derived class.
  //

  Object::~Object()
  {
    //
    // Check is this pointer correct.
    //

    if (IsPointerCorrect(this) == 0)
    {
      Fatal("ERROR: Attemp to destroy non existing object PTR#%p.\n", this);
    }
    //
    // Track created instances.
    //

    DBG_SET_DEL(this -> getClassName(), this);

    #ifdef DEBUG
    {
      instancesMutex_.lock();
      instances_.erase(this);
      instancesDestroyedCount_++;
      instancesMutex_.unlock();
    }
    #endif
  }

  //
  // Check is given this pointer points to correct Tegenaria::Object.
  //
  // ptr - this pointer to check (IN).
  //
  // RETURNS: 1 if given pointer points to correct Tegenaria::Object,
  //          0 otherwise.
  //

  bool Object::IsPointerCorrect(Object *ptr)
  {
    int found = 0;

    #ifdef DEBUG
    {
      instancesMutex_.lock();
      found = instances_.count(ptr);
      instancesMutex_.unlock();
    }
    #else
    {
      found = 1;
    }
    #endif

    return found;
  }

  int Object::GetNumberOfInstances()
  {
    int rv = -1;

    #ifdef DEBUG
      rv = instances_.size();
    #endif

    return rv;
  }

  int Object::GetCreatedCounter()
  {
    int rv = -1;

    #ifdef DEBUG
      rv = instancesCreatedCount_;
    #endif

    return rv;
  }

  int Object::GetDestroyedCounter()
  {
    int rv = -1;

    #ifdef DEBUG
      rv = instancesDestroyedCount_;
    #endif

    return rv;
  }

  int Object::GetDuplicatedCounter()
  {
    int rv = -1;

    #ifdef DEBUG
      rv = instancesDuplicatedCount_;
    #endif

    return rv;
  }

  //
  // Increase refference counter.
  //
  // WARNING! Every call to addRef() MUSTS be followed by one release() call.
  //
  // TIP #1: Object will not be destroyed until refference counter is greater
  //         than 0.
  //
  // TIP #2: Don't call destructor directly, use release() instead. If
  //         refference counter achieve 0, object will be destroyed
  //         automatically.
  //

  void Object::addRef()
  {
    refCountMutex_.lock();

    refCount_ ++;

    DEBUG3("Increased refference counter to %d for '%s' PTR#%p.\n",
               refCount_, this -> getObjectName(), this);

    refCountMutex_.unlock();
  }

  //
  // Decrease refference counter increased by addRef() before.
  //

  void Object::release()
  {
    int deleteNeeded = 0;

    //
    // Check is this pointer correct.
    //

    if (IsPointerCorrect(this) == 0)
    {
      Error("ERROR: Attemp to release non existing object PTR#%p.\n", this);

      return;
    }

    //
    // Decrease refference counter by 1.
    //

    refCountMutex_.lock();

    refCount_ --;

    DEBUG3("Decreased refference counter to %d for '%s' PTR#%p.\n",
               refCount_, this -> getObjectName(), this);

    if (refCount_ == 0)
    {
      deleteNeeded = 1;
    }

    refCountMutex_.unlock();

    //
    // Delete object if refference counter goes down to 0.
    //

    if (deleteNeeded)
    {
      delete this;
    }
  }

  int Object::getRefCounter()
  {
    return refCount_;
  }

  const char *Object::getObjectName()
  {
    return objectName_.c_str();
  }

  const char *Object::getClassName()
  {
    return className_.c_str();
  }

} /* namespace Tegenaria */
