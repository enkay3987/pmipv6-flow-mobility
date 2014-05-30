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

#include "traffic-selector.h"
#include "ns3/log.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE ("TrafficSelector");

TrafficSelector::TrafficSelector ()
{
  m_protocol = 0;
  m_sourceAddress = Ipv6Address ("::");
  m_sourcePrefix = Ipv6Prefix (uint8_t (0));
  m_destinationAddress = Ipv6Address ("::");
  m_destinationPrefix = Ipv6Prefix (uint8_t (0));
  m_sourcePortStart = 0;
  m_sourcePortEnd = 65535;
  m_destinationPortStart = 0;
  m_destinationPortEnd = 65535;
}

void TrafficSelector::SetSourceIp (Ipv6Address sourceIp)
{
  m_sourceAddress = sourceIp;
}

Ipv6Address TrafficSelector::GetSourceIp () const
{
  return m_sourceAddress;
}

void TrafficSelector::SetSourcePrefix (Ipv6Prefix sourcePrefix)
{
  m_sourcePrefix = sourcePrefix;
}

Ipv6Prefix TrafficSelector::GetSourcePrefix () const
{
  return m_sourcePrefix;
}

void TrafficSelector::SetDestinationPort (uint16_t dsp, uint16_t dep)
{
  m_destinationPortStart = dsp;
  (dep == 0) ? m_destinationPortEnd = dsp : m_destinationPortEnd = dep;
}

uint16_t TrafficSelector::GetDestinationStartPort () const
{
  return m_destinationPortStart;
}

uint16_t TrafficSelector::GetDestinationEndPort () const
{
  return m_destinationPortEnd;
}

void TrafficSelector::SetDestinationIp (Ipv6Address la)
{
  m_destinationAddress = la;
}

Ipv6Address TrafficSelector::GetDestinationIp () const
{
  return m_destinationAddress;
}

void TrafficSelector::SetDestinationPrefix (Ipv6Prefix destinationPrefix)
{
  m_destinationPrefix = destinationPrefix;
}

Ipv6Prefix TrafficSelector::GetDestinationPrefix () const
{
  return m_destinationPrefix;
}

void TrafficSelector::SetSourcePort (uint16_t ssp, uint16_t sep)
{
  m_sourcePortStart = ssp;
  (sep == 0) ? m_sourcePortEnd = ssp : m_sourcePortEnd = sep;
}

uint16_t TrafficSelector::GetSourceStartPort () const
{
  return m_sourcePortStart;
}

uint16_t TrafficSelector::GetSourceEndPort () const
{
  return m_sourcePortEnd;
}

void TrafficSelector::SetProtocol (uint8_t protocol)
{
  m_protocol = protocol;
}

uint8_t TrafficSelector::GetProtocol () const
{
  return m_protocol;
}

bool TrafficSelector::Matches (uint8_t protocol,
	                       Ipv6Address sourceAddress,
                               Ipv6Address destinationAddress,
                               uint16_t sourcePort,
                               uint16_t destinationPort)
{
  NS_LOG_FUNCTION (this << protocol << sourceAddress << destinationAddress << sourcePort << destinationPort);
  if (m_sourcePrefix.IsMatch (m_sourceAddress, sourceAddress))
    {
      NS_LOG_LOGIC ("Source Address matches.");
      if (m_destinationPrefix.IsMatch (m_destinationAddress, destinationAddress))
        {
          NS_LOG_LOGIC ("Destination Address matches.");
          if (sourcePort >= m_sourcePortStart)
            {
              NS_LOG_LOGIC ("Source Port start matches.");
              if (sourcePort <= m_sourcePortEnd)
                {
                  NS_LOG_LOGIC ("Source Port end matches.");
                  if (destinationPort >= m_destinationPortStart)
                    {
                      NS_LOG_LOGIC ("Destination Port start matches.");
                      if (destinationPort <= m_destinationPortEnd)
                        {
                          NS_LOG_LOGIC ("Destination Port end matches.");
                          if (m_protocol == protocol)
                            {
                              NS_LOG_LOGIC ("Protocol matches.");
                              NS_LOG_LOGIC ("Traffic selector matched.");
                              return true;
                            }
                        }
                    }
                }
            }
        }
    }
  NS_LOG_LOGIC ("Traffic selector not matched.");
  return false;
}

bool TrafficSelector::operator== (const TrafficSelector &ts) const
{
  return (this->m_protocol == ts.m_protocol &&
          this->m_sourceAddress == ts.m_sourceAddress &&
          this->m_sourcePrefix == ts.m_sourcePrefix &&
          this->m_sourcePortStart == ts.m_sourcePortStart &&
          this->m_sourcePortEnd == ts.m_sourcePortEnd &&
          this->m_destinationAddress == ts.m_destinationAddress &&
          this->m_destinationPrefix == ts.m_destinationPrefix &&
          this->m_destinationPortStart == ts.m_destinationPortStart &&
          this->m_destinationPortEnd == ts.m_destinationPortEnd);
}

bool TrafficSelector::operator< (const TrafficSelector &ts) const
{
  /*if (this->m_protocol == ts.m_protocol)
    {
      if (this->m_sourcePrefix.IsEqual (ts.m_sourcePrefix))
        {
          if (this->m_sourceAddress == ts.m_sourceAddress)
            {

            }
          else
            {
              return
            }

        }

      this->m_sourcePortStart == ts.m_sourcePortStart &&
      this->m_sourcePortEnd == ts.m_sourcePortEnd &&
      this->m_destinationAddress == ts.m_destinationAddress &&
      this->m_destinationPrefix == ts.m_destinationPrefix &&
      this->m_destinationPortStart == ts.m_destinationPortStart &&
      this->m_destinationPortEnd == ts.m_destinationPortEnd);
    }
  else
    {*/
      return (this->m_protocol < ts.m_protocol);
//    }
}

std::ostream& operator<< (std::ostream& os, const TrafficSelector &ts)
{
  os << int (ts.GetProtocol ()) << " "
    << ts.GetSourceIp () << ts.GetSourcePrefix ()
    << ":" << ts.GetSourceStartPort () << "-" << ts.GetSourceEndPort ()
    << " -> "
    << ts.GetDestinationIp () << ts.GetDestinationPrefix ()
    << ":" << ts.GetDestinationStartPort () << "-" << ts.GetDestinationEndPort ();
  return os;
}

#ifdef __cplusplus
extern "C"
{ /* } */
#endif

/**
 * \brief Get a hash key.
 * \param k the key
 * \param length the length of the key
 * \param level the previous hash, or an arbitrary value
 * \return hash
 * \note Adapted from Jens Jakobsen implementation (chillispot).
 */
static uint32_t lookuphash (unsigned char* k, uint32_t length, uint32_t level)
{
#define mix(a, b, c) \
  ({ \
   (a) -= (b); (a) -= (c); (a) ^= ((c) >> 13); \
   (b) -= (c); (b) -= (a); (b) ^= ((a) << 8);  \
   (c) -= (a); (c) -= (b); (c) ^= ((b) >> 13); \
   (a) -= (b); (a) -= (c); (a) ^= ((c) >> 12); \
   (b) -= (c); (b) -= (a); (b) ^= ((a) << 16); \
   (c) -= (a); (c) -= (b); (c) ^= ((b) >> 5);  \
   (a) -= (b); (a) -= (c); (a) ^= ((c) >> 3);  \
   (b) -= (c); (b) -= (a); (b) ^= ((a) << 10); \
   (c) -= (a); (c) -= (b); (c) ^= ((b) >> 15); \
   })

  typedef uint32_t  ub4;   /* unsigned 4-byte quantities */
  typedef unsigned  char ub1;   /* unsigned 1-byte quantities */
  uint32_t a = 0;
  uint32_t b = 0;
  uint32_t c = 0;
  uint32_t len = 0;

  /* Set up the internal state */
  len = length;
  a = b = 0x9e3779b9;  /* the golden ratio; an arbitrary value */
  c = level;           /* the previous hash value */

  /* handle most of the key */
  while (len >= 12)
    {
      a += (k[0] + ((ub4)k[1] << 8) + ((ub4)k[2] << 16) + ((ub4)k[3] << 24));
      b += (k[4] + ((ub4)k[5] << 8) + ((ub4)k[6] << 16) + ((ub4)k[7] << 24));
      c += (k[8] + ((ub4)k[9] << 8) + ((ub4)k[10] << 16) + ((ub4)k[11] << 24));
      mix (a, b, c);
      k += 12;
      len -= 12;
    }

  /* handle the last 11 bytes */
  c += length;
  switch (len) /* all the case statements fall through */
    {
    case 11: c += ((ub4)k[10] << 24);
    case 10: c += ((ub4)k[9] << 16);
    case 9 : c += ((ub4)k[8] << 8); /* the first byte of c is reserved for the length */
    case 8 : b += ((ub4)k[7] << 24);
    case 7 : b += ((ub4)k[6] << 16);
    case 6 : b += ((ub4)k[5] << 8);
    case 5 : b += k[4];
    case 4 : a += ((ub4)k[3] << 24);
    case 3 : a += ((ub4)k[2] << 16);
    case 2 : a += ((ub4)k[1] << 8);
    case 1 : a += k[0];
             /* case 0: nothing left to add */
    }
  mix (a, b, c);

#undef mix

  /* report the result */
  return c;
}

#ifdef __cplusplus
}
#endif

size_t TrafficSelectorHash::operator () (TrafficSelector const &ts) const
{
  std::ostringstream oss;
  oss << ts.GetProtocol ()
      << ts.GetSourceIp () << ts.GetSourcePrefix () << ts.GetSourceStartPort () << ts.GetSourceEndPort ()
      << ts.GetDestinationIp () << ts.GetDestinationPrefix () << ts.GetDestinationStartPort () << ts.GetDestinationEndPort ();
  std::string hashString = oss.str ();
  return lookuphash ((unsigned char *) hashString.c_str (), hashString.length (), 0);
}


}  // namespace ns3

