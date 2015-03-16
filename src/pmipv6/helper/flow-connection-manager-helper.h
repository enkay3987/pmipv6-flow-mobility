/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef FLOW_CONNECTION_MANAGER_HELPER_H
#define FLOW_CONNECTION_MANAGER_HELPER_H

#include "ns3/flow-connection-manager.h"
#include "ns3/node.h"
#include "ns3/wifi-module.h"
#include "ns3/net-device.h"
#include "ns3/wifi-net-device.h"
#include "ns3/lte-net-device.h"

namespace ns3 {

class FlowConnectionManagerHelper
{
public:
  void InstallWifi (Ptr<Node> node,Ptr<WifiNetDevice> netDevice,uint8_t n);
  void InstallLte(Ptr<Node> node,Ptr<LteNetDevice> lteNetDevice,uint8_t n);
  void InstallWifiBndwdthTrace(Ptr<Node> node,Ptr<NetDevice> netDevice);
  void InstallLteBndwdthTrace(Ptr<Node> node,Ptr<NetDevice> netDevice);
  Ptr<FlowConnectionManager> GetFlowConnectionManager(Ptr<Node> node);

};

}

#endif /* FLOW_CONNECTION_MANAGER_HELPER_H */

