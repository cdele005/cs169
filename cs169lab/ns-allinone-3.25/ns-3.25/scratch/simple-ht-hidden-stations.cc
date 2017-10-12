/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015 Sébastien Deronne
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
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/internet-module.h"

// This example considers two hidden stations in an 802.11n network which supports MPDU aggregation.
// The user can specify whether RTS/CTS is used and can set the number of aggregated MPDUs.
//
// Example: ./waf --run "simple-ht-hidden-stations --enableRts=1 --nMpdus=8"
//
// Network topology:
//
//   Wifi 192.168.1.0
//
//        AP
//   *    *    *
//   |    |    |
//   n1   n2   n3
//
// Packets in this simulation aren't marked with a QosTag so they are considered
// belonging to BestEffort Access Class (AC_BE).

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("SimplesHtHiddenStations");

int main (int argc, char *argv[])
{
  uint32_t payloadSize = 1472; //bytes
  uint64_t simulationTime = 10; //seconds
  uint32_t nMpdus = 1;
  uint32_t maxAmpduSize = 0;
  bool enableRts = 0;

  CommandLine cmd;
  cmd.AddValue ("nMpdus", "Number of aggregated MPDUs", nMpdus);
  cmd.AddValue ("payloadSize", "Payload size in bytes", payloadSize);
  cmd.AddValue ("enableRts", "Enable RTS/CTS", enableRts); // 1: RTS/CTS enabled; 0: RTS/CTS disabled
  cmd.AddValue ("simulationTime", "Simulation time in seconds", simulationTime);
  cmd.Parse (argc, argv);

  if (!enableRts)
    {
      Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue ("999999"));
    }
  else
    {
      Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue ("0"));
    }

  Config::SetDefault ("ns3::WifiRemoteStationManager::FragmentationThreshold", StringValue ("990000"));

  //Set the maximum size for A-MPDU with regards to the payload size
  maxAmpduSize = nMpdus * (payloadSize + 200);

  // Set the maximum wireless range to 5 meters in order to reproduce a hidden nodes scenario, i.e. the distance between hidden stations is larger than 5 meters
  Config::SetDefault ("ns3::RangePropagationLossModel::MaxRange", DoubleValue (5));

  NodeContainer wifiStaNodes;
  wifiStaNodes.Create (4);
  NodeContainer wifiApNode;
  wifiApNode.Create (1);

  YansWifiChannelHelper channel = YansWifiChannelHelper::Default ();
  channel.AddPropagationLoss ("ns3::RangePropagationLossModel"); //wireless range limited to 5 meters!

  YansWifiPhyHelper phy = YansWifiPhyHelper::Default ();
  phy.SetPcapDataLinkType (YansWifiPhyHelper::DLT_IEEE802_11_RADIO);
  phy.SetChannel (channel.Create ());

  WifiHelper wifi;
  wifi.SetStandard (WIFI_PHY_STANDARD_80211n_5GHZ);
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager", "DataMode", StringValue ("HtMcs7"), "ControlMode", StringValue ("HtMcs0"));
  WifiMacHelper mac;

  Ssid ssid = Ssid ("simple-mpdu-aggregation");
  mac.SetType ("ns3::StaWifiMac",
               "Ssid", SsidValue (ssid),
               "ActiveProbing", BooleanValue (false),
               "BE_MaxAmpduSize", UintegerValue (maxAmpduSize));

  NetDeviceContainer staDevices;
  staDevices = wifi.Install (phy, mac, wifiStaNodes);

  mac.SetType ("ns3::ApWifiMac",
               "Ssid", SsidValue (ssid),
               "BeaconInterval", TimeValue (MicroSeconds (102400)),
               "BeaconGeneration", BooleanValue (true),
               "BE_MaxAmpduSize", UintegerValue (maxAmpduSize));

  NetDeviceContainer apDevice;
  apDevice = wifi.Install (phy, mac, wifiApNode);

  // Setting mobility model
  MobilityHelper mobility;
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();

  // AP is between the two stations, each station being located at 5 meters from the AP.
  // The distance between the two stations is thus equal to 10 meters.
  // Since the wireless range is limited to 5 meters, the two stations are hidden from each other.
  positionAlloc->Add (Vector (5.0, 5.0, 0.0));
  positionAlloc->Add (Vector (0.0, 5.0, 0.0));
  positionAlloc->Add (Vector (10.0, 5.0, 0.0));
  positionAlloc->Add (Vector (5.0, 10.0, 0.0));
  positionAlloc->Add (Vector (5.0, 0.0, 0.0));
  mobility.SetPositionAllocator (positionAlloc);

  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");

  mobility.Install (wifiApNode);
  mobility.Install (wifiStaNodes);

  // Internet stack
  InternetStackHelper stack;
  stack.Install (wifiApNode);
  stack.Install (wifiStaNodes);

  Ipv4AddressHelper address;

  address.SetBase ("192.168.1.0", "255.255.255.0");
  Ipv4InterfaceContainer StaInterface;
  StaInterface = address.Assign (staDevices);
  Ipv4InterfaceContainer ApInterface;
  ApInterface = address.Assign (apDevice);

  // Setting applications
  UdpEchoServerHelper myServerA (9);
  ApplicationContainer serverAppA = myServerA.Install (wifiApNode);
  serverAppA.Start (Seconds (0.0));
  serverAppA.Stop (Seconds (simulationTime + 1));

  UdpEchoServerHelper myServerB (10);
  ApplicationContainer serverAppB = myServerB.Install (wifiApNode);
  serverAppB.Start (Seconds (0.0));
  serverAppB.Stop (Seconds (simulationTime + 1));

  UdpEchoServerHelper myServerC (11);
  ApplicationContainer serverAppC = myServerC.Install (wifiApNode);
  serverAppC.Start (Seconds (0.0));
  serverAppC.Stop (Seconds (simulationTime + 1));

  UdpEchoServerHelper myServerD (12);
  ApplicationContainer serverAppD = myServerD.Install (wifiApNode);
  serverAppD.Start (Seconds (0.0));
  serverAppD.Stop (Seconds (simulationTime + 1));

  UdpEchoClientHelper myClientA (ApInterface.GetAddress (0), 9);
  myClientA.SetAttribute ("MaxPackets", UintegerValue (4294967295u));
  myClientA.SetAttribute ("Interval", TimeValue (Time ("0.00002"))); //packets/s
  myClientA.SetAttribute ("PacketSize", UintegerValue (payloadSize));

  UdpEchoClientHelper myClientB (ApInterface.GetAddress (0), 10);
  myClientB.SetAttribute ("MaxPackets", UintegerValue (4294967295u));
  myClientB.SetAttribute ("Interval", TimeValue (Time ("0.00002"))); //packets/s
  myClientB.SetAttribute ("PacketSize", UintegerValue (payloadSize));

  UdpEchoClientHelper myClientC (ApInterface.GetAddress (0), 11);
  myClientC.SetAttribute ("MaxPackets", UintegerValue (4294967295u));
  myClientC.SetAttribute ("Interval", TimeValue (Time ("0.00002"))); //packets/s
  myClientC.SetAttribute ("PacketSize", UintegerValue (payloadSize));

  UdpEchoClientHelper myClientD (ApInterface.GetAddress (0), 12);
  myClientD.SetAttribute ("MaxPackets", UintegerValue (4294967295u));
  myClientD.SetAttribute ("Interval", TimeValue (Time ("0.00002"))); //packets/s
  myClientD.SetAttribute ("PacketSize", UintegerValue (payloadSize));

  // Saturated UDP traffic from stations to AP
  ApplicationContainer clientAppA = myClientA.Install (wifiStaNodes);
  clientAppA.Start (Seconds (1.0));
  clientAppA.Stop (Seconds (simulationTime + 1));
  
  ApplicationContainer clientAppB = myClientB.Install (wifiStaNodes);
  clientAppB.Start (Seconds (1.0));
  clientAppB.Stop (Seconds (simulationTime + 1));

  ApplicationContainer clientAppC = myClientC.Install (wifiStaNodes);
  clientAppC.Start (Seconds (1.0));
  clientAppC.Stop (Seconds (simulationTime + 1));

  ApplicationContainer clientAppD = myClientD.Install (wifiStaNodes);
  clientAppD.Start (Seconds (1.0));
  clientAppD.Stop (Seconds (simulationTime + 1));

  phy.EnablePcap ("SimpleHtHiddenStations_Ap", apDevice.Get (0));
  phy.EnablePcap ("SimpleHtHiddenStations_Sta1", staDevices.Get (0));
  phy.EnablePcap ("SimpleHtHiddenStations_Sta2", staDevices.Get (1));
  phy.EnablePcap ("SimpleHtHiddenStations_Sta3", staDevices.Get (2));
  phy.EnablePcap ("SimpleHtHiddenStations_Sta4", staDevices.Get (3));

  Simulator::Stop (Seconds (simulationTime + 1));
  Simulator::Run ();
  Simulator::Destroy ();
/*
  uint32_t totalPacketsThrough = DynamicCast<UdpServer> (serverApp.Get (0))->GetReceived ();
  double throughput = totalPacketsThrough * payloadSize * 8 / (simulationTime * 1000000.0);
  std::cout << "Throughput: " << throughput << " Mbit/s" << '\n';
*/
  return 0;
}
