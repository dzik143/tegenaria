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

#ifndef Tegenaria_Core_Firewall_H
#define Tegenaria_Core_Firewall_H

//
// ----------------------------------------------------------------------------
//
//                           Windows only definitions
//
// ----------------------------------------------------------------------------
//

#ifdef WIN32

#include <windows.h>
#include <cstdio>
#include <Shlwapi.h>

namespace Tegenaria
{
  //
  // INetworkListManager structs.
  //

  typedef enum NLM_CONNECTION_PROPERTY_CHANGE
  {
    NLM_CONNECTION_PROPERTY_CHANGE_AUTHENTICATION   = 0x01
  } NLM_CONNECTION_PROPERTY_CHANGE;

  typedef enum NLM_CONNECTIVITY
  {
    NLM_CONNECTIVITY_DISCONNECTED        = 0x0000,
    NLM_CONNECTIVITY_IPV4_NOTRAFFIC      = 0x0001,
    NLM_CONNECTIVITY_IPV6_NOTRAFFIC      = 0x0002,
    NLM_CONNECTIVITY_IPV4_SUBNET         = 0x0010,
    NLM_CONNECTIVITY_IPV4_LOCALNETWORK   = 0x0020,
    NLM_CONNECTIVITY_IPV4_INTERNET       = 0x0040,
    NLM_CONNECTIVITY_IPV6_SUBNET         = 0x0100,
    NLM_CONNECTIVITY_IPV6_LOCALNETWORK   = 0x0200,
    NLM_CONNECTIVITY_IPV6_INTERNET       = 0x0400
  } NLM_CONNECTIVITY;

  typedef enum NLM_DOMAIN_TYPE
  {
    NLM_DOMAIN_TYPE_NON_DOMAIN_NETWORK     = 0x0,
    NLM_DOMAIN_TYPE_DOMAIN_NETWORK         = 0x01,
    NLM_DOMAIN_TYPE_DOMAIN_AUTHENTICATED   = 0x02
  } NLM_DOMAIN_TYPE;

  typedef enum NLM_ENUM_NETWORK
  {
    NLM_ENUM_NETWORK_CONNECTED      = 0x01,
    NLM_ENUM_NETWORK_DISCONNECTED   = 0x02,
    NLM_ENUM_NETWORK_ALL            = 0x03
  } NLM_ENUM_NETWORK;

  typedef enum NLM_NETWORK_CATEGORY
  {
    NLM_NETWORK_CATEGORY_PUBLIC                 = 0x00,
    NLM_NETWORK_CATEGORY_PRIVATE                = 0x01,
    NLM_NETWORK_CATEGORY_DOMAIN_AUTHENTICATED   = 0x02
  } NLM_NETWORK_CATEGORY;

  typedef enum _NLM_NETWORK_CLASS
  {
    NLM_NETWORK_IDENTIFYING    = 0x01,
    NLM_NETWORK_IDENTIFIED     = 0x02,
    NLM_NETWORK_UNIDENTIFIED   = 0x03
  } NLM_NETWORK_CLASS;

  typedef enum NLM_NETWORK_PROPERTY_CHANGE
  {
    NLM_NETWORK_PROPERTY_CHANGE_CONNECTION       = 0x01,
    NLM_NETWORK_PROPERTY_CHANGE_DESCRIPTION      = 0x02,
    NLM_NETWORK_PROPERTY_CHANGE_NAME             = 0x04,
    NLM_NETWORK_PROPERTY_CHANGE_CATEGORY_VALUE   = 0x10
  } NLM_NETWORK_PROPERTY_CHANGE;

  //
  // INetworkListManager COM API.
  //

  interface IEnumNetworkConnections;

  interface INetwork : public IDispatch
  {
    public:

    virtual HRESULT STDMETHODCALLTYPE GetName(BSTR *pszNetworkName) = 0;

    virtual HRESULT STDMETHODCALLTYPE SetName(BSTR szNetworkNewName) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetDescription(BSTR *pszDescription) = 0;

    virtual HRESULT STDMETHODCALLTYPE SetDescription(BSTR szDescription) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetNetworkId(GUID *pgdGuidNetworkId) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetDomainType(NLM_DOMAIN_TYPE *pNetworkType) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetNetworkConnections(IEnumNetworkConnections **ppEnumNetworkConnection) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetTimeCreatedAndConnected(DWORD *pdwLowDateTimeCreated,
                                                                     DWORD *pdwHighDateTimeCreated,
                                                                         DWORD *pdwLowDateTimeConnected,
                                                                             DWORD *pdwHighDateTimeConnected) = 0;

    virtual HRESULT STDMETHODCALLTYPE get_IsConnectedToInternet(VARIANT_BOOL *pbIsConnected) = 0;

    virtual HRESULT STDMETHODCALLTYPE get_IsConnected(VARIANT_BOOL *pbIsConnected) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetConnectivity(NLM_CONNECTIVITY *pConnectivity) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetCategory(NLM_NETWORK_CATEGORY *pCategory) = 0;

    virtual HRESULT STDMETHODCALLTYPE SetCategory(NLM_NETWORK_CATEGORY NewCategory) = 0;
  };

  interface IEnumNetworks : public IDispatch
  {
    public:

    virtual HRESULT STDMETHODCALLTYPE get__NewEnum(IEnumVARIANT **ppEnumVar) = 0;

    virtual HRESULT STDMETHODCALLTYPE Next(ULONG celt, INetwork **rgelt,
                                               ULONG *pceltFetched) = 0;

    virtual HRESULT STDMETHODCALLTYPE Skip(ULONG celt) = 0;

    virtual HRESULT STDMETHODCALLTYPE Reset(void) = 0;

    virtual HRESULT STDMETHODCALLTYPE Clone(IEnumNetworks **ppEnumNetwork) = 0;
  };

  interface INetworkConnection : public IDispatch
  {
    public:

    virtual HRESULT STDMETHODCALLTYPE GetNetwork(INetwork **ppNetwork) = 0;

    virtual HRESULT STDMETHODCALLTYPE get_IsConnectedToInternet(VARIANT_BOOL *pbIsConnected) = 0;

    virtual HRESULT STDMETHODCALLTYPE get_IsConnected(VARIANT_BOOL *pbIsConnected) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetConnectivity(NLM_CONNECTIVITY *pConnectivity) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetConnectionId(GUID *pgdConnectionId) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetAdapterId(GUID *pgdAdapterId) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetDomainType(NLM_DOMAIN_TYPE *pDomainType) = 0;
  };

  interface IEnumNetworkConnections : public IDispatch
  {
    public:

    virtual HRESULT STDMETHODCALLTYPE get__NewEnum(IEnumVARIANT **ppEnumVar) = 0;

    virtual HRESULT STDMETHODCALLTYPE Next(ULONG celt, INetworkConnection **rgelt,
                                               ULONG *pceltFetched) = 0;

    virtual HRESULT STDMETHODCALLTYPE Skip(ULONG celt) = 0;

    virtual HRESULT STDMETHODCALLTYPE Reset(void) = 0;

    virtual HRESULT STDMETHODCALLTYPE Clone(IEnumNetworkConnections **ppEnumNetwork) = 0;
  };

  interface INetworkListManager : public IDispatch
  {
    public:

    virtual HRESULT GetNetworks(NLM_ENUM_NETWORK Flags, IEnumNetworks **ppEnumNetwork) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetNetwork(GUID gdNetworkId, INetwork **ppNetwork) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetNetworkConnections(IEnumNetworkConnections **ppEnum) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetNetworkConnection(GUID gdNetworkConnectionId, INetworkConnection **ppNetworkConnection) = 0;

    virtual HRESULT STDMETHODCALLTYPE get_IsConnectedToInternet(VARIANT_BOOL *pbIsConnected) = 0;

    virtual HRESULT STDMETHODCALLTYPE get_IsConnected(VARIANT_BOOL *pbIsConnected) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetConnectivity(NLM_CONNECTIVITY *pConnectivity) = 0;
  };

  //
  // Import FWNet classes.
  //

  //
  // Defines.
  //

  #define WBEM_CALL virtual HRESULT STDMETHODCALLTYPE

  //
  // Forward declarations.
  //

  class INetFwPolicy2;
  class INetFwRules;
  class INetFwRule;
  class NetFwRule;
  class NetFwPolicy2;

  //
  // Typedef.
  //

  typedef enum NET_FW_PROFILE_TYPE2_
  {
    NET_FW_PROFILE2_DOMAIN  = 0x1,
    NET_FW_PROFILE2_PRIVATE = 0x2,
    NET_FW_PROFILE2_PUBLIC  = 0x4,
    NET_FW_PROFILE2_ALL     = 0x7fffffff
  }
  NET_FW_PROFILE_TYPE2;

  typedef enum NET_FW_ACTION_
  {
    NET_FW_ACTION_BLOCK = 0,
    NET_FW_ACTION_ALLOW = (NET_FW_ACTION_BLOCK + 1),
    NET_FW_ACTION_MAX   = (NET_FW_ACTION_ALLOW + 1)
  }
  NET_FW_ACTION;

  typedef enum NET_FW_RULE_DIRECTION_
  {
    NET_FW_RULE_DIR_IN  = 1,
    NET_FW_RULE_DIR_OUT = (NET_FW_RULE_DIR_IN + 1),
    NET_FW_RULE_DIR_MAX = (NET_FW_RULE_DIR_OUT + 1)
  }
  NET_FW_RULE_DIRECTION;

  typedef enum NET_FW_MODIFY_STATE_
  {
    NET_FW_MODIFY_STATE_OK              = 0,
    NET_FW_MODIFY_STATE_GP_OVERRIDE     = (NET_FW_MODIFY_STATE_OK + 1),
    NET_FW_MODIFY_STATE_INBOUND_BLOCKED = (NET_FW_MODIFY_STATE_GP_OVERRIDE + 1)
  }
  NET_FW_MODIFY_STATE;

  typedef enum NET_FW_EDGE_TRAVERSAL_TYPE_
  {
    NET_FW_EDGE_TRAVERSAL_TYPE_DENY,
    NET_FW_EDGE_TRAVERSAL_TYPE_ALLOW,
    NET_FW_EDGE_TRAVERSAL_TYPE_DEFER_TO_APP,
    NET_FW_EDGE_TRAVERSAL_TYPE_DEFER_TO_USER
  }
  NET_FW_EDGE_TRAVERSAL_TYPE;

  //
  // Interfaces.
  //

  MIDL_INTERFACE("8267BBE3-F890-491C-B7B6-2DB1EF0E5D2B")
  INetFwServiceRestriction : public IDispatch
  {
    public:

    WBEM_CALL RestrictService(BSTR, BSTR, VARIANT_BOOL, VARIANT_BOOL) = 0;

    WBEM_CALL ServiceRestricted(BSTR, BSTR, VARIANT_BOOL *) = 0;

    WBEM_CALL get_Rules(INetFwRules **) = 0;
  };

  MIDL_INTERFACE("AF230D27-BABA-4E42-ACED-F524F22CFCE2")
  INetFwRule : public IDispatch
  {
    public:

    WBEM_CALL get_Name(BSTR *) = 0;

    WBEM_CALL put_Name(BSTR) = 0;

    WBEM_CALL get_Description(BSTR *) = 0;

    WBEM_CALL put_Description(BSTR) = 0;

    WBEM_CALL get_ApplicationName(BSTR *) = 0;

    WBEM_CALL put_ApplicationName(BSTR) = 0;

    WBEM_CALL get_ServiceName(BSTR *) = 0;

    WBEM_CALL put_ServiceName(BSTR) = 0;

    WBEM_CALL get_Protocol(LONG *) = 0;

    WBEM_CALL put_Protocol(LONG) = 0;

    WBEM_CALL get_LocalPorts(BSTR *) = 0;

    WBEM_CALL put_LocalPorts(BSTR) = 0;

    WBEM_CALL get_RemotePorts(BSTR *) = 0;

    WBEM_CALL put_RemotePorts(BSTR) = 0;

    WBEM_CALL get_LocalAddresses(BSTR *) = 0;

    WBEM_CALL put_LocalAddresses(BSTR) = 0;

    WBEM_CALL get_RemoteAddresses(BSTR *) = 0;

    WBEM_CALL put_RemoteAddresses(BSTR) = 0;

    WBEM_CALL get_IcmpTypesAndCodes(BSTR *) = 0;

    WBEM_CALL put_IcmpTypesAndCodes(BSTR) = 0;

    WBEM_CALL get_Direction(NET_FW_RULE_DIRECTION *) = 0;

    WBEM_CALL put_Direction(NET_FW_RULE_DIRECTION) = 0;

    WBEM_CALL get_Interfaces(VARIANT *) = 0;

    WBEM_CALL put_Interfaces(VARIANT) = 0;

    WBEM_CALL get_InterfaceTypes(BSTR *) = 0;

    WBEM_CALL put_InterfaceTypes(BSTR) = 0;

    WBEM_CALL get_Enabled(VARIANT_BOOL *) = 0;

    WBEM_CALL put_Enabled(VARIANT_BOOL) = 0;

    WBEM_CALL get_Grouping(BSTR *) = 0;

    WBEM_CALL put_Grouping(BSTR) = 0;

    WBEM_CALL get_Profiles(long *) = 0;

    WBEM_CALL put_Profiles(long) = 0;

    WBEM_CALL get_EdgeTraversal(VARIANT_BOOL *) = 0;

    WBEM_CALL put_EdgeTraversal(VARIANT_BOOL) = 0;

    WBEM_CALL get_Action(NET_FW_ACTION *) = 0;

    WBEM_CALL put_Action(NET_FW_ACTION) = 0;
  };

  MIDL_INTERFACE("9C27C8DA-189B-4DDE-89F7-8B39A316782C")
  INetFwRule2 : public INetFwRule
  {
    public:

    WBEM_CALL get_EdgeTraversalOptions(long *) = 0;

    WBEM_CALL put_EdgeTraversalOptions(long) = 0;
  };

  MIDL_INTERFACE("9C4C6277-5027-441E-AFAE-CA1F542DA009")
  INetFwRules : public IDispatch
  {
    public:

    WBEM_CALL get_Count(long *) = 0;

    WBEM_CALL Add(INetFwRule *) = 0;

    WBEM_CALL Remove(BSTR) = 0;

    WBEM_CALL Item(BSTR, INetFwRule **) = 0;

    WBEM_CALL get__NewEnum(IUnknown **) = 0;
  };

  MIDL_INTERFACE("98325047-C671-4174-8D81-DEFCD3F03186")
  IWbemClassObject : public IUnknown
  {
    public:

    WBEM_CALL get_CurrentProfileTypes(long *) = 0;

    WBEM_CALL get_FirewallEnabled(NET_FW_PROFILE_TYPE2, VARIANT_BOOL *) = 0;

    WBEM_CALL put_FirewallEnabled(NET_FW_PROFILE_TYPE2, VARIANT_BOOL) = 0;

    WBEM_CALL get_ExcludedInterfaces(NET_FW_PROFILE_TYPE2, VARIANT *) = 0;

    WBEM_CALL put_ExcludedInterfaces(NET_FW_PROFILE_TYPE2, VARIANT) = 0;

    WBEM_CALL get_BlockAllInboundTraffic(NET_FW_PROFILE_TYPE2, VARIANT_BOOL *) = 0;

    WBEM_CALL put_BlockAllInboundTraffic(NET_FW_PROFILE_TYPE2, VARIANT_BOOL) = 0;

    WBEM_CALL get_NotificationsDisabled(NET_FW_PROFILE_TYPE2, VARIANT_BOOL *) = 0;

    WBEM_CALL put_NotificationsDisabled(NET_FW_PROFILE_TYPE2, VARIANT_BOOL) = 0;

    WBEM_CALL get_UnicastResponsesToMulticastBroadcastDisabled(NET_FW_PROFILE_TYPE2, VARIANT_BOOL *) = 0;

    WBEM_CALL put_UnicastResponsesToMulticastBroadcastDisabled(NET_FW_PROFILE_TYPE2, VARIANT_BOOL) = 0;

    WBEM_CALL get_Rules(INetFwRules **) = 0;

    WBEM_CALL get_ServiceRestriction(INetFwServiceRestriction **) = 0;

    WBEM_CALL EnableRuleGroup(long, BSTR, VARIANT_BOOL) = 0;

    WBEM_CALL IsRuleGroupEnabled(long, BSTR, VARIANT_BOOL *) = 0;

    WBEM_CALL RestoreLocalFirewallDefaults() = 0;

    WBEM_CALL get_DefaultInboundAction(NET_FW_PROFILE_TYPE2, NET_FW_ACTION *) = 0;

    WBEM_CALL put_DefaultInboundAction(NET_FW_PROFILE_TYPE2, NET_FW_ACTION) = 0;

    WBEM_CALL get_DefaultOutboundAction(NET_FW_PROFILE_TYPE2, NET_FW_ACTION *) = 0;

    WBEM_CALL put_DefaultOutboundAction(NET_FW_PROFILE_TYPE2, NET_FW_ACTION) = 0;

    WBEM_CALL get_IsRuleGroupCurrentlyEnabled(BSTR, VARIANT_BOOL *) = 0;

    WBEM_CALL get_LocalPolicyModifyState(NET_FW_MODIFY_STATE *) = 0;
  };

  MIDL_INTERFACE("98325047-C671-4174-8D81-DEFCD3F03186")
  INetFwPolicy2 : public IDispatch
  {
    public:

    WBEM_CALL get_CurrentProfileTypes(long *) = 0;

    WBEM_CALL get_FirewallEnabled(NET_FW_PROFILE_TYPE2, VARIANT_BOOL *) = 0;

    WBEM_CALL put_FirewallEnabled(NET_FW_PROFILE_TYPE2, VARIANT_BOOL) = 0;

    WBEM_CALL get_ExcludedInterfaces(NET_FW_PROFILE_TYPE2, VARIANT *) = 0;

    WBEM_CALL put_ExcludedInterfaces(NET_FW_PROFILE_TYPE2, VARIANT) = 0;

    WBEM_CALL get_BlockAllInboundTraffic(NET_FW_PROFILE_TYPE2, VARIANT_BOOL *) = 0;

    WBEM_CALL put_BlockAllInboundTraffic(NET_FW_PROFILE_TYPE2, VARIANT_BOOL) = 0;

    WBEM_CALL get_NotificationsDisabled(NET_FW_PROFILE_TYPE2, VARIANT_BOOL *) = 0;

    WBEM_CALL put_NotificationsDisabled(NET_FW_PROFILE_TYPE2, VARIANT_BOOL) = 0;

    WBEM_CALL get_UnicastResponsesToMulticastBroadcastDisabled(NET_FW_PROFILE_TYPE2, VARIANT_BOOL *) = 0;

    WBEM_CALL put_UnicastResponsesToMulticastBroadcastDisabled(NET_FW_PROFILE_TYPE2, VARIANT_BOOL) = 0;

    WBEM_CALL get_Rules(INetFwRules **rules) = 0;

    WBEM_CALL get_ServiceRestriction(INetFwServiceRestriction **ServiceRestriction) = 0;

    WBEM_CALL EnableRuleGroup(long, BSTR, VARIANT_BOOL) = 0;

    WBEM_CALL IsRuleGroupEnabled(long, BSTR, VARIANT_BOOL *) = 0;

    WBEM_CALL RestoreLocalFirewallDefaults() = 0;

    WBEM_CALL get_DefaultInboundAction(NET_FW_PROFILE_TYPE2, NET_FW_ACTION *) = 0;

    WBEM_CALL put_DefaultInboundAction(NET_FW_PROFILE_TYPE2, NET_FW_ACTION) = 0;

    WBEM_CALL get_DefaultOutboundAction(NET_FW_PROFILE_TYPE2, NET_FW_ACTION *) = 0;

    WBEM_CALL put_DefaultOutboundAction(NET_FW_PROFILE_TYPE2, NET_FW_ACTION) = 0;

    WBEM_CALL get_IsRuleGroupCurrentlyEnabled(BSTR, VARIANT_BOOL *) = 0;

    WBEM_CALL get_LocalPolicyModifyState(NET_FW_MODIFY_STATE *) = 0;
  };

} /* namespace Tegenaria */

#endif /* WIN32 */

#endif /* Tegenaria_Core_Firewall_H */
