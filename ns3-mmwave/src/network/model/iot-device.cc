#include "iot-device.h"
#include <random>
#include "ns3/mmwave-helper.h"
#include "ns3/mobility-module.h"
#include "ns3/mmwave-point-to-point-epc-helper.h"
#include "ns3/point-to-point-epc-helper.h"
#include "ns3/mmwave-spectrum-value-helper.h"
#include "ns3/point-to-point-helper.h"
#include "ns3/mmwave-ue-net-device.h"
#include "ns3/mmwave-enb-net-device.h"
#include "ns3/node.h"
#include "ns3/net-device-container.h"

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


namespace ns3 {

NS_LOG_COMPONENT_DEFINE("IoTDevice");
// Constructor
IoTDevice::IoTDevice(double cpu, double energy, double bandwidth, bool charge)
    : m_cpuFrequency(cpu), m_energy(energy), m_bandwidth(bandwidth),
      m_wirelessCharging(charge), m_initialEnergy(energy) {}


// Getters
double IoTDevice::GetCpuFrequency() const { return m_cpuFrequency; }
double IoTDevice::GetEnergy() const { return m_energy; }
double IoTDevice::GetBandwidth() const { return m_bandwidth; }
bool IoTDevice::HasWirelessCharging() const { return m_wirelessCharging; }

// Function to generate multiple IoT devices
std::vector<IoTDevice> GenerateIoTDevices(int numDevices) {
    std::vector<IoTDevice> devices;
    std::default_random_engine generator;
    std::uniform_real_distribution<double> cpuDist(0.1, 1.0);
    std::uniform_real_distribution<double> bandwidthDist(0.1, 2.0);

    for (int i = 0; i < numDevices; i++) {
        double cpu = cpuDist(generator);
        double bandwidth = bandwidthDist(generator);
        double energy = 5000 + (cpu * 1000);

        devices.emplace_back(cpu, energy, bandwidth, true);
    }
    return devices;
}


// Function to compute energy consumption
double ComputeEnergyConsumption(double cpuFreq, double tau, double mu, double G) { 
    return cpuFreq * cpuFreq * tau * (mu * 1e6) * G;
}

// Function to simulate energy arrival using Poisson distribution
int GenerateChargingEnergy(double avgRate) {
    std::poisson_distribution<int> distribution(avgRate);
    std::default_random_engine generator;
    return distribution(generator);
}

void IoTDevice::UpdateEnergy(double energyConsumed, int chargingEnergy) {
    NS_LOG_INFO("Device Energy Before Training: " << m_energy << " J");

    double newEnergy = m_energy - energyConsumed;
    NS_LOG_INFO("Device Energy After Training: " << newEnergy << " J (Consumed: " << energyConsumed << " J)");

    // ✅ إذا انخفضت الطاقة أكثر من 50% من الطاقة الأصلية، يبدأ الشحن
    if (newEnergy < 0.5 * m_initialEnergy) {
        int extraCharge = GenerateChargingEnergy(1.0);  // يمكن تعديل معدل الشحن حسب الحاجة
        newEnergy += extraCharge;
        NS_LOG_INFO("Device is charging: +" << extraCharge << " J (after dropping below 50%)");
    }

    m_energy = std::max(newEnergy, 0.0);  // ✅ تجنب القيم السالبة
    NS_LOG_INFO("Device Energy After Charging: " << m_energy << " J");
}

void IoTDevice::ConnectTo5GNetwork(ns3::Ptr<ns3::mmwave::MmWaveHelper> mmwaveHelper,
    ns3::Ptr<ns3::Node> ueNode, 
    ns3::Ptr<ns3::Node> gnbNode) {
mmwaveHelper->SetSchedulerType("ns3::MmWaveFlexTtiMacScheduler");
mmwaveHelper->SetHarqEnabled(true);

ns3::NetDeviceContainer ueDevs = mmwaveHelper->InstallUeDevice(ueNode);
ns3::NetDeviceContainer gnbDevs = mmwaveHelper->InstallEnbDevice(gnbNode);

void AttachToClosestEnb(Ptr<NetDevice> ueDevice);
}




} // namespace ns3
