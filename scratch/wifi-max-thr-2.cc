/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2005,2006,2007 INRIA
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
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */


#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/mobility-module.h"
#include "ns3/config-store-module.h"
#include "ns3/wifi-module.h"
#include "ns3/internet-module.h"
#include "ns3/flow-monitor-module.h"

#include <iostream>

using namespace ns3;

long recvBytes = 0;
Time lastRecvTime, firstSendTime;

void PacketSinkRxTrace (std::string context, Ptr<const Packet> packet, const Address &address)
{
  recvBytes += packet->GetSize () + 8 + 20;
  lastRecvTime = Simulator::Now ();
}

int main (int argc, char *argv[])
{
  bool activeProbing;
  bool enableTrace;
  bool enablePcap;
  uint32_t mns; // Number of MNs
  uint32_t maxPacketCount;
  uint32_t interPacketInterval;
  double simTime;

  // Default values
  activeProbing = false; // Connection gets established only after getting beacons.
  enableTrace = enablePcap = false; // Tracing and pcap is disabled.
  maxPacketCount = 0xffffffff;  // There are infinite packets to send out.
  interPacketInterval = 100; // A packet is generated every 100 milliseconds.
  mns = 1;
  simTime = 50; // Simulation time (in secs).

  CommandLine cmd;
  cmd.AddValue ("simTime", "Simulation time (in secs)", simTime);
  cmd.AddValue ("activeProbing", "Whether MN uses active probing", activeProbing);
  cmd.AddValue ("enableTrace", "Whether trace file be generated", enableTrace);
  cmd.AddValue ("enablePcap", "Whether pcap files be generated", enablePcap);
  cmd.AddValue ("maxPackets", "The maximum packets UDP client sends out", maxPacketCount);
  cmd.AddValue ("interPacketInterval", "UDP inter packet interval (in milliseconds)", interPacketInterval);
  cmd.AddValue ("mns", "The number of MNs", mns);
  cmd.Parse(argc, argv);

  // enable rts cts all the time.
  Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue ("0"));
  // disable fragmentation
  Config::SetDefault ("ns3::WifiRemoteStationManager::FragmentationThreshold", StringValue ("2200"));

  MobilityHelper mobility;
  NodeContainer stas;
  NodeContainer ap;
  NetDeviceContainer staDevs;
  NetDeviceContainer apDevs;

  stas.Create (mns);
  ap.Create (1);

  InternetStackHelper internet;
  internet.Install (stas);
  internet.Install (ap);

  Ipv6AddressHelper ipv6AH;
  Ssid ssid = Ssid("PMIP");
  YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default ();
  wifiPhy.SetPcapDataLinkType (YansWifiPhyHelper::DLT_IEEE802_11_RADIO);
  WifiHelper wifi = WifiHelper::Default ();
  wifi.SetStandard (WIFI_PHY_STANDARD_80211a);
  wifi.SetRemoteStationManager ("ns3::AarfWifiManager");
  NqosWifiMacHelper wifiMac = NqosWifiMacHelper::Default ();
  YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default ();
  wifiPhy.SetChannel (wifiChannel.Create ());
  // setup ap.
  wifiMac.SetType ("ns3::ApWifiMac",
               "Ssid", SsidValue (ssid),
               "BeaconGeneration", BooleanValue (true),
               "BeaconInterval", TimeValue (MilliSeconds(100)),
               "EnableBeaconJitter", BooleanValue (true));
  apDevs = wifi.Install (wifiPhy, wifiMac, ap);
  ipv6AH.SetBase("c0::", 64);
  ipv6AH.Assign (apDevs);

  // setup stas.
  wifiMac.SetType ("ns3::StaWifiMac",
                    "Ssid", SsidValue (ssid),
                    "ActiveProbing", BooleanValue (activeProbing));
  staDevs = wifi.Install (wifiPhy, wifiMac, stas);
  ipv6AH.Assign (staDevs);

  // Installing mobility models
  // Assign fixed positions to APs
  Ptr<ListPositionAllocator> listPosAlloc = CreateObject<ListPositionAllocator> ();
  listPosAlloc->Add (Vector (0, 0, 0));
  mobility.SetPositionAllocator (listPosAlloc);
  mobility.Install (ap);
  // The MNs are allocated random positions in a circle with centre (0, 0) and radius 220.
  // They move randomly within the circle boundary.
  Ptr<RandomDiscPositionAllocator> randomDiscPosAlloc = CreateObject<RandomDiscPositionAllocator> ();
  randomDiscPosAlloc->SetAttribute ("Rho", StringValue ("ns3::UniformRandomVariable[Min=0|Max=20]"));
  mobility.SetPositionAllocator (randomDiscPosAlloc);
  mobility.Install (stas);

  Ipv6Address staAddress[stas.GetN ()];
  for (uint32_t i = 0; i < stas.GetN (); i++)
    {
      Ptr<Ipv6> ipv6;
      ipv6 = stas.Get (i)->GetObject<Ipv6> ();
      staAddress[i] = ipv6->GetAddress(1, 1).GetAddress ();
    }

  const uint16_t port = 6000;
  ApplicationContainer serverApps, clientApps;
  PacketSinkHelper sink ("ns3::UdpSocketFactory",
                           Inet6SocketAddress (Ipv6Address::GetAny (), port));
  serverApps = sink.Install (stas);
  for (uint32_t i = 0; i < stas.GetN (); i++)
    {
      UdpClientHelper udpClient(staAddress[i], port);
      udpClient.SetAttribute ("Interval", TimeValue (MilliSeconds (interPacketInterval)));
      udpClient.SetAttribute ("PacketSize", UintegerValue (1000));
      udpClient.SetAttribute ("MaxPackets", UintegerValue (maxPacketCount));
      ApplicationContainer app = udpClient.Install (ap.Get (0));
      clientApps.Add (app);
    }
  Config::Connect ("/NodeList/*/ApplicationList/*/$ns3::PacketSink/Rx", MakeCallback(&PacketSinkRxTrace));
  serverApps.Start (Seconds (0.0));
  serverApps.Stop (Seconds (simTime));
  clientApps.Start (Seconds (1.0));
  clientApps.Stop (Seconds (simTime - 2 < 0 ? simTime : simTime - 2));
  firstSendTime = Seconds (1.0);

  NodeContainer flowMonNodes;
  flowMonNodes.Add (ap);
  flowMonNodes.Add (stas);
  FlowMonitorHelper flowMonHelper;
  Ptr<FlowMonitor> flowMon = flowMonHelper.Install (flowMonNodes);

  Simulator::Stop (Seconds (simTime));
  Simulator::Run ();
  flowMon->SerializeToXmlStream(std::cout, 2, false, false);
  std::cout << "Total Bytes Received: " << recvBytes << " lastTime: " << lastRecvTime << std::endl;
  std::cout << "Throughput: " << recvBytes * 8 / ((lastRecvTime.GetSeconds () - firstSendTime.GetSeconds ()) * 1000 * 1000) << std::endl;
  Simulator::Destroy ();

  return 0;
}
