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

#ifndef FLOW_INTERFACE_LIST_H
#define FLOW_INTERFACE_LIST_H

#include "traffic-selector.h"

#include "ns3/ipv6-address.h"
#include "ns3/packet.h"
#include "ns3/timer.h"
#include "ns3/nstime.h"
#include "ns3/ipv6-header.h"
#include "ns3/sgi-hashmap.h"

#include <list>

namespace ns3
{

class FlowInterfaceList : public SimpleRefCount<FlowInterfaceList>
{
public:
  class Entry
  {
  public:
    Entry (Ptr<FlowInterfaceList> flowInterfaceList);
    ~Entry ();

    void SetFlowId (uint32_t flowId);
    uint32_t GetFlowId () const;

    void SetPriority (uint32_t priority);
    uint32_t GetPriority () const;

    void SetInterfaceIds (std::list<uint32_t> interfaceIds);
    std::list<uint32_t> GetInterfaceIds () const;

    void SetTrafficSelector (TrafficSelector ts);
    TrafficSelector GetTrafficSelector () const;

    void SetLifetime (Time lifetime);
    Time GetLifetime () const;

    bool Matches (uint8_t protocol,
                  Ipv6Address sourceAddress,
                  Ipv6Address destinationAddress,
                  uint16_t sourcePort,
                  uint16_t destinationPort);

  private:
    void LifetimeTimeout ();

    Ptr<FlowInterfaceList> m_flowInterfaceList;
    uint32_t m_flowId;
    uint32_t m_priority;
    std::list<uint32_t> m_interfaceIds;
    TrafficSelector m_trafficSelector;
    Time m_lifetime;
    Timer m_lifetimeTimer;
  };

  FlowInterfaceList ();
  ~FlowInterfaceList ();

  uint32_t AddFlowInterfaceEntry (Entry *entry);
  void RemoveFlowInterfaceEntry (uint32_t flowID);
  Entry* LookupFlowInterfaceEntry (uint32_t flowID);
  Entry* LookupFlowInterfaceEntry (TrafficSelector ts);
  void Flush ();
  void ResetPriority (uint32_t startPriority, uint32_t increment);

  Entry* Classify (Ptr<const Packet> packet, const Ipv6Header &ipv6Header);
  void Print (std::ostream &os);

private:
  typedef sgi::hash_map<TrafficSelector, Entry *, TrafficSelectorHash> TrafficSelectorMap;

  uint32_t flowIdCounter;
  std::list<Entry *> m_flowInterfaceList;
  TrafficSelectorMap m_trafficSelectorMap;
};

std::ostream& operator<< (std::ostream& os, const FlowInterfaceList::Entry &entry);
} /* namespace ns3 */
#endif /* FLOW_INTERFACE_LIST_H */
