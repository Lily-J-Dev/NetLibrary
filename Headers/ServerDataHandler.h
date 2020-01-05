#ifndef CTP_SERVERDATAHANDLER_H
#define CTP_SERVERDATAHANDLER_H

#include <queue>
#include "Server.h"

class ServerDataHandler
{
public:
    void Start();


    void SendMessageTo(const char* data, int dataLength, unsigned int clientUid);
    void SendMessageToAll(const char* data, int dataLength);
    void SendMessageToAllExcluding(const char* data, int dataLength, unsigned int clientUid);

    bool MessagesPending();
    DataPacket* GetNextMessage();

    std::vector<ClientInfo> GetClientInfo() {return connectedClients;} // Gets the connection information for all current clients (Read only)
private:
    void ProcessPacket(DataPacket* data);
    void ProcessNewClient(ClientInfo info);
    void ProcessDisconnectedClient(unsigned int clientUid);

    Server server;
    std::queue<DataPacket*> messages;
    std::vector<ClientInfo> connectedClients;

    std::mutex messageLock;
    std::mutex clientInfoLock;
};


#endif //CTP_SERVERDATAHANDLER_H
