/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * This program is free software; you can redistribute it and/or modify*/
#include "flow-connection-manager.h"
#include "ns3/core-module.h"

NS_LOG_COMPONENT_DEFINE ("FlowConnectionManager");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (FlowConnectionManager)
  ;

TypeId FlowConnectionManager::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::FlowConnectionManager")
      .SetParent<Object> ()
      .AddConstructor<FlowConnectionManager> ()
      ;
    return tid;
}

FlowConnectionManager::FlowConnectionManager()
{
  NS_LOG_FUNCTION (this);
  m_nWifi = 10;
  m_nLte=10;
}

void FlowConnectionManager::WifiBandwidthTrace(Ptr<Packet> packet, Mac48Address senderMac)
{
  bandwidth bw;
  bw.bytes=packet->GetSize();
  bw.time=Simulator::Now();
  wifiBndwdthMap.insert ( std::pair<Mac48Address,bandwidth>(senderMac,bw) );
  WifiBandwidthI iter;
}

double FlowConnectionManager::GetWifiBandwidth(Time time,Mac48Address apMac)
{
  uint64_t bytes=0;
  Time now=Simulator::Now();
  Time minTime=now-time;
  WifiBandwidthI iter;
    for(iter=wifiBndwdthMap.begin();iter!=wifiBndwdthMap.end();++iter)
        {
              if((const Mac48Address)iter->first == (const Mac48Address) apMac)
                {
                  if((iter->second.time) >= minTime)
                  {
                    bytes+=iter->second.bytes;
                  }
                else
                  {
                    wifiBndwdthMap.erase(iter);
                  }
                 }
        }
    double bytesDouble=(double) bytes;
    double bandwidth=bytesDouble*8;
    bandwidth=(double)(bandwidth/(time.GetSeconds()));
    return bandwidth;
}

void FlowConnectionManager::SetTimer(Time maxTime)
{
  m_maxTime=maxTime;
  m_timer = Timer (Timer::CANCEL_ON_DESTROY);
  m_timer.SetFunction (&FlowConnectionManager::RemoveEntries, this);
  m_timer.SetDelay (maxTime);
  m_timer.Schedule ();

}

void FlowConnectionManager::RemoveEntries()
{
    Time time=Simulator::Now();
    Time minTime=time-m_maxTime;
    WifiBandwidthI iter;
    if(!wifiBndwdthMap.empty())
    {
        for(iter=wifiBndwdthMap.begin();iter!=wifiBndwdthMap.end();++iter)
          {
              if((iter->second.time) < minTime)
                {
                  wifiBndwdthMap.erase(iter);
                }
          }
    }
    LteBandwidthI iter1;
    if(!lteBndwdthMap.empty())
    {
        for(iter1=lteBndwdthMap.begin();iter1!=lteBndwdthMap.end();++iter1)
          {
            if((iter1->second.time) < minTime)
              {
                lteBndwdthMap.erase(iter1);
              }
          }
    }
    m_timer.Schedule();
}

void FlowConnectionManager::setWifiMacIpMap(Ipv6Address ip,Mac48Address mac)
{
  macIpMap.insert ( std::pair<Ipv6Address,Mac48Address>(ip,mac) );
}

double FlowConnectionManager::GetWifiBndwdthByIp(Ipv6Address ip,Time time)
{
  std::map<Ipv6Address,Mac48Address>::iterator iter;
  iter=macIpMap.find((const Ipv6Address) ip);
  Mac48Address mac=iter->second;
  double bandwidth=GetWifiBandwidth(time,mac);
  return bandwidth;
}

void FlowConnectionManager::LteBandwidthTrace(Ptr<Packet> packet,uint16_t cellId)
{
  bandwidth bw;
  bw.bytes=packet->GetSize();
  bw.time=Simulator::Now();
  lteBndwdthMap.insert ( std::pair<uint16_t,bandwidth>(cellId,bw) );
}

double FlowConnectionManager::GetLteBandwidth(Time time,uint16_t cellId)
{
  uint64_t bytes=0;
  Time now=Simulator::Now();
  Time minTime=now-time;
  LteBandwidthI iter;
    for(iter=lteBndwdthMap.begin();iter!=lteBndwdthMap.end();++iter)
        {
              if(iter->first == cellId)
                {
                  if((iter->second.time) >= minTime)
                  {
                    bytes+=iter->second.bytes;
                  }
                else
                  {
                    lteBndwdthMap.erase(iter);
                  }
                 }
        }
    double bytesDouble=(double) bytes;
    double bandwidth=bytesDouble*8;
    bandwidth=(double)(bandwidth/(time.GetSeconds()));
    return bandwidth;

}

//FlowConnectionManager::FlowConnectionManager(uint8_t n=10)
//{
//  this->n = n;
//}

void FlowConnectionManager::DisplayWifiList()

{
  NS_LOG_FUNCTION (this);
  //std::cout << "\nsnr list contains:\n";
  for (std::list<snrTime>::iterator it=wifiSnrList.begin(); it!=wifiSnrList.end(); ++it)
    std::cout << "\t" << it->snr<<' '<<it->time;
}

void FlowConnectionManager::DisplayLteList()

{
  NS_LOG_FUNCTION (this);
  //std::cout << "\nsnr list contains:\n";
  for (std::list<snrTime>::iterator it=lteSnrList.begin(); it!=lteSnrList.end(); ++it)
    std::cout << "\t" << it->snr<<' '<<it->time;
}


void FlowConnectionManager:: AddLteSnr(double snr,Time t)
{
 // NS_LOG_FUNCTION (this << snr << t);
  if(lteSnrList.size()==m_nLte)
      lteSnrList.pop_back();
  struct snrTime st;
  st.snr=snr;
  st.time=t;
  lteSnrList.push_front (st);
}

void FlowConnectionManager:: AddWifiSnr(double snr,Time t)
{
  NS_LOG_FUNCTION (this << snr << t);
  if(wifiSnrList.size()==m_nWifi)
      wifiSnrList.pop_back();
  struct snrTime st;
  st.snr=snr;
  st.time=t;
  wifiSnrList.push_front (st);
//  std::cout<<"\n***new snr added to wifi****:"<<"list contains:";
//  for (std::list<snrTime>::iterator it=wifiSnrList.begin(); it!=wifiSnrList.end(); ++it)
//        std::cout << "\t" << it->snr<<' '<<it->time;

}

void FlowConnectionManager :: CallAddLteSnr(uint16_t cellId, uint16_t rnti, double rsrp, double sinr)
{
  //NS_LOG_FUNCTION (this<<cellId<<rnti<<rsrp<<sinr);
  AddLteSnr(sinr,Simulator::Now());
}

double FlowConnectionManager:: GetWifiLatestSnr()
{
  NS_LOG_FUNCTION (this);
  std::list<snrTime>::const_iterator iterator;
  iterator = wifiSnrList.begin();
  //std::cout << "\nsnr list contains:\n";
//     for (std::list<snrTime>::iterator it=wifiSnrList.begin(); it!=wifiSnrList.end(); ++it)
//       std::cout << "\t" << it->snr<<' '<<it->time;
  return (iterator->snr);
}

FlowConnectionManager::snrTime FlowConnectionManager:: GetWifiLatestSnrTime()
{
  std::list<snrTime>::const_iterator iterator;
  iterator = wifiSnrList.begin();
  return (*iterator);
}

double FlowConnectionManager:: GetWifiAvgSnr()
{
  NS_LOG_FUNCTION (this);
  double sum=0;
  std::list<snrTime>::const_iterator iterator;
  for (iterator = wifiSnrList.begin(); iterator != wifiSnrList.end(); ++iterator) {
  sum=sum+((iterator)->snr);
  }
  double size=wifiSnrList.size();
  sum=sum/size;
  return sum;
}

double FlowConnectionManager:: GetLteLatestSnr()
{
  NS_LOG_FUNCTION (this);
  std::list<snrTime>::const_iterator iterator;
  iterator = lteSnrList.begin();
  return (iterator->snr);
}

double FlowConnectionManager:: GetLteAvgSnr()
{
  NS_LOG_FUNCTION (this);
  double sum=0;
  std::list<snrTime>::const_iterator iterator;
  for (iterator = lteSnrList.begin(); iterator != lteSnrList.end(); ++iterator) {
  sum=sum+((iterator)->snr);
  }
  double size=lteSnrList.size();
  sum=sum/size;
  return sum;
}


void FlowConnectionManager::SetnWifi(uint8_t n)
{
  NS_LOG_FUNCTION (this << n);
  m_nWifi=n;
}


void FlowConnectionManager::SetnLte(uint8_t n)
{
  NS_LOG_FUNCTION (this << n);
  m_nLte=n;
}

void FlowConnectionManager::DoDispose ()
{
  Object::DoDispose ();
}
} // namespace ns3
