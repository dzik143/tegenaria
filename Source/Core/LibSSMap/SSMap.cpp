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

#include "SSMap.h"

namespace Tegenaria
{
  using namespace std;

  //
  // Save SSMap to file.
  //
  // fname - destination filename, where to dump map (IN).
  //
  // RETURNS: 0 if OK.
  //

  int SSMap::saveToFile(const string &fname)
  {
    FILE *f = fopen(fname.c_str(), "wt+");

    if (f)
    {
      for (SSMap::iterator it = begin(); it != end(); it++)
      {
        fprintf(f, "%s=%s\n", it -> first.c_str(), it -> second.c_str());
      }

      fclose(f);

      return 0;
    }

    return -1;
  }

  //
  // Save SSMap to file.
  //
  // fname - destination filename, where to dump map (IN).
  // flags - extra flags to format output file, see SSMap.h SSMAP_FLAGS_XXX defines (IN).
  //
  // RETURNS: 0 if OK.
  //

  int SSMap::saveToFileEx(const string &fname, int flags)
  {
    FILE *f = fopen(fname.c_str(), "wt+");

    if (flags & SSMAP_FLAGS_WIDE)
    {
      int len = 0;

      char fmt[32];

      for (SSMap::iterator it = begin(); it != end(); it++)
      {
        len = max(len, (int) it -> first.size());
      }

      sprintf(fmt, "%%-%ds= %%s\n", len);

      if (f)
      {
        for (SSMap::iterator it = begin(); it != end(); it++)
        {
          fprintf(f, fmt, it -> first.c_str(), it -> second.c_str());
        }

        fclose(f);

        return 0;
      }

      return saveToFile(fname);
    }

    return -1;
  }

  //
  // Load SSMap from file.
  //
  // fname - source filename, where to load map from (IN).
  //
  // RETURNS: 0 if OK.
  //

  int SSMap::loadFromFile(const string &fname)
  {
    FILE *f = fopen(fname.c_str(), "rt");

    clear();

    int valLen = 0;
    int keyLen = 0;

    if (f)
    {
      char buf[1024 * 32] = {0};

      while(fgets(buf, 1024 * 32, f))
      {
        char *key = buf;
        char *val = strstr(buf, "=");

        if (val && val[1])
        {
          val[0] = 0;

          val ++;

          keyLen = strlen(key);
          valLen = strlen(val);

          //
          // Clean up white spaces from key and value if needed.
          //

          while(*key && isspace(*key)) {key ++; keyLen --;}
          while(*val && isspace(*val)) {val ++; valLen --;}

          while(keyLen > 0 && isspace(key[keyLen - 1]))
          {
            key[keyLen - 1] = 0;

            keyLen --;
          }

          while(valLen > 0 &&
                    (isspace(val[valLen - 1]) ||
                        (val[valLen - 1] == 10) ||
                            (val[valLen - 1] == 13)))
          {
            val[valLen - 1] = 0;

            valLen --;
          }

          //
          // Add new <key><value> pair.
          //

          (*this)[key] = val;
        }
      }

      fclose(f);

      return 0;
    }

    return -1;
  }

  //
  // Serialize SSMap object into one continuos string.
  //
  // data - string, where to write serialized data (OUT).
  //

  void SSMap::saveToString(string &data)
  {
    data.clear();

    char buf[1024];

    for (SSMap::iterator it = begin(); it != end(); it++)
    {
      snprintf(buf, sizeof(buf) - 1, "%s=%s\t",
                   it -> first.c_str(), it -> second.c_str());

      data += buf;
    }
  }

  //
  // Load SSMap object from one continous string created by saveToString()
  // method before.
  //
  // WARNING: Function destroys input data[] string.
  //
  // data - source, single continous string created by saveToString() before (IN).
  //
  // RETURNS: 0 if OK.
  //

  int SSMap::loadFromString(char *data)
  {
    int exitCode = -1;

    int valLen = 0;
    int keyLen = 0;

    char *line = NULL;

    clear();

    //
    // Check args.
    //

    if (data == NULL)
    {
      goto fail;
    }

    //
    // Handle quated "string" format.
    //

    if (data[0] == '"' || data[0] == '\'')
    {
      data++;
    }

    //
    // Tokenize input string into lines.
    //

    line = strtok(data, "\t");

    while(line)
    {
      //
      // Process line.
      //

      char *key = line;
      char *val = strstr(line, "=");

      if (val && val[1])
      {
        val[0] = 0;

        val ++;

        keyLen = strlen(key);
        valLen = strlen(val);

        //
        // Clean up white spaces from key and value if needed.
        //

        while(*key && isspace(*key)) {key ++; keyLen --;}
        while(*val && isspace(*val)) {val ++; valLen --;}

        while(keyLen > 0 && isspace(key[keyLen - 1]))
        {
          key[keyLen - 1] = 0;

          keyLen --;
        }

        while(valLen > 0 &&
                  (isspace(val[valLen - 1]) ||
                      (val[valLen - 1] == 10) ||
                          (val[valLen - 1] == 13)))
        {
          val[valLen - 1] = 0;

          valLen --;
        }

        //
        // Add new <key><value> pair.
        //

        (*this)[key] = val;
      }

      //
      // Goto next line.
      //

      line = strtok(NULL, "\t");
    }

    //
    // Error handler.
    //

    exitCode = 0;

    fail:

    return exitCode;
  }

  //
  // Setters and getters.
  //

  //
  // Set lvalue key to rvalue string.
  //
  // lvalue - key, where to assign data (IN).
  // rvalue - string value to assign (IN).
  //
  // TIP#1: Use getXXX() methods to get back set value.
  //

  void SSMap::set(const char *lvalue, const char *rvalue)
  {
    if (lvalue == NULL) return;

    if (rvalue == NULL)
    {
      erase(lvalue);
    }
    else
    {
      operator[](lvalue) = rvalue;
    }
  }

  //
  // Set lvalue key to rvalue integer.
  //
  // lvalue - key, where to assign data (IN).
  // rvalue - integer value to assign (IN).

  // WARNING: rvalue will be converted to string internally.
  //
  // TIP#1: Use getXXX() methods to get back set value.
  //

  void SSMap::setInt(const char *lvalue, int rvalue)
  {
    char tmp[64] = {0};

    if (lvalue == NULL) return;

    snprintf(tmp, sizeof(tmp) - 1, "%d", rvalue);

    operator[](lvalue) = tmp;
  }

  //
  // Set lvalue key to rvalue pointer.
  //
  // WARNING: rvalue will be converted to string internally.
  //
  // lvalue - key, where to assign data (IN).
  // rvalue - pointer value to assign (IN).
  //
  // TIP#1: Use getXXX() methods to get back set value.
  //

  void SSMap::setPtr(const char *lvalue, const void *rvalue)
  {
    if (lvalue)
    {
      char tmp[32];
  //
  // FIXME: Check if %p adds '0x' and if not - check how to add it.
  //

      snprintf(tmp, sizeof(tmp) - 1, "%p", rvalue);

      operator[](lvalue) = tmp;
    }
  }

  //
  // Get value assigned to lvalue key as string.
  //
  // rvalue - key, where to search data (IN).
  //
  // RETURNS: String value assigned to key lvalue or
  //          NULL if key does not exist.
  //

  const char *SSMap::get(const char *lvalue)
  {
    if (lvalue)
    {
      iterator it = find(lvalue);

      if (it != end())
      {
        return it -> second.c_str();
      }
      else
      {
        return NULL;
      }
    }
    else
    {
      return NULL;
    }
  }

  //
  // Get value assigned to lvalue key as string.
  //
  // rvalue - key, where to search data (IN).
  //
  // RETURNS: String value assigned to key lvalue or
  //          NULL if key does not exist.
  //

  const char *SSMap::get(const string &lvalue)
  {
    return get(lvalue.c_str());
  }

  //
  // Get value assigned to lvalue key as string with not-null warranty.
  //
  // rvalue - key, where to search data (IN).
  //
  // RETURNS: String value assigned to key lvalue or
  //          empty string if key does not exist.
  //

  const char *SSMap::safeGet(const char *lvalue)
  {
    if (lvalue)
    {
      iterator it = find(lvalue);

      if (it != end())
      {
        return it -> second.c_str();
      }
      else
      {
        return "";
      }
    }
    else
    {
      return "";
    }
  }

  //
  // Get value assigned to lvalue key as string with not-null warranty.
  //
  // rvalue - key, where to search data (IN).
  //
  // RETURNS: String value assigned to key lvalue or
  //          empty string if key does not exist.
  //

  const char *SSMap::safeGet(const string &lvalue)
  {
    return safeGet(lvalue.c_str());
  }

  //
  // Get value assigned to lvalue key and convert it to integer.
  //
  // rvalue - key, where to search data (IN).
  //
  // RETURNS: Integer value assigned to key lvalue or
  //          0 if key does not exist.
  //

  int SSMap::getInt(const char *lvalue)
  {
    const char *raw = get(lvalue);

    if (raw)
    {
      return atoi(raw);
    }
    else
    {
      return 0;
    }
  }

  //
  // Get value assigned to lvalue key and convert it to integer.
  //
  // rvalue - key, where to search data (IN).
  //
  // RETURNS: Integer value assigned to key lvalue or
  //          0 if key does not exist.
  //

  int SSMap::getInt(const string &lvalue)
  {
    const char *raw = get(lvalue);

    if (raw)
    {
      return atoi(raw);
    }
    else
    {
      return 0;
    }
  }

  //
  // Get value assigned to lvalue key and convert it to hex pointer format.
  //
  // rvalue - key, where to search data (IN).
  //
  // RETURNS: Pointer value assigned to key lvalue or
  //          NULL if key does not exist.
  //

  void *SSMap::getPtr(const char *lvalue)
  {
    const char *raw = get(lvalue);

    if (raw)
    {
      return (void *) strtol(raw, NULL, 16);
    }
    else
    {
      return NULL;
    }
  }

  //
  // Check is lvalue key exist in map.
  //
  // rvalue - key, where to search data (IN).
  //
  // RETURNS: 1 if key exist,
  //          0 otherwise.
  //

  int SSMap::isset(const char *lvalue)
  {
    if (find(lvalue) == end())
    {
      return 0;
    }
    else
    {
      return 1;
    }
  }

  //
  // Check is lvalue key exist in map.
  //
  // rvalue - key, where to search data (IN).
  //
  // RETURNS: 1 if key exist,
  //          0 otherwise.
  //

  int SSMap::isset(const string &lvalue)
  {
    return isset(lvalue.c_str());
  }

  //
  // Assign string list to given lvalue.
  //
  // lvalue     - key, where to assign data (IN).
  // stringList - string's list to assign (IN).
  //

  void SSMap::setStringList(const char *lvalue, vector<string> &stringList)
  {
    string rvalue;

    for (int i = 0; i < stringList.size(); i++)
    {
      rvalue += stringList[i];

      rvalue += ";";
    }

    this -> set(lvalue, rvalue.c_str());
  }

  //
  // Get value assigned to lvalue key and convert it to string's list.
  // WARNING: Stored value must be in token1;token2;token3... format.
  //
  // stringList - stl vector, where to store readed list (OUT).
  // lvalue     - key, where to search data (IN).
  //
  // RETURNS: Pointer value assigned to key lvalue or
  //          NULL if key does not exist.
  //

  void SSMap::getStringList(vector<string> &stringList, const char *lvalue)
  {
    const char *raw = this -> get(lvalue);

    stringList.clear();

    if (raw)
    {
      string rvalue = raw;

      char *token = strtok((char *) rvalue.c_str(), ";");

      while(token)
      {
        stringList.push_back(token);

        token = strtok(NULL, ";");
      }
    }
  }
} /* namespace Tegenaria */
