#ifndef CTP_CLIENTDATAHANDLER_H
#define CTP_CLIENTDATAHANDLER_H

#include <queue>
#include "Client.h"

class ClientDataHandler
{
public:
    ClientDataHandler() = default;
    ~ClientDataHandler() = default;

    void Start();
    void SendMessage(const char* data, int dataLength);

    void ConnectToIP(const std::string& ipv4);
    bool MessagesPending();
    DataPacket* GetNextMessage();

private:
    void ProcessPacket(DataPacket* data);
    Client client;
    std::queue<DataPacket*> messages;
    std::mutex messageLock;
};

#endif //CTP_CLIENTDATAHANDLER_H