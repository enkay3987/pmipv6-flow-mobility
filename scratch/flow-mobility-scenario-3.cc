/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
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
 *
 * Author: Naveen Kamath <cs11m02@iith.ac.in>
 */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/lte-module.h"
#include "ns3/applications-module.h"
#include "ns3/point-to-point-helper.h"
#include "ns3/config-store.h"
#include "ns3/pmipv6-module.h"
#include "ns3/wifi-module.h"
#include "ns3/flow-monitor-module.h"
#include "random-generator.h"

const std::string traceFilename = "flow-mobility-scenario-3";

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("FlowMobilityScenario3");

long recvBytes = 0;
Time lastRecvTime, firstSendTime;

//struct FlowStats
//{
//  Time     timeFirstTxPacket;
//  Time     timeFirstRxPacket;
//  Time     timeLastTxPacket;
//  Time     timeLastRxPacket;
//  Time     delaySum; // delayCount == rxPackets
//  Time     jitterSum; // jitterCount == rxPackets - 1
//  uint64_t txBytes;
//  uint64_t rxBytes;
//  uint32_t txPackets;
//  uint32_t rxPackets;
//  uint32_t lostPackets;
//};

//typedef std::map<FlowId, FlowMonitor::FlowStats> FlowMap;
//typedef std::map<FlowId, FlowMonitor::FlowStats>::iterator FlowMapI;

struct WifiApPlacement
{
  Vector3D wifiPosition;
  uint16_t noOfUes;
};

void PrintCompleteNodeInfo(Ptr<Node> node)
{
  int n_interfaces, n_ipaddrs, i, j;
  Ptr<Ipv6> ipv6;
  Ptr<Ipv6L3Protocol> ipv6l3;
  Ipv6InterfaceAddress ipv6address;

  ipv6 = node->GetObject<Ipv6> ();
  ipv6l3 = node->GetObject<Ipv6L3Protocol> ();
  n_interfaces = ipv6->GetNInterfaces();
  NS_LOG_UNCOND ("No of interfaces: " << n_interfaces);
  for (i = 0; i < n_interfaces; i++)
  {
    n_ipaddrs = ipv6->GetNAddresses(i);
    NS_LOG_UNCOND ("Interface " << i << " Forwarding: " << ipv6l3->GetInterface (i)->IsForwarding ());
    for (j = 0; j < n_ipaddrs; j++)
    {
      ipv6address = ipv6->GetAddress(i, j);
      NS_LOG_UNCOND (ipv6address);
    }
  }
  OutputStreamWrapper osw = OutputStreamWrapper (&std::cout);
  Ptr<Ipv6RoutingProtocol> ipv6rp = ipv6->GetRoutingProtocol();
  ipv6rp->PrintRoutingTable(&osw);
}

void PrintLteNodesInfo (NodeContainer nodes)
{
  // Print PGW info
  NS_LOG_UNCOND ("PGW node");
  PrintCompleteNodeInfo (nodes.Get (0));

  // Print SGW info
  NS_LOG_UNCOND ("SGW node");
  PrintCompleteNodeInfo (nodes.Get (1));
}

void PrintUeNodesInfo (NodeContainer nodes)
{
  // Print UE Info
  for (uint32_t i = 0; i < nodes.GetN (); i++)
    {
      NS_LOG_UNCOND ("UE " << i);
      PrintCompleteNodeInfo (nodes.Get (i));
    }
}

void PrintWifiNodesInfo (NodeContainer nodes)
{
  // Print Wifi Nodes Info
  for (uint32_t i = 0; i < nodes.GetN (); i++)
    {
      NS_LOG_UNCOND ("UE " << i);
      PrintCompleteNodeInfo (nodes.Get (i));
    }
}

void RxTrace (std::string context, Ptr<const Packet> packet, Ptr<Ipv6> ipv6, uint32_t interfaceId)
{
  Ipv6Header ipv6Header;
  packet->PeekHeader (ipv6Header);
  NS_LOG_DEBUG (context << " " << ipv6Header << " " << interfaceId);
}

void TxTrace (std::string context, Ptr<const Packet> packet, Ptr<Ipv6> ipv6, uint32_t interfaceId)
{
  Ipv6Header ipv6Header;
  packet->PeekHeader (ipv6Header);
  NS_LOG_DEBUG (context << " " << ipv6Header << " " << interfaceId);
}

void DropTrace (std::string context, const Ipv6Header & ipv6Header, Ptr<const Packet> packet, Ipv6L3Protocol::DropReason dropReason, Ptr<Ipv6> ipv6, uint32_t interfaceId)
{
  NS_LOG_DEBUG (context << " " << ipv6Header.GetSourceAddress () << " " << ipv6Header.GetDestinationAddress () << " " << dropReason << " " << interfaceId);
}

void PacketSinkRxTrace (std::string context, Ptr<const Packet> packet, const Address &address)
{
  SeqTsHeader seqTs;
  packet->Copy ()->RemoveHeader (seqTs);
  NS_LOG_INFO (context << " " << seqTs.GetTs () << "->" << Simulator::Now() << ": " << seqTs.GetSeq());
  recvBytes += packet->GetSize () + 8 + 40;
  lastRecvTime = Simulator::Now ();
}

struct Args
{
  NodeContainer ueNodes;
  Ptr<PointToPointEpc6Pmipv6Helper> epcHelper;
  Ptr<Node> remoteHost;
  std::vector<Ipv6InterfaceContainer> ueIpIfaceList;
  Ipv6Address remoteHostAddr;
  std::vector<uint32_t> noOfFlows;
  Time clientRunningTime;
};

void InstallApplications1 (Args args)
{
  NS_LOG_UNCOND ("Installing Applications 1");
  // Install and start applications on UEs and remote host
  uint16_t dlPort = 2000;
  ApplicationContainer clientApps;
  RandomGenerator rGen (args.ueNodes.GetN ());
  RandomGenerator::SetSeed ();

  for (uint32_t j = 0; j < args.noOfFlows.size (); j++)
    {
      rGen.Reset ();
      NS_LOG_DEBUG ("Flow type: " << j);
      for (uint32_t i = 0; i < args.noOfFlows[j]; i++)
        {
          uint32_t ue = rGen.GetNext ();
          NS_LOG_DEBUG ("Application installed on UE " << ue << ", address: " << args.ueIpIfaceList[ue].GetAddress (0, 1));
          switch (j)
          {
          case 0:   // voice
            {
              UdpClientHelper dlClient (args.ueIpIfaceList[ue].GetAddress (0, 1), dlPort + j * 1000);
              dlClient.SetAttribute ("Interval", TimeValue (MilliSeconds(20)));
              dlClient.SetAttribute ("MaxPackets", UintegerValue (0xffffffff));
              dlClient.SetAttribute ("PacketSize", UintegerValue (160));
              clientApps.Add (dlClient.Install (args.remoteHost));
              break;
            }
          case 1:   // web
            {
              UdpClientHelper dlClient (args.ueIpIfaceList[ue].GetAddress (0, 1), dlPort + j * 1000);
              dlClient.SetAttribute ("Interval", TimeValue (MilliSeconds(20)));
              dlClient.SetAttribute ("MaxPackets", UintegerValue (0xffffffff));
              dlClient.SetAttribute ("PacketSize", UintegerValue (1190));
              clientApps.Add (dlClient.Install (args.remoteHost));
              break;
            }
          case 2:   // conversational video
            {
              UdpClientHelper dlClient (args.ueIpIfaceList[ue].GetAddress (0, 1), dlPort + j * 1000);
              dlClient.SetAttribute ("Interval", TimeValue (MilliSeconds(20)));
              dlClient.SetAttribute ("MaxPackets", UintegerValue (0xffffffff));
              dlClient.SetAttribute ("PacketSize", UintegerValue (940));
              clientApps.Add (dlClient.Install (args.remoteHost));
              break;
            }
          case 3:   // non-conversational video
            {
              UdpClientHelper dlClient (args.ueIpIfaceList[ue].GetAddress (0, 1), dlPort + j * 1000);
              dlClient.SetAttribute ("Interval", TimeValue (MilliSeconds(20)));
              dlClient.SetAttribute ("MaxPackets", UintegerValue (0xffffffff));
              dlClient.SetAttribute ("PacketSize", UintegerValue (1190));
              clientApps.Add (dlClient.Install (args.remoteHost));
              break;
            }
          }
        }
    }
  Config::Connect ("/NodeList/*/ApplicationList/*/$ns3::PacketSink/Rx", MakeCallback(&PacketSinkRxTrace));
  clientApps.Start (Seconds (0));
  clientApps.Stop (args.clientRunningTime);
  firstSendTime = Simulator::Now ();
}

void InstallApplications2 (Args args, std::vector<uint32_t> noOfStopFlows, Time stopFlowsRunningTime)
{
  NS_LOG_UNCOND ("Installing Applications 2");
  // Install and start applications on UEs and remote host
  uint16_t dlPort = 2000;
  ApplicationContainer clientApps1, clientApps2;
  RandomGenerator rGen (args.ueNodes.GetN ());
  RandomGenerator::SetSeed ();
  for (uint32_t j = 0; j < args.noOfFlows.size (); j++)
    {
      rGen.Reset ();
      NS_LOG_DEBUG ("Flow type: " << j);
      for (uint32_t i = 0, k = 0; i < args.noOfFlows[j]; i++)
        {
          uint32_t ue = rGen.GetNext ();
          NS_LOG_DEBUG ("Application installed on UE " << ue << ", address: " << args.ueIpIfaceList[ue].GetAddress (0, 1));
          UdpClientHelper dlClient (args.ueIpIfaceList[ue].GetAddress (0, 1), dlPort + j * 1000);
          switch (j)
          {
          case 0:   // voice
            {
              dlClient.SetAttribute ("Interval", TimeValue (MilliSeconds(20)));
              dlClient.SetAttribute ("MaxPackets", UintegerValue (0xffffffff));
              dlClient.SetAttribute ("PacketSize", UintegerValue (160));
              break;
            }
          case 1:   // web
            {
              dlClient.SetAttribute ("Interval", TimeValue (MilliSeconds(8)));
              dlClient.SetAttribute ("MaxPackets", UintegerValue (0xffffffff));
              dlClient.SetAttribute ("PacketSize", UintegerValue (450));
              break;
            }
          case 2:   // conversational video
            {
              dlClient.SetAttribute ("Interval", TimeValue (MilliSeconds(16)));
              dlClient.SetAttribute ("MaxPackets", UintegerValue (0xffffffff));
              dlClient.SetAttribute ("PacketSize", UintegerValue (750));
              break;
            }
          case 3:   // non-conversational video
            {
              dlClient.SetAttribute ("Interval", TimeValue (MilliSeconds(8)));
              dlClient.SetAttribute ("MaxPackets", UintegerValue (0xffffffff));
              dlClient.SetAttribute ("PacketSize", UintegerValue (450));
              break;
            }
          }
          if (k < noOfStopFlows[j])
            {
              clientApps2.Add (dlClient.Install (args.remoteHost));
              k++;
            }
          else
            clientApps1.Add (dlClient.Install (args.remoteHost));
        }
    }
  Config::Connect ("/NodeList/*/ApplicationList/*/$ns3::PacketSink/Rx", MakeCallback(&PacketSinkRxTrace));
  clientApps1.Start (Seconds (0));
  clientApps2.Start (Seconds (0));
  clientApps1.Stop (args.clientRunningTime);
  clientApps2.Stop (stopFlowsRunningTime);
}

void InstallServerApplications (NodeContainer nodes)
{
  ApplicationContainer serverApps;
  for (uint32_t j = 0, port = 2000; j < 4; j++, port += 1000)
    {
      for (uint32_t i = 0; i < nodes.GetN (); i++)
        {
          PacketSinkHelper dlPacketSinkHelper ("ns3::UdpSocketFactory", Inet6SocketAddress (Ipv6Address::GetAny (), port));
          serverApps.Add (dlPacketSinkHelper.Install (nodes.Get (i)));
        }
    }
  serverApps.Start (Seconds (0));
}

void StopDad (Ptr<Node> node)
{
  Ptr<Icmpv6L4Protocol> icmpv6L4Protocol = node->GetObject<Icmpv6L4Protocol> ();
  NS_ASSERT (icmpv6L4Protocol != 0);
  icmpv6L4Protocol->SetAttribute ("DAD", BooleanValue (false));
}

void CreateFlowInterfaceList (Ptr<FlowInterfaceList> flowInterfaceList)
{
  FlowInterfaceList::Entry *entry;
  TrafficSelector ts;
  std::list<uint32_t> interfaceIds;

  entry = new FlowInterfaceList::Entry (flowInterfaceList);
  entry->SetPriority (10);
  ts.SetProtocol (6);
  entry->SetTrafficSelector (ts);
  interfaceIds.push_back (8);
  interfaceIds.push_back (4);
  entry->SetInterfaceIds (interfaceIds);
  flowInterfaceList->AddFlowInterfaceEntry (entry);
}

void CreateFlowBindingList (Ptr<FlowBindingList> flowBindingList, Ptr<Node> node)
{
  FlowBindingList::Entry *entry;
  TrafficSelector ts;
  std::list<Ptr<NetDevice> > bindingIds;
  Ptr<NetDevice> lteNetDevice = node->GetDevice (1);
  Ptr<NetDevice> wifiNetDevice = node->GetDevice (2);

  entry = new FlowBindingList::Entry (flowBindingList);
  entry->SetPriority (1);
  ts.SetProtocol (6);
  entry->SetTrafficSelector (ts);
  bindingIds.push_back (lteNetDevice);
  bindingIds.push_back (wifiNetDevice);
  entry->SetBindingIds (bindingIds);
  flowBindingList->AddFlowBindingEntry (entry);
}

//void ComputeFlowStats (FlowMonitorHelper flowMonHelper, FlowMap &flowStatsMap)
//{
//  NS_LOG_UNCOND ("Compute Flow Stats");
//  Ptr<Ipv6FlowClassifier> classifier = DynamicCast<Ipv6FlowClassifier> (flowMonHelper.GetClassifier6());
//  Ptr<FlowMonitor> monitor = flowMonHelper.GetMonitor ();
//  std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats();
//  for (std::map<FlowId, FlowMonitor::FlowStats>::iterator flow = stats.begin(); flow != stats.end(); flow++)
//  {
//    FlowMapI it = flowStatsMap.find (flow->first);
//    if (it == flowStatsMap.end ())
//      {
//        FlowStats flowS;
//        flowsS.
//        flowStatsMap[flow->first] = flow->second;
//      }
//    else
//      {
//        it->second.txBytes = flow->second.txBytes - it->second.txBytes;
//        it->second.rxBytes = flow->second.rxBytes - it->second.rxBytes;
//        it->second.txPackets = flow->second.txPackets - it->second.txPackets;
//        it->second.rxPackets = flow->second.rxPackets - it->second.rxPackets;
//        it->second.lostPackets = flow->second.lostPackets - it->second.lostPackets;
//        it->second.delaySum = flow->second.delaySum - it->second.delaySum;
//        it->second.jitterSum = flow->second.jitterSum - it->second.jitterSum;
//        it->second.timeFirstTxPacket = it->second.timeLastTxPacket;
//        it->second.timeFirstRxPacket = it->second.timeLastRxPacket;
//        it->second.timeLastTxPacket = flow->second.timeLastTxPacket;
//        it->second.timeLastRxPacket = flow->second.timeLastRxPacket;
//      }
//  }
//}
//
//void PrintFlowStats (FlowMonitorHelper flowMonHelper, FlowMap &flowStatsMap)
//{
//  Ptr<Ipv6FlowClassifier> classifier = DynamicCast<Ipv6FlowClassifier> (flowMonHelper.GetClassifier6());
//  for (FlowMapI flow = flowStatsMap.begin(); flow != flowStatsMap.end(); flow++)
//    {
//      Ipv6FlowClassifier::FiveTuple  t = classifier->FindFlow (flow->first);
//      std::cout << "FlowID: " << flow->first << "( ";
//      switch(t.protocol)
//      {
//      case(6):
//        std::cout << "TCP ";
//        break;
//      case(17):
//        std::cout << "UDP ";
//        break;
//      default:
//         return ;
//      }
//
//      std::cout << t.sourceAddress << " / " << t.sourcePort << " --> " << t.destinationAddress << " / " << t.destinationPort << ")"
//      << "timeFirstTxPacket=" << flow->second.timeFirstTxPacket << " "
//      << "timeFirstRxPacket=" << flow->second.timeFirstRxPacket << " "
//      << "timeLastTxPacket=" << flow->second.timeLastTxPacket << " "
//      << "timeLastRxPacket=" << flow->second.timeLastRxPacket << " "
//      << "delaySum=" << flow->second.delaySum << " "
//      << "jitterSum=" << flow->second.jitterSum << " "
//      << "txBytes=" << flow->second.txBytes << " "
//      << "rxBytes=" << flow->second.rxBytes << " "
//      << "txPackets=" << flow->second.txPackets << " "
//      << "rxPackets=" << flow->second.rxPackets << " "
//      << "lostPackets=" << flow->second.lostPackets << " "
//      << "throughput=" << (((double)flow->second.rxBytes * 8) / ((flow->second.timeLastRxPacket.GetSeconds ()) * 1000)) << "kbps "
//      << "meanDelay=" << (flow->second.delaySum.GetSeconds() / flow->second.rxPackets) << "s "
//      << "meanJitter=" << (flow->second.jitterSum.GetSeconds() / (flow->second.rxPackets)) << "s " << std::endl;
//   }
//}

void PrintFlowStats (Ptr<FlowMonitor> flowMon)
{
  flowMon->SerializeToXmlStream (std::cout, 2, false, false);
}

/**
 * A MN having both LTE and Wifi NetDevices installed on it moves from a point where it is only connected
 * to the LTE network to where it senses a Wifi network. The MAG functionality is installed on the same
 * node as the WifiAp. This script demonstrates the flow mobility. The flow mobility functionality is installed
 * only on the LMA. Only downlink flows are run.  Flow Tracker test.
 *
 * Node 0 - SGW
 * Node 1 - PGW
 * Node 2 - Remote Host
 * Node 3 - UE
 * Node 4 - eNB
 * Node 5 - Wifi AP with MAG
 */

int
main (int argc, char *argv[])
{
  Config::SetDefault ("ns3::LteEnbRrc::SrsPeriodicity", UintegerValue (80));

  std::vector<WifiApPlacement> wifis;
  WifiApPlacement wifiApPlacement;
  wifiApPlacement.wifiPosition = Vector3D (0, -250, 0);
  wifiApPlacement.noOfUes = 10;
  wifis.push_back (wifiApPlacement);
  wifiApPlacement.wifiPosition = Vector3D (270, 100, 0);
  wifiApPlacement.noOfUes = 10;
  wifis.push_back (wifiApPlacement);
  wifiApPlacement.wifiPosition = Vector3D (-230, 180, 0);
  wifiApPlacement.noOfUes = 10;
  wifis.push_back (wifiApPlacement);

  uint16_t noOfWifis = wifis.size ();
  uint16_t noOfUes = 0;
  for (uint16_t i = 0; i < wifis.size (); i++)
    noOfUes += wifis[i].noOfUes;

  uint32_t maxPackets = 2;
  double simTime = 40;
  double interPacketInterval = 1000;
  bool enablePcap = false;
  bool enableTraces = false;

  // Command line arguments
  CommandLine cmd;
  cmd.AddValue ("simTime", "Total duration of the simulation [s])", simTime);
  cmd.AddValue ("interPacketInterval", "Inter packet interval [ms])", interPacketInterval);
  cmd.AddValue ("maxPackets", "The maximum number of packets to be sent by the application", maxPackets);
  cmd.AddValue ("enablePcap", "Set true to enable pcap capture", enablePcap);
  cmd.AddValue ("enableTraces", "Set true to enable ip traces", enableTraces);
  cmd.Parse(argc, argv);

  Mac48Address magMacAddress ("00:00:aa:bb:cc:dd");
  Ptr<LteHelper> lteHelper = CreateObject<LteHelper> ();
  Ptr<PointToPointEpc6Pmipv6Helper> epcHelper = CreateObject<PointToPointEpc6Pmipv6Helper> (magMacAddress);
  lteHelper->SetEpcHelper (epcHelper);
  epcHelper->SetupS5Interface ();
  epcHelper->SetAttribute ("S5LinkDelay", TimeValue (MilliSeconds (5)));
  Ptr<Node> pgw = epcHelper->GetPgwNode ();
  // Get PMIPv6 profiles.
  Ptr<Pmipv6ProfileHelper> profile = epcHelper->GetPmipv6ProfileHelper ();

  ConfigStore inputConfig;
  inputConfig.ConfigureDefaults();

  // parse again so you can override default values from the command line
  cmd.Parse(argc, argv);

   // Create a single RemoteHost
  NodeContainer remoteHostContainer;
  remoteHostContainer.Create (1);
  Ptr<Node> remoteHost = remoteHostContainer.Get (0);
  InternetStackHelper internet;
  internet.Install (remoteHostContainer);

  // Stop DAD on remote host as DAD is disabled on PGW as well.
  StopDad (remoteHost);

  // Create the Internet
  PointToPointHelper p2ph;
  p2ph.SetQueue ("ns3::DropTailQueue",
                "MaxPackets", UintegerValue (1000000));
  p2ph.SetDeviceAttribute ("DataRate", DataRateValue (DataRate ("100Gb/s")));
  p2ph.SetDeviceAttribute ("Mtu", UintegerValue (1500));
  p2ph.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (300)));
  NetDeviceContainer internetDevices = p2ph.Install (pgw, remoteHost);
  Ipv6AddressHelper ipv6h;
  ipv6h.SetBase ("c0::", "64");
  Ipv6InterfaceContainer internetIpIfaces = ipv6h.Assign (internetDevices);
  // interface 0 is localhost, 1 is the p2p device
  Ipv6Address remoteHostAddr = internetIpIfaces.GetAddress (1, 1);
  Ipv6Address pgwInternetAddr = internetIpIfaces.GetAddress (0, 1);

  // Create Static route for Remote Host to reach the UEs.
  Ipv6StaticRoutingHelper ipv6RoutingHelper;
  Ptr<Ipv6StaticRouting> remoteHostStaticRouting = ipv6RoutingHelper.GetStaticRouting (remoteHost->GetObject<Ipv6> ());
  remoteHostStaticRouting->AddNetworkRouteTo ("b0::", 32, pgwInternetAddr, 1);

  // Create eNB node and install mobility model
  Ptr<Node> enbNode = CreateObject<Node> ();

  // Install Mobility Model for eNB.
  MobilityHelper mobility;
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  Ptr<ListPositionAllocator> enbPositionAlloc = CreateObject<ListPositionAllocator> ();
  enbPositionAlloc->Add (Vector(0, 0, 0));
  mobility.SetPositionAllocator (enbPositionAlloc);
  mobility.Install (enbNode);

  // Install LTE Device to eNB node
  Ptr<NetDevice> enbLteDev = (lteHelper->InstallEnbDevice (NodeContainer (enbNode))).Get (0);

  // Setup Wifi network
  Ssid ssid = Ssid ("IITH");
  YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default ();
  wifiPhy.SetPcapDataLinkType (YansWifiPhyHelper::DLT_IEEE802_11_RADIO);
  WifiHelper wifi = WifiHelper::Default ();
  NqosWifiMacHelper wifiMac = NqosWifiMacHelper::Default ();
  YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default ();
  wifiPhy.SetChannel (wifiChannel.Create ());
  wifiMac.SetType ("ns3::ApWifiMac",
                   "Ssid", SsidValue (ssid),
                   "BeaconGeneration", BooleanValue (true),
                   "BeaconInterval", TimeValue (MicroSeconds (102400)),
                   "EnableBeaconJitter", BooleanValue (true));

  Ptr<Node> wifiApMag[noOfWifis];
  Ipv6InterfaceContainer wifiApMagLmaIpIfaces[noOfWifis];
  Ptr<NetDevice> wifiApMagDev[noOfWifis];
  Ipv6InterfaceContainer wifiMagApIpIfaces[noOfWifis];
  Ipv6AddressHelper ipv6hWifiMagLma;
  ipv6hWifiMagLma.SetBase ("aa::", "64");
  for (uint16_t i = 0; i < wifis.size (); i++)
    {
      wifiApMag[i] = CreateObject<Node> ();
      internet.Install (wifiApMag[i]);
      // Enable forwarding on the WifiApMag
      Ptr<Ipv6> ipv6 = wifiApMag[i]->GetObject<Ipv6> ();
      ipv6->SetAttribute ("IpForward", BooleanValue (true));

      // Create p2p network between wifiApMag and and LMA (P-GW).
      p2ph.SetDeviceAttribute ("DataRate", DataRateValue (DataRate ("100Gb/s")));
      p2ph.SetDeviceAttribute ("Mtu", UintegerValue (1500));
      p2ph.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (100)));
      NetDeviceContainer wifiApMagLmaDevs = p2ph.Install (wifiApMag[i], pgw);
      ipv6hWifiMagLma.NewNetwork ();
      wifiApMagLmaIpIfaces[i] = ipv6hWifiMagLma.Assign (wifiApMagLmaDevs);
      Ptr<Ipv6StaticRouting> wifiApMagStaticRouting = ipv6RoutingHelper.GetStaticRouting (wifiApMag[i]->GetObject<Ipv6> ());
      wifiApMagStaticRouting->SetDefaultRoute (wifiApMagLmaIpIfaces[i].GetAddress (1, 1), wifiApMagLmaIpIfaces[i].GetInterfaceIndex (0));

      // Attach Wifi AP functionality on the wifiApMag node.
      wifiApMagDev[i] = (wifi.Install (wifiPhy, wifiMac, wifiApMag[i])).Get (0);
      NetDeviceContainer wifiApMagDevs;
      wifiApMagDevs.Add (wifiApMagDev[i]);
      wifiApMagDev[i]->SetAddress (magMacAddress);
      wifiMagApIpIfaces[i] = ipv6h.AssignWithoutAddress (wifiApMagDevs);

      // Install mobility model on AP.
      Ptr<ListPositionAllocator> wifiApMagPositionAlloc = CreateObject<ListPositionAllocator> ();
      wifiApMagPositionAlloc->Add (wifis[i].wifiPosition);
      mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
      mobility.SetPositionAllocator (wifiApMagPositionAlloc);
      mobility.Install (wifiApMag[i]);

      // Add Wifi Mag functionality to WifiMag node.
      Pmipv6MagHelper magHelper;
      magHelper.SetProfileHelper (profile);
      magHelper.Install (wifiApMag[i]);
    }

  // Setup UE nodes.
  NodeContainer ues;
  ues.Create (noOfUes);

  // Set Mobility model on UEs.
  uint16_t initialUe = 0;
  for (uint16_t i = 0; i < noOfWifis; i++)
    {
      NodeContainer nodes;
      for (uint16_t j = 0; j < wifis[i].noOfUes; j++)
        {
          nodes.Add (ues.Get (j + initialUe));
        }
      // The MNs are allocated random positions within range of each WiFi AP with a radius of 100m.
      Ptr<RandomDiscPositionAllocator> randomDiscPosAlloc = CreateObject<RandomDiscPositionAllocator> ();
      randomDiscPosAlloc->SetAttribute ("Rho", StringValue ("ns3::UniformRandomVariable[Min=0|Max=100]"));
      randomDiscPosAlloc->SetX (wifis[i].wifiPosition.x);
      randomDiscPosAlloc->SetY (wifis[i].wifiPosition.y);
      mobility.SetPositionAllocator (randomDiscPosAlloc);
      mobility.Install (nodes);
      initialUe += wifis[i].noOfUes;
    }

  Ptr<NetDevice> ueLteDev[noOfUes];
  Ipv6InterfaceContainer ueIpIface[noOfUes];
  Ptr<NetDevice> ueWifiDev[noOfUes];
//  Ipv6InterfaceContainer ueWifiIfs[noOfUes];
  std::ostringstream oss;
  for (uint16_t i = 0; i < noOfUes; i++)
    {
      Ptr<Node> ueNode = ues.Get (i);
      // Install the IP stack on the UEs
      internet.Install (ueNode);
      ueLteDev[i] = (lteHelper->InstallUeDevice (NodeContainer (ueNode))).Get (0);
      // enable the UE interfaces to have link-local Ipv6 addresses.
      ueLteDev[i]->SetAddress (Mac48Address::Allocate ());
      ueIpIface[i] = epcHelper->AssignWithoutAddress (ueLteDev[i]);
      // Attach UE to eNB
      // side effect: the default EPS bearer will be activated
      lteHelper->Attach (ueLteDev[i], enbLteDev, false);

      // Add Wifi functionality to UE.
//      wifiMac.SetType ("ns3::StaWifiMac",
//                       "Ssid", SsidValue (ssid),
//                       "ActiveProbing", BooleanValue (false));
//      ueWifiDev[i] = (wifi.Install (wifiPhy, wifiMac, ueNode)).Get (0);
//
//      // Make the mac address of Wifi same as LTE.
//      ueWifiDev[i]->SetAddress (ueLteDev[i]->GetAddress ());
//      ueWifiIfs[i] = ipv6h.AssignWithoutAddress (NetDeviceContainer (ueWifiDev[i]));

      // Add profile for the UE
      Ptr<LteUeNetDevice> ueLteNetDev = DynamicCast<LteUeNetDevice> (ueLteDev[i]);
      oss.str ("");
      oss.clear ();
      oss << "node" << i << "@iith.ac.in";
      profile->AddProfile (Identifier (oss.str ().c_str ()), Identifier (Mac48Address::ConvertFrom(ueLteNetDev->GetAddress())), pgwInternetAddr, std::list<Ipv6Address> (), ueLteNetDev->GetImsi ());

      // Support flow mobility in MN.
//      Pmipv6MnHelper mnHelper;
//      mnHelper.Install (ueNode);
//      // Add entries to Flow Binding List
//      CreateFlowBindingList (mnHelper.GetFlowBindingList (ueNode), ueNode);
    }

  Ptr<EpcTft> tft[4];
  tft[0] = Create<EpcTft> ();
  tft[1] = Create<EpcTft> ();
  tft[2] = Create<EpcTft> ();
  tft[3] = Create<EpcTft> ();
  EpcTft::PacketFilter dlpf[4];
  for (uint16_t i = 0, port = 2000; i < 4; i++, port += 1000)
    {
      dlpf[i].isIpv4 = false;
      dlpf[i].localPortStart = port;
      dlpf[i].localPortEnd = port;
      tft[i]->Add (dlpf[i]);
    }
  EpsBearer bearer[4];
  bearer[0] = EpsBearer (EpsBearer::GBR_CONV_VOICE);
  bearer[1] = EpsBearer (EpsBearer::NGBR_VIDEO_TCP_DEFAULT);
  bearer[2] = EpsBearer (EpsBearer::GBR_CONV_VIDEO);
  bearer[3] = EpsBearer (EpsBearer::GBR_NON_CONV_VIDEO);
  for (uint16_t i; i < noOfUes; i++)
    {
      for (uint16_t j = 0; j < 4; j++)
        {
          lteHelper->ActivateDedicatedEpsBearer (ueLteDev[i], bearer[j], tft[j]);
        }
    }

//  // Get Flow Interface List from the LMA.
//  Ptr<Ipv6> pgwIpv6 = pgw->GetObject<Ipv6> ();
//  Ipv6FlowRoutingHelper ipv6FlowRoutingHelper;
//  Ptr<Ipv6FlowRouting> ipv6FlowRouting = ipv6FlowRoutingHelper.GetFlowRouting (pgwIpv6);
//  Ptr<FlowInterfaceList> flowInterfaceList = ipv6FlowRouting->GetFlowInterfaceList ();
//  // Add entries to Flow Interface List.
//  CreateFlowInterfaceList (flowInterfaceList);

  // Enable PCAP traces
  if (enablePcap)
    {
      p2ph.EnablePcapAll (traceFilename);
      epcHelper->EnablePcap (traceFilename, enbLteDev);
      for (int i = 0; i < noOfUes; i++)
        {
          epcHelper->EnablePcap (traceFilename, ueLteDev[i]);
          wifiPhy.EnablePcap (traceFilename, ueWifiDev[i]);
        }
      for (int i = 0; i < noOfWifis; i++)
        wifiPhy.EnablePcap (traceFilename, wifiApMagDev[i]);
    }

  // Add IP traces to all nodes.
  if (enableTraces)
    {
      // Config::Connect ("/NodeList/*/$ns3::Ipv6L3Protocol/Rx", MakeCallback (&RxTrace));
      // Config::Connect ("/NodeList/*/$ns3::Ipv6L3Protocol/Tx", MakeCallback (&TxTrace));
      Config::Connect ("/NodeList/*/$ns3::Ipv6L3Protocol/Drop", MakeCallback (&DropTrace));
    }

  // Schedule Applications.
  std::vector<Ipv6InterfaceContainer> ueIpIfaceList;
  for (int i = 0; i < noOfUes; i++)
    {
      ueIpIfaceList.push_back (ueIpIface[i]);
    }

  InstallServerApplications (ues);

  Args args1, args2;
  args1.ueNodes = args2.ueNodes= ues;
  args1.epcHelper = args2.epcHelper = epcHelper;
  args1.remoteHost = args2.remoteHost = remoteHost;
  args1.ueIpIfaceList = args2.ueIpIfaceList = ueIpIfaceList;
  args1.remoteHostAddr = args2.remoteHostAddr = remoteHostAddr;
  args1.noOfFlows.push_back (20);
  args1.noOfFlows.push_back (5);
  args1.noOfFlows.push_back (0);
  args1.noOfFlows.push_back (0);
  args1.clientRunningTime = Seconds (20);
  Simulator::Schedule (Seconds (10), &InstallApplications1, args1);

//  args2.noOfFlows.push_back (0);
//  args2.noOfFlows.push_back (18);
//  args2.noOfFlows.push_back (5);
//  args2.noOfFlows.push_back (14);
//  args2.clientRunningTime = Seconds (40);
//  std::vector<uint32_t> noOfStopFlows;
//  noOfStopFlows.push_back (0);
//  noOfStopFlows.push_back (4);
//  noOfStopFlows.push_back (5);
//  noOfStopFlows.push_back (4);
//  Time stopFlowsRunningTime = Seconds (20);
//  Simulator::Schedule (Seconds (30), &InstallApplications2, args2, noOfStopFlows, stopFlowsRunningTime);

  NodeContainer flowMonNodes;
  flowMonNodes.Add (remoteHost);
  flowMonNodes.Add (ues);
  FlowMonitorHelper flowMonHelper;
  Ptr<FlowMonitor> flowMon = flowMonHelper.Install (flowMonNodes);

//  FlowMap flowStatsMap;
//  Simulator::Schedule (Seconds (30), &ComputeFlowStats, flowMonHelper, flowStatsMap);
//  Simulator::Schedule (Seconds (30), &PrintFlowStats, flowMonHelper, flowStatsMap);
//  Simulator::Schedule (Seconds (50), &ComputeFlowStats, flowMonHelper, flowStatsMap);
//  Simulator::Schedule (Seconds (50), &PrintFlowStats, flowMonHelper, flowStatsMap);
//  Simulator::Schedule (Seconds (70), &ComputeFlowStats, flowMonHelper, flowStatsMap);
//  Simulator::Schedule (Seconds (70), &PrintFlowStats, flowMonHelper, flowStatsMap);

//  Simulator::Schedule (Seconds (30), &PrintFlowStats, flowMon);
//  Simulator::Schedule (Seconds (50), &PrintFlowStats, flowMon);

  // Run simulation
  Simulator::Stop(Seconds(simTime));
  Simulator::Run();

  flowMon->SerializeToXmlStream (std::cout, 2, false, false);
  NS_LOG_UNCOND ("Total Bytes Received: " << recvBytes << " lastTime: " << lastRecvTime);
  NS_LOG_UNCOND ("Throughput: " << recvBytes * 8 / ((lastRecvTime.GetSeconds () - firstSendTime.GetSeconds ()) * 1000 * 1000));

  Simulator::Destroy();
  return 0;
}

