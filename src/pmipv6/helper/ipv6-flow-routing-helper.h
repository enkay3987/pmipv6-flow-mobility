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
 */

#ifndef IPV6_FLOW_ROUTING_HELPER_H
#define IPV6_FLOW_ROUTING_HELPER_H

#include "ns3/ipv6.h"
#include "ns3/ipv6-flow-routing.h"
#include "ns3/ptr.h"
#include "ns3/ipv6-address.h"
#include "ns3/node.h"
#include "ns3/net-device.h"

#include "ns3/node-container.h"
#include "ns3/net-device-container.h"
#include "ns3/ipv6-routing-helper.h"

namespace ns3 {

/**
 * \brief Helper class that adds ns3::Ipv6FlowRouting objects
 *
 * This class is expected to be used in conjunction with 
 * ns3::InternetStackHelper::SetRoutingHelper
 */
class Ipv6FlowRoutingHelper : public Ipv6RoutingHelper
{
public:
  /**
   * \brief Constructor.
   */
  Ipv6FlowRoutingHelper ();

  /**
   * \brief Construct an Ipv6ListRoutingHelper from another previously 
   * initialized instance (Copy Constructor).
   */
  Ipv6FlowRoutingHelper (const Ipv6FlowRoutingHelper &);

  /**
   * \internal
   * \returns pointer to clone of this Ipv6FlowRoutingHelper
   *
   * This method is mainly for internal use by the other helpers;
   * clients are expected to free the dynamic memory allocated by this method
   */
  Ipv6FlowRoutingHelper* Copy (void) const;

  /**
   * \param node the node on which the routing protocol will run
   * \returns a newly-created routing protocol
   *
   * This method will be called by ns3::InternetStackHelper::Install
   */
  virtual Ptr<Ipv6RoutingProtocol> Create (Ptr<Node> node) const;

  /**
   * \brief Get Ipv6FlowRouting pointer from IPv6 stack.
   * \param ipv6 Ipv6 pointer
   * \return Ipv6FlowRouting pointer or 0 if not found
   */
  Ptr<Ipv6FlowRouting> GetFlowRouting (Ptr<Ipv6> ipv6) const;

private:
  /**
   * \internal
   * \brief Assignment operator declared private and not implemented to disallow
   * assignment and prevent the compiler from happily inserting its own.
   * \param o object to copy from
   * \returns a reference to the new object
   */
  Ipv6FlowRoutingHelper &operator = (const Ipv6FlowRoutingHelper &o);
};

} // namespace ns3

#endif /* IPV6_FLOW_ROUTING_HELPER_H */

