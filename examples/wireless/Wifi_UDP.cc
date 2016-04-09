/*
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
 */

/*
 * Author(s): Ganesh Kumar, Yijing Zheng
 * 
 * Network Topology with an Access Point and multiple stations
 *  
*/

#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"

using namespace ns3;

/*

n3    n2    n1     AP


*/

Ptr<PacketSink> sink[6];                         /* Pointer to the packet sink application */

double simulation_time = 10.0;

NS_LOG_COMPONENT_DEFINE ("Simple AP + Stations");

int 
main (int argc, char *argv[])
{
  int nWifi = 3;
  bool tracing = false;


  std::cout << "simulation code." << std::endl;

  // Check for valid number of wifi nodes
  // 250 should be enough, otherwise IP addresses 
  // soon become an issue
  if (nWifi > 250)
    {
      std::cout << "Too many wifi nodes, no more than 250." << std::endl;
      return 1;
    }

  NodeContainer wifiStaNodes;
  wifiStaNodes.Create (nWifi);

  NodeContainer wifiApNode;
  wifiApNode.Create(1);

  YansWifiChannelHelper channel = YansWifiChannelHelper::Default ();
  YansWifiPhyHelper phy = YansWifiPhyHelper::Default ();
  phy.SetChannel (channel.Create ());

  WifiHelper wifi = WifiHelper::Default ();
  wifi.SetRemoteStationManager ("ns3::AarfWifiManager");

  //NqosWifiMacHelper mac = NqosWifiMacHelper::Default ();
  QosWifiMacHelper mac = QosWifiMacHelper::Default ();

  //ApWifiMac.SetStandard(WIFI_PHY_STANDARD_80211g); ??

  Ssid ssid = Ssid ("ns-3-ssid");
  mac.SetType ("ns3::StaWifiMac",
               "Ssid", SsidValue (ssid),
               "ActiveProbing", BooleanValue (false));

  NetDeviceContainer staDevices;
  staDevices = wifi.Install (phy, mac, wifiStaNodes);

  mac.SetType ("ns3::ApWifiMac",
               "Ssid", SsidValue (ssid));

  NetDeviceContainer apDevices;
  apDevices = wifi.Install (phy, mac, wifiApNode);

  MobilityHelper mobility;

  mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                                 "MinX", DoubleValue (0.0),
                                 "MinY", DoubleValue (0.0),
                                 "DeltaX", DoubleValue (10.0),
                                 "DeltaY", DoubleValue (10.0),
                                 "GridWidth", UintegerValue (3));
                                 //"LayoutType", StringValue ("RowFirst"));

  //mobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
   //                          "Bounds", RectangleValue (Rectangle (-50, 50, -50, 50)));
  //Vector3D position = new Vector3D(0.0, 0.0, 0.0);
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (wifiStaNodes);

  /* Place all the nodes in a circle around the center*/
  int wifiStaNodesCount = 3;
  double phi = 0.0;
  double radius = 10;
  int centerX = 0;
  int centerY = 0;
  int centerZ = 0;
  for (NodeContainer::Iterator j = wifiStaNodes.Begin (); 
    j != wifiStaNodes.End (); 
    j++, phi += (2*3.14)/wifiStaNodesCount)
  {
    Ptr<Node> object = *j;
    Ptr<MobilityModel> position = object->GetObject<MobilityModel> ();

    Vector pos2;
    //std::cout << "Phi: " << phi << std::endl;
    pos2.x = radius * cos(phi) + centerX;
    pos2.y = radius * sin(phi) + centerY;
    pos2.z = 0;
    position->SetPosition(pos2);
  
    //Vector pos = position->GetPosition ();
    //std::cout << "x=" << pos.x << ", y=" << pos.y << ", z=" << pos.z << std::endl;
  }

  /* Set the AP Node at the center of this circle*/
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (wifiApNode); 
  Ptr<Node> APNodeObj = wifiApNode.Get(0);
  Ptr<MobilityModel> position = APNodeObj->GetObject<MobilityModel> ();
  Vector center;
  center.x = centerX;
  center.y = centerY;
  center.z = centerZ;
  position->SetPosition(center);

  InternetStackHelper stack;
  stack.Install (wifiApNode);
  stack.Install (wifiStaNodes);

  Ipv4AddressHelper address;

  address.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer stationInterfaces = address.Assign (staDevices);
  Ipv4InterfaceContainer apInterface = address.Assign (apDevices);



  for (int i = 0; i < 6; i++) {
    uint16_t sinkPort = 50000 + i;
    Address sinkAddress (InetSocketAddress (apInterface.GetAddress (0), sinkPort));
    PacketSinkHelper packetSinkHelper ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), sinkPort));
    ApplicationContainer sinkApps = packetSinkHelper.Install (wifiApNode.Get (0));
    sinkApps.Start (Seconds (0.));
    sinkApps.Stop (Seconds (simulation_time + 1));
    sink[i] = StaticCast<PacketSink> (sinkApps.Get (0));
  }

  for (int i = 0; i < nWifi; i++) {
    for (int j = 0; j < 6; j++) {
      uint16_t sinkPort = 50000 + j;
      Address sinkAddress (InetSocketAddress (apInterface.GetAddress (0), sinkPort));
      OnOffHelper onOffHelper ("ns3::UdpSocketFactory", sinkAddress);
      onOffHelper.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=100]"));
      onOffHelper.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
      onOffHelper.SetAttribute ("DataRate",StringValue ("10Mbps"));
      onOffHelper.SetAttribute ("PacketSize", UintegerValue (1024));
      onOffHelper.SetAttribute ("QosTid", UintegerValue (j + 2));

      ApplicationContainer source;

      source.Add (onOffHelper.Install (wifiStaNodes.Get(i)));
      source.Start (Seconds (0.1));
      source.Stop (Seconds (0.1 + simulation_time));
    }
  }

  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  Simulator::Stop (Seconds (simulation_time + 1));

  if (tracing == true)
    {
      //pointToPoint.EnablePcapAll ("third");
      phy.EnablePcap ("third", apDevices.Get (0));
    }

  Simulator::Run ();
  Simulator::Destroy ();

  for (int i = 0; i < 6; i++) {
    double th = sink[i]->GetTotalRx() * (double) 8/(1e6 * simulation_time);
    std::cout << "up: " << i + 2 << "\t throughput: " << th << " Mbit/s" << std::endl;
  }

  return 0;
}
