/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2009 IITP RAS
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
 * This is an example script for AODV manet routing protocol.
 *
 * Authors: Pavel Boyko <boyko@iitp.ru>
 */

#include "ns3/aodv-module.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/wifi-module.h"
#include "ns3/v4ping-helper.h"
#include <iostream>
#include <cmath>
#include "ns3/applications-module.h"

using namespace ns3;

class AodvExample
{
public:
  AodvExample ();
  /// Configure script parameters, \return true on successful configuration
  bool Configure (int argc, char **argv);
  /// Run simulation
  void Run ();


private:

  // parameters
  /// Number of nodes
  uint32_t size;
  /// Distance between nodes, meters
  double step;
  /// Simulation time, seconds
  double totalTime;
  /// Write per-device PCAP traces if true
  bool pcap;
  /// Print routes if true
  bool printRoutes;

  // network
  NodeContainer nodes;
  NetDeviceContainer devices;
  Ipv4InterfaceContainer interfaces;

private:
  void CreateNodes ();
  void CreateDevices ();
  void InstallInternetStack ();
  void InstallApplications ();
};

int main (int argc, char **argv)
{
  AodvExample test;
  if (!test.Configure (argc, argv))
    NS_FATAL_ERROR ("Configuration failed. Aborted.");

  test.Run ();

  return 0;
}

//-----------------------------------------------------------------------------
// This is the constructor which sets the initial parameters
AodvExample::AodvExample () :
  size (5), // This sets the number of nodes
  step (70), // The distance between two nodes is 70 meters
  totalTime (200), // Total simulation time is 200 seconds
  pcap (true), // enables pcap tracing
  printRoutes (true) //enables table capturing
{
}

bool
AodvExample::Configure (int argc, char **argv)
{
  // Enable AODV logs by default. Comment this if too noisy
  // LogComponentEnable("AodvRoutingProtocol", LOG_LEVEL_ALL);

  SeedManager::SetSeed (12345);
  CommandLine cmd;

  cmd.AddValue ("pcap", "Write PCAP traces.", pcap);
  cmd.AddValue ("printRoutes", "Print routing table dumps.", printRoutes);
  cmd.AddValue ("size", "Number of nodes.", size);
  cmd.AddValue ("time", "Simulation time, s.", totalTime);
  cmd.AddValue ("step", "Grid step, m", step);

  cmd.Parse (argc, argv);
  return true;
}

void
AodvExample::Run ()
{
//  Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", UintegerValue (1)); // enable rts cts all the time.
  CreateNodes ();
  CreateDevices ();
  InstallInternetStack ();
  InstallApplications ();

  std::cout << "Starting simulation for " << totalTime << " s ...\n";

  Simulator::Stop (Seconds (totalTime));
  Simulator::Run ();
  Simulator::Destroy ();
}

void
AodvExample::CreateNodes ()
{
  std::cout << "Creating " << (unsigned)size << " nodes " << step << " m apart.\n";
 // create 5 nodes
nodes.Create (size);
  // Set Name for nodes
  for (uint32_t i = 0; i < size; ++i)
    {
      std::ostringstream os;
      os << "node-" << i;
      Names::Add (os.str (), nodes.Get (i));
    }
  // Create static grid

//define the initial location of the nodes

  MobilityHelper mobility;
  // Put everybody into a line with distance step
  Ptr<ListPositionAllocator> initialAlloc =
    CreateObject<ListPositionAllocator> ();
  for (uint32_t i = 0; i < size; ++i) {
      initialAlloc->Add (Vector (step*i, 0., 0.));
  }
  mobility.SetPositionAllocator(initialAlloc);
  mobility.Install(nodes) ;
}

void
// The following function make the nodes wireless.
AodvExample::CreateDevices ()
{
  WifiMacHelper wifiMac;
  wifiMac.SetType ("ns3::AdhocWifiMac");
  YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default ();
  YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default ();
  wifiPhy.SetChannel (wifiChannel.Create ());
  WifiHelper wifi;
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager", "DataMode", StringValue ("OfdmRate6Mbps"), "RtsCtsThreshold", UintegerValue (0));
  devices = wifi.Install (wifiPhy, wifiMac, nodes);

  if (pcap)
    {
      wifiPhy.EnablePcapAll (std::string ("aodv"));
    }
}

void
AodvExample::InstallInternetStack ()
{
  AodvHelper aodv;
  // you can configure AODV attributes here using aodv.Set(name, value)
  InternetStackHelper stack;
  stack.SetRoutingHelper (aodv); // has effect on the next Install ()
  stack.Install (nodes);
  Ipv4AddressHelper address;
  address.SetBase ("10.0.0.0", "255.0.0.0");
  interfaces = address.Assign (devices);

  if (printRoutes)
    {
      Ptr<OutputStreamWrapper> routingStream = Create<OutputStreamWrapper> ("aodv.routes", std::ios::out);

        // This line record the routing table of node 0 at time 10.
      aodv.PrintRoutingTableAt (Seconds(10), nodes.Get(0), routingStream );
        // This line record the routing table of all nodes at time 10
      aodv.PrintRoutingTableAllAt (Seconds(10), routingStream );
      aodv.PrintRoutingTableAllAt (Seconds(50), routingStream );
      aodv.PrintRoutingTableAllAt (Seconds(100), routingStream );
      aodv.PrintRoutingTableAllAt (Seconds(150), routingStream );
      aodv.PrintRoutingTableAllAt (Seconds(200), routingStream );



    }
}

void
// The following function creates a UDP session between nodes 0 and 4 on port 9 which generate constant traffic every 10 seconds.
AodvExample::InstallApplications ()
{

 // First, we create a server which listent on port 9.
  UdpEchoServerHelper echoServer1 (9);

 // Using the following commands, the fourth CSMA node becomes the server of the UDP. It receives packets from the client and then sends them back to the cleint again.
  ApplicationContainer serverApps1 = echoServer1.Install (nodes.Get (4));
  serverApps1.Start (Seconds (0.0));
  serverApps1.Stop (Seconds (200.0));


 UdpEchoClientHelper echoClient1 (interfaces.GetAddress (4), 9);
  //  We set the attribute of this clent. It sends 10!!!100 packet with size 1024 bytes every one!!!!ten second.
  echoClient1.SetAttribute ("MaxPackets", UintegerValue (21)) ;
  echoClient1.SetAttribute ("Interval", TimeValue (Seconds (10)));
  echoClient1.SetAttribute ("PacketSize", UintegerValue (1024));

// We then attach the created client to the third WiFi station. The cleint start sending paclet towards the server at time 1 and stop at time 2.
  ApplicationContainer clientApps1 =  echoClient1.Install (nodes.Get (0));
  clientApps1.Start (Seconds (10));
  clientApps1.Stop (Seconds (200));


  // move node away
  Ptr<Node> node = nodes.Get (0);
  Ptr<MobilityModel> mob = node->GetObject<MobilityModel> ();

  // Using the following line, you are able to move the location of node 0 to (110, 0, 0) at time 30.
  Simulator::Schedule (Seconds (25),  &MobilityModel::SetPosition, mob, Vector (110, 0, 0));
  Simulator::Schedule (Seconds (75),  &MobilityModel::SetPosition, mob, Vector (180, 0, 0));
  Simulator::Schedule (Seconds (125),  &MobilityModel::SetPosition, mob, Vector (250, 0, 0));


}
