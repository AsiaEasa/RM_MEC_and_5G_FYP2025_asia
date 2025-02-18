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


// إرسال بيانات الموارد عبر UDP
for (int i = 0; i < numIoTDevices; i++) {
    // استخدم UdpEchoClientHelper لإرسال الحزم
    UdpEchoClientHelper echoClient(Ipv4Address("10.1.1.1"), 9); // عنوان السيرفر ورقم المنفذ
    echoClient.SetAttribute("MaxPackets", UintegerValue(1)); // إرسال حزمة واحدة
    echoClient.SetAttribute("Interval", TimeValue(Seconds(1.0))); // فترة الإرسال بين الحزم
    echoClient.SetAttribute("PacketSize", UintegerValue(512)); // حجم الحزمة (يجب أن يكون أكبر من حجم البيانات التي ترسلها)

    // تخصيص البيانات التي سيتم إرسالها
    Ptr<Packet> packet = Create<Packet>(sizeof(ResourcePacket)); // تخصيص حزمة بناءً على حجم `ResourcePacket`
    Ptr<Packet> udpPacket = Create<Packet>(reinterpret_cast<uint8_t*>(&devicesData[i]), sizeof(ResourcePacket));

    // إرسال الحزمة عبر العميل
    ApplicationContainer clientApps = echoClient.Install(sensorNodes.Get(i));
    clientApps.Start(Seconds(2.0 + i)); // تأخير في البداية لكل جهاز
    clientApps.Stop(Seconds(10.0)); // وقف التطبيق بعد وقت محدد

    // إرسال البيانات الفعلية عبر الـ socket
    Ptr<Socket> socket = Socket::CreateSocket(sensorNodes.Get(i), TypeId::LookupByName("ns3::UdpSocketFactory"));
    socket->SendTo(packet, 0, InetSocketAddress(Ipv4Address("10.1.1.1"), 9)); // إرسال الحزمة إلى السيرفر
}

// إنشاء السيرفر لاستقبال الحزم عبر UDP
UdpEchoServerHelper echoServer(9); // نفس المنفذ الذي حددناه في العميل
ApplicationContainer serverApps = echoServer.Install(gatewayNode.Get(0)); // البوابة هنا تعمل كسيرفر
serverApps.Start(Seconds(1.0)); // بدأ السيرفر بعد ثانية واحدة
serverApps.Stop(Seconds(10.0)); // توقيف السيرفر بعد 10 ثوانٍ

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
