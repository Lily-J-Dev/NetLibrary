#ifndef CTP_SERVERCONNECTION_H
#define CTP_SERVERCONNECTION_H

#include "Server.h"
#include "NetworkDevice.h"

class ServerConnection : public NetworkDevice
{
public:

    void Start(int port);

    void SendMessageTo(const char* data, int dataLength, unsigned int clientUid);
    void SendMessageToAll(const char* data, int dataLength);
    void SendMessageToAllExcluding(const char* data, int dataLength, unsigned int clientUid);

    bool AreNewClients();
    ClientInfo GetNextNewClient();

    bool AreDisconnectedClients();
    ClientInfo GetNextDisconnectedClient();

    std::vector<ClientInfo> GetClientInfo() {return connectedClients;} // Gets the connection information for all current clients (Read only)
private:
    void SendPacket(DataPacket* data) override;

    void ProcessNewClient(ClientInfo info);
    void ProcessDisconnectedClient(unsigned int clientUid);
    void ProcessDeviceSpecificEvent(DataPacket* data) override;
    void UpdateNetworkStats() override;

    Server server;
    std::queue<ClientInfo> newClients;
    std::queue<ClientInfo> disconnectedClients;
    std::vector<ClientInfo> connectedClients;

    std::mutex clientInfoLock;
};


#endif //CTP_SERVERCONNECTION_H
