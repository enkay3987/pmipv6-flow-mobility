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

#include <vector>

#include "ns3/log.h"
#include "ns3/ptr.h"
#include "ns3/names.h"
#include "ns3/node.h"
#include "ns3/ipv6.h"
#include "ns3/ipv6-route.h"
#include "ns3/ipv6-list-routing.h"
#include "ns3/assert.h"
#include "ns3/ipv6-address.h"
#include "ns3/ipv6-routing-protocol.h"

#include "ipv6-user-flow-routing-helper.h"

NS_LOG_COMPONENT_DEFINE ("Ipv6UserFlowRoutingHelper");

namespace ns3
{

Ipv6UserFlowRoutingHelper::Ipv6UserFlowRoutingHelper ()
{
}

Ipv6UserFlowRoutingHelper::Ipv6UserFlowRoutingHelper (const Ipv6UserFlowRoutingHelper &o)
{
}

Ipv6UserFlowRoutingHelper*
Ipv6UserFlowRoutingHelper::Copy (void) const
{
  return new Ipv6UserFlowRoutingHelper (*this);
}

Ptr<Ipv6RoutingProtocol>
Ipv6UserFlowRoutingHelper::Create (Ptr<Node> node) const
{
  return CreateObject<Ipv6UserFlowRouting> ();
}

Ptr<Ipv6UserFlowRouting>
Ipv6UserFlowRoutingHelper::GetUserFlowRouting (Ptr<Ipv6> ipv6) const
{
  NS_LOG_FUNCTION (this);
  Ptr<Ipv6RoutingProtocol> ipv6rp = ipv6->GetRoutingProtocol ();
  NS_ASSERT_MSG (ipv6rp, "No routing protocol associated with Ipv6");
  if (DynamicCast<Ipv6UserFlowRouting> (ipv6rp))
    {
      NS_LOG_LOGIC ("Flow routing found as the main IPv6 routing protocol.");
      return DynamicCast<Ipv6UserFlowRouting> (ipv6rp);
    } 
  if (DynamicCast<Ipv6ListRouting> (ipv6rp))
    {
      Ptr<Ipv6ListRouting> lrp = DynamicCast<Ipv6ListRouting> (ipv6rp);
      int16_t priority;
      for (uint32_t i = 0; i < lrp->GetNRoutingProtocols ();  i++)
        {
          NS_LOG_LOGIC ("Searching for flow routing in list");
          Ptr<Ipv6RoutingProtocol> temp = lrp->GetRoutingProtocol (i, priority);
          if (DynamicCast<Ipv6UserFlowRouting> (temp))
            {
              NS_LOG_LOGIC ("Found flow routing in list");
              return DynamicCast<Ipv6UserFlowRouting> (temp);
            }
        }
    }
  NS_LOG_LOGIC ("Flow routing not found");
  return 0;
}

} // namespace ns3
