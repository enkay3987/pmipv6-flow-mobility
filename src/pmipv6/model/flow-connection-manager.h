/*
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
 * Author: Bhuvana Sagi
 */
#ifndef FLOW_CONNECTION_MANAGER_H
#define FLOW_CONNECTION_MANAGER_H

#include "ns3/object.h"
#include "ns3/nstime.h"
#include "ns3/network-module.h"
#include "ns3/core-module.h"
#include "ns3/packet.h"
#include "ns3/nstime.h"
#include "ns3/timer.h"
#include <list>
#include <map>

namespace ns3{
  
class FlowConnectionManager : public Object
{
public:
  struct snrTime
  {
    double snr;
    Time time;
  };

  struct bandwidth
  {
   uint64_t bytes;
   Time time;

  };
  static TypeId GetTypeId ();
  FlowConnectionManager();
//  FlowConnectionManager(uint8_t n);
  void DisplayWifiList();
  void DisplayLteList();
  void AddWifiSnr(double snr,Time t);
  void AddLteSnr(double snr,Time t);
  void CallAddLteSnr(uint16_t cellId, uint16_t rnti, double rsrp, double sinr);
  double GetWifiLatestSnr();
  snrTime GetWifiLatestSnrTime();
  double GetWifiAvgSnr();
  double GetLteLatestSnr();
  double GetLteAvgSnr();
  void SetnWifi(uint8_t n);
  void SetnLte(uint8_t n);
  void WifiBandwidthTrace(Ptr<Packet> packet, Mac48Address senderMac);
  double GetWifiBandwidth(Time time,Mac48Address apMac);
  void LteBandwidthTrace(Ptr<Packet> packet,uint16_t cellId);
  double GetLteBandwidth(Time time,uint16_t cellId);
  void SetTimer(Time maxTime);
  void RemoveEntries();
  void setWifiMacIpMap(Ipv6Address ip,Mac48Address mac);
  double GetWifiBndwdthByIp(Ipv6Address ip,Time time);

protected:
  virtual void DoDispose ();

private:
  uint8_t m_nWifi,m_nLte;
  Timer m_timer;
  Time m_maxTime;
  std::list<snrTime> wifiSnrList,lteSnrList;
  std::multimap<Mac48Address,bandwidth> wifiBndwdthMap;
  typedef std::multimap<Mac48Address,bandwidth>::iterator WifiBandwidthI;
  std::multimap<uint16_t,bandwidth> lteBndwdthMap;
  typedef std::multimap<uint16_t,bandwidth>::iterator LteBandwidthI;
  std::map<Ipv6Address,Mac48Address> macIpMap;
};


} // namespace ns3

#endif /* FLOW_CONNECTION_MANAGER_H */
