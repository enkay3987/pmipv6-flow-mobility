/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Flow Mobility Implementation.
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
 * Author: Naveen Kamath <cs11m02@iith.ac.in>
 */

#include "flow-interface-list.h"
#include "ns3/assert.h"
#include "ns3/log.h"
#include "ns3/udp-l4-protocol.h"
#include "ns3/tcp-l4-protocol.h"
#include "ns3/udp-header.h"
#include "ns3/tcp-header.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE ("FlowInterfaceList");

FlowInterfaceList::Entry::Entry (Ptr<FlowInterfaceList> flowInterfaceList)
{
  SetFlowId (0);
  m_lifetime = Time (0);
  m_flowInterfaceList = flowInterfaceList;
  m_lifetimeTimer = Timer (Timer::CANCEL_ON_DESTROY);
}

FlowInterfaceList::Entry::~Entry()
{
  m_interfaceIds.clear ();
  m_flowInterfaceList = 0;
}

void FlowInterfaceList::Entry::SetFlowId (uint32_t flowId)
{
  m_flowId = flowId;
}

uint32_t FlowInterfaceList::Entry::GetFlowId () const
{
  return m_flowId;
}

void FlowInterfaceList::Entry::SetPriority (uint32_t priority)
{
  m_priority = priority;
}

uint32_t FlowInterfaceList::Entry::GetPriority () const
{
  return m_priority;
}

void FlowInterfaceList::Entry::SetInterfaceIds (std::list<uint32_t> interfaceIds)
{
  m_interfaceIds = interfaceIds;
}

std::list<uint32_t> FlowInterfaceList::Entry::GetInterfaceIds () const
{
  return m_interfaceIds;
}

void FlowInterfaceList::Entry::SetTrafficSelector (TrafficSelector ts)
{
  m_trafficSelector = ts;
}

TrafficSelector FlowInterfaceList::Entry::GetTrafficSelector () const
{
  return m_trafficSelector;
}

void FlowInterfaceList::Entry::SetLifetime (Time lifetime)
{
  if (!lifetime.IsZero ())
    {
      m_lifetime = lifetime;
      m_lifetimeTimer.SetFunction (&FlowInterfaceList::Entry::LifetimeTimeout, this);
      m_lifetimeTimer.SetDelay (m_lifetime);
      m_lifetimeTimer.Cancel ();
      m_lifetimeTimer.Schedule ();
    }
}

Time FlowInterfaceList::Entry::GetLifetime () const
{
  return m_lifetime;
}

bool FlowInterfaceList::Entry::Matches (uint8_t protocol,
                               Ipv6Address sourceAddress,
                               Ipv6Address destinationAddress,
                               uint16_t sourcePort,
                               uint16_t destinationPort)
{
  NS_LOG_FUNCTION (this << protocol << sourceAddress << destinationAddress << sourcePort << destinationPort);
  if (GetTrafficSelector ().Matches (protocol, sourceAddress, destinationAddress, sourcePort, destinationPort))
    return true;
  return false;
}

void FlowInterfaceList::Entry::LifetimeTimeout ()
{
  NS_LOG_FUNCTION_NOARGS ();
  m_flowInterfaceList->RemoveFlowInterfaceEntry (GetFlowId ());
}

FlowInterfaceList::FlowInterfaceList ()
{
  flowIdCounter = 0;
}

FlowInterfaceList::~FlowInterfaceList()
{
  Flush ();
}

uint32_t FlowInterfaceList::AddFlowInterfaceEntry (Entry *entry)
{
  NS_LOG_FUNCTION (this << *entry);
  std::list<FlowInterfaceList::Entry *>::iterator it;
  entry->SetFlowId (++flowIdCounter);
  NS_ASSERT (entry->GetPriority () != 0);
  for (it = m_flowInterfaceList.begin (); (it != m_flowInterfaceList.end ()) && ((*it)->GetPriority () > entry->GetPriority ()); ++it)
    {
    }
  m_flowInterfaceList.insert (it, entry);
  m_trafficSelectorMap[entry->GetTrafficSelector ()] = entry;
  return entry->GetFlowId ();
}

void FlowInterfaceList::RemoveFlowInterfaceEntry (uint32_t flowID)
{
  NS_LOG_FUNCTION (this << flowID);
  std::list<FlowInterfaceList::Entry *>::iterator it;
  for (it = m_flowInterfaceList.begin (); it != m_flowInterfaceList.end (); ++it)
    {
      if ((*it)->GetFlowId () == flowID)
        {
          m_flowInterfaceList.erase (it);
          m_trafficSelectorMap.erase ((*it)->GetTrafficSelector ());
          delete (*it);
          return;
        }
    }
}

FlowInterfaceList::Entry* FlowInterfaceList::LookupFlowInterfaceEntry (uint32_t flowID)
{
  NS_LOG_FUNCTION (this << flowID);
  std::list<FlowInterfaceList::Entry *>::iterator it;
  for (it = m_flowInterfaceList.begin (); it != m_flowInterfaceList.end (); ++it)
    {
      if ((*it)->GetFlowId () == flowID)
        {
          return *it;
        }
    }
  return 0;
}

FlowInterfaceList::Entry* FlowInterfaceList::LookupFlowInterfaceEntry (TrafficSelector ts)
{
  NS_LOG_FUNCTION (this << ts);
  TrafficSelectorMap::iterator it = m_trafficSelectorMap.find (ts);
  if (it != m_trafficSelectorMap.end ())
    return it->second;
  return 0;
}

void FlowInterfaceList::Flush ()
{
  NS_LOG_FUNCTION_NOARGS ();
  std::list<FlowInterfaceList::Entry *>::iterator it;
  for (it = m_flowInterfaceList.begin (); it != m_flowInterfaceList.end (); ++it)
    {
      delete (*it);
    }
  m_flowInterfaceList.clear ();
  m_trafficSelectorMap.clear ();
}

void FlowInterfaceList::ResetPriority (uint32_t startPriority, uint32_t increment)
{
  NS_LOG_FUNCTION (this << startPriority << increment);
  std::list<FlowInterfaceList::Entry *>::iterator it;
  uint32_t priority = (m_flowInterfaceList.size () - 1) * increment + startPriority;
  for (it = m_flowInterfaceList.begin (); it != m_flowInterfaceList.end (); ++it, priority -= increment)
    {
      (*it)->SetPriority (priority);
    }
}

FlowInterfaceList::Entry* FlowInterfaceList::Classify (Ptr<const Packet> packet, const Ipv6Header &ipv6Header)
{
  NS_LOG_FUNCTION (this << packet << ipv6Header);

  uint8_t protocol;
  Ipv6Address sourceAddress;
  Ipv6Address destinationAddress;
  uint16_t sourcePort = 0;
  uint16_t destinationPort = 0;

  protocol = ipv6Header.GetNextHeader ();
  sourceAddress = ipv6Header.GetSourceAddress ();
  destinationAddress = ipv6Header.GetDestinationAddress ();

  if (protocol == UdpL4Protocol::PROT_NUMBER)
    {
      UdpHeader udpHeader;
      packet->PeekHeader (udpHeader);
      sourcePort = udpHeader.GetSourcePort ();
      destinationPort = udpHeader.GetDestinationPort ();
    }
  else if (protocol == TcpL4Protocol::PROT_NUMBER)
    {
      TcpHeader tcpHeader;
      packet->PeekHeader (tcpHeader);
      sourcePort = tcpHeader.GetSourcePort ();
      destinationPort = tcpHeader.GetDestinationPort ();
    }
  else
    {
      NS_LOG_INFO ("Unknown protocol: " << protocol);
      return 0;  // no match
    }

  NS_LOG_INFO ("Classifying packet:"
                << " protocol = " << int (protocol)
                << " sourceAddr = "  << sourceAddress
                << " destinationAddr = " << destinationAddress
                << " sourcePort = "  << sourcePort
                << " destinationPort = " << destinationPort);

  std::list<FlowInterfaceList::Entry *>::iterator it;
  for (it = m_flowInterfaceList.begin (); it != m_flowInterfaceList.end (); ++it)
    {
      if ((*it)->Matches (protocol, sourceAddress, destinationAddress, sourcePort, destinationPort))
        {
          NS_LOG_LOGIC ("Matches with Flow ID = " << (*it)->GetFlowId ());
          return (*it);
        }
    }
  NS_LOG_LOGIC ("No match found.");
  return 0;
}

void FlowInterfaceList::Print (std::ostream &os)
{
  std::list<FlowInterfaceList::Entry *>::iterator it;
  for (it = m_flowInterfaceList.begin (); it != m_flowInterfaceList.end (); ++it)
    {
      os << *(*it) << std::endl;
    }
}

std::ostream& operator<< (std::ostream& os, const FlowInterfaceList::Entry &entry)
{
  os << "Flow Id: " << entry.GetFlowId ()
    << " Priority: " << entry.GetPriority ()
    << " Interface Ids: ";

  std::list<uint32_t> flowInterfaceList = entry.GetInterfaceIds ();
  for (std::list<uint32_t>::iterator it = flowInterfaceList.begin (); it != flowInterfaceList.end (); it++)
    {
      os << *it << " ";
    }
  os << "Traffic Selector: " << entry.GetTrafficSelector ();
  return os;
}

} /* namespace ns3 */
