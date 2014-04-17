/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2011-2013 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
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
 * Author: Jaume Nin <jnin@cttc.es>
 *         Nicola Baldo <nbaldo@cttc.es>
 *         Manuel Requena <manuel.requena@cttc.es>
 */

#include "point-to-point-epc6-pmipv6-helper.h"
#include <ns3/log.h>
#include <ns3/inet-socket-address.h>
#include <ns3/mac48-address.h>
#include <ns3/eps-bearer.h>
#include <ns3/ipv6-address.h>
#include <ns3/internet-stack-helper.h>
#include <ns3/point-to-point-helper.h>
#include <ns3/packet-socket-helper.h>
#include <ns3/packet-socket-address.h>
#include <ns3/epc-enb-application.h>
#include "ns3/epc6-sgw-application.h"

#include <ns3/lte-enb-rrc.h>
#include <ns3/epc-x2.h>
#include <ns3/lte-enb-net-device.h>
#include <ns3/lte-ue-net-device.h>
#include <ns3/epc-mme.h>
#include <ns3/epc-ue-nas.h>
#include "ns3/boolean.h"
#include "ns3/config.h"
#include "ns3/icmpv6-l4-protocol.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("PointToPointEpc6Pmipv6Helper");

NS_OBJECT_ENSURE_REGISTERED (PointToPointEpc6Pmipv6Helper);

PointToPointEpc6Pmipv6Helper::PointToPointEpc6Pmipv6Helper ()
  : m_gtpuUdpPort (2152)  // fixed by the standard
{
  NS_LOG_FUNCTION (this);

  m_s1uIpv6AddressHelper.SetBase ("a0::", 64);

  m_x2Ipv6AddressHelper.SetBase ("a1::", 64);

  m_ueAddressHelper.SetBase ("b0::", 64);
  
  // create SgwNode
  m_sgw = CreateObject<Node> ();
  InternetStackHelper internet;
  internet.Install (m_sgw);
  
  Ptr<Ipv6> sgwIpv6 = m_sgw->GetObject<Ipv6> ();
  sgwIpv6->SetAttribute ("SendIcmpv6Redirect", BooleanValue (false));
  sgwIpv6->SetAttribute ("IpForward", BooleanValue (true));

  // create S1-U socket
  Ptr<Socket> sgwS1uSocket = Socket::CreateSocket (m_sgw, TypeId::LookupByName ("ns3::UdpSocketFactory"));
  int retval = sgwS1uSocket->Bind (Inet6SocketAddress (Ipv6Address::GetAny (), m_gtpuUdpPort));
  NS_ASSERT (retval == 0);

  // create TUN device implementing tunnelling of user data over GTP-U/UDP/IP
  m_tunDevice = CreateObject<VirtualNetDevice> ();
  // allow jumbo packets
  m_tunDevice->SetAttribute ("Mtu", UintegerValue (30000));

  // yes we need this
  m_tunDevice->SetAddress (Mac48Address::Allocate ()); 

  m_sgw->AddDevice (m_tunDevice);
  NetDeviceContainer tunDeviceContainer;
  tunDeviceContainer.Add (m_tunDevice);
  
  // the TUN device is given a /32 prefix so that all the prefixes assigned
  // to UEs fall within this network. So when a packet addressed to an UE
  // arrives at the intenet to the WAN interface of the PGW it will be
  // forwarded to the TUN device.
  int32_t ifIndex = sgwIpv6->AddInterface (m_tunDevice);
  Ipv6InterfaceAddress ipv6Addr = Ipv6InterfaceAddress (Ipv6Address::MakeAutoconfiguredAddress (Mac48Address::ConvertFrom (m_tunDevice->GetAddress ()), "b0::"), 32);
  sgwIpv6->SetMetric (ifIndex, 1);
  sgwIpv6->AddAddress (ifIndex, ipv6Addr);
  sgwIpv6->SetUp (ifIndex);

  // create EpcSgwApplication
  m_sgwApp = CreateObject<Epc6SgwApplication> (m_tunDevice, sgwS1uSocket);
  m_sgw->AddApplication (m_sgwApp);
  
  // connect SgwApplication and virtual net device for tunnelling
  m_tunDevice->SetSendCallback (MakeCallback (&Epc6SgwApplication::RecvFromTunDevice, m_sgwApp));

  // Create MME and connect with SGW via S11 interface
  m_mme = CreateObject<EpcMme> ();
  m_mme->SetS11SapSgw (m_sgwApp->GetS11SapSgw ());
  m_sgwApp->SetS11SapMme (m_mme->GetS11SapMme ());
}

PointToPointEpc6Pmipv6Helper::~PointToPointEpc6Pmipv6Helper ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
PointToPointEpc6Pmipv6Helper::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::PointToPointEpc6Pmipv6Helper")
    .SetParent<EpcHelper> ()
    .AddConstructor<PointToPointEpc6Pmipv6Helper> ()
    .AddAttribute ("S1uLinkDataRate", 
                   "The data rate to be used for the next S1-U link to be created",
                   DataRateValue (DataRate ("10Gb/s")),
                   MakeDataRateAccessor (&PointToPointEpc6Pmipv6Helper::m_s1uLinkDataRate),
                   MakeDataRateChecker ())
    .AddAttribute ("S1uLinkDelay", 
                   "The delay to be used for the next S1-U link to be created",
                   TimeValue (Seconds (0)),
                   MakeTimeAccessor (&PointToPointEpc6Pmipv6Helper::m_s1uLinkDelay),
                   MakeTimeChecker ())
    .AddAttribute ("S1uLinkMtu", 
                   "The MTU of the next S1-U link to be created. Note that, because of the additional GTP/UDP/IP tunneling overhead, you need a MTU larger than the end-to-end MTU that you want to support.",
                   UintegerValue (2000),
                   MakeUintegerAccessor (&PointToPointEpc6Pmipv6Helper::m_s1uLinkMtu),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("X2LinkDataRate",
                   "The data rate to be used for the next X2 link to be created",
                   DataRateValue (DataRate ("10Gb/s")),
                   MakeDataRateAccessor (&PointToPointEpc6Pmipv6Helper::m_x2LinkDataRate),
                   MakeDataRateChecker ())
    .AddAttribute ("X2LinkDelay",
                   "The delay to be used for the next X2 link to be created",
                   TimeValue (Seconds (0)),
                   MakeTimeAccessor (&PointToPointEpc6Pmipv6Helper::m_x2LinkDelay),
                   MakeTimeChecker ())
    .AddAttribute ("X2LinkMtu",
                   "The MTU of the next X2 link to be created. Note that, because of some big X2 messages, you need a big MTU.",
                   UintegerValue (3000),
                   MakeUintegerAccessor (&PointToPointEpc6Pmipv6Helper::m_x2LinkMtu),
                   MakeUintegerChecker<uint16_t> ())
  ;
  return tid;
}

void
PointToPointEpc6Pmipv6Helper::DoDispose ()
{
  NS_LOG_FUNCTION (this);
  m_tunDevice->SetSendCallback (MakeNullCallback<bool, Ptr<Packet>, const Address&, const Address&, uint16_t> ());
  m_tunDevice = 0;
  m_sgwApp = 0;
  m_sgw->Dispose ();
}


void
PointToPointEpc6Pmipv6Helper::AddEnb (Ptr<Node> enb, Ptr<NetDevice> lteEnbNetDevice, uint16_t cellId)
{
  NS_LOG_FUNCTION (this << enb << lteEnbNetDevice << cellId);

  NS_ASSERT (enb == lteEnbNetDevice->GetNode ());

  // add an IPv6 stack to the previously created eNB
  InternetStackHelper internet;
  internet.Install (enb);
  NS_LOG_LOGIC ("number of Ipv6 ifaces of the eNB after node creation: " << enb->GetObject<Ipv6> ()->GetNInterfaces ());
  Ptr<Icmpv6L4Protocol> enbIcmpv6L4Protocol = enb->GetObject<Icmpv6L4Protocol> ();
  enbIcmpv6L4Protocol->SetAttribute ("DAD", BooleanValue (false));

  // create a point to point link between the new eNB and the SGW with
  // the corresponding new NetDevices on each side  
  NodeContainer enbSgwNodes;
  enbSgwNodes.Add (m_sgw);
  enbSgwNodes.Add (enb);
  PointToPointHelper p2ph;
  p2ph.SetDeviceAttribute ("DataRate", DataRateValue (m_s1uLinkDataRate));
  p2ph.SetDeviceAttribute ("Mtu", UintegerValue (m_s1uLinkMtu));
  p2ph.SetChannelAttribute ("Delay", TimeValue (m_s1uLinkDelay));
  NetDeviceContainer enbSgwDevices = p2ph.Install (enb, m_sgw);
  p2ph.EnablePcap ("enb-sgwpgw", enbSgwDevices);
  NS_LOG_LOGIC ("number of Ipv6 ifaces of the eNB after installing p2p dev: " << enb->GetObject<Ipv6> ()->GetNInterfaces ());
  Ptr<NetDevice> enbDev = enbSgwDevices.Get (0);
  Ptr<NetDevice> sgwDev = enbSgwDevices.Get (1);
  m_s1uIpv6AddressHelper.NewNetwork ();
  Ipv6InterfaceContainer enbSgwIpIfaces = m_s1uIpv6AddressHelper.Assign (enbSgwDevices);
  NS_LOG_LOGIC ("number of Ipv6 ifaces of the eNB after assigning Ipv6 addr to S1 dev: " << enb->GetObject<Ipv6> ()->GetNInterfaces ());
  
  Ipv6Address enbAddress = enbSgwIpIfaces.GetAddress (0, 1);
  Ipv6Address sgwAddress = enbSgwIpIfaces.GetAddress (1, 1);

  // create S1-U socket for the ENB
  Ptr<Socket> enbS1uSocket = Socket::CreateSocket (enb, TypeId::LookupByName ("ns3::UdpSocketFactory"));
  int retval = enbS1uSocket->Bind (Inet6SocketAddress (enbAddress, m_gtpuUdpPort));
  NS_ASSERT (retval == 0);
  
  // give PacketSocket powers to the eNB
  //PacketSocketHelper packetSocket;
  //packetSocket.Install (enb); 
  
  // create LTE socket for the ENB 
  Ptr<Socket> enbLteSocket = Socket::CreateSocket (enb, TypeId::LookupByName ("ns3::PacketSocketFactory"));
  PacketSocketAddress enbLteSocketBindAddress;
  enbLteSocketBindAddress.SetSingleDevice (lteEnbNetDevice->GetIfIndex ());
  enbLteSocketBindAddress.SetProtocol (Ipv6L3Protocol::PROT_NUMBER);
  retval = enbLteSocket->Bind (enbLteSocketBindAddress);
  NS_ASSERT (retval == 0);  
  PacketSocketAddress enbLteSocketConnectAddress;
  enbLteSocketConnectAddress.SetPhysicalAddress (Mac48Address::GetBroadcast ());
  enbLteSocketConnectAddress.SetSingleDevice (lteEnbNetDevice->GetIfIndex ());
  enbLteSocketConnectAddress.SetProtocol (Ipv6L3Protocol::PROT_NUMBER);
  retval = enbLteSocket->Connect (enbLteSocketConnectAddress);
  NS_ASSERT (retval == 0);  
  
  NS_LOG_INFO ("create EpcEnbApplication");
  Ptr<EpcEnbApplication> enbApp = CreateObject<EpcEnbApplication> (enbLteSocket, enbS1uSocket, enbAddress, sgwAddress, cellId);
  enb->AddApplication (enbApp);
  NS_ASSERT (enb->GetNApplications () == 1);
  NS_ASSERT_MSG (enb->GetApplication (0)->GetObject<EpcEnbApplication> () != 0, "cannot retrieve EpcEnbApplication");
  NS_LOG_LOGIC ("enb: " << enb << ", enb->GetApplication (0): " << enb->GetApplication (0));

  NS_LOG_INFO ("Create EpcX2 entity");
  Ptr<EpcX2> x2 = CreateObject<EpcX2> ();
  enb->AggregateObject (x2);

  NS_LOG_INFO ("connect S1-AP interface");
  m_mme->AddEnb (cellId, enbAddress, enbApp->GetS1apSapEnb ());
  m_sgwApp->AddEnb (cellId, enbAddress, sgwAddress);
  enbApp->SetS1apSapMme (m_mme->GetS1apSapMme ());
}


void
PointToPointEpc6Pmipv6Helper::AddX2Interface (Ptr<Node> enb1, Ptr<Node> enb2)
{
  NS_LOG_FUNCTION (this << enb1 << enb2);

  // Create a point to point link between the two eNBs with
  // the corresponding new NetDevices on each side
  NodeContainer enbNodes;
  enbNodes.Add (enb1);
  enbNodes.Add (enb2);
  PointToPointHelper p2ph;
  p2ph.SetDeviceAttribute ("DataRate", DataRateValue (m_x2LinkDataRate));
  p2ph.SetDeviceAttribute ("Mtu", UintegerValue (m_x2LinkMtu));
  p2ph.SetChannelAttribute ("Delay", TimeValue (m_x2LinkDelay));
  NetDeviceContainer enbDevices = p2ph.Install (enb1, enb2);
  NS_LOG_LOGIC ("number of Ipv6 ifaces of the eNB #1 after installing p2p dev: " << enb1->GetObject<Ipv6> ()->GetNInterfaces ());
  NS_LOG_LOGIC ("number of Ipv6 ifaces of the eNB #2 after installing p2p dev: " << enb2->GetObject<Ipv6> ()->GetNInterfaces ());
  Ptr<NetDevice> enb1Dev = enbDevices.Get (0);
  Ptr<NetDevice> enb2Dev = enbDevices.Get (1);

  m_x2Ipv6AddressHelper.NewNetwork ();
  Ipv6InterfaceContainer enbIpIfaces = m_x2Ipv6AddressHelper.Assign (enbDevices);
  NS_LOG_LOGIC ("number of Ipv6 ifaces of the eNB #1 after assigning Ipv6 addr to X2 dev: " << enb1->GetObject<Ipv6> ()->GetNInterfaces ());
  NS_LOG_LOGIC ("number of Ipv6 ifaces of the eNB #2 after assigning Ipv6 addr to X2 dev: " << enb2->GetObject<Ipv6> ()->GetNInterfaces ());

  Ipv6Address enb1X2Address = enbIpIfaces.GetAddress (0, 1);
  Ipv6Address enb2X2Address = enbIpIfaces.GetAddress (1, 1);

  // Add X2 interface to both eNBs' X2 entities
  Ptr<EpcX2> enb1X2 = enb1->GetObject<EpcX2> ();
  Ptr<LteEnbNetDevice> enb1LteDev = enb1->GetDevice (0)->GetObject<LteEnbNetDevice> ();
  uint16_t enb1CellId = enb1LteDev->GetCellId ();
  NS_LOG_LOGIC ("LteEnbNetDevice #1 = " << enb1LteDev << " - CellId = " << enb1CellId);

  Ptr<EpcX2> enb2X2 = enb2->GetObject<EpcX2> ();
  Ptr<LteEnbNetDevice> enb2LteDev = enb2->GetDevice (0)->GetObject<LteEnbNetDevice> ();
  uint16_t enb2CellId = enb2LteDev->GetCellId ();
  NS_LOG_LOGIC ("LteEnbNetDevice #2 = " << enb2LteDev << " - CellId = " << enb2CellId);

  enb1X2->AddX2Interface (enb1CellId, enb1X2Address, enb2CellId, enb2X2Address);
  enb2X2->AddX2Interface (enb2CellId, enb2X2Address, enb1CellId, enb1X2Address);

  enb1LteDev->GetRrc ()->AddX2Neighbour (enb2LteDev->GetCellId ());
  enb2LteDev->GetRrc ()->AddX2Neighbour (enb1LteDev->GetCellId ());
}


void 
PointToPointEpc6Pmipv6Helper::AddUe (Ptr<NetDevice> ueDevice, uint64_t imsi)
{
  NS_LOG_FUNCTION (this << imsi << ueDevice );
  
  m_mme->AddUe (imsi);
  m_sgwApp->AddUe (imsi);
}

void
PointToPointEpc6Pmipv6Helper::ActivateEpsBearer (Ptr<NetDevice> ueDevice, uint64_t imsi, Ptr<EpcTft> tft, EpsBearer bearer)
{
  NS_LOG_FUNCTION (this << ueDevice << imsi);
  
  m_mme->AddBearer (imsi, tft, bearer);
  Ptr<LteUeNetDevice> ueLteDevice = ueDevice->GetObject<LteUeNetDevice> ();
  if (ueLteDevice)
    {
      ueLteDevice->GetNas ()->ActivateEpsBearer (bearer, tft);
    }
}

Ptr<Node>
PointToPointEpc6Pmipv6Helper::GetPgwNode ()
{
  return m_sgw;
}

Ipv6InterfaceContainer
PointToPointEpc6Pmipv6Helper::AssignUeIpv6Address (NetDeviceContainer ueDevices)
{
  return m_ueAddressHelper.Assign (ueDevices);
}

Ipv6Address
PointToPointEpc6Pmipv6Helper::GetUeDefaultGatewayAddress ()
{
  // return the address of the tun device
  return m_sgw->GetObject<Ipv6> ()->GetAddress (1, 1).GetAddress ();
}

Ipv6InterfaceContainer
PointToPointEpc6Pmipv6Helper::AssignWithoutAddress (NetDeviceContainer ueDevices)
{
  return m_ueAddressHelper.AssignWithoutAddress (ueDevices);
}

} // namespace ns3
