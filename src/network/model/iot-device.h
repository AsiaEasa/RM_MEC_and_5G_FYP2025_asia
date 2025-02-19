#ifndef IOT_DEVICE_H
#define IOT_DEVICE_H

#include "ns3/core-module.h"

namespace ns3 {

class IoTDevice {
public:
    IoTDevice(double cpu, double energy, double bandwidth, bool charge);
    double GetCpuFrequency() const;
    double GetEnergy() const;
    double GetBandwidth() const;
    bool HasWirelessCharging() const;

private:
    double m_cpuFrequency;
    double m_energy;
    double m_bandwidth;
    bool m_wirelessCharging;
};

std::vector<IoTDevice> GenerateIoTDevices(int numDevices);

} // namespace ns3

#endif // IOT_DEVICE_H
