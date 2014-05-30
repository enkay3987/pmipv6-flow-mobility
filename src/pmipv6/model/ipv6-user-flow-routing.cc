/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
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
 * Author: Sebastien Vincent <vincent@clarinet.u-strasbg.fr>
 */

#include <iomanip>
#include "ns3/log.h"
#include "ns3/node.h"
#include "ns3/config.h"
#include "ns3/packet.h"
#include "ns3/simulator.h"
#include "ns3/ipv6-route.h"
#include "ns3/net-device.h"
#include "ns3/names.h"
#include "ns3/udp-l4-protocol.h"
#include "ns3/tcp-l4-protocol.h"
#include "ns3/tcp-header.h"
#include "ns3/udp-header.h"
#include "ns3/ipv6-routing-table-entry.h"

#include "ipv6-user-flow-routing.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("Ipv6UserFlowRouting")
  ;
NS_OBJECT_ENSURE_REGISTERED (Ipv6UserFlowRouting)
  ;

TypeId Ipv6UserFlowRouting::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::Ipv6UserFlowRouting")
    .SetParent<Ipv6RoutingProtocol> ()
    .AddConstructor<Ipv6UserFlowRouting> ()
  ;
  return tid;
}

Ipv6UserFlowRouting::Ipv6UserFlowRouting ()
  : m_ipv6 (0)
{
  NS_LOG_FUNCTION_NOARGS ();
  m_flowBindingList = Create<FlowBindingList> ();
}

Ipv6UserFlowRouting::~Ipv6UserFlowRouting ()
{
  NS_LOG_FUNCTION_NOARGS ();
}

void Ipv6UserFlowRouting::SetIpv6 (Ptr<Ipv6> ipv6)
{
  NS_LOG_FUNCTION (this << ipv6);
  NS_ASSERT (m_ipv6 == 0 && ipv6 != 0);
  uint32_t i = 0;
  m_ipv6 = ipv6;

  for (i = 0; i < m_ipv6->GetNInterfaces (); i++)
    {
      if (m_ipv6->IsUp (i))
        {
          NotifyInterfaceUp (i);
        }
      else
        {
          NotifyInterfaceDown (i);
        }
    }
}

// Formatted like output of "route -n" command
void
Ipv6UserFlowRouting::PrintRoutingTable (Ptr<OutputStreamWrapper> stream) const
{
  NS_LOG_FUNCTION (this << stream);
  std::ostream* os = stream->GetStream ();

  *os << "Node: " << m_ipv6->GetObject<Node> ()->GetId ()
      << " Time: " << Simulator::Now ().GetSeconds () << "s "
      << "Ipv6UserFlowRouting table" << std::endl;

  if (GetNRoutes () > 0)
    {
      *os << "Destination                    Next Hop                   Flag Met Ref Use If" << std::endl;
      for (uint32_t j = 0; j < GetNRoutes (); j++)
        {
          std::ostringstream dest, gw, mask, flags;
          Ipv6RoutingTableEntry route = GetRoute (j);
          dest << route.GetDest () << "/" << int(route.GetDestNetworkPrefix ().GetPrefixLength ());
          *os << std::setiosflags (std::ios::left) << std::setw (31) << dest.str ();
          gw << route.GetGateway ();
          *os << std::setiosflags (std::ios::left) << std::setw (27) << gw.str ();
          flags << "U";
          if (route.IsHost ())
            {
              flags << "H";
            }
          else if (route.IsGateway ())
            {
              flags << "G";
            }
          *os << std::setiosflags (std::ios::left) << std::setw (5) << flags.str ();
          *os << std::setiosflags (std::ios::left) << std::setw (4) << GetMetric (j);
          // Ref ct not implemented
          *os << "-" << "   ";
          // Use not implemented
          *os << "-" << "   ";
          if (Names::FindName (m_ipv6->GetNetDevice (route.GetInterface ())) != "")
            {
              *os << Names::FindName (m_ipv6->GetNetDevice (route.GetInterface ()));
            }
          else
            {
              *os << route.GetInterface ();
            }
          *os << std::endl;
        }
    }
  m_flowBindingList->Print (std::cout);
}

void Ipv6UserFlowRouting::AddHostRouteTo (Ipv6Address dst, Ipv6Address nextHop, uint32_t interface, Ipv6Address prefixToUse, uint32_t metric)
{
  NS_LOG_FUNCTION (this << dst << nextHop << interface << prefixToUse << metric);
  if (nextHop.IsLinkLocal())
    {
      NS_LOG_WARN ("Ipv6UserFlowRouting::AddHostRouteTo - Next hop should be link-local");
    }

  AddNetworkRouteTo (dst, Ipv6Prefix::GetOnes (), nextHop, interface, prefixToUse, metric);
}

void Ipv6UserFlowRouting::AddHostRouteTo (Ipv6Address dst, uint32_t interface, uint32_t metric)
{
  NS_LOG_FUNCTION (this << dst << interface << metric);
  AddNetworkRouteTo (dst, Ipv6Prefix::GetOnes (), interface, metric);
}

void Ipv6UserFlowRouting::AddNetworkRouteTo (Ipv6Address network, Ipv6Prefix networkPrefix, Ipv6Address nextHop, uint32_t interface, uint32_t metric)
{
  NS_LOG_FUNCTION (this << network << networkPrefix << nextHop << interface << metric);
  Ipv6RoutingTableEntry* route = new Ipv6RoutingTableEntry ();
  *route = Ipv6RoutingTableEntry::CreateNetworkRouteTo (network, networkPrefix, nextHop, interface);
  m_networkRoutes.push_back (std::make_pair (route, metric));
}

void Ipv6UserFlowRouting::AddNetworkRouteTo (Ipv6Address network, Ipv6Prefix networkPrefix, Ipv6Address nextHop, uint32_t interface, Ipv6Address prefixToUse, uint32_t metric)
{
  NS_LOG_FUNCTION (this << network << networkPrefix << nextHop << interface << prefixToUse << metric);
  if (nextHop.IsLinkLocal())
    {
      NS_LOG_WARN ("Ipv6UserFlowRouting::AddNetworkRouteTo - Next hop should be link-local");
    }

  Ipv6RoutingTableEntry* route = new Ipv6RoutingTableEntry ();
  *route = Ipv6RoutingTableEntry::CreateNetworkRouteTo (network, networkPrefix, nextHop, interface, prefixToUse);
  m_networkRoutes.push_back (std::make_pair (route, metric));
}

void Ipv6UserFlowRouting::AddNetworkRouteTo (Ipv6Address network, Ipv6Prefix networkPrefix, uint32_t interface, uint32_t metric)
{
  NS_LOG_FUNCTION (this << network << networkPrefix << interface);
  Ipv6RoutingTableEntry* route = new Ipv6RoutingTableEntry ();
  *route = Ipv6RoutingTableEntry::CreateNetworkRouteTo (network, networkPrefix, interface);
  m_networkRoutes.push_back (std::make_pair (route, metric));
}

void Ipv6UserFlowRouting::SetDefaultRoute (Ipv6Address nextHop, uint32_t interface, Ipv6Address prefixToUse, uint32_t metric)
{
  NS_LOG_FUNCTION (this << nextHop << interface << prefixToUse);
  AddNetworkRouteTo (Ipv6Address ("::"), Ipv6Prefix::GetZero (), nextHop, interface, prefixToUse, metric);
}

bool Ipv6UserFlowRouting::HasNetworkDest (Ipv6Address network, uint32_t interfaceIndex)
{
  NS_LOG_FUNCTION (this << network << interfaceIndex);

  /* in the network table */
  for (NetworkRoutesI j = m_networkRoutes.begin (); j != m_networkRoutes.end (); j++)
    {
      Ipv6RoutingTableEntry* rtentry = j->first;
      Ipv6Prefix prefix = rtentry->GetDestNetworkPrefix ();
      Ipv6Address entry = rtentry->GetDestNetwork ();

      if (prefix.IsMatch (network, entry) && rtentry->GetInterface () == interfaceIndex)
        {
          return true;
        }
    }

  /* beuh!!! not route at all */
  return false;
}

Ptr<Ipv6Route> Ipv6UserFlowRouting::LookupFlowRoute (Ptr<const Packet> p, const Ipv6Header &header, Ptr<NetDevice> interface)
{
  NS_LOG_FUNCTION (this << p << header << interface);
  Ptr<Ipv6Route> rtentry = 0;
  std::list<Ptr<Ipv6Route> > rtentryList;
  Ipv6Address dst = header.GetDestinationAddress ();

  uint8_t protocol = header.GetNextHeader ();
  uint16_t sourcePort, destinationPort;

  // Get the TCP/UDP port number information.
  if (protocol == UdpL4Protocol::PROT_NUMBER)
    {
      UdpHeader udpHeader;
      p->PeekHeader (udpHeader);
      sourcePort = udpHeader.GetSourcePort ();
      destinationPort = udpHeader.GetDestinationPort ();
    }
  else if (protocol == TcpL4Protocol::PROT_NUMBER)
    {
      TcpHeader tcpHeader;
      p->PeekHeader (tcpHeader);
      sourcePort = tcpHeader.GetSourcePort ();
      destinationPort = tcpHeader.GetDestinationPort ();
    }
  else
    {
      return 0;
    }

  NS_LOG_INFO (header << " " << int (protocol) << " " << sourcePort << " " << destinationPort);
  // Find the highest priority interface through which the packet can be sent among the matched routes
  // by checking the flow binding list (if classification can be done).
  FlowBindingList::Entry *fblEntry = m_flowBindingList->Classify (p, header);
  if (fblEntry)
    {
      // Loop over the routes and add the routes which match into the rtentryList.
      for (NetworkRoutesI it = m_networkRoutes.begin (); it != m_networkRoutes.end (); it++)
        {
          Ipv6RoutingTableEntry* j = it->first;
          Ipv6Prefix mask = j->GetDestNetworkPrefix ();
          Ipv6Address entry = j->GetDestNetwork ();

          NS_LOG_LOGIC ("Searching for route to " << dst);
          if (mask.IsMatch (dst, entry))
            {
              NS_LOG_LOGIC ("Found global network route " << j);
              /* if interface is given, check the route will output on this interface */
              if (!interface || interface == m_ipv6->GetNetDevice (j->GetInterface ()))
                {
                  Ipv6RoutingTableEntry* route = j;
                  uint32_t interfaceIdx = route->GetInterface ();
                  Ptr<Ipv6Route> rtentryTemp = Create<Ipv6Route> ();
                  if (route->GetGateway ().IsAny ())
                    {
                      rtentryTemp->SetSource (SourceAddressSelection (interfaceIdx, route->GetDest ()));
                    }
                  else if (route->GetDest ().IsAny ()) /* default route */
                    {
                      rtentryTemp->SetSource (SourceAddressSelection (interfaceIdx, route->GetPrefixToUse ().IsAny () ? dst : route->GetPrefixToUse ()));
                    }
                  else
                    {
                      rtentryTemp->SetSource (SourceAddressSelection (interfaceIdx, route->GetGateway ()));
                    }
                  rtentryTemp->SetDestination (route->GetDest ());
                  rtentryTemp->SetGateway (route->GetGateway ());
                  rtentryTemp->SetOutputDevice (m_ipv6->GetNetDevice (interfaceIdx));
                  rtentryList.push_back (rtentryTemp);
                }
            }
        }
      std::list<Ptr<NetDevice> > bindingIds = fblEntry->GetBindingIds ();
      for (std::list<Ptr<NetDevice> >::iterator bindingIdsIt = bindingIds.begin (); bindingIdsIt != bindingIds.end (); bindingIdsIt++)
        {
          for (std::list<Ptr<Ipv6Route> >::iterator routeIt = rtentryList.begin (); routeIt != rtentryList.end (); routeIt++)
            {
              if ((*routeIt)->GetOutputDevice ()->GetIfIndex () == (*bindingIdsIt)->GetIfIndex ())
                {
                  NS_LOG_LOGIC ("Selected output device " << ((*routeIt)->GetOutputDevice ())->GetIfIndex ());
                  NS_LOG_LOGIC ("Matching route via " << (*routeIt)->GetDestination () << " (through " << (*routeIt)->GetGateway () << ") at the end");
                  return *routeIt;
                }
            }
        }
    }
  return 0;
}

void Ipv6UserFlowRouting::DoDispose ()
{
  NS_LOG_FUNCTION_NOARGS ();

  for (NetworkRoutesI j = m_networkRoutes.begin ();  j != m_networkRoutes.end (); j = m_networkRoutes.erase (j))
    {
      delete j->first;
    }
  m_networkRoutes.clear ();
  m_ipv6 = 0;
  m_flowBindingList = 0;
  Ipv6RoutingProtocol::DoDispose ();
}

uint32_t Ipv6UserFlowRouting::GetNRoutes () const
{
  return m_networkRoutes.size ();
}

Ipv6RoutingTableEntry Ipv6UserFlowRouting::GetDefaultRoute ()
{
  NS_LOG_FUNCTION_NOARGS ();
  Ipv6Address dst ("::");
  uint32_t shortestMetric = 0xffffffff;
  Ipv6RoutingTableEntry* result = 0;

  for (NetworkRoutesI it = m_networkRoutes.begin (); it != m_networkRoutes.end (); it++)
    {
      Ipv6RoutingTableEntry* j = it->first;
      uint32_t metric = it->second;
      Ipv6Prefix mask = j->GetDestNetworkPrefix ();
      uint16_t maskLen = mask.GetPrefixLength ();
      Ipv6Address entry = j->GetDestNetwork ();

      if (maskLen)
        {
          continue;
        }

      if (metric > shortestMetric)
        {
          continue;
        }
      shortestMetric = metric;
      result = j;
    }

  if (result)
    {
      return result;
    }
  else
    {
      return Ipv6RoutingTableEntry ();
    }
}

Ipv6RoutingTableEntry Ipv6UserFlowRouting::GetRoute (uint32_t index) const
{
  NS_LOG_FUNCTION (this << index);
  uint32_t tmp = 0;

  for (NetworkRoutesCI it = m_networkRoutes.begin (); it != m_networkRoutes.end (); it++)
    {
      if (tmp == index)
        {
          return it->first;
        }
      tmp++;
    }
  NS_ASSERT (false);
  // quiet compiler.
  return 0;
}

uint32_t Ipv6UserFlowRouting::GetMetric (uint32_t index) const
{
  NS_LOG_FUNCTION_NOARGS ();
  uint32_t tmp = 0;

  for (NetworkRoutesCI it = m_networkRoutes.begin (); it != m_networkRoutes.end (); it++)
    {
      if (tmp == index)
        {
          return it->second;
        }
      tmp++;
    }
  NS_ASSERT (false);
  // quiet compiler.
  return 0;
}

void Ipv6UserFlowRouting::RemoveRoute (uint32_t index)
{
  NS_LOG_FUNCTION (this << index);
  uint32_t tmp = 0;

  for (NetworkRoutesI it = m_networkRoutes.begin (); it != m_networkRoutes.end (); it++)
    {
      if (tmp == index)
        {
          delete it->first;
          m_networkRoutes.erase (it);
          return;
        }
      tmp++;
    }
  NS_ASSERT (false);
}

void Ipv6UserFlowRouting::RemoveRoute (Ipv6Address network, Ipv6Prefix prefix, uint32_t ifIndex, Ipv6Address prefixToUse)
{
  NS_LOG_FUNCTION (this << network << prefix << ifIndex);

  for (NetworkRoutesI it = m_networkRoutes.begin (); it != m_networkRoutes.end (); it++)
    {
      Ipv6RoutingTableEntry* rtentry = it->first;
      if (network == rtentry->GetDest () && rtentry->GetInterface () == ifIndex
          && rtentry->GetPrefixToUse () == prefixToUse)
        {
          delete it->first;
          m_networkRoutes.erase (it);
          return;
        }
    }
}

Ptr<FlowBindingList> Ipv6UserFlowRouting::GetFlowBindingList ()
{
  return m_flowBindingList;
}

void Ipv6UserFlowRouting::SetupRxTrace (Ptr<Node> node)
{
  NS_LOG_FUNCTION (this << node);
  std::ostringstream oss;
  oss << "/NodeList/" << node->GetId () << "/$ns3::Ipv6L3Protocol/Rx";
  Config::ConnectWithoutContext (oss.str (), MakeCallback (&Ipv6UserFlowRouting::HandlePacketRx, this));
}

void Ipv6UserFlowRouting::HandlePacketRx (Ptr<const Packet> packet, Ptr<Ipv6> ipv6, uint32_t interfaceId)
{
  NS_LOG_FUNCTION (this << packet << ipv6 << interfaceId);
  Ptr<Packet> p = packet->Copy ();
  Ipv6Header ipv6Header;
  p->RemoveHeader (ipv6Header);
  Ipv6Address src, dst;
  src = ipv6Header.GetSourceAddress ();
  dst = ipv6Header.GetDestinationAddress ();
  // Consider only global IP addresses.
  if (dst.IsLinkLocal () || dst.IsMulticast ())
    return;
  uint8_t protocol = ipv6Header.GetNextHeader ();
  uint16_t srcPort, dstPort;
  // Get the TCP/UDP port number information.
  if (protocol == UdpL4Protocol::PROT_NUMBER)
    {
      UdpHeader udpHeader;
      p->PeekHeader (udpHeader);
      srcPort = udpHeader.GetSourcePort ();
      dstPort = udpHeader.GetDestinationPort ();
    }
  else if (protocol == TcpL4Protocol::PROT_NUMBER)
    {
      TcpHeader tcpHeader;
      p->PeekHeader (tcpHeader);
      srcPort = tcpHeader.GetSourcePort ();
      dstPort = tcpHeader.GetDestinationPort ();
    }
  else
    {
      return;
    }
  NS_LOG_LOGIC (int (protocol) << " " << src << ":" << srcPort << "->" << dst << ":" << dstPort);

  TrafficSelector ts;
  ts.SetProtocol (protocol);
  ts.SetSourceIp (dst);
  ts.SetSourcePrefix (Ipv6Prefix (128));
  ts.SetSourcePort (dstPort);
  ts.SetDestinationIp (src);
  ts.SetDestinationPrefix (Ipv6Prefix (128));
  ts.SetDestinationPort (srcPort);

  FlowBindingList::Entry *entry = m_flowBindingList->LookupFlowBindingEntry (ts);
  std::list<Ptr<NetDevice> > bindingIds;
  if (!entry)
    {
      NS_LOG_LOGIC ("Creating new flow binding list entry.");
      FlowBindingList::Entry *entry = new FlowBindingList::Entry (m_flowBindingList);
      entry->SetLifetime (Seconds (5));
      entry->SetTrafficSelector (ts);
      entry->SetPriority (10000);
      for (uint32_t i = 1; i < m_ipv6->GetNInterfaces (); i++)
        {
          // Create binding id list such that the interface on which the packet
          // received is on a higher priority.
          if (interfaceId != i)
            {
              bindingIds.push_back (m_ipv6->GetNetDevice (i));
            }
          else
            {
              bindingIds.push_front (m_ipv6->GetNetDevice (i));
            }
        }
      entry->SetBindingIds (bindingIds);
      m_flowBindingList->AddFlowBindingEntry (entry);
      m_flowBindingList->Print (std::cout);
    }
  else
    {
      // Reset the timer.
      entry->SetLifetime (Seconds (5));
      Ptr<NetDevice> rxDev = m_ipv6->GetNetDevice (interfaceId);
      bindingIds = entry->GetBindingIds ();
      Ptr<NetDevice> dev = (*(bindingIds.begin ()));
      if (!(rxDev == dev))
        {
          NS_LOG_LOGIC ("Setting new bindingsIds order.");
          bindingIds.remove (rxDev);
          bindingIds.push_front (rxDev);
          entry->SetBindingIds (bindingIds);
          m_flowBindingList->Print (std::cout);
        }
    }
}

Ptr<Ipv6Route> Ipv6UserFlowRouting::RouteOutput (Ptr<Packet> p, const Ipv6Header &header, Ptr<NetDevice> oif, Socket::SocketErrno &sockerr)
{
  NS_LOG_FUNCTION (this << header << oif);
  Ipv6Address destination = header.GetDestinationAddress ();
  Ptr<Ipv6Route> rtentry = 0;

  if (destination.IsMulticast ())
    {
      // This routing protocol doesn't handle multicast ipv6 addresses.
      NS_LOG_LOGIC ("Multicast destination not handled.");
    }

//  rtentry = LookupStatic (destination, oif);
  rtentry = LookupFlowRoute (p, header, oif);
  if (rtentry)
    {
      sockerr = Socket::ERROR_NOTERROR;
    }
  else
    {
      sockerr = Socket::ERROR_NOROUTETOHOST;
    }
  return rtentry;
}

bool Ipv6UserFlowRouting::RouteInput (Ptr<const Packet> p, const Ipv6Header &header, Ptr<const NetDevice> idev,
                                    UnicastForwardCallback ucb, MulticastForwardCallback mcb,
                                    LocalDeliverCallback lcb, ErrorCallback ecb)
{
  NS_LOG_FUNCTION (this << p << header << header.GetSourceAddress () << header.GetDestinationAddress () << idev);
  return false;
}

void Ipv6UserFlowRouting::NotifyInterfaceUp (uint32_t i)
{
  for (uint32_t j = 0; j < m_ipv6->GetNAddresses (i); j++)
    {
      if (m_ipv6->GetAddress (i, j).GetAddress () != Ipv6Address ()
          && m_ipv6->GetAddress (i, j).GetPrefix () != Ipv6Prefix ())
        {
          if (m_ipv6->GetAddress (i, j).GetPrefix () == Ipv6Prefix (128))
            {
              /* host route */
              AddHostRouteTo (m_ipv6->GetAddress (i, j).GetAddress (), i);
            }
          else
            {
              AddNetworkRouteTo (m_ipv6->GetAddress (i, j).GetAddress ().CombinePrefix (m_ipv6->GetAddress (i, j).GetPrefix ()),
                                 m_ipv6->GetAddress (i, j).GetPrefix (), i);
            }
        }
    }
}

void Ipv6UserFlowRouting::NotifyInterfaceDown (uint32_t i)
{
  NS_LOG_FUNCTION (this << i);
  uint32_t j = 0;
  uint32_t max = GetNRoutes ();

  /* remove all static routes that are going through this interface */
  while (j < max)
    {
      Ipv6RoutingTableEntry route = GetRoute (j);

      if (route.GetInterface () == i)
        {
          RemoveRoute (j);
        }
      else
        {
          j++;
        }
    }
}

void Ipv6UserFlowRouting::NotifyAddAddress (uint32_t interface, Ipv6InterfaceAddress address)
{
  if (!m_ipv6->IsUp (interface))
    {
      return;
    }

  Ipv6Address networkAddress = address.GetAddress ().CombinePrefix (address.GetPrefix ());
  Ipv6Prefix networkMask = address.GetPrefix ();

  if (address.GetAddress () != Ipv6Address () && address.GetPrefix () != Ipv6Prefix ())
    {
      AddNetworkRouteTo (networkAddress, networkMask, interface);
    }
}

void Ipv6UserFlowRouting::NotifyRemoveAddress (uint32_t interface, Ipv6InterfaceAddress address)
{
  if (!m_ipv6->IsUp (interface))
    {
      return;
    }

  Ipv6Address networkAddress = address.GetAddress ().CombinePrefix (address.GetPrefix ());
  Ipv6Prefix networkMask = address.GetPrefix ();

  // Remove all static routes that are going through this interface
  // which reference this network
  for (uint32_t j = 0; j < GetNRoutes (); j++)
    {
      Ipv6RoutingTableEntry route = GetRoute (j);

      if (route.GetInterface () == interface
          && route.IsNetwork ()
          && route.GetDestNetwork () == networkAddress
          && route.GetDestNetworkPrefix () == networkMask)
        {
          RemoveRoute (j);
        }
    }
}

void Ipv6UserFlowRouting::NotifyAddRoute (Ipv6Address dst, Ipv6Prefix mask, Ipv6Address nextHop, uint32_t interface, Ipv6Address prefixToUse)
{
  NS_LOG_INFO (this << dst << mask << nextHop << interface << prefixToUse);
  if (dst != Ipv6Address::GetZero ())
    {
      AddNetworkRouteTo (dst, mask, nextHop, interface);
    }
  else /* default route */
    {
      /* this case is mainly used by configuring default route following RA processing,
       * in case of multiple prefix in RA, the first will configured default route
       */

      /* for the moment, all default route has the same metric
       * so according to the longest prefix algorithm,
       * the default route chosen will be the last added
       */
      SetDefaultRoute (nextHop, interface, prefixToUse);
    }
}

void Ipv6UserFlowRouting::NotifyRemoveRoute (Ipv6Address dst, Ipv6Prefix mask, Ipv6Address nextHop, uint32_t interface, Ipv6Address prefixToUse)
{
  NS_LOG_FUNCTION (this << dst << mask << nextHop << interface);
  if (dst != Ipv6Address::GetZero ())
    {
      for (NetworkRoutesI j = m_networkRoutes.begin (); j != m_networkRoutes.end ();)
        {
          Ipv6RoutingTableEntry* rtentry = j->first;
          Ipv6Prefix prefix = rtentry->GetDestNetworkPrefix ();
          Ipv6Address entry = rtentry->GetDestNetwork ();

          if (dst == entry && prefix == mask && rtentry->GetInterface () == interface)
            {
              delete j->first;
              j = m_networkRoutes.erase (j);
            }
          else
            {
              ++j;
            }
        }
    }
  else
    {
      /* default route case */
      RemoveRoute (dst, mask, interface, prefixToUse);
    }
}

Ipv6Address Ipv6UserFlowRouting::SourceAddressSelection (uint32_t interface, Ipv6Address dest)
{
  NS_LOG_FUNCTION (this << interface << dest);
  Ipv6Address ret;

  /* first address of an IPv6 interface is link-local ones */
  ret = m_ipv6->GetAddress (interface, 0).GetAddress ();

  if (dest == Ipv6Address::GetAllNodesMulticast () || dest == Ipv6Address::GetAllRoutersMulticast () || dest == Ipv6Address::GetAllHostsMulticast ())
    {
      return ret;
    }

  /* usually IPv6 interfaces have one link-local address and one global address */

  for (uint32_t i = 1; i < m_ipv6->GetNAddresses (interface); i++)
    {
      Ipv6InterfaceAddress test = m_ipv6->GetAddress (interface, i);
      Ipv6InterfaceAddress dst(dest);

      if (test.GetScope() == dst.GetScope())
        {
          return test.GetAddress ();
        }
    }

  return ret;
}

} /* namespace ns3 */

