#include "iot-device.h"
#include <random>

namespace ns3 {

// Constructor
IoTDevice::IoTDevice(double cpu, double energy, double bandwidth, bool charge)
    : m_cpuFrequency(cpu), m_energy(energy), m_bandwidth(bandwidth), m_wirelessCharging(charge) {}

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

} // namespace ns3
