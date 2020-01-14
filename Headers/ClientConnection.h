#ifndef CTP_CLIENTCONNECTION_H
#define CTP_CLIENTCONNECTION_H

#include <queue>
#include <map>
#include "Client.h"
#include "NetworkDevice.h"
#include "ConnectionInfo.h"

class ClientConnection : public NetworkDevice
{
public:
    ClientConnection() = default;
    ~ClientConnection() = default;

    void SendMessageToServer(const char* data, int dataLength);

    void ConnectToIP(const std::string& ipv4, int port);
    void Disconnect();

    ConnectionInfo GetConnectionInfo();

    unsigned int GetUID();
private:
    void ProcessDeviceSpecificEvent(DataPacket* data) override;
    void SendPacket(DataPacket *data) override;
    void UpdateNetworkStats() override;
    Client client;

    unsigned int uid = 0;
    ConnectionInfo connectionInfo;

    std::mutex clientInfoLock;

    std::chrono::steady_clock::time_point timeOfLastPing = std::chrono::steady_clock::now();
    bool waitingForPing = false;
};

#endif //CTP_CLIENTCONNECTION_H