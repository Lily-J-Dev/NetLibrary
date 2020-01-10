#ifndef CTP_CLIENTDATAHANDLER_H
#define CTP_CLIENTDATAHANDLER_H

#include <queue>
#include <map>
#include "Client.h"
#include "NetworkDevice.h"

class ClientDataHandler : public NetworkDevice
{
public:
    ClientDataHandler() = default;
    ~ClientDataHandler() = default;

    void SendMessageToServer(const char* data, int dataLength);

    void ConnectToIP(const std::string& ipv4, int port);

private:
    void SendPacket(DataPacket *packet) override;
    Client client;
};

#endif //CTP_CLIENTDATAHANDLER_H