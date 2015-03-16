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

const std::string traceFilename = "flow-mobility-scenario-simple-3";

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("FlowMobilityScenarioSimple3");

long recvBytes = 0;
Time lastRecvTime, firstSendTime;
std::vector<uint32_t> wifiFlowIds;

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

void PrintNodesInfo (NodeContainer nodes)
{
  // Print PGW info
  NS_LOG_UNCOND ("PGW node");
  PrintCompleteNodeInfo (nodes.Get (0));

  // Print SGW info
  NS_LOG_UNCOND ("SGW node");
  PrintCompleteNodeInfo (nodes.Get (1));

  // Print EnB Info
  NS_LOG_UNCOND ("EnB");
  PrintCompleteNodeInfo (nodes.Get (2));

  // Print UE Info
  NS_LOG_UNCOND ("UE");
  PrintCompleteNodeInfo (nodes.Get (3));

  // Print remote host info
  NS_LOG_UNCOND ("Remote host");
  PrintCompleteNodeInfo (nodes.Get (4));

  // Print Wifi Ap Mag info
  for (uint32_t i = 5; i < nodes.GetN(); i++)
    {
      NS_LOG_UNCOND ("Wifi Ap Mag " << i - 5);
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
  recvBytes += packet->GetSize () + 8 + 40;
  lastRecvTime = Simulator::Now ();
}

struct Args
{
  Ptr<Node> ueNode;
  Ptr<PointToPointEpc6Pmipv6Helper> epcHelper;
  Ptr<Node> remoteHost;
  Ipv6InterfaceContainer ueIpIface;
  Ipv6Address remoteHostAddr;
  Time clientRunningTime;
};

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
  uint32_t flowId;

  entry = new FlowInterfaceList::Entry (flowInterfaceList);
  entry->SetPriority (10);
  ts.SetProtocol (17);
  entry->SetTrafficSelector (ts);
  interfaceIds.push_back (8);
  interfaceIds.push_back (4);
  entry->SetInterfaceIds (interfaceIds);
  flowInterfaceList->AddFlowInterfaceEntry (entry);

  entry = new FlowInterfaceList::Entry (flowInterfaceList);
  entry->SetPriority (15);
  ts.SetProtocol (17);
  ts.SetDestinationPort (2000);
  entry->SetTrafficSelector (ts);
  interfaceIds.clear ();
  interfaceIds.push_back (4);
  interfaceIds.push_back (8);
  entry->SetInterfaceIds (interfaceIds);
  flowId = flowInterfaceList->AddFlowInterfaceEntry (entry);
  wifiFlowIds.push_back (flowId);
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
  ts.SetProtocol (17);
  entry->SetTrafficSelector (ts);
  bindingIds.push_back (lteNetDevice);
  bindingIds.push_back (wifiNetDevice);
  entry->SetBindingIds (bindingIds);
  flowBindingList->AddFlowBindingEntry (entry);
}

void PrintFlowStats (Ptr<FlowMonitor> flowMon)
{
  flowMon->SerializeToXmlStream (std::cout, 2, false, false);
}

double CalcDistance (Vector3D p1, Vector3D p2)
{
  return sqrt (pow (p1.x - p2.x, 2) + pow (p1.y - p2.y, 2) + pow (p1.z - p2.z, 2));
}

void InstallUeMobilityModel (Ptr<Node> ue, double v)
{
  Vector3D position[6] = {
      Vector3D (0, 270, 0),
      Vector3D (-210, 180, 0),
      Vector3D (-210, -160, 0),
      Vector3D (0, -250, 0),
      Vector3D (230, -160, 0),
      Vector3D (250, 130, 0)
  };

  MobilityHelper mobHelper;
  mobHelper.SetMobilityModel ("ns3::WaypointMobilityModel");
  mobHelper.Install (ue);

  Ptr<MobilityModel> mobModel = ue->GetObject<MobilityModel> ();
  if (mobModel == 0)
    {
      return;
    }
  Ptr<WaypointMobilityModel> wpMobModel = DynamicCast<WaypointMobilityModel> (mobModel);
  if (wpMobModel == 0)
    return;

  Waypoint wp[6];
  Time t;
  for (int i = 0; i < 6; i++)
    {
      wp[i].position = position[i];
      if (i == 0)
        wp[i].time = Time (Seconds (0));
      else
        t = wp[i].time = Seconds (CalcDistance (position[i], position[(i + 1) % 6]) / v) + t;
      NS_LOG_DEBUG (wp[i]);
      wpMobModel->AddWaypoint (wp[i]);
    }
}

void InstallApplications (Args args, int n)
{
  NS_LOG_UNCOND ("Installing Applications");
  // Install and start applications on UEs and remote host
  uint16_t dlPort = 2000;
  ApplicationContainer clientApps;
  ApplicationContainer serverApps;

  for (int i = 0; i < n; i++, dlPort += 1000)
    {
      PacketSinkHelper dlPacketSinkHelper ("ns3::UdpSocketFactory", Inet6SocketAddress (Ipv6Address::GetAny (), dlPort));
      serverApps.Add (dlPacketSinkHelper.Install (args.ueNode));

      UdpClientHelper dlClient (args.ueIpIface.GetAddress (0, 1), dlPort);
      dlClient.SetAttribute ("Interval", TimeValue (MilliSeconds (1000)));
      dlClient.SetAttribute ("MaxPackets", UintegerValue (0xffff));
      dlClient.SetAttribute ("PacketSize", UintegerValue (100));
      clientApps.Add (dlClient.Install (args.remoteHost));
    }
  Config::Connect ("/NodeList/*/ApplicationList/*/$ns3::PacketSink/Rx", MakeCallback(&PacketSinkRxTrace));
  serverApps.Start (Seconds (0));
  clientApps.Start (Seconds (0));
  clientApps.Stop (args.clientRunningTime);
}

Ipv6InterfaceContainer Assign (const NetDeviceContainer &c, Ipv6Address ipv6Address, Ipv6Prefix prefix)
{
  Ipv6InterfaceContainer retval;

  for (uint32_t i = 0; i < c.GetN (); ++i)
    {
      Ptr<NetDevice> device = c.Get (i);

      Ptr<Node> node = device->GetNode ();
      NS_ASSERT_MSG (node, "Ipv6AddressHelper::Allocate (): Bad node");

      Ptr<Ipv6> ipv6 = node->GetObject<Ipv6> ();
      NS_ASSERT_MSG (ipv6, "Ipv6AddressHelper::Allocate (): Bad ipv6");
      int32_t ifIndex = 0;

      ifIndex = ipv6->GetInterfaceForDevice (device);
      if (ifIndex == -1)
        {
          ifIndex = ipv6->AddInterface (device);
        }
      NS_ASSERT_MSG (ifIndex >= 0, "Ipv6AddressHelper::Allocate (): "
                     "Interface index not found");

      Ipv6InterfaceAddress ipv6Addr = Ipv6InterfaceAddress (ipv6Address, prefix);
      ipv6->SetMetric (ifIndex, 1);
      ipv6->AddAddress (ifIndex, ipv6Addr);
      ipv6->SetUp (ifIndex);

      retval.Add (ipv6, ifIndex);
    }
  return retval;
}

void SwitchFlowstoWifi (Ptr<FlowInterfaceList> flowInterfaceList)
{
  NS_LOG_FUNCTION (flowInterfaceList);
  FlowInterfaceList::Entry *entry;
  std::list<uint32_t> interfaceIds;
  for (uint32_t i = 0; i < wifiFlowIds.size (); i++)
    {
      entry = flowInterfaceList->LookupFlowInterfaceEntry (wifiFlowIds[i]);
      interfaceIds.push_back (4);
      interfaceIds.push_back (8);
      entry->SetInterfaceIds (interfaceIds);
    }
}

void SwitchFlowstoLte (Ptr<FlowInterfaceList> flowInterfaceList)
{
  NS_LOG_FUNCTION (flowInterfaceList);
  FlowInterfaceList::Entry *entry;
  std::list<uint32_t> interfaceIds;
  for (uint32_t i = 0; i < wifiFlowIds.size (); i++)
    {
      entry = flowInterfaceList->LookupFlowInterfaceEntry (wifiFlowIds[i]);
      interfaceIds.push_back (8);
      interfaceIds.push_back (4);
      entry->SetInterfaceIds (interfaceIds);
    }
}

void RetrieveLatestWifiSnr(Ptr<Node> ue, Ptr<FlowInterfaceList> flowInterfaceList)
{
  NS_LOG_FUNCTION (ue << flowInterfaceList);
  Ptr<FlowConnectionManager> flowConnectionManager = ue->GetObject<FlowConnectionManager>();
  FlowConnectionManager::snrTime snrTime = flowConnectionManager->GetWifiLatestSnrTime();
  NS_LOG_INFO (snrTime.time << " " << snrTime.snr);
  if (Simulator::Now () - snrTime.time > Seconds (2))
    {
      // The SNR value is stale and it is assumed that the wifi connection is lost.
      NS_LOG_DEBUG ("Switching flows back to LTE.");
//      SwitchFlowstoLte (flowInterfaceList);
    }
  else if (snrTime.snr > 4)
    {
      // Wifi SNR is greater than 4.
      NS_LOG_DEBUG ("Switching flows onto Wi-Fi.");
//      SwitchFlowstoWifi (flowInterfaceList);
    }
  Simulator::Schedule(Seconds(0.1), &RetrieveLatestWifiSnr, ue, flowInterfaceList);
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

  LogComponentEnable ("FlowMobilityScenarioSimple3", static_cast<LogLevel>(LOG_LEVEL_DEBUG | LOG_PREFIX_TIME));

  std::vector<WifiApPlacement> wifis;
  WifiApPlacement wifiApPlacement;
  wifiApPlacement.wifiPosition = Vector3D (0, -250, 0);
  wifiApPlacement.noOfUes = 10;
  wifis.push_back (wifiApPlacement);
  wifiApPlacement.wifiPosition = Vector3D (230, 110, 0);
  wifiApPlacement.noOfUes = 10;
  wifis.push_back (wifiApPlacement);
  wifiApPlacement.wifiPosition = Vector3D (-190, 140, 0);
  wifiApPlacement.noOfUes = 10;
  wifis.push_back (wifiApPlacement);

  uint16_t noOfWifis = wifis.size ();
  uint16_t noOfUes = 0;
  for (uint16_t i = 0; i < wifis.size (); i++)
    noOfUes += wifis[i].noOfUes;

  uint32_t maxPackets = 2;
  double simTime = 140;
  double interPacketInterval = 1000;
  bool enablePcap = false;
  bool enableTraces = false;
  double v = 10;

  // Command line arguments
  CommandLine cmd;
  cmd.AddValue ("simTime", "Total duration of the simulation [s])", simTime);
  cmd.AddValue ("interPacketInterval", "Inter packet interval [ms])", interPacketInterval);
  cmd.AddValue ("maxPackets", "The maximum number of packets to be sent by the application", maxPackets);
  cmd.AddValue ("velocity", "Velocity in m/s", v);
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
  Ipv6Address wifiIpv6Address = Ipv6Address::MakeAutoconfiguredAddress (magMacAddress, "b0::");
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
      wifiMagApIpIfaces[i] = Assign (wifiApMagDev[i], wifiIpv6Address, Ipv6Prefix (64));

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
  Ptr<Node> ue = CreateObject<Node> ();

  // Set Mobility model on UE.
  InstallUeMobilityModel (ue, v);

  Ptr<NetDevice> ueLteDev;
  Ipv6InterfaceContainer ueIpIface;
  Ptr<NetDevice> ueWifiDev;
  Ipv6InterfaceContainer ueWifiIfs;

  // Install the IP stack on the UEs
  internet.Install (ue);
  ueLteDev = (lteHelper->InstallUeDevice (NodeContainer (ue))).Get (0);
  // enable the UE interfaces to have link-local Ipv6 addresses.
  ueLteDev->SetAddress (Mac48Address::Allocate ());
  ueIpIface = epcHelper->AssignWithoutAddress (ueLteDev);
  // Attach UE to eNB
  // side effect: the default EPS bearer will be activated
  lteHelper->Attach (ueLteDev, enbLteDev, false);
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
  for (uint16_t j = 0; j < 4; j++)
    {
      lteHelper->ActivateDedicatedEpsBearer (ueLteDev, bearer[j], tft[j]);
    }

  // Add Wifi functionality to UE.
  wifiMac.SetType ("ns3::StaWifiMac",
      "Ssid", SsidValue (ssid),
      "ActiveProbing", BooleanValue (false));
  ueWifiDev = (wifi.Install (wifiPhy, wifiMac, ue)).Get (0);

  // Make the mac address of Wifi same as LTE.
  ueWifiDev->SetAddress (ueLteDev->GetAddress ());
  ueWifiIfs = ipv6h.AssignWithoutAddress (NetDeviceContainer (ueWifiDev));

  // Add profile for the UE
  Ptr<LteUeNetDevice> ueLteNetDev = DynamicCast<LteUeNetDevice> (ueLteDev);
  profile->AddProfile (Identifier ("node1@iith.ac.in"), Identifier (Mac48Address::ConvertFrom(ueLteNetDev->GetAddress())), pgwInternetAddr, std::list<Ipv6Address> (), ueLteNetDev->GetImsi ());

  // Support flow mobility in MN.
  Pmipv6MnHelper mnHelper;
  mnHelper.Install (ue);
  // Add entries to Flow Binding List
  CreateFlowBindingList (mnHelper.GetFlowBindingList (ue), ue);

  // Get Flow Interface List from the LMA.
  Ptr<Ipv6> pgwIpv6 = pgw->GetObject<Ipv6> ();
  Ipv6FlowRoutingHelper ipv6FlowRoutingHelper;
  Ptr<Ipv6FlowRouting> ipv6FlowRouting = ipv6FlowRoutingHelper.GetFlowRouting (pgwIpv6);
  Ptr<FlowInterfaceList> flowInterfaceList = ipv6FlowRouting->GetFlowInterfaceList ();
  // Add entries to Flow Interface List.
  CreateFlowInterfaceList (flowInterfaceList);

  // Installing FlowConnectionManager on UE
  // Setup Wifi SNR receiving function every 100 ms.
  FlowConnectionManagerHelper flowConnectionManagerH;
  flowConnectionManagerH.InstallWifi (ue, DynamicCast<WifiNetDevice>(ueWifiDev), 10);
  Ptr<FlowConnectionManager> flowConnectionManager=flowConnectionManagerH.GetFlowConnectionManager(ue);
//  Simulator::Schedule(Seconds(0.1), &RetrieveLatestWifiSnr, ue, flowInterfaceList);

  // Enable PCAP traces
  if (enablePcap)
    {
      p2ph.EnablePcapAll (traceFilename);
      epcHelper->EnablePcap (traceFilename, enbLteDev);
      epcHelper->EnablePcap (traceFilename, ueLteDev);
      wifiPhy.EnablePcap (traceFilename, ueWifiDev);
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
  Args args;
  args.ueNode = ue;
  args.epcHelper = epcHelper;
  args.remoteHost = remoteHost;
  args.ueIpIface = ueIpIface;
  args.remoteHostAddr = remoteHostAddr;
  args.clientRunningTime = Seconds (simTime - 15);
  Simulator::Schedule (Seconds (10), &InstallApplications, args, 2);

  NodeContainer flowMonNodes;
  flowMonNodes.Add (remoteHost);
  flowMonNodes.Add (ue);
  FlowMonitorHelper flowMonHelper;
  Ptr<FlowMonitor> flowMon = flowMonHelper.Install (flowMonNodes);

  // Print Information
  NodeContainer nodes;
  nodes.Add (epcHelper->GetPgwNode ());
  nodes.Add (epcHelper->GetSgwNode ());
  nodes.Add (enbNode);
  nodes.Add (ue);
  nodes.Add (remoteHost);
  for (uint16_t i = 0; i < noOfWifis; i++)
    nodes.Add (wifiApMag[i]);
  PrintNodesInfo (nodes);
  Simulator::Schedule (Seconds (30), &PrintNodesInfo, nodes);
  Simulator::Schedule (Seconds (80), &PrintNodesInfo, nodes);
  Simulator::Schedule (Seconds (135), &PrintNodesInfo, nodes);

  // Run simulation
  Simulator::Stop(Seconds(simTime));
  Simulator::Run();

  flowMon->SerializeToXmlStream (std::cout, 2, false, false);
  NS_LOG_UNCOND ("Total Bytes Received: " << recvBytes << " lastTime: " << lastRecvTime);
  NS_LOG_UNCOND ("Throughput: " << recvBytes * 8 / ((lastRecvTime.GetSeconds () - firstSendTime.GetSeconds ()) * 1000 * 1000));

  Simulator::Destroy();
  return 0;
}

