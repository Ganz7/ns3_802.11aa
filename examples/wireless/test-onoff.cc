#include <fstream>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/mobility-module.h"
#include "ns3/config-store-module.h"
#include "ns3/wifi-module.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cassert>


using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("on_off_agg_wifi");

//This is a simple example in order to study TCP over wifi infrastructure
//mode
//
//Network topology:
//
//  Wifi 10.1.1.0
//
//
//   *    *    *
//   |    |    |        (infrastructure MODE)
//   n1   n2   n3
//

static void
RttChange (Ptr<OutputStreamWrapper> stream, Time oldRtt, Time newRtt)
{
  *stream->GetStream () << Simulator::Now ().GetSeconds () << "\t" <<
(newRtt/1000000) << std::endl;
}

void
CwndChangeWriteFile (Ptr<OutputStreamWrapper> stream, uint32_t oldCwnd,
uint32_t newCwnd)
{
  *stream->GetStream () << Simulator::Now ().GetSeconds () << "\t" <<
newCwnd << std::endl;
}

void
DoCwndTraceConnectOnSocketAfterAppStarts (Ptr<Application> app,
Ptr<OutputStreamWrapper> stream, Ptr<OutputStreamWrapper> stream2)
{
  Ptr<Socket> appSocket = app->GetObject<OnOffApplication> ()->GetSocket ();
  appSocket->TraceConnectWithoutContext ("CongestionWindow",
MakeBoundCallback (&CwndChangeWriteFile, stream));
  appSocket->TraceConnectWithoutContext ("RTT", MakeBoundCallback
(&RttChange,stream2));
}

int
main (int argc, char *argv[])
{


			LogComponentEnable ("EdcaTxopN", LOG_LEVEL_INFO);
			//LogComponentEnable ("DcaTxop", LOG_LEVEL_INFO);

        Config::SetDefault ("ns3::WifiMacQueue::MaxPacketNumber",
UintegerValue (120)); // default is 400

Config::SetDefault("ns3::TcpL4Protocol::SocketType",TypeIdValue(TypeId::LookupByName("ns3::TcpNewReno")));

        NodeContainer wifiNodes;
        wifiNodes.Create (2);
        NodeContainer wifiApNode;
        wifiApNode.Create (1);

    YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default ();
    YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default ();
    wifiPhy.SetChannel (wifiChannel.Create ());

    WifiHelper wifi = WifiHelper::Default ();

        QosWifiMacHelper wifiMac = QosWifiMacHelper::Default ();
        wifi.SetRemoteStationManager
("ns3::ConstantRateWifiManager","DataMode", StringValue ("OfdmRate54Mbps"));

    Ssid ssid = Ssid ("ns-3-802.11n");
        wifiMac.SetType ("ns3::StaWifiMac",
               "Ssid", SsidValue (ssid),
               "ActiveProbing", BooleanValue (false));
        wifiMac.SetMsduAggregatorForAc (AC_BE,
"ns3::MsduStandardAggregator",
                //                 "MaxAmsduSize", UintegerValue (2048));
                                 "MaxAmsduSize", UintegerValue (2398));
                //                 "MaxAmsduSize", UintegerValue (7935));
        NetDeviceContainer staDevices;
        staDevices = wifi.Install (wifiPhy, wifiMac, wifiNodes);
        Config::Set
("/NodeList/0/DeviceList/0/Mac/BE_EdcaTxopN/BlockAckThreshold",
UintegerValue (64));
        Config::Set
("/NodeList/1/DeviceList/0/Mac/BE_EdcaTxopN/BlockAckThreshold",
UintegerValue (64));
        wifiMac.SetType ("ns3::ApWifiMac",
               "Ssid", SsidValue (ssid));
        wifiMac.SetMsduAggregatorForAc (AC_BE,
"ns3::MsduStandardAggregator",
                                 "MaxAmsduSize", UintegerValue (7935));

        NetDeviceContainer apDevice;
        apDevice = wifi.Install (wifiPhy, wifiMac, wifiApNode);
        Config::Set
("/NodeList/2/DeviceList/0/Mac/BE_EdcaTxopN/BlockAckThreshold",
UintegerValue (64));


        MobilityHelper mobility;
    mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                    "MinX", DoubleValue (0.0),
                    "MinY", DoubleValue (0.0),
                    "DeltaX", DoubleValue (5.0),
                    "DeltaY", DoubleValue (10.0),
                    "GridWidth", UintegerValue (3),
                    "LayoutType", StringValue ("RowFirst"));

    mobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
                                     "Bounds", RectangleValue (Rectangle
(-50, 50, -50, 50)));
    mobility.Install (wifiNodes);

    mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
    mobility.Install (wifiApNode);

        InternetStackHelper internet;
    internet.Install (wifiNodes);
    internet.Install (wifiApNode);

    Ipv4AddressHelper address;
    address.SetBase ("192.168.1.0", "255.255.255.0");
    Ipv4InterfaceContainer wifiNodesInterfaces;
        Ipv4InterfaceContainer apNodeInterface;

    wifiNodesInterfaces = address.Assign (staDevices);
    apNodeInterface = address.Assign (apDevice);

        uint16_t sinkPort = 8080;
    Address sinkAddress (InetSocketAddress (apNodeInterface.GetAddress (0),
sinkPort));
    PacketSinkHelper packetSinkHelper ("ns3::TcpSocketFactory",
InetSocketAddress (Ipv4Address::GetAny (), sinkPort));
    ApplicationContainer sinkApps = packetSinkHelper.Install
(wifiApNode.Get (0));
    sinkApps.Start (Seconds (0.));
    sinkApps.Stop (Seconds (60.));

    OnOffHelper onOffHelper ("ns3::TcpSocketFactory", sinkAddress);
    onOffHelper.SetAttribute ("OnTime", StringValue
("ns3::ConstantRandomVariable[Constant=1]"));
    onOffHelper.SetAttribute ("OffTime", StringValue
("ns3::ConstantRandomVariable[Constant=0]"));
    onOffHelper.SetAttribute ("DataRate",StringValue ("54Mbps"));
    onOffHelper.SetAttribute ("PacketSize", UintegerValue (1024));

    ApplicationContainer source;

    source.Add (onOffHelper.Install (wifiNodes.Get(0)));
    source.Start (Seconds (1.1));
    source.Stop (Seconds (60.0));

    AsciiTraceHelper asciiTraceHelper;
    Ptr<OutputStreamWrapper> stream = asciiTraceHelper.CreateFileStream
("on_off_cwnd_agg.dat");

    AsciiTraceHelper asciiTraceHelper2;
    Ptr<OutputStreamWrapper> stream2 = asciiTraceHelper2.CreateFileStream
("on_off_RTT_agg.log");

    Simulator::Schedule (Seconds (2.0) + NanoSeconds (1.0),
&DoCwndTraceConnectOnSocketAfterAppStarts, wifiNodes.Get
(0)->GetApplication (0), stream, stream2);

    Simulator::Stop (Seconds (60));
    Simulator::Run ();
    Simulator::Destroy ();

  return 0;
}

