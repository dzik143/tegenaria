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
// Includes.
//

#pragma qcbuild_set_file_title("Manage firewall rules (Windows only)");

#include "Net.h"
#include "Firewall.h"

namespace Tegenaria
{
  #ifdef WIN32

  //
  // Defines.
  //

  #define HFAIL(X) if ((hresult = (X))) goto fail

  #define HFAILEX(X, ...) if ((hresult = (X))) {Error(__VA_ARGS__); goto fail;}

  #define COM_FREE(X) if ((X) != NULL) {(X) -> Release(); X = NULL;}

  //
  // Define guids to import NetFW interfaces.
  //

  const unsigned char CLSID_NetFwPolicy2_raw[] =
  {
    0x7f, 0xc9, 0xb3, 0xe2,
    0xe1, 0x6a, 0xac, 0x41,
    0x81, 0x7a, 0xf6, 0xf9,
    0x21, 0x66, 0xd7, 0xdd
  };

  const unsigned char CLSID_NetFwRule_raw[] =
  {
    0x3e, 0xc4, 0x5b, 0x2c,
    0x69, 0x33, 0x33, 0x4c,
    0xab, 0x0c, 0xbe, 0x94,
    0x69, 0x67, 0x7a, 0xf4
  };

  const unsigned char CLSID_NetFwRule2_raw[] =
  {
    0xda, 0xc8, 0x27, 0x9c,
    0x9b, 0x18, 0xde, 0x4d,
    0x89, 0xf7, 0x8b, 0x39,
    0xa3, 0x16, 0x78, 0x2c,
  };

  const unsigned char IID_INetFwPolicy2_raw[] =
  {
    0x47, 0x50, 0x32, 0x98,
    0x71, 0xc6, 0x74, 0x41,
    0x8d, 0x81, 0xde, 0xfc,
    0xd3, 0xf0, 0x31, 0x86
  };

  const unsigned char IID_INetFwRule_raw[] =
  {
    0x27, 0x0d, 0x23, 0xaf,
    0xba, 0xba, 0x42, 0x4e,
    0xac, 0xed, 0xf5, 0x24,
    0xf2, 0x2c, 0xfc, 0xe2
  };

  const unsigned char IID_INetFwRule2_raw[] =
  {
    0xda, 0xc8, 0x27, 0x9c,
    0x9b, 0x18, 0xde, 0x4d,
    0x89, 0xf7, 0x8b, 0x39,
    0xa3, 0x16, 0x78, 0x2c
  };

  const CLSID &CLSID_NetFwPolicy2 = (CLSID &) CLSID_NetFwPolicy2_raw;
  const CLSID &CLSID_NetFwRule    = (CLSID &) CLSID_NetFwRule_raw;
  const CLSID &CLSID_NetFwRule2   = (CLSID &) CLSID_NetFwRule2_raw;

  const IID &IID_INetFwPolicy2 = (IID &) IID_INetFwPolicy2_raw;
  const IID &IID_INetFwRule    = (IID &) IID_INetFwRule_raw;
  const IID &IID_INetFwRule2   = (IID &) IID_INetFwRule2_raw;

  #endif

  //
  // Add firewall rule.
  //
  // WARNING: Function needs admin privileges.
  //
  // name    - name of rule (IN).
  // group   - name of rule's group (IN).
  // desc    - rule's description (IN).
  // appPath - full path to target application (IN).
  //
  // RETURNS: 0 if OK.
  //

  int NetFirewallRuleAdd(const char *name, const char *group,
                             const char *desc, const char *appPath, int protocol)
  {
    DBG_ENTER("NetFirewallRuleAdd");

    int exitCode = -1;

    //
    // Windows.
    //

    #ifdef WIN32
    {
      HRESULT hresult = S_OK;

      INetFwRules *rulesSet = NULL;
      INetFwRule  *rule     = NULL;
      INetFwRule2 *rule2    = NULL;

      BSTR nameBstr    = NULL;
      BSTR groupBstr   = NULL;
      BSTR descBstr    = NULL;
      BSTR appPathBstr = NULL;

      wchar_t nameW[1024]    = {0};
      wchar_t groupW[1024]   = {0};
      wchar_t descW[1024]    = {0};
      wchar_t appPathW[1024] = {0};

      INetFwPolicy2 *policy = NULL;

      //
      // Initialize COM API.
      //

      hresult = CoInitializeEx(0, COINIT_APARTMENTTHREADED);

      FAILEX(hresult && hresult != RPC_E_CHANGED_MODE,
                 "ERROR: Cannot initialize COM API.\n");

      //
      // Import NetFwPolicy2 class via COM.
      //

      HFAILEX(CoCreateInstance(CLSID_NetFwPolicy2, NULL, CLSCTX_INPROC_SERVER,
                                    IID_INetFwPolicy2, (void **) &policy),
                                        "ERROR: Cannot import NetFwPolicy2 class.\n");

      //
      // NetFW classes needs dynamic BSTR UTF16 strings.
      // Convert from ASCII to static UTF16 first.
      //

      _snwprintf(nameW, sizeof(nameW) / sizeof(nameW[0]), L"%hs", name);
      _snwprintf(groupW, sizeof(groupW) / sizeof(groupW[0]), L"%hs", group);
      _snwprintf(descW, sizeof(descW) / sizeof(descW[0]), L"%hs", desc);
      _snwprintf(appPathW, sizeof(appPathW) / sizeof(appPathW[0]), L"%hs", appPath);

      DBG_MSG("Name        : [%ls]\n", nameW);
      DBG_MSG("Group       : [%ls]\n", groupW);
      DBG_MSG("Description : [%ls]\n", descW);
      DBG_MSG("Application : [%ls]\n", appPathW);
      DBG_MSG("Protocol    : [%d]\n", protocol);

      //
      // Create dynamic BSTR UTF16 strings from static UTF16.
      //

      nameBstr    = SysAllocString(nameW);
      groupBstr   = SysAllocString(groupW);
      descBstr    = SysAllocString(descW);
      appPathBstr = SysAllocString(appPathW);

      FAILEX(nameBstr == NULL, "ERROR: Out of memory.\n");
      FAILEX(groupBstr == NULL, "ERROR: Out of memory.\n");
      FAILEX(descBstr == NULL, "ERROR: Out of memory.\n");
      FAILEX(appPathBstr == NULL, "ERROR: Out of memory.\n");

      //
      // Get rules collection from policy.
      //

      HFAILEX(policy -> get_Rules(&rulesSet),
                  "ERROR: Cannot to retrieve firewall rules collection");

      //
      // Create new, empty rule.
      //

      HFAILEX(CoCreateInstance(CLSID_NetFwRule, NULL, CLSCTX_INPROC_SERVER,
                                   IID_INetFwRule, (void **) &rule),
                                       "ERROR: Cannot create NetFwRule object.\n");

      //
      // Fill empty rule with target name, group, desc, app.
      //

      HFAILEX(rule -> put_Name(nameBstr),
                  "ERROR: Cannot to set rule's name.\n");
    /*
      HFAILEX(rule -> put_Grouping(groupBstr),
                  "ERROR: Cannot to set rule's group.\n");
    */

      HFAILEX(rule -> put_Description(descBstr),
                  "ERROR: Cannot to set rule's description.\n");

      HFAILEX(rule -> put_Direction(NET_FW_RULE_DIR_IN),
                  "ERROR: Cannot to set rule's direction.\n");

      HFAILEX(rule -> put_Action(NET_FW_ACTION_ALLOW),
                  "ERROR: Cannot to set rule's action.\n");

      HFAILEX(rule -> put_ApplicationName(appPathBstr),
                  "ERROR: Cannot to set rule's' application.\n");

      HFAILEX(rule -> put_Protocol(protocol),
                  "ERROR: Cannot to set rule's protocol.\n");

      HFAILEX(rule -> put_Profiles(NET_FW_PROFILE2_DOMAIN | NET_FW_PROFILE2_PRIVATE),
                  "ERROR: Cannot to set rule's profiles.\n");

      //
      // Switch rule to enable state.
      //

      HFAILEX(rule -> put_Enabled(VARIANT_TRUE),
                  "ERROR: Cannot to enable rule.\n");

      //
      // Check if INetFwRule2 interface is available (i.e Windows7+)
      // If supported, then use EdgeTraversalOptions
      // Else use the EdgeTraversal boolean flag.
      //

      if (SUCCEEDED(rule -> QueryInterface(IID_INetFwRule2, (void **) &rule2)))
      {
        HFAILEX(rule2 -> put_EdgeTraversalOptions(NET_FW_EDGE_TRAVERSAL_TYPE_DEFER_TO_APP),
                    "ERROR: Cannot to set edge traversal.\n");
      }
      else
      {
        HFAILEX(rule -> put_EdgeTraversal(VARIANT_TRUE),
                    "ERROR: Cannot to set edge traversal.\n");
      }

      //
      // Add rule to rules collection.
      //

      HFAIL(rulesSet -> Add(rule));

      //
      // Clean up.
      //

      exitCode = 0;

      fail:

      if (exitCode)
      {
        Error("ERROR: Cannot add firewall rule.\n"
                  "Error code is : 0x%x\n", (unsigned int) hresult);
      }

      SysFreeString(nameBstr);
      SysFreeString(groupBstr);
      SysFreeString(descBstr);
      SysFreeString(appPathBstr);

      COM_FREE(policy);
      COM_FREE(rulesSet);
      COM_FREE(rule);
      COM_FREE(rule2);

      CoUninitialize();
    }

    //
    // Linux, MacOS.
    //

    #else
    {
      Error("ERROR: NetFirewallRuleAdd() not implemented on this OS.\n");

      exitCode = -1;
    }
    #endif

    DBG_LEAVE("NetFirewallRuleAdd");

    return exitCode;
  }

  //
  // Remove firewall rule.
  //
  // WARNING: Function needs admin privileges.
  //
  // name - name of rule to remove (IN).
  //
  // RETURNS: 0 if OK.
  //

  int NetFirewallRuleDel(const char *name)
  {
    DBG_ENTER("NetFirewallRuleDel");

    int exitCode = -1;

    //
    // Windows
    //

    #ifdef WIN32
    {
      HRESULT hresult = S_OK;

      INetFwRules *rulesSet = NULL;

      BSTR nameBstr = NULL;

      wchar_t nameW[1024] = {0};

      INetFwPolicy2 *policy = NULL;

      //
      // Initialize COM API.
      //

      hresult = CoInitializeEx(0, COINIT_APARTMENTTHREADED);

      FAILEX(hresult && hresult != RPC_E_CHANGED_MODE,
                 "ERROR: Cannot initialize COM API.\n");

      //
      // Import NetFwPolicy2 class via COM.
      //

      HFAILEX(CoCreateInstance(CLSID_NetFwPolicy2, NULL, CLSCTX_INPROC_SERVER,
                                    IID_INetFwPolicy2, (void **) &policy),
                                        "ERROR: Cannot import NetFwPolicy2 class.\n");

      //
      // NetFW classes needs dynamic BSTR UTF16 strings.
      // Convert from ASCII to static UTF16 first.
      //

      _snwprintf(nameW, sizeof(nameW) / sizeof(nameW[0]), L"%hs", name);

      //
      // Create dynamic BSTR UTF16 strings from static UTF16.
      //

      nameBstr = SysAllocString(nameW);

      FAILEX(nameBstr == NULL, "ERROR: Out of memory.\n");

      //
      // Get rules collection from policy.
      //

      HFAILEX(policy -> get_Rules(&rulesSet),
                  "ERROR: Cannot to retrieve firewall rules collection");

      //
      // Remove rule from rules collection.
      //

      HFAIL(rulesSet -> Remove(nameBstr));

      //
      // Clean up.
      //

      exitCode = 0;

      fail:

      if (exitCode)
      {
        Error("ERROR: Cannot remove firewall rule '%s'.\n"
                  "Error code is : 0x%x\n", name, (unsigned int) hresult);
      }

      SysFreeString(nameBstr);

      COM_FREE(policy);
      COM_FREE(rulesSet);

      CoUninitialize();
    }

    //
    // Linux, MacOS.
    //

    #else
    {
      Error("ERROR: NetFirewallRuleDel() not implemented on this OS.\n");

      exitCode = -1;
    }
    #endif

    DBG_LEAVE("NetFirewallRuleDel");

    return exitCode;
  }
} /* namespace Tegenaria */
