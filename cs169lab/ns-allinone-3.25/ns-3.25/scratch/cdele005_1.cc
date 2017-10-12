#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/internet-module.h"

#include <fstream>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("SimplesHtHiddenStations");

// count each packet received/sent per node
uint32_t countEchoClientSend9 = 0;
uint32_t countEchoClientSend10 = 0;
uint32_t countEchoClientSend11 = 0;
uint32_t countEchoClientSend12 = 0;
uint32_t countEchoClientReceive9 = 0;
uint32_t countEchoClientReceive10 = 0;
uint32_t countEchoClientReceive11 = 0;
uint32_t countEchoClientReceive12 = 0;

void
UdpEchoClientTrace (std::string context, Ptr<const Packet> packet)
{
	if (context == "/NodeList/0/ApplicationList/0/$ns3::UdpEchoClient/Tx" 
	|| context == "/NodeList/0/ApplicationList/1/$ns3::UdpEchoClient/Tx"
	|| context == "/NodeList/0/ApplicationList/2/$ns3::UdpEchoClient/Tx"
	|| context == "/NodeList/0/ApplicationList/3/$ns3::UdpEchoClient/Tx")
		{countEchoClientSend9++;}

	else if (context == "/NodeList/1/ApplicationList/0/$ns3::UdpEchoClient/Tx" 
	|| context == "/NodeList/1/ApplicationList/1/$ns3::UdpEchoClient/Tx"
	|| context == "/NodeList/1/ApplicationList/2/$ns3::UdpEchoClient/Tx"
	|| context == "/NodeList/1/ApplicationList/3/$ns3::UdpEchoClient/Tx")
		{countEchoClientSend10++;}

	else if (context == "/NodeList/2/ApplicationList/0/$ns3::UdpEchoClient/Tx" 
	|| context == "/NodeList/2/ApplicationList/1/$ns3::UdpEchoClient/Tx"
	|| context == "/NodeList/2/ApplicationList/2/$ns3::UdpEchoClient/Tx"
	|| context == "/NodeList/2/ApplicationList/3/$ns3::UdpEchoClient/Tx")
		{countEchoClientSend11++;}

	else if (context == "/NodeList/3/ApplicationList/0/$ns3::UdpEchoClient/Tx" 
	|| context == "/NodeList/3/ApplicationList/1/$ns3::UdpEchoClient/Tx"
	|| context == "/NodeList/3/ApplicationList/2/$ns3::UdpEchoClient/Tx"
	|| context == "/NodeList/3/ApplicationList/3/$ns3::UdpEchoClient/Tx")
		{countEchoClientSend12++;}

	//NS_LOG_UNCOND (context << ": UdpEchoClient packet sent");
	//countEchoClientSend9++;
}

void
UdpEchoServerTrace (std::string context, Ptr<const Packet> packet)
{
	if (context == "/NodeList/4/ApplicationList/0/$ns3::UdpEchoServer/Rx") 
		{countEchoClientReceive9++;}

	else if (context == "/NodeList/4/ApplicationList/1/$ns3::UdpEchoServer/Rx") 
		{countEchoClientReceive10++;}
	
	else if (context == "/NodeList/4/ApplicationList/2/$ns3::UdpEchoServer/Rx") 
		{countEchoClientReceive11++;}

	else if (context == "/NodeList/4/ApplicationList/3/$ns3::UdpEchoServer/Rx") 
		{countEchoClientReceive12++;}

	//NS_LOG_UNCOND (context << ": UdpEchoClient packet received");
	//countEchoClientReceive9++;
}

int main (int argc, char *argv[])
{
        uint32_t payloadSize = 1472; //bytes
        uint64_t simulationTime = 10; //seconds
        uint32_t nMpdus = 1;
        uint32_t maxAmpduSize = 0;
        bool enableRts = 1; //0
        std::string packetInterval = "0.001"; //"1"

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

        
        maxAmpduSize = nMpdus * (payloadSize + 200);

        Config::SetDefault ("ns3::RangePropagationLossModel::MaxRange", DoubleValue (5));

        NodeContainer wifiStaNodes;
        wifiStaNodes.Create (1);
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

        MobilityHelper mobility;
        Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();

        positionAlloc->Add (Vector (5.0, 5.0, 0.0));
        positionAlloc->Add (Vector (0.0, 5.0, 0.0));
        
        mobility.SetPositionAllocator (positionAlloc);

        mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");

        mobility.Install (wifiApNode);
        mobility.Install (wifiStaNodes);
       
        InternetStackHelper stack;
        stack.Install (wifiApNode);
        stack.Install (wifiStaNodes);

        Ipv4AddressHelper address;
	address.SetBase ("192.168.1.0", "255.255.255.0");
        Ipv4InterfaceContainer StaInterface;
        StaInterface = address.Assign (staDevices);
        Ipv4InterfaceContainer ApInterface;
        ApInterface = address.Assign (apDevice);

	// UDP server port 9
        UdpEchoServerHelper myServer9 (9);
        ApplicationContainer serverApp9 = myServer9.Install (wifiApNode);
        serverApp9.Start (Seconds (0.0));
        serverApp9.Stop (Seconds (simulationTime + 1));

	// set number of packets to infinity	
        uint32_t infinity = 200000000u;

	// client n1 --> AP(9)
        UdpEchoClientHelper myClient9 (ApInterface.GetAddress (0), 9);
        myClient9.SetAttribute ("MaxPackets", UintegerValue (infinity));
        myClient9.SetAttribute ("Interval", TimeValue (Time (packetInterval))); //packets/s
        myClient9.SetAttribute ("PacketSize", UintegerValue (payloadSize));
	ApplicationContainer clientApp9 = myClient9.Install (wifiStaNodes.Get(0));
        clientApp9.Start (Seconds (1.0));
        clientApp9.Stop (Seconds (simulationTime + 1));
        
        phy.EnablePcap ("SimpleHtHiddenStations_Ap", apDevice.Get (0));
        phy.EnablePcap ("SimpleHtHiddenStations_Sta1", staDevices.Get (0));

        Config::Connect("/NodeList/*/ApplicationList/*/$ns3::UdpEchoClient/Tx", 
		     MakeCallback (&UdpEchoClientTrace));
        Config::Connect("/NodeList/*/ApplicationList/*/$ns3::UdpEchoServer/Rx", 
		     MakeCallback (&UdpEchoServerTrace));

        //std::cout << "test1" << std::endl;
        //Config::SetDefault ("ns3::DcfState::backoff_type", UintegerValue (1));
        //std::cout << "test2" << std::endl;

        Simulator::Stop (Seconds (simulationTime + 1));
        Simulator::Run ();
        Simulator::Destroy ();

        // print sent/received/lost packets
        std::cout << "sent packets (Node 0) = " << countEchoClientSend9 << std::endl;
        std::cout << "received     (Node 0) = " << countEchoClientReceive9 << std::endl;
        std::cout << "lost packets (Node 0) = " << countEchoClientSend9 - countEchoClientReceive9 << std::endl << std::endl;

	// print throughput
  	uint32_t totalPacketsThrough9 = countEchoClientReceive9; //DynamicCast<UdpServer> (serverApp.Get (0))->GetReceived ();

  	double throughput9 = totalPacketsThrough9 * payloadSize * 8 / (simulationTime * 1000000.0);

  	std::cout << "Node 0 Throughput: " << throughput9 << " Mbit/s" << '\n';
	
	// output total delay from file
	std::ifstream fin;
	std::string in_file = "scratch/output.txt";
	std::string total_delay;
	fin.open(in_file.c_str());
	fin >> total_delay;
	fin.close();

	std::cout << "Total Delay:       " << total_delay << std::endl;
	
        return 0;
}
