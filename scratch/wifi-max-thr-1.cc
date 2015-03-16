/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 *
 * Proxy Mobile IPv6 (RFC5213) Implementation in NS3 
 *
 * Korea Univerity of Technology and Education (KUT)
 * Electronics and Telecommunications Research Institute (ETRI)
 *
 * Author: Hyon-Young Choi <commani@gmail.com>
 *
 */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/applications-module.h"
#include "ns3/wifi-module.h"
#include "ns3/csma-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/bridge-module.h"
#include "ns3/pmipv6-module.h"

#include "ns3/ipv6-static-routing.h"
#include "ns3/ipv6-static-source-routing.h"
#include "ns3/ipv6-routing-table-entry.h"
#include "ns3/flow-monitor-module.h"

#include <sstream>

NS_LOG_COMPONENT_DEFINE ("WifiMaxThr");

const std::string traceFilename = "wifi-max-thr";

using namespace ns3;

static void udpRx (std::string context, Ptr<const Packet> packet, const Address &address)
{
  SeqTsHeader seqTs;
  packet->Copy ()->RemoveHeader (seqTs);
  NS_LOG_DEBUG (context << " " << seqTs.GetTs () << "->" << Simulator::Now() << ": " << seqTs.GetSeq());
}

static void createApp (NodeContainer cn, NodeContainer stas, double simTime, uint32_t interPacketInterval, uint32_t maxPacketCount)
{
  Ipv6Address staAddress[stas.GetN ()];
  for (uint32_t i = 0; i < stas.GetN (); i++)
    {
      Ptr<Ipv6> ipv6;
      ipv6 = stas.Get (i)->GetObject<Ipv6> ();
      staAddress[i] = ipv6->GetAddress(1, 1).GetAddress ();
    }

  const uint16_t port = 6000;
  ApplicationContainer serverApps, clientApps;

  NS_LOG_INFO ("Installing UDP server on MNs");
  PacketSinkHelper sink ("ns3::UdpSocketFactory",
                           Inet6SocketAddress (Ipv6Address::GetAny (), port));
  serverApps = sink.Install (stas);
  NS_LOG_INFO ("Installing UDP clients on CN");
  for (uint32_t i = 0; i < stas.GetN (); i++)
    {
      UdpClientHelper udpClient(staAddress, port);
      udpClient.SetAttribute ("Interval", TimeValue (MilliSeconds (interPacketInterval)));
      udpClient.SetAttribute ("PacketSize", UintegerValue (1000));
      udpClient.SetAttribute ("MaxPackets", UintegerValue (maxPacketCount));
      ApplicationContainer app = udpClient.Install (cn.Get (0));
      clientApps.Add (app);
    }
  Config::Connect ("/NodeList/*/ApplicationList/*/$ns3::PacketSink/Rx", MakeCallback(&udpRx));
  serverApps.Start (Seconds (0.0));
  serverApps.Stop (Seconds (simTime));
  clientApps.Start (Seconds (1.0));
  clientApps.Stop (Seconds (simTime - 2 < 0 ? simTime : simTime - 2));
}

void StopDad (NodeContainer nodes)
{
  for (uint32_t i = 0; i < nodes.GetN (); i++)
    {
      Ptr<Node> node = nodes.Get (i);
      Ptr<Icmpv6L4Protocol> icmpv6L4Protocol = node->GetObject<Icmpv6L4Protocol> ();
      NS_ASSERT (icmpv6L4Protocol != 0);
      icmpv6L4Protocol->SetAttribute ("DAD", BooleanValue (false));
    }
}

int main (int argc, char *argv[])
{
  // disable fragmentation for frames below 2200 bytes
  Config::SetDefault ("ns3::WifiRemoteStationManager::FragmentationThreshold", StringValue ("2200"));
  // turn off RTS/CTS for frames below 2200 bytes
  Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue ("2200"));

  NodeContainer stas;
  NodeContainer cn;
  NodeContainer lma;
  NodeContainer mags;
  NodeContainer aps;

  NodeContainer outerNet;
  NodeContainer magApNet;
  NodeContainer lmaMagNet;
  
  NetDeviceContainer outerDevs;
  NetDeviceContainer lmaMagDevs;
  NetDeviceContainer magApDevs;
  NetDeviceContainer apDevs;
  NetDeviceContainer brDevs;
  NetDeviceContainer staDevs;

  Ipv6InterfaceContainer outerIfs;
  Ipv6InterfaceContainer lmaMagIfs;
  Ipv6InterfaceContainer magApIfs;
  Ipv6InterfaceContainer staIfs;

  bool activeProbing;
  bool enableTrace;
  bool enablePcap;
  uint32_t mns; // Number of MNs
  double velocity;
  uint32_t packetSize;
  uint32_t maxPacketCount;
  uint32_t interPacketInterval;
  double simTime;

  // Default values
  velocity = 2; // 2 m/s
  activeProbing = false; // Connection gets established only after getting beacons.
  enableTrace = enablePcap = false; // Tracing and pcap is disabled.
  packetSize = 1024;  // packet size is 1024 bytes.
  maxPacketCount = 0xffffffff;  // There are infinite packets to send out.
  interPacketInterval = 100; // A packet is generated every 100 milliseconds.
  mns = 1;
  simTime = 50; // Simulation time (in secs).

  CommandLine cmd;
  cmd.AddValue ("simTime", "Simulation time (in secs)", simTime);
  cmd.AddValue ("velocity", "Velocity of the MN", velocity);
  cmd.AddValue ("activeProbing", "Whether MN uses active probing", activeProbing);
  cmd.AddValue ("enableTrace", "Whether trace file be generated", enableTrace);
  cmd.AddValue ("enablePcap", "Whether pcap files be generated", enablePcap);
  cmd.AddValue ("packetSize", "UDP packet size", packetSize);
  cmd.AddValue ("maxPackets", "The maximum packets UDP client sends out", maxPacketCount);
  cmd.AddValue ("interPacketInterval", "UDP inter packet interval (in milliseconds)", interPacketInterval);
  cmd.AddValue ("mn", "The number of MNs", mns);
  cmd.Parse(argc, argv);

  NS_LOG_INFO ("Creating nodes.");
  lma.Create(1);
  mags.Create (1);
  aps.Create(1);
  cn.Create(1);
  stas.Create(mns);

  InternetStackHelper internet;
  NS_LOG_INFO ("Installing Internet stack.");
  internet.Install (lma);
  internet.Install (mags);
  internet.Install (aps);
  internet.Install (cn);
  internet.Install (stas);

  outerNet.Add(lma);
  outerNet.Add(cn);

  lmaMagNet.Add (lma);
  lmaMagNet.Add (mags);
  magApNet.Add (mags);
  magApNet.Add (aps);

  // MAG's MAC Address (for unified default gateway of MN)
  Mac48Address magMacAddr("00:00:AA:BB:CC:DD");
  Ipv6InterfaceContainer iifc;
  CsmaHelper csma, csmaInternet;
  PointToPointHelper pointToPoint;

  // Link between CN and LMA is 50Mbps with 100ms delay.
  pointToPoint.SetDeviceAttribute ("DataRate", DataRateValue (DataRate(50000000)));
  pointToPoint.SetChannelAttribute ("Delay", TimeValue (MilliSeconds(100)));
  pointToPoint.SetDeviceAttribute ("Mtu", UintegerValue (1400));

  NS_LOG_INFO ("Creating the Internet network to the CN.");
  outerDevs = pointToPoint.Install (outerNet);
  Ipv6AddressHelper addressHelper;
  addressHelper.SetBase ("3ffe:aa::", "64");
  outerIfs = addressHelper.Assign (outerDevs);
  outerIfs.SetForwarding (0, true);
  outerIfs.SetDefaultRouteInAllNodes (0);

  // All other links are 50Mbps with 10ms delay.
  csma.SetChannelAttribute ("DataRate", DataRateValue (DataRate(50000000)));
  csma.SetChannelAttribute ("Delay", TimeValue (MilliSeconds(10)));
  csma.SetDeviceAttribute ("Mtu", UintegerValue (1500));
  pointToPoint.SetDeviceAttribute ("DataRate", DataRateValue (DataRate(50000000)));
  pointToPoint.SetChannelAttribute ("Delay", TimeValue (MilliSeconds(10)));
  pointToPoint.SetDeviceAttribute ("Mtu", UintegerValue (1500));

  // Each MAG is connected to LMA via a point-to-point link.
  NS_LOG_INFO("Creating core network.");
  addressHelper.SetBase ("3ffe::", "64");
  lmaMagDevs = csma.Install (lmaMagNet);
  iifc = addressHelper.Assign (lmaMagDevs);
  addressHelper.NewNetwork ();
  lmaMagIfs.Add (iifc);
  lmaMagIfs.SetForwarding (0, true);
  lmaMagIfs.SetDefaultRouteInAllNodes (0);

  // Setting common WLAN parameters
  Ssid ssid = Ssid("PMIP");
  YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default ();
  wifiPhy.SetPcapDataLinkType (YansWifiPhyHelper::DLT_IEEE802_11_RADIO);
  WifiHelper wifi = WifiHelper::Default ();
  wifi.SetStandard (WIFI_PHY_STANDARD_80211g);
  NqosWifiMacHelper wifiMac = NqosWifiMacHelper::Default ();
  YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default ();
  wifiPhy.SetChannel (wifiChannel.Create ());
  wifiMac.SetType ("ns3::ApWifiMac",
               "Ssid", SsidValue (ssid),
               "BeaconGeneration", BooleanValue (true),
               "BeaconInterval", TimeValue (MilliSeconds(100)),
               "EnableBeaconJitter", BooleanValue (true));

  // Setting the network between MAGs and APs.
  csma.SetChannelAttribute ("DataRate", DataRateValue (DataRate(50000000)));
  csma.SetChannelAttribute ("Delay", TimeValue (MilliSeconds(1)));
  csma.SetDeviceAttribute ("Mtu", UintegerValue (1500));
  NS_LOG_INFO ("Setting up MAG-AP network.");
  BridgeHelper bridge;
  addressHelper.SetBase ("3ffe:1:0:0::", "64");
  magApDevs = csma.Install (magApNet);
  magApDevs.Get(0)->SetAddress (magMacAddr);
  magApIfs = addressHelper.Assign (magApDevs.Get (0));
  addressHelper.NewNetwork ();
  apDevs = wifi.Install (wifiPhy, wifiMac, aps);
  brDevs = bridge.Install (aps.Get(0), NetDeviceContainer (apDevs, magApDevs.Get (1)));
  iifc = addressHelper.AssignWithoutAddress (magApDevs.Get (1));
  magApIfs.Add (iifc);
  magApIfs.SetForwarding (0, true);
  magApIfs.SetDefaultRouteInAllNodes (0);

  NS_LOG_INFO ("Setting up MN.");
  wifiMac.SetType ("ns3::StaWifiMac",
                   "Ssid", SsidValue (ssid),
                   "ActiveProbing", BooleanValue (activeProbing));
  staDevs = wifi.Install (wifiPhy, wifiMac, stas);
  staIfs = addressHelper.AssignWithoutAddress (staDevs);

  // Installing mobility models
  NS_LOG_INFO ("Installing mobility models.");
  MobilityHelper mobility;
  // Assign fixed positions to APs
  Ptr<ListPositionAllocator> listPosAlloc = CreateObject<ListPositionAllocator> ();
  listPosAlloc->Add (Vector (0, 0, 0));
  mobility.SetPositionAllocator (listPosAlloc);
  mobility.Install (aps);
  // Assign fixed position to CN
  listPosAlloc = CreateObject<ListPositionAllocator> ();
  listPosAlloc->Add (Vector (250, 20, 0));
  mobility.SetPositionAllocator (listPosAlloc);
  mobility.Install (cn);
  // Assign fixed position to LMA
  listPosAlloc = CreateObject<ListPositionAllocator> ();
  listPosAlloc->Add (Vector (20, 20, 0));
  mobility.SetPositionAllocator (listPosAlloc);
  mobility.Install (lma);
  // Assign fixed position to MAGs close to APs.
  listPosAlloc = CreateObject<ListPositionAllocator> ();
  listPosAlloc->Add (Vector (-5, 0, 0));
  mobility.SetPositionAllocator (listPosAlloc);
  mobility.Install (mags);
  // The MNs are allocated random positions in a circle with centre (0, 0) and radius 220.
  // They move randomly within the circle boundary.
  Ptr<RandomDiscPositionAllocator> randomDiscPosAlloc = CreateObject<RandomDiscPositionAllocator> ();
  randomDiscPosAlloc->SetAttribute ("Rho", StringValue ("ns3::UniformRandomVariable[Min=0|Max=20]"));
  mobility.SetPositionAllocator (randomDiscPosAlloc);
  mobility.Install (stas);

  // Attach PMIPv6 agents
  NS_LOG_INFO("Creating Pmipv6 profiles.");
  Ptr<Pmipv6ProfileHelper> profile = Create<Pmipv6ProfileHelper> ();
  std::ostringstream oss;
  // Adding profile for each station
  for (uint32_t i = 0; i < staDevs.GetN (); i++)
    {
      oss.str ("");
      oss.clear ();
      oss << "sta_" << i << "@iith.ac.in";
      profile->AddProfile (Identifier (oss.str ().c_str ()), Identifier (Mac48Address::ConvertFrom (staDevs.Get (i)->GetAddress ())), lmaMagIfs.GetAddress (0, 1), std::list<Ipv6Address> ());
    }

  NS_LOG_INFO("Setting up Pmipv6 on LMA.");
  Pmipv6LmaHelper lmahelper;
  lmahelper.SetPrefixPoolBase (Ipv6Address ("3ffe:aa:4::"), 48);
  lmahelper.SetProfileHelper (profile);
  lmahelper.Install (lma.Get (0));
  
  NS_LOG_INFO("Setting up Pmipv6 on MAGs.");
  Pmipv6MagHelper maghelper;
  maghelper.SetProfileHelper(profile);
  maghelper.Install (mags.Get (0), magApIfs.GetAddress (0, 0), aps.Get (0));

  // Setting up applications
  Simulator::Schedule (Seconds(3), &createApp, cn, stas, simTime, interPacketInterval, maxPacketCount);

  // Enabling trace/pcap capture
  if (enableTrace)
  {
    AsciiTraceHelper ascii;
    csma.EnableAsciiAll (ascii.CreateFileStream (traceFilename));
  }
  if (enablePcap)
  {
    csma.EnablePcapAll (traceFilename, false);
    pointToPoint.EnablePcapAll (traceFilename, false);
    wifiPhy.EnablePcap (traceFilename, apDevs);
    wifiPhy.EnablePcap (traceFilename, staDevs);
  }

  StopDad (lma);
  StopDad (mags);
  StopDad (aps);
  StopDad (stas);
  StopDad (cn);

  NodeContainer flowMonNodes;
  flowMonNodes.Add (cn);
  flowMonNodes.Add (stas);
  FlowMonitorHelper flowMonHelper;
  Ptr<FlowMonitor> flowMon = flowMonHelper.Install (flowMonNodes);

  Simulator::Stop (Seconds (simTime));
  Simulator::Run ();
  flowMon->SerializeToXmlStream(std::cout, 2, false, false);
  Simulator::Destroy ();
  return 0;
}

