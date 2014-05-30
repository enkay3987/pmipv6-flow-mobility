/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Flow mobility implementation
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

#include <iomanip>
#include "ns3/log.h"
#include "ns3/node.h"
#include "ns3/packet.h"
#include "ns3/simulator.h"
#include "ns3/ipv6-route.h"
#include "ns3/net-device.h"
#include "ns3/names.h"
#include "ns3/ipv6-routing-table-entry.h"
#include "ns3/tcp-header.h"
#include "ns3/udp-header.h"
#include "ns3/tcp-l4-protocol.h"
#include "ns3/udp-l4-protocol.h"

#include "ipv6-flow-routing.h"
#include "tunnel-net-device.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("Ipv6FlowRouting")
  ;
NS_OBJECT_ENSURE_REGISTERED (Ipv6FlowRouting)
  ;

TypeId Ipv6FlowRouting::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::Ipv6FlowRouting")
    .SetParent<Ipv6RoutingProtocol> ()
    .AddConstructor<Ipv6FlowRouting> ()
  ;
  return tid;
}

Ipv6FlowRouting::Ipv6FlowRouting ()
  : m_ipv6 (0)
{
  NS_LOG_FUNCTION_NOARGS ();
  m_flowInterfaceList = Create<FlowInterfaceList> ();
}

Ipv6FlowRouting::~Ipv6FlowRouting ()
{
  NS_LOG_FUNCTION_NOARGS ();
}

// Formatted like output of "route -n" command
void
Ipv6FlowRouting::PrintRoutingTable (Ptr<OutputStreamWrapper> stream) const
{
  NS_LOG_FUNCTION (this << stream);
  std::ostream* os = stream->GetStream ();

  *os << "Node: " << m_ipv6->GetObject<Node> ()->GetId ()
      << " Time: " << Simulator::Now ().GetSeconds () << "s "
      << "Ipv6FlowRouting table" << std::endl;

  if (GetNRoutes () > 0)
    {
      *os << "Destination                    Next Hop                   Flag Ref Use If" << std::endl;
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
  if (m_flowInterfaceList != 0)
    m_flowInterfaceList->Print (*os);
}

void Ipv6FlowRouting::AddHostRouteTo (Ipv6Address dst, Ipv6Address nextHop, uint32_t interface, Ipv6Address prefixToUse, uint32_t metric)
{
  NS_LOG_FUNCTION (this << dst << nextHop << interface << prefixToUse << metric);
  if (nextHop.IsLinkLocal())
    {
      NS_LOG_WARN ("Ipv6FlowRouting::AddHostRouteTo - Next hop should be link-local");
    }

  AddNetworkRouteTo (dst, Ipv6Prefix::GetOnes (), nextHop, interface, prefixToUse, metric);
}

void Ipv6FlowRouting::AddHostRouteTo (Ipv6Address dst, uint32_t interface, uint32_t metric)
{
  NS_LOG_FUNCTION (this << dst << interface << metric);
  AddNetworkRouteTo (dst, Ipv6Prefix::GetOnes (), interface, metric);
}

void Ipv6FlowRouting::AddNetworkRouteTo (Ipv6Address network, Ipv6Prefix networkPrefix, Ipv6Address nextHop, uint32_t interface, uint32_t metric)
{
  NS_LOG_FUNCTION (this << network << networkPrefix << nextHop << interface << metric);
  Ipv6RoutingTableEntry* route = new Ipv6RoutingTableEntry ();
  *route = Ipv6RoutingTableEntry::CreateNetworkRouteTo (network, networkPrefix, nextHop, interface);
  m_networkRoutes.push_back (std::make_pair (route, metric));
}

void Ipv6FlowRouting::AddNetworkRouteTo (Ipv6Address network, Ipv6Prefix networkPrefix, Ipv6Address nextHop, uint32_t interface, Ipv6Address prefixToUse, uint32_t metric)
{
  NS_LOG_FUNCTION (this << network << networkPrefix << nextHop << interface << prefixToUse << metric);
  if (nextHop.IsLinkLocal())
    {
      NS_LOG_WARN ("Ipv6FlowRouting::AddNetworkRouteTo - Next hop should be link-local");
    }

  Ipv6RoutingTableEntry* route = new Ipv6RoutingTableEntry ();
  *route = Ipv6RoutingTableEntry::CreateNetworkRouteTo (network, networkPrefix, nextHop, interface, prefixToUse);
  m_networkRoutes.push_back (std::make_pair (route, metric));
}

void Ipv6FlowRouting::AddNetworkRouteTo (Ipv6Address network, Ipv6Prefix networkPrefix, uint32_t interface, uint32_t metric)
{
  NS_LOG_FUNCTION (this << network << networkPrefix << interface);
  Ipv6RoutingTableEntry* route = new Ipv6RoutingTableEntry ();
  *route = Ipv6RoutingTableEntry::CreateNetworkRouteTo (network, networkPrefix, interface);
  m_networkRoutes.push_back (std::make_pair (route, metric));
}

void Ipv6FlowRouting::DoDispose ()
{
  NS_LOG_FUNCTION_NOARGS ();

  for (NetworkRoutesI j = m_networkRoutes.begin ();  j != m_networkRoutes.end (); j = m_networkRoutes.erase (j))
    {
      delete j->first;
    }
  m_networkRoutes.clear ();

  if (m_flowInterfaceList != 0)
    {
      m_flowInterfaceList->Flush ();
      m_flowInterfaceList = 0;
    }

  m_ipv6 = 0;
  Ipv6RoutingProtocol::DoDispose ();
}

uint32_t Ipv6FlowRouting::GetNRoutes () const
{
  return m_networkRoutes.size ();
}

Ipv6RoutingTableEntry Ipv6FlowRouting::GetRoute (uint32_t index) const
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

void Ipv6FlowRouting::RemoveRoute (uint32_t index)
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

void Ipv6FlowRouting::RemoveRoute (Ipv6Address network, Ipv6Prefix prefix, uint32_t ifIndex, Ipv6Address prefixToUse)
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

Ptr<FlowInterfaceList> Ipv6FlowRouting::GetFlowInterfaceList ()
{
  return m_flowInterfaceList;
}

Ptr<Ipv6Route> Ipv6FlowRouting::RouteOutput (Ptr<Packet> p, const Ipv6Header &header, Ptr<NetDevice> oif, Socket::SocketErrno &sockerr)
{
  NS_LOG_FUNCTION (this << header << oif);
  return 0;
}

bool Ipv6FlowRouting::RouteInput (Ptr<const Packet> p, const Ipv6Header &header, Ptr<const NetDevice> idev,
                                    UnicastForwardCallback ucb, MulticastForwardCallback mcb,
                                    LocalDeliverCallback lcb, ErrorCallback ecb)
{
  NS_LOG_FUNCTION (this << p << header << header.GetSourceAddress () << header.GetDestinationAddress () << idev);
  NS_ASSERT (m_ipv6 != 0);
  // Check if input device supports IP
  NS_ASSERT (m_ipv6->GetInterfaceForDevice (idev) >= 0);
  uint32_t iif = m_ipv6->GetInterfaceForDevice (idev);
  Ipv6Address dst = header.GetDestinationAddress ();

  if (dst.IsMulticast ())
    {
      NS_LOG_LOGIC ("Multicast destination");
        {
          NS_LOG_LOGIC ("Multicast route not found");
          return false; // Let other routing protocols try to handle this
        }
    }
  // Check if input device supports IP forwarding
  if (m_ipv6->IsForwarding (iif) == false)
    {
      NS_LOG_LOGIC ("Forwarding disabled for this interface");
      ecb (p, header, Socket::ERROR_NOROUTETOHOST);
      return false;
    }
  // Next, try to find a route
  NS_LOG_LOGIC ("Unicast destination");
  Ptr<Ipv6Route> rtentry = LookupFlowRoute (p, header);

  if (rtentry != 0)
    {
      NS_LOG_LOGIC ("Found unicast destination- calling unicast callback");
      ucb (idev, rtentry, p, header);  // unicast forwarding callback
      return true;
    }
  else
    {
      NS_LOG_LOGIC ("Did not find unicast destination- returning false");
      return false; // Let other routing protocols try to handle this
    }
}

void Ipv6FlowRouting::NotifyInterfaceUp (uint32_t i)
{
  NS_LOG_FUNCTION (this << i);
}

void Ipv6FlowRouting::NotifyInterfaceDown (uint32_t i)
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

void Ipv6FlowRouting::NotifyAddAddress (uint32_t interface, Ipv6InterfaceAddress address)
{
  NS_LOG_FUNCTION (this << interface << address);
}

void Ipv6FlowRouting::NotifyRemoveAddress (uint32_t interface, Ipv6InterfaceAddress address)
{
  NS_LOG_FUNCTION (this << interface << address);
}

void Ipv6FlowRouting::NotifyAddRoute (Ipv6Address dst, Ipv6Prefix mask, Ipv6Address nextHop, uint32_t interface, Ipv6Address prefixToUse)
{
  NS_LOG_FUNCTION (this << dst << mask << nextHop << interface << prefixToUse);
}

void Ipv6FlowRouting::NotifyRemoveRoute (Ipv6Address dst, Ipv6Prefix mask, Ipv6Address nextHop, uint32_t interface, Ipv6Address prefixToUse)
{
  NS_LOG_FUNCTION (this << dst << mask << nextHop << interface);
}

void Ipv6FlowRouting::SetIpv6 (Ptr<Ipv6> ipv6)
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

Ptr<Ipv6Route> Ipv6FlowRouting::LookupFlowRoute (Ptr<const Packet> p, const Ipv6Header &header, Ptr<NetDevice> interface)
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
  FlowInterfaceList::Entry *filEntry = m_flowInterfaceList->Classify (p, header);
  if (filEntry)
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
                  rtentryTemp->SetDestination (route->GetDest ());
                  rtentryTemp->SetGateway (route->GetGateway ());
                  rtentryTemp->SetOutputDevice (m_ipv6->GetNetDevice (interfaceIdx));
                  rtentryList.push_back (rtentryTemp);
                }
            }
        }
      std::list<uint32_t> interfaceIds = filEntry->GetInterfaceIds ();
      for (std::list<uint32_t>::iterator interfaceIdsIt = interfaceIds.begin (); interfaceIdsIt != interfaceIds.end (); interfaceIdsIt++)
        {
          for (std::list<Ptr<Ipv6Route> >::iterator routeIt = rtentryList.begin (); routeIt != rtentryList.end (); routeIt++)
            {
              Ptr<TunnelNetDevice> tunDev = DynamicCast<TunnelNetDevice> ((*routeIt)->GetOutputDevice ());
              if (tunDev != 0)
                {
                  if (tunDev->GetAccessTechonologyType () == *interfaceIdsIt)
                    {
                      // Set rtentry to selected route.
                      NS_LOG_LOGIC ("Selected output device " << tunDev->GetIfIndex ());
                      NS_LOG_LOGIC ("Matching route via " << (*routeIt)->GetDestination () << " (throught " << (*routeIt)->GetGateway () << ") at the end");
                      return *routeIt;
                    }
                }
            }
        }
    }
  return 0;
}

Ipv6Address Ipv6FlowRouting::SourceAddressSelection (uint32_t interface, Ipv6Address dest)
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

