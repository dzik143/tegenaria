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
// Purpose: Generic implementation of access lists.
//

#include "Secure.h"

namespace Tegenaria
{
  //
  // Create empty, NULL access list.
  //

  SecureAcl::SecureAcl()
  {
    this -> clear();
  }

  //
  // Init access list from raw string.
  //
  // String format is: "user1:rights1;user2:rights2...*:otherRights"
  //
  // Example lists are:
  //
  //   "jan:R;jozek:F;*:D" - jan can read, jozek can read and write, others have no access
  //   "jan:F;*:R"         - jan can read and write, others can read only
  //   "*:F"               - all can read and write
  //   "*:D"               - nobody have access
  //   ""                  - invalid data, * terminator missing.
  //
  //   R = read only
  //   F = full access
  //   D = access denied
  //
  //   * = others users not included in list explicite. SHOULD be the
  //   last entry on the access list.
  //
  // Parameters:
  //
  // acl - string containing correct access list in format describet above (IN).
  //
  // RETURNS: 0 if OK,
  //         -1 otherwise.
  //

  int SecureAcl::initFromString(const char *acl)
  {
    DBG_ENTER3("SecureAcl::initFromString");

    int exitCode = -1;

    char *tmp    = NULL;
    char *token  = NULL;
    char *user   = NULL;
    char *rights = NULL;
    char *delim  = NULL;

    //
    // Clear current list on startup.
    //

    this -> clear();

    //
    // Check args.
    //

    FAILEX(acl == NULL, "ERROR: 'acl' cannot be NULL in SeucureAcl::initFromString().\n");

    //
    // Duplicate input string.
    // We will tokenize it with destructive way.
    //

    tmp = strdup(acl);

    //
    // Tokenize input string.
    // We except list in format: "user1:rights1;user2:rights2;..."
    //

    token = strtok(tmp, ";");

    while(token)
    {
      //
      // One element is user:rights.
      // Split into user and message part.
      //

      delim = strchr(token, ':');

      FAILEX(delim == NULL, "ERROR: Missing ':' delimer in ACL string.\n");

      *delim = 0;

      user   = token;
      rights = delim + 1;

      //
      // Put to rights into user |-> rights map.
      //

      DEBUG3("SecureAcl::initFromString : Granting rights [%s] to [%s].\n", rights, user);

      rights_[user] = encodeRights(rights);

      //
      // Go to next entry on list.
      //

      token = strtok(NULL, ";");
    }

    //
    // Error handler.
    //

    exitCode = 0;

    fail:

    if (exitCode)
    {
      Error("ERROR: Cannot init access list object from '%s' string.\n", acl);

      this -> clear();
    }

    if (tmp)
    {
      free(tmp);
    }

    DBG_LEAVE3("SecureAcl::initFromString");

    return exitCode;
  }

  //
  // Set user rights to given value.
  //
  // WARNING#1: If user has already some rights granted function will OVERWRITE
  //            them with new one.
  //
  // TIP#1: To read back rights granted to user use getRights() method.
  //
  // user   - username, which we want grant rights to (IN).
  //
  // rights - rights, which we want to grant in binary representation i.e.
  //          combination of SECURE_ACL_XXX flags defined in Secure.h (IN).
  //
  // RETURNS: 0 if OK.
  //

  int SecureAcl::setRights(const char *user, int rights)
  {
    DBG_ENTER3("SecureAcl::setRights");

    int exitCode = -1;

    //
    // Check args.
    //

    FAILEX(user == NULL, "ERROR: 'user' cannot be NULL in SecureAcl::setRights().\n");

    //
    // Grant rights to user.
    //

    rights_[user] = rights;

    //
    // Error handler.
    //

    exitCode = 0;

    fail:

    DBG_LEAVE3("SecureAcl::setRights");

    return exitCode;
  }

  //
  // Set user rights to given value.
  //
  // WARNING#1: If user has already some rights granted function will OVERWRITE
  //            them with new one.
  //
  // TIP#1: To read back rights granted to user use getRights() method.
  //
  // user   - username, which we want grant rights to (IN).
  //
  // rights - rights, which we want to grant as human readable string i.e.
  //          combination of SECURE_ACL_SYMBOL_XXX chars defined in Secure.h.
  //          Example string is "D" (deny) or "F" (Full access) (IN).
  //
  // RETURNS: 0 if OK.
  //

  int SecureAcl::setRights(const char *user, const char *rights)
  {
    return this -> setRights(user, encodeRights(rights));
  }

  //
  // Set rights granted to others users to given value. Others means all users
  // not mentioned in ACL explicite.
  //
  // WARNING#1: If others has already some rights granted function will OVERWRITE
  //            them with new one.
  //
  // rights - rights, which we want to grant in binary representation i.e.
  //          combination of SECURE_ACL_XXX flags defined in Secure.h (IN).
  //
  // RETURNS: 0 if OK.
  //

  int SecureAcl::setOthersRights(int rights)
  {
    return this -> setRights("*", rights);
  }

  //
  // Set rights granted to others users to given value. Others means all users
  // not mentioned in ACL explicite.
  //
  // WARNING#1: If others has already some rights granted function will OVERWRITE
  //            them with new one.
  //
  // rights - rights, which we want to grant as human readable string i.e.
  //          combination of SECURE_ACL_SYMBOL_XXX chars defined in Secure.h.
  //          Example string is "D" (deny) or "F" (Full access) (IN).
  //
  // RETURNS: 0 if OK.
  //

  int SecureAcl::setOthersRights(const char *rights)
  {
    return this -> setRights("*", encodeRights(rights));
  }

  //
  // Remove user from accesslist. After that user has no any rights granted.
  //
  // user - user, which we want revoke rights for (IN).
  //
  // RETURNS: 0 if OK.
  //

  int SecureAcl::revokeAll(const char *user)
  {
    DBG_ENTER3("SecureAcl::revokeAll");

    int exitCode = -1;

    //
    // Check args.
    //

    FAILEX(user == NULL, "ERROR: 'user' cannot be NULL in SecureAcl::revokeAll().\n");

    //
    // Remove user from righrs map.
    //

    rights_.erase(user);

    //
    // Error handler.
    //

    exitCode = 0;

    fail:

    DBG_LEAVE3("SecureAcl::removeAll");

    return exitCode;
  }

  //
  // Gather rights for given users.
  //
  // user - name of user, which we want rights for (IN).
  //
  // RETURNS: Rights granted to given user.
  //

  int SecureAcl::getRights(const char *user)
  {
    DBG_ENTER3("SecureAcl::getRights");

    int ret = SECURE_ACL_DENY;

    map<string, int>::iterator it;

    //
    // Check args.
    //

    FAILEX(user == NULL, "ERROR: 'User' cannot be NULL in SecureAcl::getRights().\n");

    //
    // Try find user in rights map.
    //

    it = rights_.find(user);

    //
    // User exists in map, get rights from map.
    //

    if (it != rights_.end())
    {
      ret = it -> second;
    }

    //
    // User does not exist in map, get rights for others user (*).
    //

    else
    {
      ret = rights_["*"];
    }

    //
    // Error handler.
    //

    fail:

    DBG_LEAVE3("SecureAcl::getRights");

    return ret;
  }

  //
  // Gather rights for given users.
  //
  // user - name of user, which we want rights for (IN).
  //
  // RETURNS: Rights granted to given user as human readable string.
  //

  string SecureAcl::getRightsString(const char *user)
  {
    return decodeRights(this -> getRights(user));
  }

  //
  // Convert access list stored inside object into ACL string.
  //

  string SecureAcl::toString()
  {
    DBG_ENTER3("SecureAcl::toString");

    string ret;

    int othersRights;

    map<string, int>::iterator it;

    //
    // Iterate over whole [user] |-> [rights] map.
    //

    for (it = rights_.begin(); it != rights_.end(); it++)
    {
      //
      // Others rights. Save for future.
      // We want put it as last entry in string.
      //

      if (it -> first == "*")
      {
        othersRights = it -> second;
      }

      //
      // Ordinary user, put to string.
      //

      else
      {
        ret += it -> first;
        ret += ':';
        ret += decodeRights(it -> second);
        ret += ';';
      }
    }

    //
    // Add others rights as * keyword.
    //

    ret += "*:";
    ret += decodeRights(othersRights);
    ret += ';';

    DBG_LEAVE3("SecureAcl::toString");

    return ret;
  }

  //
  // Revoke all grant from all users stored accesslist.
  //

  void SecureAcl::clear()
  {
    DBG_ENTER3("SecureAcl::clear");

    rights_.clear();

    rights_["*"] = SECURE_ACL_DENY;

    DBG_LEAVE3("SecureAcl::clear");
  }

  //
  // Encode rights string into its binary representation.
  // Rights in binary representation is combination of SECURE_ACL_XXX flags
  // defined in Secure.h file.
  //
  // WARNING#1: If input rights string is incorrect function assign SECURE_ACL_DENY
  //            rights value.
  //
  // rights - human readable string representing rights to encode, for list of
  //          allowed chars see SECURE_ACL_SYMBOL_XXX defines in Secure.h (IN).
  //
  // RETURNS: Binary representation of rights.
  //

  int SecureAcl::encodeRights(const char *str)
  {
    DBG_ENTER3("SecureAcl::encodeRights");

    int rights = 0;

    //
    // Translate rights from string into rights codes.
    //

    if (str)
    {
      for (int i = 0; str[i]; i++)
      {
        switch(str[i])
        {
          case SECURE_ACL_SYMBOL_DENY  : rights |= SECURE_ACL_DENY; break;
          case SECURE_ACL_SYMBOL_FULL  : rights |= SECURE_ACL_FULL; break;
          case SECURE_ACL_SYMBOL_READ  : rights |= SECURE_ACL_READ; break;
          case SECURE_ACL_SYMBOL_WRITE : rights |= SECURE_ACL_WRITE; break;
          case SECURE_ACL_SYMBOL_ERASE : rights |= SECURE_ACL_ERASE; break;
        }
      }
    }

    //
    // Normalize rights.
    //

    if ((rights == 0) || (rights & SECURE_ACL_DENY))
    {
      rights = SECURE_ACL_DENY;
    }
    else if (rights & SECURE_ACL_FULL)
    {
      rights = SECURE_ACL_FULL;
    }

    DBG_LEAVE3("SecureAcl::encodeRights");

    return rights;
  }

  //
  // Decode rights value into string.
  // String contains chars representing single rights eg. "RW" means READ+WRITE.
  // Full list of allowed rights chars is defined at SECURE_ACL_SYMBOL_XXX defines
  // in Secure.h file
  //
  // WARNING#1: If input rights value is incorrect function assign "D" (access denied)
  //            rights.
  //
  // rights - binary encoded rights i.e. combination of SECURE_ACL_XXX flags
  //          defined in Secure.h (IN).
  //
  // RETURNS: Human readable string representing rights.
  //

  string SecureAcl::decodeRights(int rights)
  {
    DBG_ENTER3("SecureAcl::decodeRights");

    string ret;

    //
    // Deny.
    //

    if ((rights == 0) || (rights & SECURE_ACL_DENY))
    {
      ret = SECURE_ACL_SYMBOL_DENY;
    }

    //
    // Full.
    //

    else if (rights & SECURE_ACL_FULL)
    {
      ret = SECURE_ACL_SYMBOL_FULL;
    }

    //
    // Complex list.
    // Create composition from single rights.
    //

    else
    {
      if (rights & SECURE_ACL_READ)  ret += SECURE_ACL_SYMBOL_READ;
      if (rights & SECURE_ACL_WRITE) ret += SECURE_ACL_SYMBOL_WRITE;
      if (rights & SECURE_ACL_ERASE) ret += SECURE_ACL_SYMBOL_ERASE;
    }

    DBG_LEAVE3("SecureAcl::decodeRights");

    return ret;
  }
} /* namespace Tegenaria */
