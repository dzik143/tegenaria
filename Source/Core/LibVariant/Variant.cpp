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

#include "Variant.h"

namespace Tegenaria
{
  Variant::printAsText(FILE *f, unsigned int flags)
  {
    int rv = -1;

    switch(type_)
    {
      case VARIANT_STRING:
      {
        if (flags & VARIANT_PRINT_USE_QUOTATION)
        {
          rv = fprintf(f, "'%s'",  dataString_ -> c_str());
        }
        else
        {
          rv = fprintf(f, "%s",  dataString_ -> c_str());
        }

        break;
      }

      case VARIANT_INTEGER:   rv = fprintf(f, "%d",  valueInteger_); break;
      case VARIANT_FLOAT:     rv = fprintf(f, "%f",  valueFloat_); break;
      case VARIANT_DOUBLE:    rv = fprintf(f, "%lf", valueDouble_); break;
      case VARIANT_BOOLEAN:   rv = fprintf(f, "%s",  valueBoolean_ ? "true" : "false"); break;
      case VARIANT_UNDEFINED: rv = fprintf(f, "undefined"); break;
      case VARIANT_OBJECT:    rv = fprintf(f, "<object>"); break;

      case VARIANT_ARRAY:
      {
        const char *sep = "";

        fprintf(f, "[");

        for (int i = 0; i < dataArray_ -> size(); i++)
        {
          fprintf(f, sep);
          (*dataArray_)[i].printAsText(f, flags | VARIANT_PRINT_USE_QUOTATION);
          sep = ", ";
        }

        fprintf(f, "]");

        break;
      }

      case VARIANT_MAP:
      {
        map<string, Variant>::iterator it;

        const char *sep = "";

        fprintf(f, "{");

        for (it = dataMap_ -> begin(); it != dataMap_ -> end(); it++)
        {
          fprintf(f, sep);
          fprintf(f, "'%s': ", it -> first.c_str());
          it -> second.printAsText(f, flags | VARIANT_PRINT_USE_QUOTATION);
          sep = ", ";
        }

        fprintf(f, "}");

        break;
      }

      default:
      {
        Fatal("ERROR! Unhandled variant type [%d] while printing.\n", type_);
      }
    }

    return rv;
  }

  const char *Variant::getTypeName()
  {
    const char *rv = "[error]";

    switch (type_)
    {
      case VARIANT_STRING:    rv = "string";    break;
      case VARIANT_INTEGER:   rv = "integer";   break;
      case VARIANT_FLOAT:     rv = "float";     break;
      case VARIANT_DOUBLE:    rv = "float";     break;
      case VARIANT_BOOLEAN:   rv = "boolean";   break;
      case VARIANT_UNDEFINED: rv = "undefined"; break;
      case VARIANT_ARRAY:     rv = "array";     break;
      case VARIANT_MAP:       rv = "map";       break;
      case VARIANT_OBJECT:    rv = "object";    break;

      default:
      {
        Fatal("ERROR! Unhandled variant type [%d] while getting type name.\n", type_);
      }
    }

    return rv;
  }

  int Variant::vaprint(FILE *f, unsigned int flags, int count, ...)
  {
    va_list ap;

    va_start(ap, count);

    const char *sep = "";

    for (int i = 0; i < count; i++)
    {
      fprintf(f, sep);

      Variant *item = va_arg(ap, Variant*);

      item -> printAsText(f, flags);

      sep = " ";
    }

    va_end(ap);

    fprintf(f, "\n");
  }

  const string Variant::toStdString() const
  {
    string rv;

    char buf[128];

    switch(type_)
    {
      case VARIANT_UNDEFINED: rv = "undefined"; break;
      case VARIANT_NULL:      rv = "null"; break;
      case VARIANT_INTEGER:   _snprintf(buf, sizeof(buf) - 1, "%d", valueInteger_); rv = buf; break;
      case VARIANT_FLOAT:     _snprintf(buf, sizeof(buf) - 1, "%f", valueFloat_);   rv = buf; break;
      case VARIANT_DOUBLE:    _snprintf(buf, sizeof(buf) - 1, "%lf", valueDouble_); rv = buf; break;
      case VARIANT_STRING:    rv = *dataString_; break;
      case VARIANT_BOOLEAN:   _snprintf(buf, sizeof(buf) - 1, "%s", valueBoolean_ ? "true" : "false"); rv = buf; break;
      case VARIANT_PTR:       rv = "[ptr]"; break;
      case VARIANT_OBJECT:    rv = "[object]"; break;

      case VARIANT_ARRAY:
      {
        const char *sep = "";

        rv = "[";

        for (int i = 0; i < dataArray_ -> size(); i++)
        {
          rv += sep;

          if ((*dataArray_)[i].isString())
          {
            rv += "'" + (*dataArray_)[i].toStdString() + "'";
          }
          else
          {
            rv += (*dataArray_)[i].toStdString();
          }

          sep = ", ";
        }

        rv += "]";

        break;
      }

      case VARIANT_MAP:
      {
        map<string, Variant>::iterator it;

        const char *sep = "";

        rv = "{";

        for (it = dataMap_ -> begin(); it != dataMap_ -> end(); it++)
        {
          rv += sep;
          rv += "'" + (it -> first) + "': ";

          if (it -> second.isString())
          {
            rv += "'" + it -> second.toStdString() + "'";
          }
          else
          {
            rv += it -> second.toStdString();
          }

          sep = ", ";
        }

        rv += "}";

        break;
      }

      default:
      {
        Fatal("ERROR! Unhandled type [%d] in Variant::toString()\n", type_);
      }
    }

    return rv;
  }

  const Variant Variant::toString() const
  {
    return Variant::createString(this -> toStdString().c_str());
  }

  const Variant Variant::toStringEscapedForC() const
  {
    string text        = this -> toStdString();
    string escapedText = "";

    for (int i = 0; i < text.size(); i++)
    {
      switch(text[i])
      {
        case '\\':
        {
          escapedText += "\\\\";

          break;
        }

        case '"':
        {
          escapedText += "\\\"";

          break;
        }

        default:
        {
          escapedText += text[i];
        }
      }
    }

    return Variant::createString(escapedText.c_str());
  }

  Variant Variant::characterPeek = Variant::createString("?");

} /* namespace Tegenaria */
