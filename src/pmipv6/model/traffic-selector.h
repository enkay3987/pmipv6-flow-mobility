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

#ifndef TRAFFIC_SELECTOR_H
#define TRAFFIC_SELECTOR_H

#include "ns3/ipv6-address.h"

namespace ns3
{
class TrafficSelector
{
public:
  TrafficSelector ();

  void SetProtocol (uint8_t protocol);
  uint8_t GetProtocol() const;

  void SetSourceIp (Ipv6Address sourceIp);
  Ipv6Address GetSourceIp () const;
  void SetSourcePrefix (Ipv6Prefix sourcePrefix);
  Ipv6Prefix GetSourcePrefix () const;

  void SetDestinationIp (Ipv6Address destinationIp);
  Ipv6Address GetDestinationIp () const;
  void SetDestinationPrefix (Ipv6Prefix destinationPrefix);
  Ipv6Prefix GetDestinationPrefix () const;

  void SetSourcePort (uint16_t ssp, uint16_t sep = 0);
  uint16_t GetSourceStartPort () const;
  uint16_t GetSourceEndPort () const;

  void SetDestinationPort (uint16_t dsp, uint16_t dep = 0);
  uint16_t GetDestinationStartPort () const;
  uint16_t GetDestinationEndPort () const;

  bool Matches (uint8_t protocol,
                Ipv6Address sourceAddress,
                Ipv6Address destinationAddress,
                uint16_t sourcePort,
                uint16_t destinationPort);

  bool operator== (const TrafficSelector &ts) const;

  bool operator< (const TrafficSelector &ts) const;

private:
  uint8_t m_protocol;     /**< type of protocol field */
  Ipv6Address m_sourceAddress; /**< Source IPv6 address */
  Ipv6Prefix m_sourcePrefix;    /**< Source IPv6 prefix */
  Ipv6Address m_destinationAddress;  /**< Destination IPv6 address*/
  Ipv6Prefix m_destinationPrefix;     /**< Destination IPv6 prefix*/
  uint16_t m_sourcePortStart;   /**< start of the source port number range*/
  uint16_t m_sourcePortEnd;     /**< end of the source port number range*/
  uint16_t m_destinationPortStart;  /**< start of the destination port number range*/
  uint16_t m_destinationPortEnd;    /**< end of the destination port number range*/
};

std::ostream& operator<< (std::ostream& os, const TrafficSelector &ts);

class TrafficSelectorHash : public std::unary_function<TrafficSelector, size_t>
{
public:
  /**
   * \brief Unary operator to hash TrafficSelector.
   * \param x TrafficSelector to hash
   */
  size_t operator () (TrafficSelector const &ts) const;
};
}
#endif /* TRAFFICSELECTOR_H */
