/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2011 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
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
 * Author: Jaume Nin <jaume.nin@cttc.cat>
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

const std::string traceFilename = "pmipv6-lte-wifi-2";

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("Pmipv6LteWifi3");

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
  NS_LOG_UNCOND (context << " " << seqTs.GetTs () << "->" << Simulator::Now() << ": " << seqTs.GetSeq());
}

struct Args
{
  Ptr<Node> ueNode;
  Ptr<PointToPointEpc6Pmipv6Helper> epcHelper;
  Ptr<Node> remoteHost;
  Ipv6InterfaceContainer ueIpIface;
  double interPacketInterval;
  uint32_t maxPackets;
  Ipv6Address remoteHostAddr;
};

void InstallApplications (Args args)
{
  NS_LOG_UNCOND ("Installing Applications");
  // Install and start applications on UEs and remote host
  uint16_t dlPort = 1234;
  uint16_t ulPort = 2000;
  ApplicationContainer clientApps;
  ApplicationContainer serverApps;

  PacketSinkHelper dlPacketSinkHelper ("ns3::UdpSocketFactory", Inet6SocketAddress (Ipv6Address::GetAny (), dlPort));
  PacketSinkHelper ulPacketSinkHelper ("ns3::UdpSocketFactory", Inet6SocketAddress (Ipv6Address::GetAny (), ulPort));
  serverApps.Add (dlPacketSinkHelper.Install (args.ueNode));
  serverApps.Add (ulPacketSinkHelper.Install (args.remoteHost));

  UdpClientHelper dlClient (args.ueIpIface.GetAddress (0, 1), dlPort);
  dlClient.SetAttribute ("Interval", TimeValue (MilliSeconds(args.interPacketInterval)));
  dlClient.SetAttribute ("MaxPackets", UintegerValue (args.maxPackets));
  dlClient.SetAttribute ("PacketSize", UintegerValue (100));
  UdpClientHelper ulClient (args.remoteHostAddr, ulPort);
  ulClient.SetAttribute ("Interval", TimeValue (MilliSeconds(args.interPacketInterval)));
  ulClient.SetAttribute ("MaxPackets", UintegerValue(args.maxPackets));
  ulClient.SetAttribute ("PacketSize", UintegerValue (100));

  clientApps.Add (dlClient.Install (args.remoteHost));
  clientApps.Add (ulClient.Install (args.ueNode));
  Config::Connect ("/NodeList/*/ApplicationList/*/$ns3::PacketSink/Rx", MakeCallback(&PacketSinkRxTrace));
  serverApps.Start (Seconds (1));
  clientApps.Start (Seconds (1));
}

void PrintNodesInfo (Ptr<PointToPointEpc6Pmipv6Helper> epcHelper, NodeContainer nodes)
{
  // Print PGW info
  NS_LOG_UNCOND ("PGW node");
  PrintCompleteNodeInfo (epcHelper->GetPgwNode ());

  // Print SGW info
  NS_LOG_UNCOND ("SGW node");
  PrintCompleteNodeInfo (epcHelper->GetSgwNode ());

  // Print EnB Info
  NS_LOG_UNCOND ("EnB");
  PrintCompleteNodeInfo (nodes.Get (0));

  // Print UE Info
  NS_LOG_UNCOND ("UE");
  PrintCompleteNodeInfo (nodes.Get (1));

  // Print remote host info
  NS_LOG_UNCOND ("Remote host");
  PrintCompleteNodeInfo (nodes.Get (2));

  // Print Wifi Ap Mag info
  NS_LOG_UNCOND ("Wifi Ap Mag");
  PrintCompleteNodeInfo (nodes.Get (3));
}

void StopDad (Ptr<Node> node)
{
  Ptr<Icmpv6L4Protocol> icmpv6L4Protocol = node->GetObject<Icmpv6L4Protocol> ();
  NS_ASSERT (icmpv6L4Protocol != 0);
  icmpv6L4Protocol->SetAttribute ("DAD", BooleanValue (false));
}

/**
 * A MN having both LTE and Wifi NetDevices installed on it moves from a point where it is only connected
 * to the LTE network to where it senses a Wifi network and initiates a vertical handover over to the Wifi
 * network. The MAG functionality is installed on the same node as the WifiAp.
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
  uint32_t maxPackets = 0xff;
  double simTime = 30;
  double interPacketInterval = 1000;
  bool enablePcap = false;

  // Command line arguments
  CommandLine cmd;
  cmd.AddValue ("simTime", "Total duration of the simulation [s])", simTime);
  cmd.AddValue ("interPacketInterval", "Inter packet interval [ms])", interPacketInterval);
  cmd.AddValue ("maxPackets", "The maximum number of packets to be sent by the application", maxPackets);
  cmd.AddValue ("enablePcap", "Set true to enable pcap capture", enablePcap);
  cmd.Parse(argc, argv);

  Mac48Address magMacAddress ("00:00:aa:bb:cc:dd");
  Ptr<LteHelper> lteHelper = CreateObject<LteHelper> ();
  Ptr<PointToPointEpc6Pmipv6Helper> epcHelper = CreateObject<PointToPointEpc6Pmipv6Helper> (magMacAddress);
  lteHelper->SetEpcHelper (epcHelper);
  epcHelper->SetupS5Interface ();
  Ptr<Node> pgw = epcHelper->GetPgwNode ();

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

  // Create eNB and UE nodes.
  Ptr<Node> ueNode = CreateObject<Node> ();
  Ptr<Node> enbNode = CreateObject<Node> ();

  // Install Mobility Model for UE and eNB.
  MobilityHelper mobility;
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  Ptr<ListPositionAllocator> enbPositionAlloc = CreateObject<ListPositionAllocator> ();
  enbPositionAlloc->Add (Vector(0, 0, 0));
  mobility.SetPositionAllocator (enbPositionAlloc);
  mobility.Install (enbNode);
  Ptr<ListPositionAllocator> uePositionAlloc = CreateObject<ListPositionAllocator> ();
  uePositionAlloc->Add (Vector (-200, 20, 0));
  mobility.SetPositionAllocator (uePositionAlloc);
  mobility.SetMobilityModel ("ns3::ConstantVelocityMobilityModel");
  mobility.Install (ueNode);
  Ptr<ConstantVelocityMobilityModel> cvm = ueNode->GetObject<ConstantVelocityMobilityModel> ();
  cvm->SetVelocity (Vector (7.0, 0, 0)); //move to left to right 7.0m/s

  // Install LTE Devices to the UE and eNB nodes
  Ptr<NetDevice> enbLteDev = (lteHelper->InstallEnbDevice (NodeContainer (enbNode))).Get (0);
  Ptr<NetDevice> ueLteDev = (lteHelper->InstallUeDevice (NodeContainer (ueNode))).Get (0);

  // Install the IP stack on the UEs and enable the interfaces to have link-local Ipv6 addresses.
  internet.Install (ueNode);
  ueLteDev->SetAddress (Mac48Address::Allocate ());
  Ipv6InterfaceContainer ueIpIface;
  ueIpIface = epcHelper->AssignWithoutAddress (ueLteDev);

  // Attach UE to eNB
  // side effect: the default EPS bearer will be activated
  lteHelper->Attach (ueLteDev, enbLteDev, false);

  // Setup Wifi network
  Ptr<Node> wifiApMag;
  wifiApMag = CreateObject<Node> ();
  internet.Install (wifiApMag);
  // Enable forwarding on the WifiApMag
  Ptr<Ipv6> ipv6 = wifiApMag->GetObject<Ipv6> ();
  ipv6->SetAttribute ("IpForward", BooleanValue (true));

  // Create p2p network between wifiApMag and and LMA (P-GW).
  p2ph.SetDeviceAttribute ("DataRate", DataRateValue (DataRate ("100Gb/s")));
  p2ph.SetDeviceAttribute ("Mtu", UintegerValue (1500));
  p2ph.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (100)));
  NetDeviceContainer wifiApMagLmaDevs = p2ph.Install (wifiApMag, pgw);
  ipv6h.SetBase ("aa::", "64");
  Ipv6InterfaceContainer wifiApMagLmaIpIfaces = ipv6h.Assign (wifiApMagLmaDevs);
  Ptr<Ipv6StaticRouting> wifiApMagStaticRouting = ipv6RoutingHelper.GetStaticRouting (wifiApMag->GetObject<Ipv6> ());
  wifiApMagStaticRouting->SetDefaultRoute (wifiApMagLmaIpIfaces.GetAddress (1, 1), wifiApMagLmaIpIfaces.GetInterfaceIndex (0));

  // Attach Wifi AP functionality on the wifiApMag node.
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
  Ptr<NetDevice> wifiApMagDev = (wifi.Install (wifiPhy, wifiMac, wifiApMag)).Get (0);
  NetDeviceContainer wifiApMagDevs;
  wifiApMagDevs.Add (wifiApMagDev);
  wifiApMagDev->SetAddress (magMacAddress);
  ipv6h.SetBase (Ipv6Address ("b0::"), Ipv6Prefix (64));
  Ipv6InterfaceContainer wifiMagApIpIfaces = ipv6h.Assign (wifiApMagDevs);

  // Install mobility model on AP.
  Ptr<ListPositionAllocator> wifiApMagPositionAlloc = CreateObject<ListPositionAllocator> ();
  wifiApMagPositionAlloc->Add (Vector(20, 0, 0));
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.SetPositionAllocator (wifiApMagPositionAlloc);
  mobility.Install (wifiApMag);

  // Add Wifi functionality to UE.
  wifiMac.SetType ("ns3::StaWifiMac",
                   "Ssid", SsidValue (ssid),
                   "ActiveProbing", BooleanValue (false));
  NS_LOG_UNCOND ("Installing Wifi device on UE");
  Ptr<NetDevice> ueWifiDev = (wifi.Install (wifiPhy, wifiMac, ueNode)).Get (0);
  // Make the mac address of Wifi same as LTE.
  ueWifiDev->SetAddress (ueLteDev->GetAddress ());
  Ipv6InterfaceContainer ueWifiIfs = ipv6h.AssignWithoutAddress (NetDeviceContainer (ueWifiDev));

  // Add Wifi Mag functionality to WifiMag node.
  Pmipv6MagHelper magHelper;
  magHelper.SetProfileHelper (epcHelper->GetPmipv6ProfileHelper ());
  magHelper.Install (wifiApMag);

  // Add PMIPv6 profiles.
  Ptr<Pmipv6ProfileHelper> profile = epcHelper->GetPmipv6ProfileHelper ();

  // Add profile for the UE
  Ptr<LteUeNetDevice> ueLteNetDev = DynamicCast<LteUeNetDevice> (ueLteDev);
  profile->AddProfile (Identifier ("node1@iith.ac.in"), Identifier (Mac48Address::ConvertFrom(ueLteNetDev->GetAddress())), pgwInternetAddr, std::list<Ipv6Address> (), ueLteNetDev->GetImsi ());

  // Enable PCAP traces
  if (enablePcap)
    {
      p2ph.EnablePcapAll (traceFilename);
      epcHelper->EnablePcap (traceFilename, ueLteDev);
      epcHelper->EnablePcap (traceFilename, enbLteDev);
      wifiPhy.EnablePcap (traceFilename, wifiApMagDev);
      wifiPhy.EnablePcap (traceFilename, ueWifiDev);
    }

  // Add IP traces to all nodes.
  Config::Connect ("/NodeList/*/$ns3::Ipv6L3Protocol/Rx", MakeCallback (&RxTrace));
  Config::Connect ("/NodeList/*/$ns3::Ipv6L3Protocol/Tx", MakeCallback (&TxTrace));
  Config::Connect ("/NodeList/*/$ns3::Ipv6L3Protocol/Drop", MakeCallback (&DropTrace));

  // Schedule Applications.
  Args args;
  args.ueNode = ueNode;
  args.epcHelper = epcHelper;
  args.remoteHost = remoteHost;
  args.ueIpIface = ueIpIface;
  args.interPacketInterval = interPacketInterval;
  args.maxPackets = maxPackets;
  args.remoteHostAddr = remoteHostAddr;
  Simulator::Schedule (Seconds (5), &InstallApplications, args);

  // Print Information
  NodeContainer nodes;
  nodes.Add (enbNode);
  nodes.Add (ueNode);
  nodes.Add (remoteHost);
  nodes.Add (wifiApMag);
  PrintNodesInfo (epcHelper, nodes);

  // Run simulation
  Simulator::Stop(Seconds(simTime));
  Simulator::Run();
  Simulator::Destroy();
  return 0;
}

