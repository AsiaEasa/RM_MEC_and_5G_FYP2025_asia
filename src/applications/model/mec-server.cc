#include "ns3/mec-server.h"
#include "ns3/socket-factory.h"
#include "ns3/simulator.h"
#include "ns3/log.h"

// Defines a logging component for the MecServer class. This allows controlled logging and debugging output in ns-3 using different log levels (e.g., NS_LOG_INFO, NS_LOG_DEBUG).
// Seconed function Ensures that MecServer is properly registered in the ns-3 TypeId system, enabling features like attribute configuration and runtime type checking.
namespace ns3 {

NS_LOG_COMPONENT_DEFINE("MecServer");
NS_OBJECT_ENSURE_REGISTERED(MecServer);

TypeId MecServer::GetTypeId(void) {
  static TypeId tid = TypeId("ns3::MecServer")
    .SetParent<Application>()
    .SetGroupName("Applications")
    .AddConstructor<MecServer>();
  return tid;
}

MecServer::MecServer() {}

MecServer::~MecServer() {}

void MecServer::StartApplication() {
  NS_LOG_INFO("MEC Server Started");

  if (!m_socket) {
    TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
    m_socket = Socket::CreateSocket(GetNode(), tid);
    m_socket->Bind(InetSocketAddress(Ipv4Address::GetAny(), 8080));

    // Fix: Use HandleRead method only if it's declared in mec-server.h
    m_socket->SetRecvCallback(MakeCallback(&MecServer::HandleRead, this));
  }
}

void MecServer::StopApplication() {
  NS_LOG_INFO("MEC Server Stopped");

  if (m_socket) {
    m_socket->Close();
    m_socket = 0;
  }
}

// Fix: Add HandleRead implementation
void MecServer::HandleRead(Ptr<Socket> socket) {
  NS_LOG_INFO("MEC Server received data");
}

} // namespace ns3
