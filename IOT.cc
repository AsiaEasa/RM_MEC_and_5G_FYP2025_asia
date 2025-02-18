#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/mobility-module.h"
#include "ns3/netanim-module.h"
#include <iostream>
#include <vector>
#include <tuple>
#include <random>
#include <cstdint>  // لاستخدام uint8_t
#include <cstring>  // يمكن أن تكون مفيدة للبعض العمليات على البيانات
#include "ns3/wifi-module.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/mobility-helper.h"


using namespace ns3;

NS_LOG_COMPONENT_DEFINE("IoT_FL_Simulation");
// تعريف هيكل بيانات الحزمة
struct ResourcePacket {
    double cpuFrequency;  // التردد المركزي (GHz)
    double energy;        // الطاقة (Joules)
    double bandwidth;     // عرض النطاق الترددي (Mbps)
    bool wirelessCharging; // هل يوجد شحن لاسلكي

    // Constructor for easy initialization
    ResourcePacket(double cpu, double e, double bw, bool charge)
        : cpuFrequency(cpu), energy(e), bandwidth(bw), wirelessCharging(charge) {}
};

// إنشاء الاجهزه وبياناتها 
std::vector<ResourcePacket> GenerateIoTDevices(int numDevices) {
    std::vector<ResourcePacket> devices;
    std::default_random_engine generator;
    std::uniform_real_distribution<double> cpuDist(0.1, 1.0); // توزيع CPU بين 0.1 و 1.0 GHz
    std::uniform_real_distribution<double> bandwidthDist(0.1, 2.0); // توزيع عرض النطاق بين 0.1 و 2.0 Mbps

    for (int i = 0; i < numDevices; i++) {
        double cpu = cpuDist(generator);
        double bandwidth = bandwidthDist(generator);
        double energy = 5000 + (cpu * 1000); // طاقة ابتدائية تعتمد على CPU

        devices.emplace_back(cpu, energy, bandwidth, true); // الشحن اللاسلكي دائمًا true
    }

    return devices;
}

int numIoTDevices = 4;
std::vector<ResourcePacket> devicesData = GenerateIoTDevices(numIoTDevices);


// دالة لحساب استهلاك الطاقة
double ComputeEnergyConsumption(double cpuFreq, double tau, double mu, double G) {
    return cpuFreq * cpuFreq * tau * (mu * 1e6) * G;
}


// دالة لمحاكاة وصول الطاقة باستخدام توزيع بواسون
// تستبدل بي RF
int GenerateChargingEnergy(double avgRate) {
    std::poisson_distribution<int> distribution(avgRate);
    std::default_random_engine generator;
    return distribution(generator);
}


int main(int argc, char *argv[]) {
    LogComponentEnable("IoT_FL_Simulation", LOG_LEVEL_INFO);

    NodeContainer sensorNodes, gatewayNode, enbNodes;
    sensorNodes.Create(numIoTDevices);
    gatewayNode.Create(1);
    enbNodes.Create(1);

    // تثبيت البروتوكولات على العقد
    InternetStackHelper internet;
    internet.Install(sensorNodes);
    internet.Install(gatewayNode);

    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211n);

    // إعداد القناة الفيزيائية
    YansWifiPhyHelper phy;
    YansWifiChannelHelper channel = YansWifiChannelHelper::Default();
    phy.SetChannel(channel.Create());
    
    // إعداد MAC
    WifiMacHelper mac;
    mac.SetType("ns3::AdhocWifiMac");
    
    // تثبيت أجهزة Wi-Fi على الأجهزة
    NetDeviceContainer wifiDevices;
    wifiDevices = wifi.Install(phy, mac, sensorNodes);
    wifiDevices = wifi.Install(phy, mac, gatewayNode);

    // تعيين عناوين IP
    Ipv4AddressHelper ipv4;
    ipv4.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer wifiInterfaces = ipv4.Assign(wifiDevices);

    MobilityHelper mobility;
    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();
    
    // تحديد مواقع الأجهزة
    for (int i = 0; i < numIoTDevices; i++) {
        positionAlloc->Add(Vector(i * 10, 20, 0));
    }
    
    // إضافة مواقع لعقد الشبكة الأخرى
    positionAlloc->Add(Vector(50, 50, 0)); // gatewayNode
    positionAlloc->Add(Vector(60, 60, 0)); // enbNode
    positionAlloc->Add(Vector(70, 70, 0)); // pgw
    
    mobility.SetPositionAllocator(positionAlloc);
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    
    // تطبيق الحركة على جميع العقد
    mobility.Install(sensorNodes);
    mobility.Install(gatewayNode);
    mobility.Install(enbNodes);
    

    for (int i = 0; i < numIoTDevices; i++) {
        NS_LOG_INFO("Device " << i + 1 << " -> CPU: " << devicesData[i].cpuFrequency 
                               << " GHz, Energy: " << devicesData[i].energy 
                               << " J, Bandwidth: " << devicesData[i].bandwidth 
                               << " Mbps, Wireless Charging: " 
                               << (devicesData[i].wirelessCharging ? "Yes" : "No"));
    }

    std::vector<int> selectedDevices;
    for (int i = 0; i < numIoTDevices; i++) {
        if (devicesData[i].energy > 2000 && devicesData[i].cpuFrequency >= 0.2) {
            selectedDevices.push_back(i);
        }
    }
    NS_LOG_INFO("Selected devices for federated training: " << selectedDevices.size());

    double avgEnergyArrivalRate = 5.0;
    double tau = 1e-28;
    double mu = 1e6;
    double G = 7000;

    for (int id : selectedDevices) {
        double energyConsumed = ComputeEnergyConsumption(devicesData[id].cpuFrequency, tau, mu, G);
        int chargingEnergy = GenerateChargingEnergy(avgEnergyArrivalRate);
        devicesData[id].energy = std::max(devicesData[id].energy - energyConsumed + chargingEnergy, 0.0);
        NS_LOG_INFO("Device " << id + 1 << " trained the model and sent updates, remaining energy: " 
                     << devicesData[id].energy << " J");
    }

    PacketSinkHelper packetSinkHelper("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), 9));
    ApplicationContainer sinkApp = packetSinkHelper.Install(gatewayNode.Get(0));
    sinkApp.Start(Seconds(1.0));
    sinkApp.Stop(Seconds(10.0));
    
    // إعداد تطبيق OnOff لتوليد البيانات
    OnOffHelper onoff("ns3::UdpSocketFactory", InetSocketAddress(wifiInterfaces.GetAddress(0), 9));
    onoff.SetAttribute("DataRate", StringValue("500kbps"));
    onoff.SetAttribute("PacketSize", UintegerValue(sizeof(ResourcePacket)));
    
    for (int i = 0; i < numIoTDevices; i++) {
        ApplicationContainer clientApp = onoff.Install(sensorNodes.Get(i));
        clientApp.Start(Seconds(2.0 + i));
        clientApp.Stop(Seconds(10.0));
    }

    AnimationInterface anim("iot-simulation.xml");
    Simulator::Stop(Seconds(20.0));
    Simulator::Run();
    for (uint32_t i = 0; i < NodeList::GetNNodes(); i++) { // دي تجربه عشان كان في ايرور في الانيميشن فيه عقد مامعمول ليها موبيليتي
        Ptr<Node> node = NodeList::GetNode(i);
        if (node->GetObject<MobilityModel>() == nullptr) {
            std::cout << "تحذير: العقدة " << i << " ليس لديها نموذج حركة!" << std::endl;
        }
    }    
    Simulator::Destroy();
    return 0;
}
