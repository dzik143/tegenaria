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

#ifndef Tegenaria_Core_Variant_H
#define Tegenaria_Core_Variant_H

#undef DEBUG

#include <assert.h>
#include <cstring>
#include <string>
#include <vector>
#include <unordered_map>

#include <Tegenaria/Debug.h>
#include "VariantArray.h"
#include "VariantMap.h"
#include "VariantString.h"

using namespace std;

#define VARIANT_PRINT_USE_QUOTATION (1 << 0)

#define VARIANT_DEFINE_ARITHMETIC_OP2(_FUNCTION_, _OP_)                                 \
  Variant _FUNCTION_ (const Variant &y)                                                 \
  {                                                                                     \
    Variant rv = Variant::createUndefined();                                            \
                                                                                        \
    const Variant &x = *this;                                                           \
                                                                                        \
    switch (x.type_)                                                                    \
    {                                                                                   \
      case VARIANT_INTEGER:                                                             \
      {                                                                                 \
        /*                                                                              \
         * Integer _OP_ ...                                                             \
         */                                                                             \
                                                                                        \
        switch (y.type_)                                                                \
        {                                                                               \
          case VARIANT_INTEGER:                                                         \
          {                                                                             \
            /*                                                                          \
             * Integer _OP_ Integer = Integer                                           \
             */                                                                         \
                                                                                        \
            rv = Variant::createInteger(x.valueInteger_ _OP_ y.valueInteger_);          \
                                                                                        \
            break;                                                                      \
          }                                                                             \
                                                                                        \
          case VARIANT_FLOAT:                                                           \
          {                                                                             \
            /*                                                                          \
             * Integer _OP_ Float = Float                                               \
             */                                                                         \
                                                                                        \
            rv = Variant::createFloat(float(x.valueInteger_) _OP_ y.valueFloat_);       \
                                                                                        \
            break;                                                                      \
          }                                                                             \
                                                                                        \
          case VARIANT_DOUBLE:                                                          \
          {                                                                             \
            /*                                                                          \
             * Integer _OP_ Double = Double                                             \
             */                                                                         \
                                                                                        \
            rv = Variant::createDouble(double(x.valueInteger_) _OP_ y.valueDouble_);    \
                                                                                        \
            break;                                                                      \
          }                                                                             \
        }                                                                               \
                                                                                        \
        break;                                                                          \
      }                                                                                 \
                                                                                        \
      case VARIANT_FLOAT:                                                               \
      {                                                                                 \
        /*                                                                              \
         * Float _OP_ ...                                                               \
         */                                                                             \
                                                                                        \
        switch (y.type_)                                                                \
        {                                                                               \
          case VARIANT_INTEGER:                                                         \
          {                                                                             \
            /*                                                                          \
             * Float _OP_ Integer = Float                                               \
             */                                                                         \
                                                                                        \
            rv = Variant::createFloat(x.valueFloat_ _OP_ float(y.valueInteger_));       \
                                                                                        \
            break;                                                                      \
          }                                                                             \
                                                                                        \
          case VARIANT_FLOAT:                                                           \
          {                                                                             \
            /*                                                                          \
             * Float _OP_ Float = Float                                                 \
             */                                                                         \
                                                                                        \
            rv = Variant::createFloat(x.valueFloat_ _OP_ y.valueFloat_);                \
                                                                                        \
            break;                                                                      \
          }                                                                             \
                                                                                        \
          case VARIANT_DOUBLE:                                                          \
          {                                                                             \
            /*                                                                          \
             * Float _OP_ Double = Double                                               \
             */                                                                         \
                                                                                        \
            rv = Variant::createDouble(double(x.valueFloat_) _OP_ y.valueDouble_);      \
                                                                                        \
            break;                                                                      \
          }                                                                             \
        }                                                                               \
                                                                                        \
        break;                                                                          \
      }                                                                                 \
                                                                                        \
      case VARIANT_DOUBLE:                                                              \
      {                                                                                 \
        /*                                                                              \
         * Double _OP_ ...                                                              \
         */                                                                             \
                                                                                        \
        switch (y.type_)                                                                \
        {                                                                               \
          case VARIANT_INTEGER:                                                         \
          {                                                                             \
            /*                                                                          \
             * Double _OP_ Integer = Double                                             \
             */                                                                         \
                                                                                        \
            rv = Variant::createDouble(x.valueDouble_ _OP_ double(y.valueInteger_));    \
                                                                                        \
            break;                                                                      \
          }                                                                             \
                                                                                        \
          case VARIANT_FLOAT:                                                           \
          {                                                                             \
            /*                                                                          \
             * Double _OP_ Float = Double                                               \
             */                                                                         \
                                                                                        \
            rv = Variant::createDouble(x.valueDouble_ _OP_ double(y.valueFloat_));      \
                                                                                        \
            break;                                                                      \
          }                                                                             \
                                                                                        \
          case VARIANT_DOUBLE:                                                          \
          {                                                                             \
            /*                                                                          \
             * Double _OP_ Double = Double                                              \
             */                                                                         \
                                                                                        \
            rv = Variant::createDouble(x.valueDouble_ _OP_ y.valueDouble_);             \
                                                                                        \
            break;                                                                      \
          }                                                                             \
        }                                                                               \
                                                                                        \
        break;                                                                          \
      }                                                                                 \
    }                                                                                   \
                                                                                        \
    return rv;                                                                          \
  }

//
// Helper macro to define compare operators: <, >, <=, >=
//

#define VARIANT_DEFINE_COMPARE_OP2(_FUNCTION_, _OP_)                                    \
  Variant _FUNCTION_ (const Variant &y)                                                 \
  {                                                                                     \
    Variant rv = Variant::createUndefined();                                            \
                                                                                        \
    const Variant &x = *this;                                                           \
                                                                                        \
    switch (x.type_)                                                                    \
    {                                                                                   \
      case VARIANT_INTEGER:                                                             \
      {                                                                                 \
        /*                                                                              \
         * Integer _OP_ ...                                                             \
         */                                                                             \
                                                                                        \
        switch (y.type_)                                                                \
        {                                                                               \
          case VARIANT_INTEGER:                                                         \
          {                                                                             \
            rv = Variant::createBoolean(x.valueInteger_ _OP_ y.valueInteger_);          \
                                                                                        \
            break;                                                                      \
          }                                                                             \
                                                                                        \
          case VARIANT_FLOAT:                                                           \
          {                                                                             \
            rv = Variant::createBoolean(float(x.valueInteger_) _OP_ y.valueFloat_);     \
                                                                                        \
            break;                                                                      \
          }                                                                             \
                                                                                        \
          case VARIANT_DOUBLE:                                                          \
          {                                                                             \
            rv = Variant::createBoolean(double(x.valueInteger_) _OP_ y.valueDouble_);   \
                                                                                        \
            break;                                                                      \
          }                                                                             \
        }                                                                               \
                                                                                        \
        break;                                                                          \
      }                                                                                 \
                                                                                        \
      case VARIANT_FLOAT:                                                               \
      {                                                                                 \
        /*                                                                              \
         * Float _OP_ ...                                                               \
         */                                                                             \
                                                                                        \
        switch (y.type_)                                                                \
        {                                                                               \
          case VARIANT_INTEGER:                                                         \
          {                                                                             \
            rv = Variant::createBoolean(x.valueFloat_ _OP_ float(y.valueInteger_));     \
                                                                                        \
            break;                                                                      \
          }                                                                             \
                                                                                        \
          case VARIANT_FLOAT:                                                           \
          {                                                                             \
            rv = Variant::createBoolean(x.valueFloat_ _OP_ y.valueFloat_);              \
                                                                                        \
            break;                                                                      \
          }                                                                             \
                                                                                        \
          case VARIANT_DOUBLE:                                                          \
          {                                                                             \
            rv = Variant::createBoolean(double(x.valueFloat_) _OP_ y.valueDouble_);     \
                                                                                        \
            break;                                                                      \
          }                                                                             \
        }                                                                               \
                                                                                        \
        break;                                                                          \
      }                                                                                 \
                                                                                        \
      case VARIANT_DOUBLE:                                                              \
      {                                                                                 \
        /*                                                                              \
         * Double _OP_ ...                                                              \
         */                                                                             \
                                                                                        \
        switch (y.type_)                                                                \
        {                                                                               \
          case VARIANT_INTEGER:                                                         \
          {                                                                             \
            rv = Variant::createBoolean(x.valueDouble_ _OP_ double(y.valueInteger_));   \
                                                                                        \
            break;                                                                      \
          }                                                                             \
                                                                                        \
          case VARIANT_FLOAT:                                                           \
          {                                                                             \
            rv = Variant::createBoolean(x.valueDouble_ _OP_ double(y.valueFloat_));     \
                                                                                        \
            break;                                                                      \
          }                                                                             \
                                                                                        \
          case VARIANT_DOUBLE:                                                          \
          {                                                                             \
            rv = Variant::createBoolean(x.valueDouble_ _OP_ y.valueDouble_);            \
                                                                                        \
            break;                                                                      \
          }                                                                             \
        }                                                                               \
                                                                                        \
        break;                                                                          \
      }                                                                                 \
    }                                                                                   \
                                                                                        \
    return rv;                                                                          \
  }

namespace Tegenaria
{
  class Variant //: public Object
  {
    public:

    // TODO: Make it thread-safe.
    static Variant characterPeek;

    enum Type
    {
      //
      // Special cases - undefined and null.
      //

      VARIANT_UNDEFINED,
      VARIANT_NULL,

      //
      // Primitives.
      //

      VARIANT_INTEGER,
      VARIANT_FLOAT,
      VARIANT_DOUBLE,
      VARIANT_STRING,
      VARIANT_BOOLEAN,

      //
      // Complex containers.
      //

      VARIANT_PTR,
      VARIANT_ARRAY,
      VARIANT_MAP,
      VARIANT_OBJECT
    };

    int type_;
    int sharedDataRefCount_;

    union
    {
      int    valueInteger_;
      bool   valueBoolean_;
      double valueDouble_;
      float  valueFloat_;

      void *valuePtr_;

      VariantString *dataString_;
      VariantArray  *dataArray_;
      VariantMap    *dataMap_;
    };

    //
    // Constructors and destructors.
    //

    Variant(int type = VARIANT_UNDEFINED) : type_(type) //Object("Variant"), type_(VARIANT_UNDEFINED)
    {
      DEBUG3("Variant: Created variable PTR [%p]\n", this);
    }

    Variant(const Variant &ref)// : Object("Variant")
    {
      switch (ref.type_)
      {
        case VARIANT_ARRAY:  ref.dataArray_  -> addRef(); break;
        case VARIANT_MAP:    ref.dataMap_    -> addRef(); break;
        case VARIANT_OBJECT: ref.dataMap_    -> addRef(); break;
        case VARIANT_STRING: ref.dataString_ -> addRef(); break;
      }

      memcpy(this, &ref, sizeof(ref));

      DEBUG3("Variant: Duplicated variable PTR [%p] type [%d] into PTR [%p]\n", &ref, ref.type_, this);
    }

    Variant &operator=(const Variant &ref)
    {
      switch (type_)
      {
        case VARIANT_ARRAY:  dataArray_  -> release(); break;
        case VARIANT_MAP:    dataMap_    -> release(); break;
        case VARIANT_OBJECT: dataMap_    -> release(); break;
        case VARIANT_STRING: dataString_ -> release(); break;
      }

      switch (ref.type_)
      {
        case VARIANT_ARRAY:  ref.dataArray_  -> addRef(); break;
        case VARIANT_MAP:    ref.dataMap_    -> addRef(); break;
        case VARIANT_OBJECT: ref.dataMap_    -> addRef(); break;
        case VARIANT_STRING: ref.dataString_ -> addRef(); break;
      }

      memcpy(this, &ref, sizeof(ref));

      DEBUG3("Variant: Assigned variable PTR [%p] type [%d] into PTR [%p]\n", &ref, ref.type_, this);

      return *this;
    }

    ~Variant()
    {
      DEBUG3("Variant: Going to destroy variable PTR [%p]\n", this);

      switch(type_)
      {
        case VARIANT_ARRAY:
        {
          dataArray_ -> release();

          break;
        }

        case VARIANT_MAP:
        case VARIANT_OBJECT:
        {
          dataMap_ -> release();

          break;
        }

        case VARIANT_STRING:
        {
          dataString_ -> release();

          break;
        }
      }
    }

    //
    // isXxx() to check current stored type.
    //

    bool isInteger() {return (type_ == VARIANT_INTEGER);}
    bool isBoolean() {return (type_ == VARIANT_BOOLEAN);}
    bool isFloat()   {return (type_ == VARIANT_FLOAT) || (type_ == VARIANT_DOUBLE);}
    bool isPointer() {return (type_ == VARIANT_PTR);}
    bool isString()  {return (type_ == VARIANT_STRING);}
    bool isArray()   {return (type_ == VARIANT_ARRAY);}
    bool isMap()     {return (type_ == VARIANT_MAP);}
    bool isObject()  {return (type_ == VARIANT_OBJECT);}

    //
    // createXxx() to crete new variable with selected type.
    //

    static Variant createInteger(int value)
    {
      Variant rv;
      rv.type_     = VARIANT_INTEGER;
      rv.valueInteger_ = value;
      return rv;
    }

    static Variant createBoolean(bool value)
    {
      Variant rv;
      rv.type_         = VARIANT_BOOLEAN;
      rv.valueBoolean_ = value;
      return rv;
    }

    static Variant createFloat(double value)
    {
      return createDouble(value);
    }

    static Variant createDouble(double value)
    {
      Variant rv;
      rv.type_        = VARIANT_DOUBLE;
      rv.valueDouble_ = value;
      return rv;
    }

    static Variant createPointer(void *ptr)
    {
      Variant rv;
      rv.type_     = VARIANT_PTR;
      rv.valuePtr_ = ptr;
      return rv;
    }

    static Variant createString(const char *text = NULL)
    {
      Variant rv;

      rv.type_ = VARIANT_STRING;

      if (text)
      {
        rv.dataString_ = new VariantString(text);
      }
      else
      {
        rv.dataString_ = new VariantString();
      }

      return rv;
    }

    static Variant createUndefined()
    {
      Variant rv;
      rv.type_ = VARIANT_UNDEFINED;
      return rv;
    }

    static Variant createNull()
    {
      Variant rv;
      rv.type_ = VARIANT_NULL;
      return rv;
    }

    static Variant createArray()
    {
      Variant rv;
      rv.type_      = VARIANT_ARRAY;
      rv.dataArray_ = new VariantArray();
      return rv;
    }

    static Variant createMap()
    {
      Variant rv;
      rv.type_    = VARIANT_MAP;
      rv.dataMap_ = new VariantMap();
      return rv;
    }

    static Variant createObject(const char *className, const char *baseClassName = NULL)
    {
      Variant rv;
      rv.type_    = VARIANT_OBJECT;
      rv.dataMap_ = new VariantMap();
      (*rv.dataMap_)["__className"] = Variant::createString(className);

      if (baseClassName)
      {
        (*rv.dataMap_)["__baseClassName"] = Variant::createString(baseClassName);
      }

      return rv;
    }

    //
    // Getters.
    //

    const char *getTypeName();

    //
    // Two
    //

    VARIANT_DEFINE_ARITHMETIC_OP2(defaultPlusOperator, +);
    VARIANT_DEFINE_ARITHMETIC_OP2(operator-, -);
    VARIANT_DEFINE_ARITHMETIC_OP2(operator*, *);
    VARIANT_DEFINE_ARITHMETIC_OP2(defaultDivOperator, /);

    VARIANT_DEFINE_COMPARE_OP2(operator<, <);
    VARIANT_DEFINE_COMPARE_OP2(operator>, >);
    VARIANT_DEFINE_COMPARE_OP2(operator<=, <=);
    VARIANT_DEFINE_COMPARE_OP2(operator>=, >=);
    VARIANT_DEFINE_COMPARE_OP2(defaultEqOperator, ==);

    Variant operator/ (const Variant &y)
    {
      Variant rv;

      const Variant &x = *this;

      if (x.type_ == VARIANT_INTEGER && y.type_ == VARIANT_INTEGER)
      {
        //
        // Special case: Integer / Integer = Float
        //

        rv = Variant::createDouble(double(x.valueInteger_) / double(y.valueInteger_));
      }
      else
      {
        //
        // Default scenario - use C-like beheavior.
        //

        rv = defaultDivOperator(y);
      }

      return rv;
    }

    Variant operator+ (const Variant &y)
    {
      Variant rv;

      const Variant &x = *this;

      if (x.type_ == VARIANT_STRING && y.type_ == VARIANT_STRING)
      {
        //
        // Special case: Strings concatenation.
        //

        rv = Variant::createString();

        *(rv.dataString_)  = *(x.dataString_);
        *(rv.dataString_) += *(y.dataString_);
      }
      else
      {
        //
        // Default scenario - use C-like beheavior.
        //

        rv = defaultPlusOperator(y);
      }

      return rv;
    }

    Variant divAsInteger(const Variant &y)
    {
      const Variant &x = *this;

      Variant rv = defaultDivOperator(y);

      if (rv.type_ == VARIANT_FLOAT)
      {
        rv.type_         = VARIANT_INTEGER;
        rv.valueInteger_ = int(rv.valueFloat_);
      }
      else if (rv.type_ == VARIANT_DOUBLE)
      {
        rv.type_         = VARIANT_INTEGER;
        rv.valueInteger_ = int(rv.valueDouble_);
      }

      return rv;
    }

    Variant operator&& (const Variant &y)
    {
      const Variant &x = *this;

      Variant rv = Variant::createUndefined();

      if (x.type_ == VARIANT_BOOLEAN && y.type_ == VARIANT_BOOLEAN)
      {
        rv.type_         = VARIANT_BOOLEAN;
        rv.valueBoolean_ = x.valueBoolean_ && y.valueBoolean_;
      }

      return rv;
    }

    Variant operator|| (const Variant &y)
    {
      const Variant &x = *this;

      Variant rv = Variant::createUndefined();

      if (x.type_ == VARIANT_BOOLEAN && y.type_ == VARIANT_BOOLEAN)
      {
        rv.type_         = VARIANT_BOOLEAN;
        rv.valueBoolean_ = x.valueBoolean_ || y.valueBoolean_;
      }

      return rv;
    }

    Variant operator== (const Variant &y)
    {
      const Variant &x = *this;

      Variant rv = Variant::createBoolean(0);

      if (x.type_ == VARIANT_STRING && y.type_ == VARIANT_STRING)
      {
        //
        // Special case - strings compare.
        //

        rv = createBoolean((*x.dataString_) == (*y.dataString_));
      }
      else if (x.type_ == VARIANT_BOOLEAN && y.type_ == VARIANT_BOOLEAN)
      {
        //
        // Special case - (boolean == boolean)
        //

        rv = createBoolean(x.valueBoolean_ == y.valueBoolean_);
      }
      else
      {
        //
        // General case - use C-style buildin operator.
        //

        if (defaultEqOperator(y).isTrue())
        {
          rv.valueBoolean_ = true;
        }
      }

      return rv;
    }

    Variant operator!= (const Variant &y)
    {
      Variant rv = operator==(y);

      rv.valueBoolean_ = !rv.valueBoolean_;

      return rv;
    }

    Variant operator-() const
    {
      Variant rv = createUndefined();

      switch(type_)
      {
        case VARIANT_INTEGER:
        {
          rv = createInteger(-this -> valueInteger_);

          break;
        }

        case VARIANT_FLOAT:
        {
          rv = createFloat(-this -> valueFloat_);

          break;
        }

        case VARIANT_DOUBLE:
        {
          rv = createDouble(-this -> valueDouble_);

          break;
        }
      }

      return rv;
    }

    //
    // Boolean helpers.
    //

    bool isTrue()
    {
      return (type_ == VARIANT_BOOLEAN) && valueBoolean_;
    }

    bool isFalse()
    {
      return (type_ == VARIANT_BOOLEAN) && (!valueBoolean_);
    }

    //
    // Array specific.
    //

    const Variant arraySize()
    {
      Variant rv = createInteger(-1);

      if (type_ == VARIANT_ARRAY)
      {
        rv.valueInteger_ = dataArray_ -> size();
      }

      return rv;
    }

    void arrayPush(const Variant &item)
    {
      if (type_ == VARIANT_ARRAY)
      {
        dataArray_ -> push_back(item);
      }
    }

    Variant &arrayAccess(size_t idx)
    {
      assert(type_ == VARIANT_ARRAY);

      if (dataArray_ -> size() <= idx)
      {
        dataArray_ -> resize(idx + 1);
      }

      return (*dataArray_)[idx];
    }

    Variant &arrayAccess(const Variant &idx)
    {
      assert(idx.type_ == VARIANT_INTEGER);

      return arrayAccess(idx.valueInteger_);
    }

    //
    // Map specific.
    //

    Variant &mapAccess(const string &key)
    {
      assert(type_ == VARIANT_MAP || type_ == VARIANT_OBJECT);

      return (*dataMap_)[key];
    }

    Variant &mapAccess(const Variant &key)
    {
      assert(key.type_ == VARIANT_STRING);

      return mapAccess(*key.dataString_);
    }

    //
    // String specific.
    //

    Variant &stringAccess(const Variant &idx)
    {
      assert(idx.type_ == VARIANT_INTEGER);
      assert(type_ == VARIANT_STRING);

      return stringAccess(idx.valueInteger_);
    }

    Variant &stringAccess(size_t idx)
    {
      assert(type_ == VARIANT_STRING);

      if (dataString_ -> size() <= idx)
      {
        dataString_ -> resize(idx + 1);
      }

      // TODO: Make it thread-safe.
      (*characterPeek.dataString_)[0] = dataString_ -> at(idx);
      return Variant::characterPeek;
    }

    //
    // Common helper for maps and arrays.
    //

    Variant &access(const Variant &index)
    {
      switch(type_)
      {
        case VARIANT_MAP:    return mapAccess(index);
        case VARIANT_OBJECT: return mapAccess(index);
        case VARIANT_ARRAY:  return arrayAccess(index);
        case VARIANT_STRING: return stringAccess(index);

        default:
        {
          Fatal("ERROR! array or map expected.\n");
        }
      }
    }

    //
    // Common helper for objects.
    //

    const Variant propertyGet(const Variant &name)
    {
      return mapAccess(name);
    }

    void propertySet(const Variant &name, const Variant &value)
    {
      mapAccess(name) = value;
    }

    const Variant getClassName()
    {
      return (*dataMap_)["__className"];
    }

    const Variant getBaseClassName()
    {
      return (*dataMap_)["__baseClassName"];
    }

    const string toStdString() const;
    const Variant toString() const;
    const Variant toStringEscapedForC() const;

    //
    // I/O utils.
    //

    int printAsText(FILE *f, unsigned int flags = 0);

    static int vaprint(FILE *f, unsigned int flags, int count, ...);

    //
    // Generic len() wrapper (type-independent).
    //

    Variant length()
    {
      int rv = 0;

      switch(type_)
      {
        case VARIANT_UNDEFINED:
        case VARIANT_NULL:
        {
          rv = 0;

          break;
        }

        case VARIANT_INTEGER:
        case VARIANT_FLOAT:
        case VARIANT_DOUBLE:
        case VARIANT_BOOLEAN:
        {
          rv = 1;

          break;
        }

        case VARIANT_STRING:
        {
          rv = dataString_ -> size();

          break;
        }

        case VARIANT_ARRAY:
        {
          rv = dataArray_ -> size();

          break;
        }

        case VARIANT_MAP:
        {
          rv = dataMap_ -> size();

          break;
        }

        case VARIANT_OBJECT:
        {
          Fatal("len(object) not implemented");

          break;
        }
      }

      return Variant::createInteger(rv);
    }
  };

} /* Tegenaria */

#endif /* Tegenaria_Core_Variant_H */
