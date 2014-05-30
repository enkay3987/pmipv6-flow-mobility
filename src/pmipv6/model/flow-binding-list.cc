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

#include "flow-binding-list.h"
#include "ns3/assert.h"
#include "ns3/log.h"
#include "ns3/udp-l4-protocol.h"
#include "ns3/tcp-l4-protocol.h"
#include "ns3/udp-header.h"
#include "ns3/tcp-header.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE ("FlowBindingList");

FlowBindingList::Entry::Entry (Ptr<FlowBindingList> flowBindingList)
{
  SetFlowId (0);
  m_lifetime = Time (0);
  m_flowBindingList = flowBindingList;
  m_lifetimeTimer = Timer (Timer::CANCEL_ON_DESTROY);
}

FlowBindingList::Entry::~Entry()
{
  m_bindingIds.clear ();
  m_flowBindingList = 0;
}

void FlowBindingList::Entry::SetFlowId (uint32_t flowId)
{
  m_flowId = flowId;
}

uint32_t FlowBindingList::Entry::GetFlowId () const
{
  return m_flowId;
}

void FlowBindingList::Entry::SetPriority (uint32_t priority)
{
  m_priority = priority;
}

uint32_t FlowBindingList::Entry::GetPriority () const
{
  return m_priority;
}

void FlowBindingList::Entry::SetBindingIds (std::list<Ptr<NetDevice> > bindingIds)
{
  m_bindingIds = bindingIds;
}

std::list<Ptr<NetDevice> > FlowBindingList::Entry::GetBindingIds () const
{
  return m_bindingIds;
}

void FlowBindingList::Entry::SetTrafficSelector (TrafficSelector ts)
{
  m_trafficSelector = ts;
}

TrafficSelector FlowBindingList::Entry::GetTrafficSelector () const
{
  return m_trafficSelector;
}

void FlowBindingList::Entry::SetLifetime (Time lifetime)
{
  if (!lifetime.IsZero ())
    {
      m_lifetime = lifetime;
      m_lifetimeTimer.SetFunction (&FlowBindingList::Entry::LifetimeTimeout, this);
      m_lifetimeTimer.SetDelay (m_lifetime);
      m_lifetimeTimer.Cancel ();
      m_lifetimeTimer.Schedule ();
    }
}

Time FlowBindingList::Entry::GetLifetime () const
{
  return m_lifetime;
}

bool FlowBindingList::Entry::Matches (uint8_t protocol,
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

void FlowBindingList::Entry::LifetimeTimeout ()
{
  NS_LOG_FUNCTION_NOARGS ();
  m_flowBindingList->RemoveFlowBindingEntry (GetFlowId ());
}

FlowBindingList::FlowBindingList ()
{
  flowIdCounter = 0;
}

FlowBindingList::~FlowBindingList()
{
  Flush ();
}

uint32_t FlowBindingList::AddFlowBindingEntry (Entry *entry)
{
  NS_LOG_FUNCTION (this << *entry);
  std::list<FlowBindingList::Entry *>::iterator it;
  entry->SetFlowId (++flowIdCounter);
  NS_ASSERT (entry->GetPriority () != 0);
  for (it = m_flowBindingList.begin (); (it != m_flowBindingList.end ()) && ((*it)->GetPriority () > entry->GetPriority ()); ++it)
    {
    }
  m_flowBindingList.insert (it, entry);
  m_trafficSelectorMap.insert (std::pair<TrafficSelector, Entry *> (entry->GetTrafficSelector (), entry));
  return entry->GetFlowId ();
}

void FlowBindingList::RemoveFlowBindingEntry (uint32_t flowID)
{
  NS_LOG_FUNCTION (this << flowID);
  std::list<FlowBindingList::Entry *>::iterator it;
  for (it = m_flowBindingList.begin (); it != m_flowBindingList.end (); ++it)
    {
      if ((*it)->GetFlowId () == flowID)
        {
          m_flowBindingList.erase (it);
          m_trafficSelectorMap.erase ((*it)->GetTrafficSelector ());
          delete (*it);
          return;
        }
    }
}

FlowBindingList::Entry* FlowBindingList::LookupFlowBindingEntry (uint32_t flowID)
{
  NS_LOG_FUNCTION (this << flowID);
  std::list<FlowBindingList::Entry *>::iterator it;
  for (it = m_flowBindingList.begin (); it != m_flowBindingList.end (); ++it)
    {
      if ((*it)->GetFlowId () == flowID)
        {
          return *it;
        }
    }
  return 0;
}

FlowBindingList::Entry* FlowBindingList::LookupFlowBindingEntry (TrafficSelector ts)
{
  NS_LOG_FUNCTION (this << ts);
  TrafficSelectorMap::iterator it = m_trafficSelectorMap.find (ts);
  if (it != m_trafficSelectorMap.end ())
    return it->second;
  return 0;
}

void FlowBindingList::Flush ()
{
  NS_LOG_FUNCTION_NOARGS ();
  std::list<FlowBindingList::Entry *>::iterator it;
  for (it = m_flowBindingList.begin (); it != m_flowBindingList.end (); ++it)
    {
      delete (*it);
    }
  m_flowBindingList.clear ();
  m_trafficSelectorMap.clear ();
}

void FlowBindingList::ResetPriority (uint32_t startPriority, uint32_t increment)
{
  NS_LOG_FUNCTION (this << startPriority << increment);
  std::list<FlowBindingList::Entry *>::iterator it;
  uint32_t priority = (m_flowBindingList.size () - 1) * increment + startPriority;
  for (it = m_flowBindingList.begin (); it != m_flowBindingList.end (); ++it, priority -= increment)
    {
      (*it)->SetPriority (priority);
    }
}

FlowBindingList::Entry* FlowBindingList::Classify (Ptr<const Packet> packet, const Ipv6Header &ipv6Header)
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

  std::list<FlowBindingList::Entry *>::iterator it;
  for (it = m_flowBindingList.begin (); it != m_flowBindingList.end (); ++it)
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

void FlowBindingList::Print (std::ostream &os)
{
  std::list<FlowBindingList::Entry *>::iterator it;
  for (it = m_flowBindingList.begin (); it != m_flowBindingList.end (); ++it)
    {
      os << *(*it) << std::endl;
    }
}

std::ostream& operator<< (std::ostream& os, const FlowBindingList::Entry &entry)
{
  os << "Flow Id: " << entry.GetFlowId ()
    << " Priority: " << entry.GetPriority ()
    << " Binding Ids: ";

  std::list<Ptr<NetDevice> > flowBindingList = entry.GetBindingIds ();
  for (std::list<Ptr<NetDevice> >::iterator it = flowBindingList.begin (); it != flowBindingList.end (); it++)
    {
      os << *it << " (" << (*it)->GetIfIndex () << ") ";
    }
  os << "Traffic Selector: " << entry.GetTrafficSelector ();
  return os;
}

} /* namespace ns3 */
