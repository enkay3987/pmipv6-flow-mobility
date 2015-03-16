#ifndef FLOW_MONITOR_PRINT_H
#define FLOW_MONITOR_PRINT_H
#include "ns3/flow-monitor-module.h"

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
     if(t.destinationPort==5000){
     totalBytesReceived += (double) flow->second.rxBytes * 8;
     totalTimeReceiving += flow->second.timeLastRxPacket.GetSeconds ();
     totalPacketsReceived += flow->second.rxPackets;
     totalPacketsDropped += flow->second.txPackets - flow->second.rxPackets;
     }
     if (perFlowInfo) {
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
    std::cout << "-------------------Final Output-------------------------------" << std::endl;
    //~ std::cout << "  Total Bytes Received :" << totalBytesReceived << std::endl;
    //~ std::cout << "  Total Time Taken :" << totalTimeReceiving << std::endl;
    std::cout << "Average Throughput :" << std::fixed << std::showpoint << std::setprecision(2) << totalBytesReceived / totalTimeReceiving << "bps" << std::endl;
    std::cout  << "MacTxDrop Count :" << MacTxDropCount << "\nPhyTxDrop Count :" << PhyTxDropCount << "\nPhyRxDrop Count :" << PhyRxDropCount << "\n";
    std::cout <<"FrameDrop Count :" << DropCount << "\nCollision Count :" << CollisionCount << "\nDrop Count :" << MacRxDropCount + MacTxDropCount << "\n";
}
#endif
