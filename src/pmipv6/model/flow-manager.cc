/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * This program is free software; you can redistribute it and/or modify*/
#include "flow-manager.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/pmipv6-profile.h"
#include "ns3/flow-connection-manager.h"
#include "ns3/flow-connection-manager-helper.h"
#include<sstream>
#include<iostream>

NS_LOG_COMPONENT_DEFINE ("FlowManager");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (FlowManager)
    ;

#ifdef __cplusplus
extern "C"
{ /* } */
#endif

static uint32_t lookuphash (unsigned char* k, uint32_t length, uint32_t level)
{
  NS_LOG_FUNCTION (k << length << level);
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
    case 9: c += ((ub4)k[8] << 8);  /* the first byte of c is reserved for the length */
    case 8: b += ((ub4)k[7] << 24);
    case 7: b += ((ub4)k[6] << 16);
    case 6: b += ((ub4)k[5] << 8);
    case 5: b += k[4];
    case 4: a += ((ub4)k[3] << 24);
    case 3: a += ((ub4)k[2] << 16);
    case 2: a += ((ub4)k[1] << 8);
    case 1: a += k[0];
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

TypeId FlowManager::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::FlowManager")
  .SetParent<Object> ()
  ;
  return tid;
}

void FlowManager::SetMaxDelay(Time delay)
{
  m_maxDelay=delay;
}

void FlowManager::HandlePacket(Ptr<Packet> packet,Ipv6Address magIpv6,uint8_t link)
{
  MagFlow magFlow;
  Ipv6Header ipv6Header;
  Ptr<Packet> packetCopy = packet->Copy ();
  packetCopy->RemoveHeader(ipv6Header);
  magFlow.m_flow.m_srcIp=ipv6Header.GetSourceAddress();
  magFlow.m_flow.m_destIp=ipv6Header.GetDestinationAddress();
  magFlow.m_flow.m_protocol=ipv6Header.GetNextHeader();
  Ipv6Address srcIp=magFlow.m_flow.m_srcIp;
  Ipv6Address destIp=magFlow.m_flow.m_destIp;
  uint8_t protocol=magFlow.m_flow.m_protocol;
  uint16_t srcPort,destPort;
  if(magFlow.m_flow.m_protocol==6)
    {
      TcpHeader tcpHeader;
      packetCopy->RemoveHeader(tcpHeader);
      magFlow.m_flow.m_srcPort=tcpHeader.GetSourcePort();
      magFlow.m_flow.m_destPort=tcpHeader.GetDestinationPort();
      srcPort=magFlow.m_flow.m_srcPort;
      destPort= magFlow.m_flow.m_destPort;
    }
  else if(magFlow.m_flow.m_protocol==17)
    {
      UdpHeader udpHeader;
      packetCopy->RemoveHeader(udpHeader);
      magFlow.m_flow.m_srcPort=udpHeader.GetSourcePort();
      magFlow.m_flow.m_destPort=udpHeader.GetDestinationPort();
      srcPort=magFlow.m_flow.m_srcPort;
      destPort= magFlow.m_flow.m_destPort;
    }
  else
    {
      return;
    }
  magFlow.m_magIpv6=magIpv6;
 FlowStats* stats = LookupFlow (magFlow);
  if(stats==NULL)
    {
      stats = new FlowStats ();
      stats->packets++;
      stats->bytes+=packet->GetSize();
      if (stats->packets == 1)
        {
          stats->timeFirstPacket = Simulator::Now();
        }
      stats->timeLastPacket =Simulator::Now();
      stats->magFlow.m_magIpv6=magIpv6;
      stats->magFlow.m_flow.m_destIp=destIp;
      stats->magFlow.m_flow.m_srcIp=srcIp;
      stats->magFlow.m_flow.m_destPort=destPort;
      stats->magFlow.m_flow.m_srcPort=srcPort;
      stats->magFlow.m_flow.m_protocol=protocol;
      stats->link=link;
      AttCacheI iter;
      iter = m_att.find (magIpv6);
      stats->att=iter->second;
      stats->node=GetNode(srcIp,destIp);
      FillSnr(stats);
      SetSnrTimer(stats);
      SetTimer(stats);
      AddFlow(stats);
    }
  else
    {
      stats->packets++;
      stats->bytes+=packet->GetSize();
      if (stats->packets == 1)
      {
        stats->timeFirstPacket = Simulator::Now();
      }
      stats->timeLastPacket =Simulator::Now();
      stats->magFlow.m_magIpv6=magIpv6;
      stats->link=link;
      AttCacheI iter;
      iter = m_att.find (magIpv6);
      stats->att=iter->second;
      SetTimer(stats);
    }
  std::cout<<"\n****flow stats*****";
  std::cout<<"\nmag ipv6:"<<magIpv6;
  std::cout<<"\nsrouce ip:"<<magFlow.m_flow.m_srcIp;
  std::cout<<"\ndestination ip:"<<magFlow.m_flow.m_destIp;
  std::cout<<"\nprotocol:"<<int (magFlow.m_flow.m_protocol);
  std::cout<<"\nSrouce port:"<< magFlow.m_flow.m_srcPort;
  std::cout<<"\ndestination port:"<<magFlow.m_flow.m_destPort;
  std::cout<<"\ntime first packet:"<<stats->timeFirstPacket;
  std::cout<<"\ntime last packet:"<< stats->timeLastPacket;
  std::cout<<"\ntype of link:"<< int (stats->link);
  std::cout<<"\naccess technology type:"<< int (stats->att);
  std::cout<<"\nno of packets in flow:"<<stats->packets;
  std::cout<<"\nno of bytes in the flow:"<<stats->bytes;
  std::cout<<"\nsnr value is:"<<stats->snr;
}

void FlowManager::SetPmipv6Profile(Ptr<Pmipv6Profile> profile)
{
  m_pmipv6Profile=profile;
}

void FlowManager::SetNodesHash(Identifier id,Ptr<Node> node)
{
  m_nodesHash[id]=node;
}

Ptr<Node> FlowManager::LookupNodesHash(Identifier id)
{
//  NodesHashCacheI iter;
//  for(iter=m_nodesHash.begin();iter!=m_nodesHash.end();++iter)
//    {
//      if((const Identifier) iter->first == (const Identifier) id)
//        {
//          Ptr<Node> node=iter->second;
//          return node;
//        }
//    }
  if(m_nodesHash.find((const Identifier) id) != m_nodesHash.end())
    {
      Ptr<Node> node=m_nodesHash[id];
      return node;
    }
    return NULL;
}

Ptr<Node> FlowManager::GetNode(Ipv6Address srcAddr,Ipv6Address destAddr)
{
  Ipv6Address srcPrefix=srcAddr.CombinePrefix(Ipv6Prefix (64));
  Ipv6Address destPrefix=destAddr.CombinePrefix(Ipv6Prefix(64));
  Identifier mnSrcIdentifier=m_pmipv6Profile->LookupMnIdForIdentifier(srcPrefix);
  if(mnSrcIdentifier.GetLength() != 0)
    {
      Ptr<Node> node=LookupNodesHash(mnSrcIdentifier);
      if(node!=NULL)
        return node;
    }
  else
    {
      Identifier mnDestIdentifier=m_pmipv6Profile->LookupMnIdForIdentifier(destPrefix);
      if(mnDestIdentifier.GetLength() != 0)
        {
          Ptr<Node> node=LookupNodesHash(mnDestIdentifier);
          if(node != NULL)
            return node;
        }
    }
  return NULL;
}

void FlowManager::FillSnr(FlowStats* flowStats)
{
  FlowConnectionManagerHelper fcH;
  Ptr<FlowConnectionManager> flowConnectionManager=fcH.GetFlowConnectionManager(flowStats->node);
  if(flowStats->att == 4)
    {
      flowStats->snr=flowConnectionManager->GetWifiLatestSnr();
      std::cout<<"\n************getting latest wifi snr**********";
    }
  else
    flowStats->snr=flowConnectionManager->GetLteLatestSnr();
  SetSnrTimer(flowStats);
}

void FlowManager::SetTimeForSnr(Time snrTime)
{
  m_timeForSnr=snrTime;
}

void FlowManager::SetSnrTimer(FlowStats* stats)
{
  NS_ASSERT (!m_timeForSnr.IsZero ());

  stats->snrTimer = Timer (Timer::CANCEL_ON_DESTROY);
  stats->snrTimer.SetFunction (&FlowManager::FillSnr, this);
  stats->snrTimer.SetArguments(stats);
  stats->snrTimer.SetDelay (m_timeForSnr);
  stats->snrTimer.Schedule ();

}

void FlowManager::AddFlow(FlowStats* flowStats)
{
  std::cout<<"\nadding new flow the hash map already contains:";
  FlowStatsCacheI iter;
    for(iter=m_flowStats.begin();iter!=m_flowStats.end();++iter)
      std::cout<<"\nmag flow:\nmag ipv6:"<<iter->first.m_magIpv6<<",src ip:"<<iter->first.m_flow.m_srcIp<<",dest ip:"<<iter->first.m_flow.m_destIp<<",src port:"<<iter->first.m_flow.m_srcPort<<",dest prt:"<<iter->first.m_flow.m_destPort;
  m_flowStats[flowStats->magFlow]=flowStats;
  std::cout<<"\nnew flow added,the new flow contains:";
  iter = m_flowStats.find (flowStats->magFlow);
  std::cout<<"\nmag flow:\nmag ipv6:"<<iter->first.m_magIpv6<<",src ip:"<<iter->first.m_flow.m_srcIp<<",dest ip:"<<iter->first.m_flow.m_destIp<<",src port:"<<iter->first.m_flow.m_srcPort<<",dest prt:"<<iter->first.m_flow.m_destPort;
}

void FlowManager::RemoveFlow(MagFlow magFlow)
{
  FlowStatsCacheI iter;
  iter = m_flowStats.find (magFlow);
  if (iter == m_flowStats.end ())
    return;
  FlowStats* flowStats=iter->second;
  flowStats->snrTimer.~Timer();
  delete(flowStats);
  m_flowStats.erase(magFlow);
}

void FlowManager::SetTimer(FlowStats* stats)
{
  NS_ASSERT (!m_maxDelay.IsZero ());

  stats->timer = Timer (Timer::CANCEL_ON_DESTROY);
  stats->timer.SetFunction (&FlowManager::RemoveFlow, this);
  stats->timer.SetArguments(stats->magFlow);
  stats->timer.SetDelay (m_maxDelay);
  stats->timer.Schedule ();
}

void FlowManager::AddAtt(Ipv6Address magIpv6,uint8_t att)
{
  m_att[magIpv6]=att;
}

FlowManager::FlowStats* FlowManager:: LookupFlow(MagFlow magFlow)
{
  FlowStatsCacheI iter;
  iter = m_flowStats.find ((const MagFlow) magFlow);
  std::cout<<"\nsize of hash:"<<m_flowStats.size();
  if (iter == m_flowStats.end ())
    {
      return NULL;
    }

    return iter->second;
}

std::list<FlowManager::FlowStats*> FlowManager:: LookupFlows(Ipv6Address magIpv6)
{
  FlowStatsCacheI iter;
  std::list<FlowStats*> flowStats;
  for(iter=m_flowStats.begin();iter!=m_flowStats.end();++iter)
    {
      if(iter->second->magFlow.m_magIpv6==magIpv6)
        {
          flowStats.push_front(iter->second);
        }
    }
    return flowStats;
}

std::list<FlowManager::FlowStats*> FlowManager::GetFlowStats(Flow flow)
{
  FlowStatsCacheI iter;
  std::list<FlowStats*> flowStats;
  for(iter=m_flowStats.begin();iter!=m_flowStats.end();++iter)
    {
      if((const Flow) iter->second->magFlow.m_flow == (const Flow) flow)
        {
          flowStats.push_front(iter->second);
        }
    }
    return flowStats;
}

bool operator == (Flow const &flow1, Flow const &flow2)
{
  return (flow1.m_protocol == flow2.m_protocol &&
    flow1.m_srcIp == flow2.m_srcIp &&
    flow1.m_destIp == flow2.m_destIp &&
    flow1.m_srcPort == flow2.m_srcPort &&
    flow1.m_destPort == flow2.m_destPort);
}

bool operator == (MagFlow const &magFlow1, MagFlow const &magFlow2)
{
  return (magFlow1.m_flow == magFlow2.m_flow &&
      magFlow1.m_magIpv6 == magFlow2.m_magIpv6);
}

size_t FlowHash::operator () (Flow const &x) const
{
  unsigned char* buf;
  std::ostringstream oss (std::ostringstream::ate); ;
  oss << x.m_protocol << x.m_srcIp << x.m_destIp << x.m_srcPort << x.m_destPort;
  std::string str =  oss.str();
  buf=(unsigned char*)str.c_str();
  return lookuphash (buf, str.length(), 0);
}

size_t MagFlowHash::operator () (MagFlow const &x) const
{
  unsigned char* buf;
  std::ostringstream oss (std::ostringstream::ate); 
  oss << x.m_magIpv6 << x.m_flow.m_protocol << x.m_flow.m_srcIp << x.m_flow.m_destIp << x.m_flow.m_srcPort << x.m_flow.m_destPort;
  std::string str =  oss.str();
  buf=(unsigned char*)str.c_str();
  return lookuphash (buf,str.length(), 0);
}

size_t MnIdentifierHash::operator () (Identifier const &x) const
{
  uint8_t buf[Identifier::MAX_SIZE];

  x.CopyTo (buf, Identifier::MAX_SIZE);

  return lookuphash (buf, x.GetLength(), 0);
}

void FlowManager::DoDispose ()
{
  FlowStatsCacheI iter;
    for(iter=m_flowStats.begin();iter!=m_flowStats.end();++iter)
     {
        FlowStats *flowStats=iter->second;
        delete(flowStats);
     }
    m_flowStats.clear();
  //Object::DoDispose ();
}
}//namespace ns3
