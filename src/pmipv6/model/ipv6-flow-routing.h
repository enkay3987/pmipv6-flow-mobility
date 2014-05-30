/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2007-2009 Strasbourg University
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

#ifndef IPV6_FLOW_ROUTING_H
#define IPV6_FLOW_ROUTING_H

#include <stdint.h>

#include <list>

#include "ns3/ptr.h"
#include "ns3/ipv6-address.h"
#include "ns3/ipv6.h"
#include "ns3/ipv6-header.h"
#include "ns3/ipv6-routing-protocol.h"

#include "flow-interface-list.h"

namespace ns3 {

class Packet;
class NetDevice;
class Ipv6Interface;
class Ipv6Route;
class Node;
class Ipv6RoutingTableEntry;

/**
 * \ingroup pmipv6
 * \defgroup Ipv6FlowRouting Ipv6FlowRouting
 */
/**
 * \ingroup Ipv6FlowRouting
 * \class Ipv6FlowRouting
 *
 * \brief Flow based routing protocol for IP version 6 stacks.
 *
 * This class provides a basic set of methods for inserting static
 * unicast routes into the Ipv6 routing system. This particular
 * protocol is designed to be inserted into an Ipv6ListRouting
 * protocol but can be used also as a standalone protocol.
 *
 * The Ipv6FlowRouting class inherits from the abstract base class
 * Ipv6RoutingProtocol that defines the interface methods that a routing
 * protocol must support.
 *
 * \see Ipv6RoutingProtocol
 * \see Ipv6ListRouting
 * \see Ipv6ListRouting::AddRoutingProtocol
 */
class Ipv6FlowRouting : public Ipv6RoutingProtocol
{
public:
  /**
   * \brief The interface Id associated with this class.
   * \return type identifier
   */
  static TypeId GetTypeId ();

  Ipv6FlowRouting ();
  virtual ~Ipv6FlowRouting ();

  /**
   * \brief Add route to host.
   * \param dest destination address
   * \param nextHop next hop address to route the packet.
   * \param interface interface index
   * \param prefixToUse prefix that should be used for source address for this destination
   * \param metric metric of route in case of multiple routes to same destination
   */
  void AddHostRouteTo (Ipv6Address dest, Ipv6Address nextHop, uint32_t interface, Ipv6Address prefixToUse = Ipv6Address ("::"), uint32_t metric = 0);

  /**
   * \brief Add route to host.
   * \param dest destination address.
   * \param interface interface index
   * \param metric metric of route in case of multiple routes to same destination
   */
  void AddHostRouteTo (Ipv6Address dest, uint32_t interface, uint32_t metric = 0);

  /**
   * \brief Add route to network.
   * \param network network address
   * \param networkPrefix network prefix*
   * \param nextHop next hop address to route the packet
   * \param interface interface index
   * \param metric metric of route in case of multiple routes to same destination
   */
  void AddNetworkRouteTo (Ipv6Address network, Ipv6Prefix networkPrefix, Ipv6Address nextHop, uint32_t interface, uint32_t metric = 0);

  /**
   * \brief Add route to network.
   * \param network network address
   * \param networkPrefix network prefix*
   * \param nextHop next hop address to route the packet.
   * \param interface interface index
   * \param prefixToUse prefix that should be used for source address for this destination
   * \param metric metric of route in case of multiple routes to same destination
   */
  void AddNetworkRouteTo (Ipv6Address network, Ipv6Prefix networkPrefix, Ipv6Address nextHop, uint32_t interface, Ipv6Address prefixToUse, uint32_t metric = 0);

  /**
   * \brief Add route to network.
   * \param network network address
   * \param networkPrefix network prefix
   * \param interface interface index
   * \param metric metric of route in case of multiple routes to same destination
   */
  void AddNetworkRouteTo (Ipv6Address network, Ipv6Prefix networkPrefix, uint32_t interface, uint32_t metric = 0);

  /**
   * \brief Get the number or entries in the routing table.
   * \return number of entries
   */
  uint32_t GetNRoutes () const;

  /**
   * \brief Get a specified route.
   * \param i index
   * \return the route whose index is i
   */
  Ipv6RoutingTableEntry GetRoute (uint32_t i) const;

  /**
   * \brief Remove a route from the routing table.
   * \param i index
   */
  void RemoveRoute (uint32_t i);

  /**
   * \brief Remove a route from the routing table.
   * \param network IPv6 network
   * \param prefix IPv6 prefix
   * \param ifIndex interface index
   * \param prefixToUse IPv6 prefix to use with this route (multihoming)
   */
  void RemoveRoute (Ipv6Address network, Ipv6Prefix prefix, uint32_t ifIndex, Ipv6Address prefixToUse);

  /**
   * \brief Returns the Flow Interface List.
   * \return Pointer to Flow Interface List.
   */
  Ptr<FlowInterfaceList> GetFlowInterfaceList ();

  // Inherited from Ipv6RoutingProtocol

  virtual Ptr<Ipv6Route> RouteOutput (Ptr<Packet> p, const Ipv6Header &header, Ptr<NetDevice> oif, Socket::SocketErrno &sockerr);

  virtual bool RouteInput  (Ptr<const Packet> p, const Ipv6Header &header, Ptr<const NetDevice> idev,
                            UnicastForwardCallback ucb, MulticastForwardCallback mcb,
                            LocalDeliverCallback lcb, ErrorCallback ecb);

  virtual void NotifyInterfaceUp (uint32_t interface);
  virtual void NotifyInterfaceDown (uint32_t interface);
  virtual void NotifyAddAddress (uint32_t interface, Ipv6InterfaceAddress address);
  virtual void NotifyRemoveAddress (uint32_t interface, Ipv6InterfaceAddress address);
  virtual void NotifyAddRoute (Ipv6Address dst, Ipv6Prefix mask, Ipv6Address nextHop, uint32_t interface, Ipv6Address prefixToUse = Ipv6Address::GetZero ());
  virtual void NotifyRemoveRoute (Ipv6Address dst, Ipv6Prefix mask, Ipv6Address nextHop, uint32_t interface, Ipv6Address prefixToUse = Ipv6Address::GetZero ());
  virtual void SetIpv6 (Ptr<Ipv6> ipv6);



  /**
   * \brief Print the Routing Table entries
   *
   * \param stream the ostream the Routing table is printed to
   */
  virtual void PrintRoutingTable (Ptr<OutputStreamWrapper> stream) const;

protected:
  /**
   * \brief Dispose this object.
   */
  virtual void DoDispose ();

private:
  /// Container for the network routes
  typedef std::list<std::pair <Ipv6RoutingTableEntry *, uint32_t> > NetworkRoutes;

  /// Const Iterator for container for the network routes
  typedef std::list<std::pair <Ipv6RoutingTableEntry *, uint32_t> >::const_iterator NetworkRoutesCI;

  /// Iterator for container for the network routes
  typedef std::list<std::pair <Ipv6RoutingTableEntry *, uint32_t> >::iterator NetworkRoutesI;

  /**
   * \brief Lookup the route which the packet matches to and check the flow binding list for the higher
   * priority interface to route the packet.
   * \param p Packet layer 4 onwards.
   * \param header The Ipv6 header of the packet.
   * \param interface output interface if any (put 0 otherwise)
   * \return Ipv6Route to route the packet
   */
  Ptr<Ipv6Route> LookupFlowRoute (Ptr<const Packet> p, const Ipv6Header &header, Ptr<NetDevice> = 0);

  /**
   * \brief Choose the source address to use with destination address.
   * \param interface interface index
   * \param dest IPv6 destination address
   * \return IPv6 source address to use
   */
  Ipv6Address SourceAddressSelection (uint32_t interface, Ipv6Address dest);

  /**
   * \brief the forwarding table for network.
   */
  NetworkRoutes m_networkRoutes;

  /**
   * \brief Ipv6 reference.
   */
  Ptr<Ipv6> m_ipv6;

  /**
   * \brief Specifies the interface priority for flows based on traffic selectors.
   */
  Ptr<FlowInterfaceList> m_flowInterfaceList;
};

} /* namespace ns3 */

#endif /* IPV6_FLOW_ROUTING_H */

