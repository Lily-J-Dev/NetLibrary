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
    ~ClientConnection();

    void SendMessageToServer(const char* data, int dataLength);

    void ConnectToIP(const std::string& ipv4, int port);
    void Disconnect();

    ConnectionInfo GetConnectionInfo();

    unsigned int GetUID();
private:
    void ProcessDeviceSpecificEvent(DataPacket* data) override;
    void SendPacket(DataPacket *data) override;
    Client client;

    unsigned int uid = 0;
    ConnectionInfo connectionInfo;

    std::queue<DataPacket*> outQueue;
    std::mutex outQueueLock;
    std::mutex deleteLock;
    std::mutex clientInfoLock;
    std::atomic_bool running;
};

#endif //CTP_CLIENTCONNECTION_H