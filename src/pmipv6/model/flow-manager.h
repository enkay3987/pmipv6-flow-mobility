#ifndef FLOW_MANAGER_H
#define FLOW_MANAGER_H

#include "ns3/object.h"
#include "ns3/ipv6-address.h"
#include "ns3/nstime.h"
#include "ns3/timer.h"
#include "ns3/sgi-hashmap.h"
#include "ns3/packet.h"
#include "ns3/internet-module.h"
#include "ns3/pmipv6-profile.h"
#include "ns3/identifier.h"
#include<string>
#include<list>


namespace ns3{

class FlowManager;
class Flow;
class MagFlow;

class Flow
{
public:
  uint8_t m_protocol;
  Ipv6Address m_srcIp;
  Ipv6Address m_destIp;
  uint16_t m_srcPort;
  uint16_t m_destPort;
  friend bool operator == (Flow const &flow1, Flow const &flow2);
};

bool operator == (Flow const &flow1, Flow const &flow2);

class FlowHash : public std::unary_function<Flow, size_t>
{
public:
 /**
  * \brief Unary operator to hash FLow.
  * \param x Flow to hash
  * \returns the hash of the Flow
  */
 size_t operator () (Flow const &x) const;
};

class MagFlow
{
public:
  Flow m_flow;
  Ipv6Address m_magIpv6;
  friend bool operator == (MagFlow const &magFlow1, MagFlow const &magFlow2);
};

bool operator == (MagFlow const &magFlow1, MagFlow const &magFlow2);

class MagFlowHash : public std::unary_function<MagFlow, size_t>
 {
public:
 /**
  * \brief Unary operator to hash FLow.
  * \param x Flow to hash
  * \returns the hash of the Flow
  */
 size_t operator () (MagFlow const &x) const;
 };

class MnIdentifierHash : public std::unary_function<Identifier, size_t>
{
public:
  /**
   * \brief Unary operator to hash Identifier.
   * \param x Identifier to hash
   */
  size_t operator () (Identifier const &x) const;
};

class FlowManager : public Object
{
public:
  struct FlowStats
  {
    MagFlow magFlow;
    Time  timeFirstPacket;
    Time  timeLastPacket;
    uint32_t packets;
    uint64_t bytes;
    Timer timer;
    uint8_t link;
    uint8_t att;
    Ptr<Node> node;
    double snr;
    Timer snrTimer;
  };
  static TypeId GetTypeId ();
  void SetMaxDelay(Time delay);
  void HandlePacket(Ptr<Packet> packet,Ipv6Address magIpv6,uint8_t link);
  FlowStats* LookupFlow(MagFlow magFlow);
  std::list<FlowStats*> LookupFlows(Ipv6Address MagIpv6);
  void AddFlow(FlowStats* flowStats);
  void RemoveFlow(MagFlow magFlow);
  void SetTimer(FlowStats* stats);
  void AddAtt(Ipv6Address magIpv6,uint8_t att);
  Ptr<Node> GetNode(Ipv6Address srcAddr,Ipv6Address destAddr);
  std::list<FlowStats*> GetFlowStats(Flow flow);
  void FillSnr(FlowStats* flowStats);
  void SetPmipv6Profile(Ptr<Pmipv6Profile> profile);
  void SetNodesHash(Identifier id,Ptr<Node> node);
  Ptr<Node> LookupNodesHash(Identifier id);
  void SetSnrTimer(FlowStats* stats);
  void SetTimeForSnr(Time snrTime);

protected:
  virtual void DoDispose ();

private:
  typedef sgi::hash_map <MagFlow,FlowStats*, MagFlowHash> FlowStatsCache;
  typedef sgi::hash_map <MagFlow,FlowStats*, MagFlowHash>::iterator FlowStatsCacheI;
  typedef sgi::hash_map<Ipv6Address,uint8_t,Ipv6AddressHash> AttCache;
  typedef sgi::hash_map<Ipv6Address,uint8_t,Ipv6AddressHash>::iterator AttCacheI;
  typedef sgi::hash_map<Identifier,Ptr<Node>,MnIdentifierHash > NodesHashCache;
  typedef sgi::hash_map<Identifier,Ptr<Node>,MnIdentifierHash>::iterator NodesHashCacheI;

  FlowStatsCache m_flowStats;
  AttCache m_att;
  Time m_maxDelay;
  Time m_timeForSnr;
  Ptr<Pmipv6Profile> m_pmipv6Profile;
  NodesHashCache m_nodesHash;
};

}
#endif
