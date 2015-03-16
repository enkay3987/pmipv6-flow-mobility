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

#include "ns3/lte-helper.h"
#include "ns3/epc-helper.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/lte-module.h"
#include "ns3/applications-module.h"
#include "ns3/point-to-point-helper.h"
#include "ns3/config-store.h"
#include "ns3/flow-monitor-module.h"
#include "random-generator.h"

using namespace ns3;

/**
 * Sample simulation script for LTE+EPC. It instantiates several eNodeB,
 * attaches one UE per eNodeB starts a flow for each UE to  and from a remote host.
 * It also  starts yet another flow between each UE pair.
 */
NS_LOG_COMPONENT_DEFINE ("FlowMobilityIpv41");

struct WifiApPlacement
{
  Vector3D wifiPosition;
  uint16_t noOfUes;
};

struct Args
{
  NodeContainer ueNodes;
  Ptr<PointToPointEpcHelper> epcHelper;
  Ptr<Node> remoteHost;
  std::vector<Ipv4InterfaceContainer> ueIpIfaceList;
  Ipv4Address remoteHostAddr;
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
          NS_LOG_DEBUG ("Application installed on UE " << ue << ", address: " << args.ueIpIfaceList[ue].GetAddress (0));
          UdpClientHelper dlClient (args.ueIpIfaceList[ue].GetAddress (0), dlPort + j * 1000);
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
              dlClient.SetAttribute ("Interval", TimeValue (MilliSeconds(20)));
              dlClient.SetAttribute ("MaxPackets", UintegerValue (0xffffffff));
              dlClient.SetAttribute ("PacketSize", UintegerValue (970));
              break;
            }
          case 2:   // conversational video
            {
              dlClient.SetAttribute ("Interval", TimeValue (MilliSeconds(25)));
              dlClient.SetAttribute ("MaxPackets", UintegerValue (0xffffffff));
              dlClient.SetAttribute ("PacketSize", UintegerValue (970));
              break;
            }
          case 3:   // non-conversational video
            {
              dlClient.SetAttribute ("Interval", TimeValue (MilliSeconds(20)));
              dlClient.SetAttribute ("MaxPackets", UintegerValue (0xffffffff));
              dlClient.SetAttribute ("PacketSize", UintegerValue (970));
              break;
            }
          }
          clientApps.Add (dlClient.Install (args.remoteHost));
        }
    }
  clientApps.Start (Seconds (0));
  clientApps.Stop (args.clientRunningTime);
}

void InstallApplications2 (Args args, std::vector<uint32_t> noOfStopFlows, Time stopFlowsRunningTime)
{
  NS_LOG_UNCOND ("Installing Applications 2");
  // Install and start applications on UEs and remote host
//  uint16_t dlPort = 2000;
//  ApplicationContainer clientApps1, clientApps2;
//  RandomGenerator rGen (args.ueNodes.GetN ());
//  RandomGenerator::SetSeed ();
//  for (uint32_t j = 0; j < args.noOfFlows.size (); j++)
//    {
//      rGen.Reset ();
//      NS_LOG_DEBUG ("Flow type: " << j);
//      for (uint32_t i = 0, k = 0; i < args.noOfFlows[j]; i++)
//        {
//          uint32_t ue = rGen.GetNext ();
//          NS_LOG_DEBUG ("Application installed on UE " << ue << ", address: " << args.ueIpIfaceList[ue].GetAddress (0));
//          UdpClientHelper dlClient (args.ueIpIfaceList[ue].GetAddress (0), dlPort + j * 1000);
//          switch (j)
//          {
//          case 0:   // voice
//            {
//              dlClient.SetAttribute ("Interval", TimeValue (MilliSeconds(20)));
//              dlClient.SetAttribute ("MaxPackets", UintegerValue (0xffffffff));
//              dlClient.SetAttribute ("PacketSize", UintegerValue (160));
//              break;
//            }
//          case 1:   // web
//            {
//              dlClient.SetAttribute ("Interval", TimeValue (MilliSeconds(2)));
//              dlClient.SetAttribute ("MaxPackets", UintegerValue (0xffffffff));
//              dlClient.SetAttribute ("PacketSize", UintegerValue (970));
//              break;
//            }
//          case 2:   // conversational video
//            {
//              dlClient.SetAttribute ("Interval", TimeValue (MilliSeconds(2.5)));
//              dlClient.SetAttribute ("MaxPackets", UintegerValue (0xffffffff));
//              dlClient.SetAttribute ("PacketSize", UintegerValue (970));
//              break;
//            }
//          case 3:   // non-conversational video
//            {
//              dlClient.SetAttribute ("Interval", TimeValue (MilliSeconds(2)));
//              dlClient.SetAttribute ("MaxPackets", UintegerValue (0xffffffff));
//              dlClient.SetAttribute ("PacketSize", UintegerValue (970));
//              break;
//            }
//          }
//          if (k < noOfStopFlows[j])
//            {
//              clientApps2.Add (dlClient.Install (args.remoteHost));
//              k++;
//            }
//          else
//            clientApps1.Add (dlClient.Install (args.remoteHost));
//        }
//    }
//
//  Config::Connect ("/NodeList/*/ApplicationList/*/$ns3::PacketSink/Rx", MakeCallback(&PacketSinkRxTrace));
//  clientApps1.Start (Seconds (0));
//  clientApps2.Start (Seconds (0));
//  clientApps1.Stop (args.clientRunningTime);
//  clientApps2.Stop (stopFlowsRunningTime);
}

void InstallServerApplications (NodeContainer nodes)
{
  ApplicationContainer serverApps;
  for (uint32_t j = 0, port = 2000; j < 4; j++, port += 1000)
    {
      for (uint32_t i = 0; i < nodes.GetN (); i++)
        {
          PacketSinkHelper dlPacketSinkHelper ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), port));
          serverApps.Add (dlPacketSinkHelper.Install (nodes.Get (i)));
        }
    }
  serverApps.Start (Seconds (0));
}

void printStats (FlowMonitorHelper &flowmon_helper, bool perFlowInfo) {
  Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(flowmon_helper.GetClassifier());
  std::string proto;
  Ptr<FlowMonitor> monitor = flowmon_helper.GetMonitor ();
  std::map < FlowId, FlowMonitor::FlowStats > stats = monitor->GetFlowStats();
  double totalTimeReceiving, totalBytesReceived;
  uint64_t totalPacketsReceived, totalPacketsDropped;

  totalBytesReceived = 0, totalPacketsDropped = 0, totalPacketsReceived = 0, totalTimeReceiving = 0;
  for (std::map< FlowId, FlowMonitor::FlowStats>::iterator flow = stats.begin(); flow != stats.end(); flow++)
    {
      Ipv4FlowClassifier::FiveTuple  t = classifier->FindFlow(flow->first);
      switch(t.protocol)
      {
      case(6):
         proto = "TCP";
         break;
      case(17):
         proto = "UDP";
         break;
      default:
         exit(1);
      }
     totalBytesReceived += (double) flow->second.rxBytes;
     totalTimeReceiving += flow->second.timeLastRxPacket.GetSeconds () - flow->second.timeFirstTxPacket.GetSeconds ();
     totalPacketsReceived += flow->second.rxPackets;
     totalPacketsDropped += flow->second.txPackets - flow->second.rxPackets;
     if (perFlowInfo)
       {
         std::cout << "FlowID: " << flow->first << " (" << proto << " "
                   << t.sourceAddress << " / " << t.sourcePort << " --> "
                   << t.destinationAddress << " / " << t.destinationPort << ")" << std::endl;
         std::cout << "  Tx Bytes: " << flow->second.txBytes << std::endl;
         std::cout << "  Rx Bytes: " << flow->second.rxBytes << std::endl;
         std::cout << "  Tx Packets: " << flow->second.txPackets << std::endl;
         std::cout << "  Rx Packets: " << flow->second.rxPackets << std::endl;
         std::cout << "  Lost Packets:   " << flow->second.lostPackets << "\n";
         std::cout << "  Drop Packets:   " << flow->second.packetsDropped.size() << "\n";
         std::cout << "  Time LastRxPacket: " << flow->second.timeLastRxPacket.GetSeconds () << "s" << std::endl;
         std::cout << "  Lost Packets: " << flow->second.lostPackets << std::endl;
         std::cout << "  Pkt Lost Ratio: " << ((double)flow->second.txPackets-(double)flow->second.rxPackets)/(double)flow->second.txPackets << std::endl;
         std::cout << "  Throughput: " << ( ((double)flow->second.rxBytes*8) / (flow->second.timeLastRxPacket.GetSeconds ()) ) << "bps" << std::endl;
         std::cout << "  Mean{Delay}: " << (flow->second.delaySum.GetSeconds()/flow->second.rxPackets) << std::endl;
         std::cout << "  Mean{Jitter}: " << (flow->second.jitterSum.GetSeconds()/(flow->second.rxPackets)) << std::endl;
       }
    }
  std::cout << "  Total Bytes Received :" << totalBytesReceived << std::endl;
  std::cout << "  Total Time Taken :" << totalTimeReceiving << std::endl;
  std::cout << "Average Throughput :" << std::fixed << std::showpoint << totalBytesReceived * 8 / totalTimeReceiving << "bps" << std::endl;
}


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

  uint16_t noOfWifis = wifis.size ();
  uint16_t noOfUes = 0;
  for (uint16_t i = 0; i < wifis.size (); i++)
    noOfUes += wifis[i].noOfUes;

  double simTime = 30;
  bool enablePcap = false;
  bool enableTraces = false;

  // Command line arguments
  CommandLine cmd;
  cmd.AddValue ("simTime", "Total duration of the simulation [s])", simTime);
  cmd.AddValue ("enablePcap", "Set true to enable pcap capture", enablePcap);
  cmd.AddValue ("enableTraces", "Set true to enable ip traces", enableTraces);
  cmd.Parse(argc, argv);

  Ptr<LteHelper> lteHelper = CreateObject<LteHelper> ();
  Ptr<PointToPointEpcHelper>  epcHelper = CreateObject<PointToPointEpcHelper> ();
  epcHelper->SetAttribute ("S1uLinkDelay", TimeValue (MilliSeconds (10)));
  lteHelper->SetEpcHelper (epcHelper);

  // parse again so you can override default values from the command line
  cmd.Parse(argc, argv);

  Ptr<Node> pgw = epcHelper->GetPgwNode ();

   // Create a single RemoteHost
  NodeContainer remoteHostContainer;
  remoteHostContainer.Create (1);
  Ptr<Node> remoteHost = remoteHostContainer.Get (0);
  InternetStackHelper internet;
  internet.Install (remoteHostContainer);

  // Create the Internet
  PointToPointHelper p2ph;
  p2ph.SetDeviceAttribute ("DataRate", DataRateValue (DataRate ("100Gb/s")));
  p2ph.SetDeviceAttribute ("Mtu", UintegerValue (1500));
  p2ph.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (300)));
  NetDeviceContainer internetDevices = p2ph.Install (pgw, remoteHost);
  Ipv4AddressHelper ipv4h;
  ipv4h.SetBase ("1.0.0.0", "255.0.0.0");
  Ipv4InterfaceContainer internetIpIfaces = ipv4h.Assign (internetDevices);
  // interface 0 is localhost, 1 is the p2p device
  Ipv4Address remoteHostAddr = internetIpIfaces.GetAddress (1);

  Ipv4StaticRoutingHelper ipv4RoutingHelper;
  Ptr<Ipv4StaticRouting> remoteHostStaticRouting = ipv4RoutingHelper.GetStaticRouting (remoteHost->GetObject<Ipv4> ());
  remoteHostStaticRouting->AddNetworkRouteTo (Ipv4Address ("7.0.0.0"), Ipv4Mask ("255.0.0.0"), 1);

  // Create eNB node and install mobility model
  Ptr<Node> enbNode = CreateObject<Node> ();

  // Install Mobility Model for eNB.
  MobilityHelper mobility;
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  Ptr<ListPositionAllocator> enbPositionAlloc = CreateObject<ListPositionAllocator> ();
  enbPositionAlloc->Add (Vector (0, 0, 0));
  mobility.SetPositionAllocator (enbPositionAlloc);
  mobility.Install (enbNode);

  // Install LTE Device to eNB node
  Ptr<NetDevice> enbLteDev = (lteHelper->InstallEnbDevice (NodeContainer (enbNode))).Get (0);

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
  Ipv4InterfaceContainer ueIpIface[noOfUes];
  for (uint16_t i = 0; i < noOfUes; i++)
    {
      Ptr<Node> ueNode = ues.Get (i);
      // Install the IP stack on the UEs
      internet.Install (ueNode);
      ueLteDev[i] = (lteHelper->InstallUeDevice (NodeContainer (ueNode))).Get (0);
      ueIpIface[i] = epcHelper->AssignUeIpv4Address (NetDeviceContainer (ueLteDev[i]));
      // Attach UE to eNB
      // side effect: the default EPS bearer will be activated
      lteHelper->Attach (ueLteDev[i], enbLteDev);
    }

  // Install Server applications on client.
  InstallServerApplications (ues);

  // Schedule Applications.
  std::vector<Ipv4InterfaceContainer> ueIpIfaceList;
  for (int i = 0; i < noOfUes; i++)
    {
      ueIpIfaceList.push_back (ueIpIface[i]);
    }

  Args args1, args2;
  args1.ueNodes = args2.ueNodes= ues;
  args1.epcHelper = args2.epcHelper = epcHelper;
  args1.remoteHost = args2.remoteHost = remoteHost;
  args1.ueIpIfaceList = args2.ueIpIfaceList = ueIpIfaceList;
  args1.remoteHostAddr = args2.remoteHostAddr = remoteHostAddr;
  args1.noOfFlows.push_back (50);
  args1.noOfFlows.push_back (2);
  args1.noOfFlows.push_back (15);
  args1.noOfFlows.push_back (6);
  args1.clientRunningTime = Seconds (60);
  Simulator::Schedule (Seconds (10), &InstallApplications1, args1);

  args2.noOfFlows.push_back (0);
  args2.noOfFlows.push_back (30);
  args2.noOfFlows.push_back (20);
  args2.noOfFlows.push_back (30);
  args2.clientRunningTime = Seconds (40);
  std::vector<uint32_t> noOfStopFlows;
  noOfStopFlows.push_back (0);
  noOfStopFlows.push_back (16);
  noOfStopFlows.push_back (5);
  noOfStopFlows.push_back (14);
  Time stopFlowsRunningTime = Seconds (20);
  Simulator::Schedule (Seconds (30), &InstallApplications2, args2, noOfStopFlows, stopFlowsRunningTime);

  FlowMonitorHelper flowMonitorHelper;
  NodeContainer flowMonitorNodes;
  flowMonitorNodes.Add (ues);
  flowMonitorNodes.Add (remoteHost);
  Ptr<FlowMonitor> flowMonitor = flowMonitorHelper.Install (flowMonitorNodes);

  Simulator::Stop(Seconds(simTime));
  Simulator::Run();
  flowMonitor->SerializeToXmlStream (std::cout, 2, false, true);
  printStats (flowMonitorHelper, false);
  Simulator::Destroy();
  return 0;

}

