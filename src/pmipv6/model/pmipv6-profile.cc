/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Proxy Mobile IPv6 (PMIPv6) (RFC5213) Implementation
 *
 * Copyright (c) 2010 KUT, ETRI
 * (Korea Univerity of Technology and Education)
 * (Electronics and Telecommunications Research Institute)
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Hyon-Young Choi <commani@gmail.com>
 */

#include "ns3/log.h"
#include "ns3/uinteger.h"
#include "ns3/node.h"

#include "pmipv6-profile.h"

NS_LOG_COMPONENT_DEFINE ("Pmipv6Profile");

namespace ns3
{

NS_OBJECT_ENSURE_REGISTERED (Pmipv6Profile);

TypeId Pmipv6Profile::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::Pmipv6Profile")
    .SetParent<Object> ()
	.AddConstructor<Pmipv6Profile>()
    ;
  return tid;
} 

Pmipv6Profile::Pmipv6Profile ()
{
  NS_LOG_FUNCTION_NOARGS ();
}

Pmipv6Profile::~Pmipv6Profile ()
{
  NS_LOG_FUNCTION_NOARGS ();
  Flush ();
}

void Pmipv6Profile::DoDispose ()
{
  NS_LOG_FUNCTION_NOARGS ();
  Flush ();
  Object::DoDispose ();
}

Pmipv6Profile::Entry* Pmipv6Profile::Lookup (Identifier id)
{
  NS_LOG_FUNCTION (this << id);
  
  if (m_profileList.find (id) != m_profileList.end ())
    {
      Pmipv6Profile::Entry* entry = m_profileList[id];
      return entry;
    }
  return 0;
}

Pmipv6Profile::Entry* Pmipv6Profile::Add (Identifier id)
{
  NS_LOG_FUNCTION (this << id);
  NS_ASSERT (m_profileList.find (id) == m_profileList.end());
  
  Pmipv6Profile::Entry* entry = new Pmipv6Profile::Entry (this);
  m_profileList[id] = entry;
  return entry;
}

void Pmipv6Profile::Remove (Pmipv6Profile::Entry* entry)
{
  NS_LOG_FUNCTION_NOARGS ();

  for (ProfileListI i = m_profileList.begin () ; i != m_profileList.end () ; i++)
    {
      if ((*i).second == entry)
        {
          m_profileList.erase (i);
          delete entry;
          return;
        }
    }
}

void Pmipv6Profile::Flush ()
{
  NS_LOG_FUNCTION_NOARGS ();

  for (ProfileListI i = m_profileList.begin () ; i != m_profileList.end () ; i++)
    {
      delete (*i).second; /* delete the pointer Pmipv6Profile::Entry */
    }

  m_profileList.erase (m_profileList.begin (), m_profileList.end ());
}

Pmipv6Profile::Entry::Entry (Pmipv6Profile* pf)
  : m_profile (pf)
{
  NS_LOG_FUNCTION_NOARGS ();
}

Identifier Pmipv6Profile::Entry::GetMnIdentifier() const
{
  NS_LOG_FUNCTION_NOARGS ();
  
  return m_mnIdentifier;
}

void Pmipv6Profile::Entry::SetMnIdentifier(Identifier mnId)
{
  NS_LOG_FUNCTION ( this << mnId );
  
  m_mnIdentifier = mnId;
}

Identifier Pmipv6Profile::Entry::GetMnLinkIdentifier() const
{
  NS_LOG_FUNCTION_NOARGS ();
  
  return m_mnLinkIdentifier;
}

void Pmipv6Profile::Entry::SetMnLinkIdentifier(Identifier mnLinkId)
{
  NS_LOG_FUNCTION ( this << mnLinkId );
  
  m_mnLinkIdentifier = mnLinkId;
}

std::list<Ipv6Address> Pmipv6Profile::Entry::GetHomeNetworkPrefixes() const
{
  NS_LOG_FUNCTION_NOARGS ();
  
  return m_homeNetworkPrefixes;
}

void Pmipv6Profile::Entry::SetHomeNetworkPrefixes(std::list<Ipv6Address> hnps)
{
  NS_LOG_FUNCTION_NOARGS ();
  
  m_homeNetworkPrefixes = hnps;
}

void Pmipv6Profile::Entry::AddHomeNetworkPrefix(Ipv6Address hnp)
{
  NS_LOG_FUNCTION (this << hnp);
  
  m_homeNetworkPrefixes.push_back(hnp);
}

Ipv6Address Pmipv6Profile::Entry::GetLmaAddress() const
{
  NS_LOG_FUNCTION_NOARGS();
  
  return m_lmaAddress;
}

void Pmipv6Profile::Entry::SetLmaAddress(Ipv6Address lmaa)
{
  NS_LOG_FUNCTION ( this << lmaa );
  
  m_lmaAddress = lmaa;
}

} /* namespace ns3 */
