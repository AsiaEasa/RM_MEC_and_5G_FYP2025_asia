#ifndef MEC_SERVER_H
#define MEC_SERVER_H

#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/socket.h"

// Mec server class is a class that inherits from the Application class
// MecServer represents a network application that can run within an ns-3 simulation
namespace ns3 {

class MecServer : public Application
{
// A static function that returns a TypeId
public:
  static TypeId GetTypeId(void);
  MecServer();
  virtual ~MecServer();
// override functions from the Application class. First one
// to create a socket, bind it to a port, and start receiving data.
// seconed one to clean up resources like closing sockets or canceling events.
protected:
  virtual void StartApplication(); // Called at start
  virtual void StopApplication();  // Called at stop

 private:
  void HandleRead(Ptr<Socket> socket); // Declare missing function
// A Ptr<Socket> (smart pointer to a Socket object) that will be used for network communication.
  Ptr<Socket> m_socket;
// used to schedule or track events in ns-3).
// Likely used for timing-based events like retransmissions or timeouts
  EventId m_event;
};

} // namespace ns3

#endif // MEC_SERVER_H
