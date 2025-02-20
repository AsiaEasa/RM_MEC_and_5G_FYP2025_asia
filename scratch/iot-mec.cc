/* المسار: /home/user_name/ns-allinone-3.43/ns-3.43/scratch/IOT.cc */
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/mobility-module.h"
#include "ns3/netanim-module.h"
#include "ns3/mec-server.h"
#include "ns3/iot-device.h"
#include <iostream>
#include <vector>
#include <random>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("IoT_FL_Simulation");


void SendDeviceSpecs(Ptr<Socket> socket, IoTDevice device) {
    std::ostringstream msg;
    msg << device.GetCpuFrequency() << "," 
        << device.GetEnergy() << "," 
        << device.GetBandwidth() << "," 
        << device.HasWirelessCharging();
        
    Ptr<Packet> packet = Create<Packet>((uint8_t*) msg.str().c_str(), msg.str().length());
    socket->Send(packet);
    NS_LOG_INFO("Device sent specs: " << msg.str());
}

int main(int argc, char *argv[]) {
    LogComponentEnable("IoT_FL_Simulation", LOG_LEVEL_INFO);
    LogComponentEnable("IoTDevice", LOG_LEVEL_INFO); // ----------------- Add this to allow the iot-device to use it --------
    LogComponentEnable("MecServer", LOG_LEVEL_INFO); // ----------------- Add this to allow the mec-server to use it --------
    int numIoTDevices = 4;

    NodeContainer iotNodes, mecServerNode;
    iotNodes.Create(numIoTDevices);
    mecServerNode.Create(1);

    PointToPointHelper p2p;
    p2p.SetDeviceAttribute("DataRate", StringValue("10Mbps"));
    p2p.SetChannelAttribute("Delay", StringValue("2ms"));

    NetDeviceContainer devices;
    for (int i = 0; i < numIoTDevices; i++) {
        NetDeviceContainer link = p2p.Install(iotNodes.Get(i), mecServerNode.Get(0));
        devices.Add(link);
    }

    InternetStackHelper internet;
    internet.Install(iotNodes);
    internet.Install(mecServerNode);

    Ipv4AddressHelper ipv4;
    ipv4.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces = ipv4.Assign(devices);

    MobilityHelper mobility;
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");

    mobility.Install(iotNodes);
    mobility.Install(mecServerNode);


    for (uint32_t i = 0; i < iotNodes.GetN(); ++i) {
        Ptr<MobilityModel> mob = iotNodes.Get(i)->GetObject<MobilityModel>();
        mob->SetPosition(Vector(i * 10.0, 0.0, 0.0)); // توزيع العقد
    }
    mecServerNode.Get(0)->GetObject<MobilityModel>()->SetPosition(Vector(50.0, 0.0, 0.0)); // وضع السيرفر

    // ✅ إنشاء أجهزة IoT باستخدام دالة GenerateIoTDevices من ملف iot-device.cc
    std::vector<IoTDevice> devicesData = GenerateIoTDevices(numIoTDevices);

    Ptr<MecServer> mecServer = CreateObject<MecServer>();
    mecServerNode.Get(0)->AddApplication(mecServer);
    mecServer->SetStartTime(Seconds(1.0));
    mecServer->SetStopTime(Seconds(15.0));

    uint16_t port = 8080;
    TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
    for (int i = 0; i < numIoTDevices; i++) {
        Ptr<Socket> clientSocket = Socket::CreateSocket(iotNodes.Get(i), tid);
        clientSocket->Connect(InetSocketAddress(interfaces.GetAddress(iotNodes.GetN() - 1), port));
        Simulator::Schedule(Seconds(2.0 + i), &SendDeviceSpecs, clientSocket, devicesData[i]);
    }

    // قبل نحسب استهلاك الطاقه مفروض نضيف النموذج ونرسله للاجهزه وندخل الداتا للاجهزه ويتم التدريب 
    // ------------ADD NEW to compute the energy----------------------
    double avgEnergyArrivalRate = 5.0;
    double tau = 1e-28;
    double mu = 1e22; // مفروض يحددها السيرفر ويحدد معاها عدد دورات التدريب للجهاز
    double G = 7000;

    for (int i = 0; i < numIoTDevices; i++) {
        double energyConsumed = ComputeEnergyConsumption(devicesData[i].GetCpuFrequency(), tau, mu, G);
        int chargingEnergy = GenerateChargingEnergy(avgEnergyArrivalRate);
        devicesData[i].UpdateEnergy(energyConsumed, chargingEnergy);
    }

    // ------------------------------------------------------------------------
    
    AnimationInterface anim("iot-mec-simulation.xml");
    anim.EnablePacketMetadata(true);
    Simulator::Stop(Seconds(20.0));
    Simulator::Run();
    Simulator::Destroy();
    return 0;
}
