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

const std::string traceFilename = "flow-mobility-scenario-1";

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("FlowMobilityScenario1");

long recvBytes = 0;
Time lastRecvTime, firstSendTime;

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
  NS_LOG_DEBUG (context << " " << seqTs.GetTs () << "->" << Simulator::Now() << ": " << seqTs.GetSeq());
  recvBytes += packet->GetSize () + 8 + 20;
  lastRecvTime = Simulator::Now ();
}

struct Args
{
  NodeContainer ueNodes;
  Ptr<PointToPointEpc6Pmipv6Helper> epcHelper;
  Ptr<Node> remoteHost;
  std::vector<Ipv6InterfaceContainer> ueIpIfaceList;
  double interPacketInterval;
  uint32_t maxPackets;
  Ipv6Address remoteHostAddr;
};

void InstallApplications (Args args)
{
  NodeContainer nodes;
  nodes.Add (args.epcHelper->GetPgwNode ());
  nodes.Add (args.epcHelper->GetSgwNode ());
  PrintLteNodesInfo (nodes);
  PrintUeNodesInfo (args.ueNodes);
  NS_LOG_UNCOND ("Installing Applications");
  // Install and start applications on UEs and remote host
  uint16_t dlPort = 2000;
  ApplicationContainer clientApps;
  ApplicationContainer serverApps;

  for (uint32_t i = 0; i < args.ueNodes.GetN (); i++)
    {
      PacketSinkHelper dlPacketSinkHelper ("ns3::UdpSocketFactory", Inet6SocketAddress (Ipv6Address::GetAny (), dlPort));
      serverApps.Add (dlPacketSinkHelper.Install (args.ueNodes.Get (i)));

      UdpClientHelper dlClient (args.ueIpIfaceList[i].GetAddress (0, 1), dlPort);
      dlClient.SetAttribute ("Interval", TimeValue (MilliSeconds(args.interPacketInterval)));
      dlClient.SetAttribute ("MaxPackets", UintegerValue (args.maxPackets));
      dlClient.SetAttribute ("PacketSize", UintegerValue (100));
      clientApps.Add (dlClient.Install (args.remoteHost));
    }
  Config::Connect ("/NodeList/*/ApplicationList/*/$ns3::PacketSink/Rx", MakeCallback(&PacketSinkRxTrace));
  serverApps.Start (Seconds (0));
  clientApps.Start (Seconds (0));
  firstSendTime = Simulator::Now ();
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

void InstallUeMobility (std::vector<WifiApPlacement> wifis, NodeContainer ues)
{
  // Set Mobility model on UEs.
  uint16_t initialUe = 0;
  for (uint16_t i = 0; i < wifis.size (); i++)
    {
      NodeContainer nodes;
      for (uint16_t j = 0; j < wifis[i].noOfUes; j++)
        {
          nodes.Add (ues.Get (j + initialUe));
        }
      // The MNs are allocated random positions within range of each WiFi AP with a radius of 100m.
      Ptr<RandomDiscPositionAllocator> randomDiscPosAlloc = CreateObject<RandomDiscPositionAllocator> ();
      randomDiscPosAlloc->SetAttribute ("Rho", StringValue ("ns3::UniformRandomVariable[Min=0|Max=50]"));
      randomDiscPosAlloc->SetX (wifis[i].wifiPosition.x);
      randomDiscPosAlloc->SetY (wifis[i].wifiPosition.y);
      MobilityHelper mobility;
      mobility.SetPositionAllocator (randomDiscPosAlloc);
      mobility.Install (nodes);
      initialUe += wifis[i].noOfUes;
    }
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
  wifiApPlacement.noOfUes = 17;
  wifis.push_back (wifiApPlacement);
  wifiApPlacement.wifiPosition = Vector3D (270, 100, 0);
  wifiApPlacement.noOfUes = 17;
  wifis.push_back (wifiApPlacement);
  wifiApPlacement.wifiPosition = Vector3D (-230, 180, 0);
  wifiApPlacement.noOfUes = 16;
  wifis.push_back (wifiApPlacement);

  // -200 to 140

  uint16_t noOfWifis = wifis.size ();
  uint16_t noOfUes = 0;
  for (uint16_t i = 0; i < wifis.size (); i++)
    noOfUes += wifis[i].noOfUes;

  uint32_t maxPackets = 2;
  double simTime = 30;
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
      p2ph.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (10)));
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

  // The MNs are allocated random positions within range of each WiFi AP with a radius of 10m.
  Ptr<RandomDiscPositionAllocator> randomDiscPosAlloc = CreateObject<RandomDiscPositionAllocator> ();
  randomDiscPosAlloc->SetAttribute ("Rho", StringValue ("ns3::UniformRandomVariable[Min=0|Max=10]"));
  randomDiscPosAlloc->SetX (0);
  randomDiscPosAlloc->SetY (0);
  mobility.SetPositionAllocator (randomDiscPosAlloc);
  mobility.Install (ues);
  //Actual mobility.
  Simulator::Schedule (Seconds (2), &InstallUeMobility, wifis, ues);

  Ptr<NetDevice> ueLteDev[noOfUes];
  Ipv6InterfaceContainer ueIpIface[noOfUes];
  Ptr<NetDevice> ueWifiDev[noOfUes];
  Ipv6InterfaceContainer ueWifiIfs[noOfUes];
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
      wifiMac.SetType ("ns3::StaWifiMac",
                       "Ssid", SsidValue (ssid),
                       "ActiveProbing", BooleanValue (false));
      ueWifiDev[i] = (wifi.Install (wifiPhy, wifiMac, ueNode)).Get (0);

      // Make the mac address of Wifi same as LTE.
      ueWifiDev[i]->SetAddress (ueLteDev[i]->GetAddress ());
      ueWifiIfs[i] = ipv6h.AssignWithoutAddress (NetDeviceContainer (ueWifiDev[i]));

      // Add profile for the UE
      Ptr<LteUeNetDevice> ueLteNetDev = DynamicCast<LteUeNetDevice> (ueLteDev[i]);
      oss.str ("");
      oss.clear ();
      oss << "node" << i << "@iith.ac.in";
      profile->AddProfile (Identifier (oss.str ().c_str ()), Identifier (Mac48Address::ConvertFrom(ueLteNetDev->GetAddress())), pgwInternetAddr, std::list<Ipv6Address> (), ueLteNetDev->GetImsi ());

      // Support flow mobility in MN.
      Pmipv6MnHelper mnHelper;
      mnHelper.Install (ueNode);
      // Add entries to Flow Binding List
      CreateFlowBindingList (mnHelper.GetFlowBindingList (ueNode), ueNode);
    }

  // Get Flow Interface List from the LMA.
  Ptr<Ipv6> pgwIpv6 = pgw->GetObject<Ipv6> ();
  Ipv6FlowRoutingHelper ipv6FlowRoutingHelper;
  Ptr<Ipv6FlowRouting> ipv6FlowRouting = ipv6FlowRoutingHelper.GetFlowRouting (pgwIpv6);
  Ptr<FlowInterfaceList> flowInterfaceList = ipv6FlowRouting->GetFlowInterfaceList ();
  // Add entries to Flow Interface List.
  CreateFlowInterfaceList (flowInterfaceList);

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
      Config::Connect ("/NodeList/*/$ns3::Ipv6L3Protocol/Rx", MakeCallback (&RxTrace));
      Config::Connect ("/NodeList/*/$ns3::Ipv6L3Protocol/Tx", MakeCallback (&TxTrace));
      Config::Connect ("/NodeList/*/$ns3::Ipv6L3Protocol/Drop", MakeCallback (&DropTrace));
    }

  // Schedule Applications.
  std::vector<Ipv6InterfaceContainer> ueIpIfaceList;
  for (int i = 0; i < noOfUes; i++)
    {
      ueIpIfaceList.push_back (ueIpIface[i]);
    }

  Args args;
  args.ueNodes = ues;
  args.epcHelper = epcHelper;
  args.remoteHost = remoteHost;
  args.ueIpIfaceList = ueIpIfaceList;
  args.interPacketInterval = interPacketInterval;
  args.maxPackets = maxPackets;
  args.remoteHostAddr = remoteHostAddr;
  Simulator::Schedule (Seconds (5), &InstallApplications, args);

  NodeContainer flowMonNodes;
  flowMonNodes.Add (remoteHost);
  flowMonNodes.Add (ues);
  FlowMonitorHelper flowMonHelper;
  Ptr<FlowMonitor> flowMon = flowMonHelper.Install (flowMonNodes);

  // Run simulation
  Simulator::Stop(Seconds(simTime));
  Simulator::Run();
  flowMon->SerializeToXmlStream (std::cout, 2, false, false);
  NS_LOG_UNCOND ("Total Bytes Received: " << recvBytes << " lastTime: " << lastRecvTime);
  NS_LOG_UNCOND ("Throughput: " << recvBytes * 8 / ((lastRecvTime.GetSeconds () - firstSendTime.GetSeconds ()) * 1000 * 1000));

  Simulator::Destroy();
  return 0;
}

