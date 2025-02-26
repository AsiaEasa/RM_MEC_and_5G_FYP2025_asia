/* [0/2] Re-checking...ed directories...
[2/2] Linking CXX....42-test7-default
[INFO] UE 1 متصل بـ 5G eNB
[INFO] UE 2 متصل بـ 5G eNB
[INFO] UE 3 متصل بـ 5G eNB
[INFO] UE 4 متصل بـ 5G eNB
[INFO] الخادم متصل بـ 5G eNB
[DEBUG] Server listening on port 9000
[SUCCESS] UE 1 bound to 10.1.1.1:9000
[DEBUG] UE 1 (IP: 10.1.1.1) sending to 10.1.1.5 on port 9000
[SUCCESS] UE 2 bound to 10.1.1.2:9001
[DEBUG] UE 2 (IP: 10.1.1.2) sending to 10.1.1.5 on port 9000
[SUCCESS] UE 3 bound to 10.1.1.3:9002
[DEBUG] UE 3 (IP: 10.1.1.3) sending to 10.1.1.5 on port 9000
[SUCCESS] UE 4 bound to 10.1.1.4:9003
[DEBUG] UE 4 (IP: 10.1.1.4) sending to 10.1.1.5 on port 9000
[DEBUG] Edge Server IP: 10.1.1.5
[INFO] Packet sent: 1024 bytes from 10.1.1.1 on port 9000
[INFO] Packet sent: 1024 bytes from 10.1.1.2 on port 9001
[INFO] Packet sent: 1024 bytes from 10.1.1.3 on port 9002
[INFO] Packet sent: 1024 bytes from 10.1.1.4 on port 9003
ReceivePacket function called!
[DEBUG] ReceivePacket function called! */
#include "ns3/applications-module.h"
#include "ns3/command-line.h"
#include "ns3/config-store-module.h"
#include "ns3/internet-module.h"
#include "ns3/mmwave-helper.h"
#include "ns3/mobility-module.h"
#include "ns3/netanim-module.h"
#include "ns3/point-to-point-helper.h"
#include <iostream>
#include "ns3/flow-monitor-module.h"


using namespace ns3;
using namespace mmwave;
using namespace std;

NS_LOG_COMPONENT_DEFINE("MmWaveEdgeServerSimulation");


void ReceivePacket(Ptr<Socket> socket)
{
    NS_LOG_INFO("ReceivePacket function called!");
    std::cout << "[DEBUG] ReceivePacket function called!" << std::endl;
    Ptr<Packet> packet;
    Address from;
    while ((packet = socket->RecvFrom(from)))
    {
        InetSocketAddress addr = InetSocketAddress::ConvertFrom(from);
        std::cout << "[INFO] Packet received from " << addr.GetIpv4() 
                  << " on port " << addr.GetPort()
                  << " | Size: " << packet->GetSize() << " bytes" << std::endl;
    }
}

void SendPacket(Ptr<Socket> socket, Ptr<Packet> packet)
{
    Address peerAddress;
    socket->GetSockName(peerAddress);    
    InetSocketAddress peerInetAddr = InetSocketAddress::ConvertFrom(peerAddress);

    socket->Send(packet);
    std::cout << "[INFO] Packet sent: " << packet->GetSize() << " bytes"
              << " from " << peerInetAddr.GetIpv4() << " on port " << peerInetAddr.GetPort()
              << std::endl;
}


int main(int argc, char *argv[])
{
    ns3::PacketMetadata::Enable();
    LogComponentEnable("MmWaveEdgeServerSimulation", LOG_LEVEL_INFO);
    uint16_t numEnb = 1;
    uint16_t numUe = 4;
    double simTime = 20.0;
    uint16_t port = 9000;

    CommandLine cmd;
    cmd.AddValue("numEnb", "Number of eNBs", numEnb);
    cmd.AddValue("numUe", "Number of UEs", numUe);
    cmd.AddValue("simTime", "Total duration of the simulation [s]", simTime);
    cmd.Parse(argc, argv);

    Ptr<MmWaveHelper> mmwaveHelper = CreateObject<MmWaveHelper>();

    NodeContainer enbNodes;
    enbNodes.Create(numEnb);
    
    NodeContainer ueNodes;
    ueNodes.Create(numUe);
    
    NodeContainer edgeServer;
    edgeServer.Create(1);
    
    InternetStackHelper internet;
    internet.Install(ueNodes);
    internet.Install(edgeServer);
    
    MobilityHelper mobility;
    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();
    positionAlloc->Add(Vector(0.0, 100.0, 0.0));
    positionAlloc->Add(Vector(50.0, 100.0, 0.0));
    for (uint32_t i = 0; i < numUe; i++)
    {
        positionAlloc->Add(Vector(20.0 * i, 50.0, 0.0));
    }
    
    mobility.SetPositionAllocator(positionAlloc);
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(enbNodes);
    mobility.Install(edgeServer);
    mobility.SetMobilityModel("ns3::RandomWalk2dMobilityModel", "Bounds", RectangleValue(Rectangle(0, 200, 0, 200)));
    mobility.Install(ueNodes);
    
    NetDeviceContainer enbDevs = mmwaveHelper->InstallEnbDevice(enbNodes);
    NetDeviceContainer ueDevs = mmwaveHelper->InstallUeDevice(ueNodes);
    NetDeviceContainer edgeDevs = mmwaveHelper->InstallUeDevice(edgeServer);
    
    mmwaveHelper->AttachToClosestEnb(ueDevs, enbDevs);
    mmwaveHelper->AttachToClosestEnb(edgeDevs, enbDevs);
    
    for (uint32_t i = 0; i < numUe; i++)
    {
        std::cout << "[INFO] UE " << i + 1 << " متصل بـ 5G eNB\n";
    }
    
    // التأكد من أن الخادم متصل
    if (edgeDevs.GetN() > 0)
    {
        std::cout << "[INFO] الخادم متصل بـ 5G eNB\n";
    }
    else
    {
        std::cout << "[ERROR] فشل اتصال الخادم بشبكة 5G!\n";
    }
    
    Ipv4AddressHelper ipv4;
    ipv4.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer ueIpIface = ipv4.Assign(ueDevs);
    Ipv4InterfaceContainer edgeIpIface = ipv4.Assign(edgeDevs);
    
    Ptr<Socket> serverSocket = Socket::CreateSocket(edgeServer.Get(0), TypeId::LookupByName("ns3::UdpSocketFactory"));
    serverSocket->Bind(InetSocketAddress(Ipv4Address::GetAny(), port));
    serverSocket->SetRecvCallback(MakeCallback(&ReceivePacket));

    std::cout << "[DEBUG] Server listening on port " << port << std::endl;

    
    Simulator::Schedule(Seconds(6.0), &ReceivePacket, serverSocket); // تأكيد استقبال الحزم
    

    for (uint32_t i = 0; i < numUe; i++)
    {
        Ptr<Socket> clientSocket = Socket::CreateSocket(ueNodes.Get(i), TypeId::LookupByName("ns3::UdpSocketFactory"));
    
        // ربط الجهاز بعنوان IP الخاص به
        Ipv4Address localIp = ueIpIface.GetAddress(i);

        if (clientSocket->Bind(InetSocketAddress(localIp, port + i)) == -1) {
            std::cerr << "[ERROR] Failed to bind UE " << i + 1 << " on " << localIp << std::endl;
        } else {
            std::cout << "[SUCCESS] UE " << i + 1 << " bound to " << localIp << ":" << port + i << std::endl;
        }
        
        clientSocket->Connect(InetSocketAddress(edgeIpIface.GetAddress(0), port));
    
        std::cout << "[DEBUG] UE " << i + 1 << " (IP: " << localIp << ") sending to " 
                  << edgeIpIface.GetAddress(0) << " on port " << port << std::endl;
    
        Simulator::Schedule(Seconds(2.0 + i), &SendPacket, clientSocket, Create<Packet>(1024));
    }
    
    std::cout << "[DEBUG] Edge Server IP: " << edgeIpIface.GetAddress(0) << std::endl;
    
    AnimationInterface anim("5g-edge-server.xml");
    anim.UpdateNodeColor(edgeServer.Get(0), 128, 0, 128);
    for (uint32_t i = 0; i < enbNodes.GetN(); ++i)
    {
        anim.UpdateNodeColor(enbNodes.Get(i), 0, 255, 0);
    }
    for (uint32_t i = 0; i < ueNodes.GetN(); ++i)
    {
        anim.UpdateNodeColor(ueNodes.Get(i), 0, 0, 255);
    }
    
    anim.SetMobilityPollInterval(Seconds(0.1));

    // إنشاء أداة لمراقبة التدفق
    Ptr<FlowMonitor> flowMonitor;
    FlowMonitorHelper flowHelper;
    flowMonitor = flowHelper.InstallAll();

    // تشغيل المحاكاة
    Simulator::Stop(Seconds(simTime));
    Simulator::Run();

    // حفظ البيانات وتحليلها بعد انتهاء المحاكاة
    flowMonitor->SerializeToXmlFile("flow-monitor.xml", true, true);

    Simulator::Destroy();


    return 0;
}
