/**
 * CONFIGURATION:
 * Node id:				node0     _____ node1    _____ node2
 * IP address				10.0.0.1        10.0.0.2       10.0.0.3
 * Position Vector (code lines ~335)	(0,0,0)         (50,0,0)	(100,0,0)	meters
 * Tx Power (code lines ~375)		40 mW		40 mW		40 mW 		= 16.0206 dBm 
 * 
 * INFORMATION/PURPOSE:
 * Distances between nodes should be set in such why that node0 and node2 are hidden for themselves, node1 should "connect them". Transmition power node0 / receiver sensitivity should be lowered as well so they are not in their radio range. Traffic should go through node1 if AODV routing is enabled. Some flags can be set at the beginning of main function (lines ~265 and further).
 * 
 * The code is based on several example files (e.g. aodv.cc, fifth.cc, wifi-hidden-terminal.cc) and my additions. I made some modification on on-offapplication.cc, but it works on unchanged ver 3.19 as well.
 * Important code lines (approx)
 * Traffic: 430
 * Sinks: 420
 * AODV: 410
 */
#include "ns3/core-module.h"
#include "ns3/propagation-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/mobility-module.h"
#include "ns3/internet-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/wifi-module.h"
#include "ns3/random-variable-stream.h"
#include "ns3/tag-buffer.h"
#include "ns3/aodv-module.h"

using namespace ns3;

enum RateDistribution
{
  RD_CBR = 0, /** Constant Bit Rate  */
  RD_EXP = 1  /** Exponential Variable Bit Rate (Poisson) */
};

enum L4Protocol
{
  UDP = 0,
  TCP = 1
};

enum TrafficType
{
  AUDIO = 0,    /** packet 160B, priority VO, CBR  */
  VIDEO = 1,    /** packet 1000B, priority VI, CBR  */
  DATA_BE = 2,  /** packet 500B, priority BE, Poisson  */
  DATA_BK = 3,  /** packet 500B, priority BK, Poisson  */
};

class MyApp : public Application 
{
public:

  MyApp ();
  virtual ~MyApp();

  void Setup (uint32_t id, Ptr<Socket> socket, Address address, uint32_t packetSize, uint32_t nPackets, DataRate dataRate, UserPriority userPriority, RateDistribution rateDistribution);

private:
  virtual void StartApplication (void);
  virtual void StopApplication (void);

  void ScheduleTx (void);
  void SendPacket (void);

  Ptr<Socket>     m_socket;
  Address         m_peer;
  uint32_t        m_packetSize;
  uint32_t        m_nPackets;
  DataRate        m_dataRate;
  EventId         m_sendEvent;
  bool            m_running;
  uint32_t        m_packetsSent;
  uint8_t         m_tid;
  UserPriority    m_userPriority;
  RateDistribution m_rateDistribution;
  uint32_t         m_id;
  Time time;
};

MyApp::MyApp ()
  : m_socket (0), 
    m_peer (), 
    m_packetSize (0), 
    m_nPackets (0), 
    m_dataRate (0), 
    m_sendEvent (), 
    m_running (false), 
    m_packetsSent (0),
    m_userPriority (UP_BE),
    m_id (0)
{
}

MyApp::~MyApp()
{
  m_socket = 0;
}

void
MyApp::Setup (uint32_t id, Ptr<Socket> socket, Address address, uint32_t packetSize, uint32_t nPackets, DataRate dataRate, UserPriority userPriority, RateDistribution rateDistribution)
{
  m_id = id;
  m_socket = socket;
  m_peer = address;
  m_packetSize = packetSize;
  m_nPackets = nPackets;
  m_dataRate = dataRate;
  m_userPriority = userPriority;
  m_rateDistribution = rateDistribution;
}

void
MyApp::StartApplication (void)
{
  m_running = true;
  m_packetsSent = 0;
  m_socket->Bind ();
  m_socket->Connect (m_peer);
  SendPacket ();

}

void 
MyApp::StopApplication (void)
{
  m_running = false;

  if (m_sendEvent.IsRunning ())
    {
      Simulator::Cancel (m_sendEvent);
    }

  if (m_socket)
    {
      m_socket->Close ();
    }
}

void 
MyApp::SendPacket (void)
{
  Ptr<Packet> packet = Create<Packet> (m_packetSize);

  QosTag qosTag;
  qosTag.SetUserPriority (m_userPriority);
  packet->AddPacketTag (qosTag);  

  m_socket->Send (packet);

  if (++m_packetsSent < m_nPackets)
    {
      ScheduleTx ();
    }
}

void 
MyApp::ScheduleTx (void)
{
    double period;

    if (m_running)
    {
      switch(m_rateDistribution)
      {      
        case RD_CBR: 
          period = m_packetSize * 8 / static_cast<double> (m_dataRate.GetBitRate ());
          break;
        case RD_EXP:
          Ptr<ExponentialRandomVariable> exp = CreateObject<ExponentialRandomVariable> ();
          double mean = m_packetSize * 8 / static_cast<double> (m_dataRate.GetBitRate ());
          exp->SetAttribute ("Mean", DoubleValue (mean));
          period = exp->GetValue();
          break;
      } 
    Time tNext (Seconds (period));      
    m_sendEvent = Simulator::Schedule (tNext, &MyApp::SendPacket, this);      
     
    }
}

void setTraffic (uint32_t id, Ptr< Node > sourceNode, Address sinkAddress, L4Protocol l4Protocol, TrafficType trafficType, DataRate dataRate, double startTime, double stopTime)
{
  uint32_t numPackets;
  uint32_t packetSize;
  UserPriority userPriority;
  RateDistribution rateDistribution;

  Ptr<Socket> ns3TcpSocket = Socket::CreateSocket (sourceNode, (l4Protocol ? (TcpSocketFactory::GetTypeId ()) : (UdpSocketFactory::GetTypeId ())));
  Ptr<MyApp> app = CreateObject<MyApp> ();

  switch(trafficType)
  {
    case AUDIO:
        packetSize = 160;
        userPriority = UP_VO;
        rateDistribution = RD_CBR;
        break;
    case VIDEO:
        packetSize = 1000;
        userPriority = UP_VI;
        rateDistribution = RD_CBR;
        break;
    case DATA_BE:
        packetSize = 500;
        userPriority = UP_BE;
        rateDistribution = RD_EXP;
        break;
    case DATA_BK:
        packetSize = 500;
        userPriority = UP_BK;
        rateDistribution = RD_EXP;
        break;
/* //MC unused traffic categories
    case VI:
        packetSize = 160;
        userPriority = UP_VI;
        rateDistribution = RD_CBR;
        break;
    case DA_BE:
        packetSize = 160;
        userPriority = UP_BE;
        rateDistribution = RD_CBR;
        break;
    case DA_BK:
        packetSize = 160;
        userPriority = UP_BK;
        rateDistribution = RD_CBR;
        break;
*/
  }  

  numPackets = dataRate.GetBitRate () * (stopTime - startTime) / packetSize / 7;
  app->Setup (id, ns3TcpSocket, sinkAddress, packetSize, numPackets, dataRate, userPriority, rateDistribution);
  sourceNode->AddApplication (app);
  app->SetStartTime (Seconds (startTime));
  app->SetStopTime (Seconds (stopTime));
}

Address setSink(Ptr< Node > sinkNode, Ipv4Address nodeAddress, L4Protocol l4Protocol, uint16_t sinkPort, double startTime, double stopTime)
{
  PacketSinkHelper packetSinkHelper ((l4Protocol ? "ns3::TcpSocketFactory" : "ns3::UdpSocketFactory"), InetSocketAddress (Ipv4Address::GetAny (), sinkPort));

  Address sinkAddress (InetSocketAddress (nodeAddress, sinkPort));
  ApplicationContainer sinkApps = packetSinkHelper.Install (sinkNode);
  sinkApps.Start (Seconds (startTime));
  sinkApps.Stop (Seconds (stopTime));

  return sinkAddress;
}

void initARP (Ptr< Node > sourceNode, Address sinkAddress, double startTime)
{
  Ptr<Socket> ns3TcpSocket = Socket::CreateSocket (sourceNode, UdpSocketFactory::GetTypeId ());
  Ptr<MyApp> app = CreateObject<MyApp> ();
  
  app->Setup (0, ns3TcpSocket, sinkAddress, 100, 1, 0, UP_VO, RD_CBR);
  sourceNode->AddApplication (app);
  app->SetStartTime (Seconds (startTime));
  app->SetStopTime (Seconds (startTime + 0.1));
}


int main (int argc, char **argv)
{
  bool RANDOM_ON = false;	// if set each run will generate another results, if not set each simulation will produce the same values
  bool AODV_ON = true;		// if set AODV routing is enabled, if not set AODV is off (if nodes are hidden flows will not reach destination)
  bool DELAY_ON = false;	// if set delay stats will be printed
  bool JITTER_ON = false;	// if set jitter stats will be printed
  bool enableCtsRts = true;	// if set CTS/RTS frames are generation (hidden nodes protection)
  bool enableQosMac = true;	// if set QoS is set, if not set simple DCF is used
  uint32_t flowId = 0;		// if 0 all flows will be shown, if set 'N' then N first flows will be ommited
  uint32_t flowIdOffset = 0;	
  //LogComponentEnable("AodvRoutingProtocol", LOG_LEVEL_ALL);
  //LogComponentEnable("AodvRoutingTable", LogLevel(LOG_LEVEL_ALL | LOG_PREFIX_NODE));

  
  SeedManager RngSeedManager;
  if(RANDOM_ON){
    srand (time (NULL));
    int runNumber = rand () % 1000; 
    int seedNumber = rand () % 1000; 
    RngSeedManager.SetRun(runNumber);
    RngSeedManager.SetSeed(seedNumber);
  }else{
    RngSeedManager.SetRun(1);
    RngSeedManager.SetSeed(1);
  }
/* Random variables test code
  UniformVariable a;
  UniformVariable b;40 mW
  std::cout << "--------------- \nSome random numbers:\n";
      for (int i = 0; i < 5; i++)
      {
	NS_LOG_UNCOND ("a: " << a.GetInteger(0, 99) << ", b: " << b.GetInteger(0, 99));
	cout << " rand(): " << rand();
      }

  std::cout << "\n---------------\n";
*/

  uint32_t totalRxBytes = 0;
//  double freq;
//  double width;

  Histogram hist;


  // 0. Enable or disable CTS/RTS
  UintegerValue ctsThr = (enableCtsRts ? UintegerValue (100) : UintegerValue (2200));
  UintegerValue segmentSize = 1400;
  UintegerValue delAckCount = 10;
  UintegerValue initialCwnd = 10;
  TimeValue     delAckTimeout = Seconds(0.1);

  Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", ctsThr);

  // 1. Create 3 nodes 
  NodeContainer nodes;
  nodes.Create (3);

  // 2. Place nodes somehow, this is required by every wireless simulation
/*
  for (size_t i = 0; i < 3; ++i)
  {
      nodes.Get (i)->AggregateObject (CreateObject<ConstantPositionMobilityModel> ());
  }

// nodes.Get(0)->SetAttribute("TxPower", DoubleValue (23.0));
// nodes.Get(2)->SetAttribute("TxPower", DoubleValue (23.0)); 
*/
/*
  Ptr<MobilityModel> mobility;

  mobility = nodes.Get (0) ->GetObject<MobilityModel> ();
  mobility->SetPosition (Vector(0,0,0));

  mobility = nodes.Get (1) ->GetObject<MobilityModel> ();
  mobility->SetPosition (Vector(60,0,0));

  mobility = nodes.Get (2) ->GetObject<MobilityModel> ();
  mobility->SetPosition (Vector(120,0,0));
*/

MobilityHelper mobility;
Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
positionAlloc->Add (Vector (0.0, 0.0, 0.0));
positionAlloc->Add (Vector (60.0, 0.0, 0.0));
positionAlloc->Add (Vector (120.0, 0.0, 0.0));
mobility.SetPositionAllocator (positionAlloc);
mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
mobility.Install (nodes);

  // 3. Create propagation loss matrix
/**  
  Ptr<MatrixPropagationLossModel> lossModel = CreateObject<MatrixPropagationLossModel> ();
  lossModel->SetDefaultLoss (200); 
  lossModel->SetLoss (nodes.Get (0)->GetObject<MobilityModel>(), nodes.Get (1)->GetObject<MobilityModel>(), 60); 
  lossModel->SetLoss (nodes.Get (2)->GetObject<MobilityModel>(), nodes.Get (1)->GetObject<MobilityModel>(), 60); 
  lossModel->SetLoss (nodes.Get (0)->GetObject<MobilityModel>(), nodes.Get (2)->GetObject<MobilityModel>(), 160); 
**/
  // 
  
  // 4. Create & setup wifi channel
  Ptr<YansWifiChannel> wifiChannel = CreateObject <YansWifiChannel> ();
 // wifiChannel->SetPropagationLossModel (CreateObject <FriisPropagationLossModel> ());
  wifiChannel->SetPropagationLossModel (CreateObject <LogDistancePropagationLossModel> ());
  wifiChannel->SetPropagationDelayModel (CreateObject <ConstantSpeedPropagationDelayModel> ());

  
/*  
  YansWifiChannelHelper wifiChannel;
  wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
  wifiChannel.AddPropagationLoss ("ns3::FriisPropagationLossModel");
*/
  
  // 5. Install wireless devices
  WifiHelper wifi;
  wifi.SetStandard (WIFI_PHY_STANDARD_80211b);
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager", 
                                "DataMode",StringValue ("DsssRate11Mbps"), 
                                "ControlMode",StringValue("DsssRate11Mbps"));
  YansWifiPhyHelper wifiPhy =  YansWifiPhyHelper::Default ();
  wifiPhy.SetChannel (wifiChannel);
  
  // transmission power: 40 mW = 16.0206 dBm

  wifiPhy.Set ("TxPowerStart", DoubleValue(16.0206));
  wifiPhy.Set ("TxPowerEnd", DoubleValue(16.0206));
  wifiPhy.Set ("TxPowerLevels", UintegerValue(1));
  wifiPhy.Set ("TxGain", DoubleValue(0) );
  wifiPhy.Set ("RxGain", DoubleValue(0) );

//Alternatively you can change the energy detection threshold to define the radio range, e,g,,

//  phy.Set ("EnergyDetectionThreshold", DoubleValue(-71.9842));
//  phy.Set ("CcaMode1Threshold", DoubleValue(-74.9842));
  
  
  

  QosWifiMacHelper qosWifiMac = QosWifiMacHelper::Default (); 
  NqosWifiMacHelper nqosWifiMac = NqosWifiMacHelper::Default ();

  NetDeviceContainer devices;
  
  if(enableQosMac)
  {
    qosWifiMac.SetType ("ns3::AdhocWifiMac");
    devices = wifi.Install (wifiPhy, qosWifiMac, nodes);
  }
  else
  {
    nqosWifiMac.SetType ("ns3::AdhocWifiMac");
    devices = wifi.Install (wifiPhy, nqosWifiMac, nodes);
  }

  wifiPhy.EnablePcap ("qoss", nodes);


  // 6. Install TCP/IP stack & assign IP addresses & use AODV
  InternetStackHelper internet;
  if(AODV_ON){
    AodvHelper aodv;
    // you can configure AODV attributes here using aodv.Set(name, value)
    internet.SetRoutingHelper (aodv); // has effect on the next Install ()
  }
  internet.Install (nodes);
  Ipv4AddressHelper ipv4;
  ipv4.SetBase ("10.0.0.0", "255.0.0.0");
  Ipv4InterfaceContainer interfaces = ipv4.Assign (devices);

  // 7. Install applications
  /// Info: setTraffic (uint32_t id, Ptr< Node > sourceNode, Address sinkAddress, L4Protocol l4Protocol, TrafficType trafficType, DataRate dataRate, double startTime, double stopTime)
  Address sinkAddress0;
  Address sinkAddress1;
  Address sinkAddress2;
  sinkAddress0 = setSink(nodes.Get (0), interfaces.GetAddress(0), UDP, 8080, 0.11, 15.11);
  sinkAddress1 = setSink(nodes.Get (1), interfaces.GetAddress(1), UDP, 8080, 0.12, 15.12);
  sinkAddress2 = setSink(nodes.Get (2), interfaces.GetAddress(2), UDP, 8080, 0.13, 15.13);
  
///Some test flows below:
  initARP (nodes.Get (2), sinkAddress1, 0.21);
  initARP (nodes.Get (2), sinkAddress1, 0.22);
  
  initARP (nodes.Get (0), sinkAddress1, 0.31);
  initARP (nodes.Get (0), sinkAddress1, 0.32);
  
  initARP (nodes.Get (0), sinkAddress2, 0.41);
  initARP (nodes.Get (0), sinkAddress2, 0.42);

  initARP (nodes.Get (1), sinkAddress0, 0.51);
  initARP (nodes.Get (1), sinkAddress0, 0.52);
 
  initARP (nodes.Get (1), sinkAddress2, 0.61);
  initARP (nodes.Get (1), sinkAddress2, 0.62);
  
///Main Traffic 3 and 4
  ///setTraffic (1, nodes.Get (0), sinkAddress1, UDP, DATA_BK, DataRate ("5000kbps"), 1., 11.);
  ///setTraffic (2, nodes.Get (2), sinkAddress1, UDP, DATA_BE, DataRate ("3000kbps"), 1., 11.);
  setTraffic (3, nodes.Get (0), sinkAddress2, UDP, DATA_BK, DataRate ("1600kbps"), 1.11, 11.11);
  setTraffic (4, nodes.Get (0), sinkAddress2, UDP, AUDIO, DataRate ("256kbps"), 1.12, 11.12);
  
  
  // 8. Install FlowMonitor on all nodes
  FlowMonitorHelper flowmon;
  Ptr<FlowMonitor> monitor = flowmon.InstallAll ();
  monitor->CheckForLostPackets(Seconds(0.1));
  

  // 9. Run simulation for X seconds
  Simulator::Stop (Seconds (12));
  Simulator::Run ();

  // 10. Print per flow statistics
  monitor->SerializeToXmlFile("FlowMonitor.xml",true, true);

  std::cout << "------------------------------------------------\n";
  if(enableCtsRts) std::cout << "RTS/CTS enabled\n\n"; 
  else std::cout << "RTS/CTS disabled\n\n";
  if(enableQosMac) std::cout << "Qos Wifi Mac\n\n"; 
  else std::cout << "Non-Qos Wifi Mac\n\n";
  if(AODV_ON) std::cout << "AODV enabled\n\n"; 
  else std::cout << "AODV disabled\n\n";

  Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmon.GetClassifier ());
  std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats ();

  for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin (); i != stats.end (); ++i)
  if(i->first > flowId) //Some stats
  {
      Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (i->first);
      std::cout << "\nFlow " << (i->first - flowIdOffset) << " (" << t.sourceAddress << " -> "<< t.destinationAddress << ")\n";       
    
      std::cout << "  Tx Packets:   " << i->second.txPackets << "\n";
      std::cout << "  Rx Packets:   " << i->second.rxPackets << "\n\n";
      std::cout << "  Lost packets: " << (i->second.lostPackets) << "\n\n";

      if(DELAY_ON){
	double count;
	std::cout << "  Avg delay: " << ((i->second.delaySum)/(i->second.rxPackets))/1000000 << "ms\n";

	std::cout << "  Delay Histogram:\n";
	hist = i->second.delayHistogram;
	//width = 1000 * hist.GetBinWidth (0); // All bins have the same //width

	count = hist.GetBinCount (0);   
	//freq = count / (i->second.rxPackets) * 100; // Normalize to 100          
	std::cout << "  0-1 ms -> " << count << "\n";// << " packets (" << //freq << "%)\n";

	count = 0;
	for (uint32_t index = 1; index < 10; index++)
	{  
	  if(index < hist.GetNBins ()) count += hist.GetBinCount (index);
	}     
	//freq = count / (i->second.rxPackets) * 100; // Normalize to 100          
	std::cout << "  1-10 ms -> " << count << "\n";// << " packets (" << //freq << "%)\n";

	count =0;
	for (uint32_t index = 10; index < 100; index++)
	{  
	  if(index < hist.GetNBins ()) count += hist.GetBinCount (index);
	}      
	//freq = count / (i->second.rxPackets) * 100; // Normalize to 100          
	std::cout << "  10-100 ms -> " << count << "\n";// << " packets (" << //freq << "%)\n";
	
	count = 0;
	for (uint32_t index = 100; index < 500; index++)
	{  
	  if(index < hist.GetNBins ()) count += hist.GetBinCount (index);
	}      
	//freq = count / (i->second.rxPackets) * 100; // Normalize to 100          
	std::cout << "  100-500 ms -> " << count << "\n";// << " packets (" << //freq << "%)\n";
	
	count = 0;
	for (uint32_t index = 500; index < hist.GetNBins (); index++)
	{  
	  if(index < hist.GetNBins ()) count += hist.GetBinCount (index);
	}      
	//freq = count / (i->second.rxPackets) * 100; // Normalize to 100          
	std::cout << "  > 500 ms -> " << count << "\n\n";// << " packets (" << //freq << "%)\n\n";
      }

      if(JITTER_ON){
	double count;
	std::cout << "  Avg jitter: " << ((i->second.jitterSum)/(i->second.rxPackets))/1000000 << "ms\n";
	std::cout << "  Jiter Histogram:\n";
	hist = i->second.jitterHistogram;
	//width = 1000 * hist.GetBinWidth (0); // All bins have the same //width

	count = hist.GetBinCount (0);   
	//freq = count / (i->second.rxPackets) * 100; // Normalize to 100          
	std::cout << "  0-1 ms -> " << count << "\n";// << " packets (" << //freq << "%)\n";

	count = 0;
	for (uint32_t index = 1; index < 10; index++)
	{  
	  if(index < hist.GetNBins ()) count += hist.GetBinCount (index);
	}      
	//freq = count / (i->second.rxPackets) * 100; // Normalize to 100          
	std::cout << "  1-10 ms -> " << count << "\n";// << " packets (" << //freq << "%)\n";

	count = 0;
	for (uint32_t index = 10; index < 100; index++)
	{  
	  if(index < hist.GetNBins ()) count += hist.GetBinCount (index);
	}      
	//freq = count / (i->second.rxPackets) * 100; // Normalize to 100          
	std::cout << "  10-100 ms -> " << count << "\n";// << " packets (" << //freq << "%)\n";
	
	count = 0;
	for (uint32_t index = 100; index < 500; index++)
	{  
	  if(index < hist.GetNBins ()) count += hist.GetBinCount (index);
	}      
	//freq = count / (i->second.rxPackets) * 100; // Normalize to 100          
	std::cout << "  100-500 ms -> " << count << "\n";// << " packets (" << //freq << "%)\n";
	
	count = 0;
	for (uint32_t index = 500; index < hist.GetNBins (); index++)
	{  
	  if(index < hist.GetNBins ()) count += hist.GetBinCount (index);
	}      
	//freq = count / (i->second.rxPackets) * 100; // Normalize to 100          
	std::cout << "  > 500 ms -> " << count << "\n";// << " packets (" << //freq << "%)\n";
      }
      totalRxBytes += (i->second.rxBytes+36*i->second.rxPackets);

  }
  
  std::cout << "\n--------------------\n";
  Ptr<OutputStreamWrapper> routingStream = Create<OutputStreamWrapper> ("aodv.routes", std::ios::out);
  
  // 11. Cleanup
  Simulator::Destroy ();

 return 0;
}
