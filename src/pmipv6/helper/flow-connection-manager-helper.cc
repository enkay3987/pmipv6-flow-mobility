/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "flow-connection-manager-helper.h"
#include "ns3/flow-connection-manager.h"
#include "ns3/wifi-module.h"
//#include "ns3/regular-wifi-mac.h"
//#include "ns3/mac-low.h"
#include "ns3/lte-helper.h"
#include "ns3/lte-module.h"
//#include "ns3/wifi-net-device.h"
#include "ns3/net-device.h"


NS_LOG_COMPONENT_DEFINE ("FlowConnectionManagerHelper");

namespace ns3 {

void FlowConnectionManagerHelper::InstallWifi (Ptr<Node> node,Ptr<WifiNetDevice> netDevice,uint8_t n)
{
  NS_LOG_FUNCTION (this << node<<netDevice<<n);
  Ptr<FlowConnectionManager> cm=CreateObject<FlowConnectionManager> ();

  if(node->GetObject<FlowConnectionManager>() == NULL)
    node->AggregateObject (cm);
  else
    cm=node->GetObject<FlowConnectionManager>();
  cm->SetnWifi(n);

  Ptr<WifiMac> wifiMac=netDevice->GetMac();
  Ptr<RegularWifiMac> regularWifiMac=DynamicCast<RegularWifiMac> (wifiMac);
  Ptr<MacLow> macLow=regularWifiMac->GetMacLow();
  macLow->SetWifiSnrCallback(MakeCallback (&FlowConnectionManager::AddWifiSnr, cm));
 }

void FlowConnectionManagerHelper::InstallLte(Ptr<Node> node,Ptr<LteNetDevice> lteNetDevice,uint8_t n)
{
  NS_LOG_FUNCTION (this << node);
  Ptr<FlowConnectionManager> cm=CreateObject<FlowConnectionManager> ();

  if(node->GetObject<FlowConnectionManager>() == NULL)
    node->AggregateObject (cm);
  else
    cm=node->GetObject<FlowConnectionManager>();
  cm->SetnLte(n);

  uint32_t nodeid = node->GetId ();
  uint32_t deviceid =lteNetDevice->GetIfIndex ();
  std::ostringstream oss;
  oss << "/NodeList/" << nodeid << "/DeviceList/" << deviceid << "/LteUePhy/ReportCurrentCellRsrpSinr";
  Config::ConnectWithoutContext (oss.str (),
                     MakeCallback (&FlowConnectionManager::CallAddLteSnr,cm));
}

void  FlowConnectionManagerHelper::InstallWifiBndwdthTrace(Ptr<Node> node,Ptr<NetDevice> netDevice)
{
  NS_LOG_FUNCTION (this << node << netDevice);
  Ptr<FlowConnectionManager> cm=CreateObject<FlowConnectionManager> ();
  node->AggregateObject (cm);
  Ptr<WifiNetDevice> apNetDevice=DynamicCast<WifiNetDevice> (netDevice);
  apNetDevice->SetBndwdthCallback(MakeCallback (&FlowConnectionManager::WifiBandwidthTrace , cm));

}

void FlowConnectionManagerHelper::InstallLteBndwdthTrace(Ptr<Node> node,Ptr<NetDevice> netDevice)
{
  NS_LOG_FUNCTION (this << node << netDevice);
  Ptr<FlowConnectionManager> cm=CreateObject<FlowConnectionManager> ();
  node->AggregateObject (cm);
  Ptr<LteNetDevice> lteNetDevice=DynamicCast<LteNetDevice> (netDevice);
  Ptr<LteEnbNetDevice> lteEnbNetDevice=DynamicCast<LteEnbNetDevice> (lteNetDevice);
  lteEnbNetDevice->SetLteBndwdthCallback(MakeCallback (&FlowConnectionManager::LteBandwidthTrace , cm));
}

Ptr<FlowConnectionManager> FlowConnectionManagerHelper::GetFlowConnectionManager(Ptr<Node> node)
{
   Ptr<FlowConnectionManager> cm=node->GetObject<FlowConnectionManager>();
   return cm;
}
}

