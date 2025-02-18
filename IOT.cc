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

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("IoT_FL_Simulation");

// بنية الجهاز
struct IoTDevice {
    double cpuFrequency; // CPU - cycle freqyency for local training
    double energy; //Energy level for client k
    double bandwidth; // wireless bandwidth for client k
    bool wirelessCharging;

    IoTDevice(double cpu, double e, double bw, bool charge)
        : cpuFrequency(cpu), energy(e), bandwidth(bw), wirelessCharging(charge) {}
};

// إنشاء الاجهزه وبياناتها 
std::vector<IoTDevice> GenerateIoTDevices(int numDevices) {
    std::vector<IoTDevice> devices;
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

    int numIoTDevices = 4;
    NodeContainer sensorNodes, gatewayNode;
    sensorNodes.Create(numIoTDevices);
    gatewayNode.Create(1); // بستبدله بي الخادم الطرفي لامه بيأدي هنا نفس مهامه

    // إعداد الشبكة
    // استبدلو بي الجيل الخامس
    PointToPointHelper p2p;
    p2p.SetDeviceAttribute("DataRate", StringValue("10Mbps"));
    p2p.SetChannelAttribute("Delay", StringValue("2ms"));

    NetDeviceContainer devices;
    for (int i = 0; i < numIoTDevices; i++) {
        devices.Add(p2p.Install(sensorNodes.Get(i), gatewayNode.Get(0)));
    }

    InternetStackHelper internet;
    internet.Install(sensorNodes);
    internet.Install(gatewayNode);

    Ipv4AddressHelper ipv4;
    ipv4.SetBase("10.1.1.0", "255.255.255.0");
    ipv4.Assign(devices);

    // إعداد الحركة للأجهزة
    MobilityHelper mobility;
    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();
    for (int i = 0; i < numIoTDevices; i++) {
        positionAlloc->Add(Vector(i * 10, 20, 0));
    }
    positionAlloc->Add(Vector(50, 50, 0)); // موقع الـ Gateway

    mobility.SetPositionAllocator(positionAlloc);
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(sensorNodes);
    mobility.Install(gatewayNode);

    // إنشاء الأجهزة وطباعة بياناتها
    std::vector<IoTDevice> devicesData = GenerateIoTDevices(numIoTDevices);
    for (int i = 0; i < numIoTDevices; i++) {
        NS_LOG_INFO("Device " << i + 1 << " -> CPU: " << devicesData[i].cpuFrequency 
                               << " GHz, Energy: " << devicesData[i].energy 
                               << " J, Bandwidth: " << devicesData[i].bandwidth 
                               << " Mbps, Wireless Charging: " 
                               << (devicesData[i].wirelessCharging ? "Yes" : "No"));
    }

    // اختيار الأجهزة للتدريب
    // هنا حيدخل كود الخوارزميه
    std::vector<int> selectedDevices;
    for (int i = 0; i < numIoTDevices; i++) {
        if (devicesData[i].energy > 2000 && devicesData[i].cpuFrequency >= 0.2) {
            selectedDevices.push_back(i);
        }
    }
    NS_LOG_INFO("Selected devices for federated training: " << selectedDevices.size());

    // إعداد متغيرات استهلاك وإعادة شحن الطاقة
    double avgEnergyArrivalRate = 5.0;  // معدل وصول الطاقة
    double tau = 1e-28;  // Effective switched capacitance
    double mu = 1e6;  // 1 MB محولة إلى بايت - corresponding training data requirements - دي بحددها السيرفر
    double G = 7000;  //   Number of CPU cycle required to process one bit local data

    // تنفيذ عملية التعلم الفيدرالي وتحديث الطاقة
    for (int id : selectedDevices) {

        double energyConsumed = ComputeEnergyConsumption(devicesData[id].cpuFrequency, tau, mu, G);
        int chargingEnergy = GenerateChargingEnergy(avgEnergyArrivalRate);

        // تحديث حالة الطاقة
        devicesData[id].energy = std::max(devicesData[id].energy - energyConsumed + chargingEnergy, 0.0);

        NS_LOG_INFO("Device " << id + 1 << " trained the model and sent updates, remaining energy: " 
                     << devicesData[id].energy << " J");
    }

    // إعداد التواصل بين الأجهزة والبوابة
    UdpEchoServerHelper echoServer(9);
    ApplicationContainer serverApps = echoServer.Install(gatewayNode.Get(0));
    serverApps.Start(Seconds(1.0));
    serverApps.Stop(Seconds(10.0));

    for (int i = 0; i < numIoTDevices; i++) {
        UdpEchoClientHelper echoClient(Ipv4Address("10.1.1.1"), 9);
        echoClient.SetAttribute("MaxPackets", UintegerValue(5));
        echoClient.SetAttribute("Interval", TimeValue(Seconds(1.0)));
        echoClient.SetAttribute("PacketSize", UintegerValue(512));

        ApplicationContainer clientApps = echoClient.Install(sensorNodes.Get(i));
        clientApps.Start(Seconds(2.0 + i));
        clientApps.Stop(Seconds(10.0));
    }

    // دعم NetAnim
    AnimationInterface anim("iot-simulation.xml");
    for (uint32_t i = 0; i < sensorNodes.GetN(); i++) {
        anim.SetConstantPosition(sensorNodes.Get(i), i * 10, 20);
    }
    anim.SetConstantPosition(gatewayNode.Get(0), 50, 50);

    anim.UpdateNodeColor(sensorNodes.Get(0), 255, 0, 0);  
    anim.UpdateNodeColor(gatewayNode.Get(0), 0, 0, 255);  

    anim.EnablePacketMetadata(true);  
    anim.EnableIpv4RouteTracking("route-tracking.xml", Seconds(0), Seconds(10), Seconds(0.25));

    Simulator::Stop(Seconds(20.0));
    Simulator::Run();
    Simulator::Destroy();

    return 0;
}
