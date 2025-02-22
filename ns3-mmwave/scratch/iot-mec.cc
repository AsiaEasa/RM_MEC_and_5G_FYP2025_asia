/* المسار: /home/user_name/ns-allinone-3.43/ns-3.43/scratch/IOT.cc */
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/point-to-point-epc-helper.h"
#include "ns3/mobility-module.h"
#include "ns3/netanim-module.h"
#include "ns3/mmwave-helper.h"
#include "ns3/mmwave-point-to-point-epc-helper.h"
#include "ns3/mmwave-ue-net-device.h"
#include "ns3/mmwave-enb-net-device.h"
#include "ns3/mmwave-spectrum-value-helper.h"
#include "ns3/mec-server.h"
#include "ns3/iot-device.h"
#include <iostream>
#include <vector>
#include "ns3/point-to-point-helper.h"
#include "ns3/vector.h"
#include "ns3/mmwave-phy-mac-common.h"
#include "ns3/mmwave-phy.h"
#include "ns3/mmwave-mac.h"
#include "ns3/antenna-model.h"
#include "ns3/point-to-point-module.h"
#include "ns3/config-store-module.h"
#include "ns3/command-line.h"
#include "ns3/netanim-module.h"


using namespace ns3;
using namespace mmwave;

NS_LOG_COMPONENT_DEFINE("IoT_5G_Simulation");

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
    LogComponentEnable("IoT_5G_Simulation", LOG_LEVEL_INFO);
    LogComponentEnable("IoTDevice", LOG_LEVEL_INFO); // ----------------- Add this to allow the iot-device to use it --------
    LogComponentEnable("MecServer", LOG_LEVEL_INFO); // ----------------- Add this to allow the mec-server to use it --------

    int numIoTDevices = 4;

    NodeContainer ueNodes, gNbNode, mecServerNode;
    ueNodes.Create(numIoTDevices);
    gNbNode.Create(1);
    mecServerNode.Create(1);

    MobilityHelper mobility;
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(ueNodes);
    mobility.Install(gNbNode);
    mobility.Install(mecServerNode);

    for (uint32_t i = 0; i < ueNodes.GetN(); ++i) {
        ueNodes.Get(i)->GetObject<MobilityModel>()->SetPosition(Vector(i * 10.0, 0.0, 0.0));
        if (!ueNodes.Get(i)->GetObject<MobilityModel>()) {
            ueNodes.Get(i)->AggregateObject(CreateObject<ConstantPositionMobilityModel>());
        }
    }
    gNbNode.Get(0)->GetObject<MobilityModel>()->SetPosition(Vector(50.0, 0.0, 0.0));
    mecServerNode.Get(0)->GetObject<MobilityModel>()->SetPosition(Vector(60.0, 0.0, 0.0));

    Ptr<MmWaveHelper> mmwaveHelper = CreateObject<MmWaveHelper>();
    Ptr<MmWavePointToPointEpcHelper> epcHelper = CreateObject<MmWavePointToPointEpcHelper>();
    mmwaveHelper->SetEpcHelper(epcHelper);

    Ptr<Node> pgw = epcHelper->GetPgwNode();
    
    // ✅ **إضافة eNB إلى EPC لتجنب الخطأ**
    NetDeviceContainer gNbDevice = mmwaveHelper->InstallEnbDevice(gNbNode);
    epcHelper->AssignUeIpv4Address(NetDeviceContainer(gNbDevice));    

    InternetStackHelper internet;
    internet.Install(ueNodes);
    internet.Install(mecServerNode);
    internet.Install(gNbNode);

    PointToPointHelper p2p;
    p2p.SetDeviceAttribute("DataRate", StringValue("10Gbps"));
    p2p.SetChannelAttribute("Delay", StringValue("2ms"));

    NetDeviceContainer mecToPgw = p2p.Install(mecServerNode.Get(0), pgw);
    Ipv4AddressHelper ipv4;
    ipv4.SetBase("10.1.2.0", "255.255.255.0");
    ipv4.Assign(mecToPgw);

    uint16_t mecPort = 4000;
    PacketSinkHelper mecSink("ns3::UdpSocketFactory",
                             InetSocketAddress(Ipv4Address::GetAny(), mecPort));
    ApplicationContainer mecServerApp = mecSink.Install(mecServerNode.Get(0));
    mecServerApp.Start(Seconds(0.1));
    
    NetDeviceContainer ueDevices = mmwaveHelper->InstallUeDevice(ueNodes);
    NetDeviceContainer mecDevice = mmwaveHelper->InstallEnbDevice(mecServerNode);

    // ✅ **طباعة عدد التطبيقات على eNB قبل حدوث الخطأ**
    Ptr<MmWaveEnbNetDevice> enbDev = DynamicCast<MmWaveEnbNetDevice>(gNbDevice.Get(0));
    if (enbDev) {
        Ptr<MmWavePhy> phy = enbDev->GetPhy();
        if (phy) {
            NS_LOG_INFO("PHY موجود ويمكن استخدامه لإعداد الـ Beamforming.");
        } else {
            NS_LOG_ERROR("لم يتم العثور على PHY.");
        }

        // ✅ **طباعة عدد التطبيقات على eNB**
        NS_LOG_INFO("عدد التطبيقات على gNB: " << gNbNode.Get(0)->GetNApplications());
    }

    Ipv4InterfaceContainer ueIpIfaces = epcHelper->AssignUeIpv4Address(NetDeviceContainer(ueDevices));
    for (uint32_t i = 0; i < ueNodes.GetN(); i++) {
        Ptr<Node> ue = ueNodes.Get(i);
        Ptr<Ipv4> ipv4 = ue->GetObject<Ipv4>();
        ipv4->SetForwarding(1, true);
    }
    mmwaveHelper->AttachToClosestEnb(ueDevices, gNbDevice);

    
    // ✅ إنشاء أجهزة IoT باستخدام دالة GenerateIoTDevices من ملف iot-device.cc
    std::vector<IoTDevice> devicesData = GenerateIoTDevices(numIoTDevices);

    Ptr<MecServer> mecServer = CreateObject<MecServer>();
    mecServerNode.Get(0)->AddApplication(mecServer);
    mecServer->SetStartTime(Seconds(1.0));
    mecServer->SetStopTime(Seconds(15.0));

 
    for (uint32_t i = 0; i < ueNodes.GetN(); i++) {
        devicesData[i].ConnectTo5GNetwork(mmwaveHelper, ueNodes.Get(i), gNbNode.Get(0));
    }

    
    ApplicationContainer clientApps;
    for (uint32_t i = 0; i < ueNodes.GetN(); i++) {
        Ptr<Node> ue = ueNodes.Get(i);
        Ipv4Address mecServerAddr = mecServerNode.Get(0)->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal();

        UdpClientHelper mecClient(mecServerAddr, mecPort);
        mecClient.SetAttribute("Interval", TimeValue(MilliSeconds(500)));
        mecClient.SetAttribute("MaxPackets", UintegerValue(1000));

        ApplicationContainer clientApp = mecClient.Install(ue);
        clientApp.Start(Seconds(0.1));
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

    AnimationInterface anim("iot-5g-mec-simulation.xml");

    // تلوين أجهزة IoT بالأزرق
    for (uint32_t i = 0; i < ueNodes.GetN(); i++) {
        anim.UpdateNodeColor(ueNodes.Get(i)->GetId(), 0, 0, 255);
    }

    // تلوين محطة gNB بالأحمر
    anim.UpdateNodeColor(gNbNode.Get(0)->GetId(), 255, 0, 0);

    // تلوين خادم MEC بالأخضر
    anim.UpdateNodeColor(mecServerNode.Get(0)->GetId(), 0, 255, 0);


    anim.EnablePacketMetadata(true);
    Simulator::Stop(Seconds(50.0));
    Simulator::Run();
    Simulator::Destroy();
    return 0;
}
