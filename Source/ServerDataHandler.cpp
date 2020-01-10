#include "ServerDataHandler.h"
#include <iterator>

void ServerDataHandler::Start(int port)
{
    server.processPacket = std::bind(&ServerDataHandler::ProcessPacket, this,  std::placeholders::_1);
    server.processNewClient = std::bind(&ServerDataHandler::ProcessNewClient, this,  std::placeholders::_1);
    server.processDisconnectedClient = std::bind(&ServerDataHandler::ProcessDisconnectedClient, this,  std::placeholders::_1);
    server.Start(port);
}

void ServerDataHandler::SendPacket(DataPacket* packet)
{
    server.SendMessageToClient(packet->data, packet->dataLength, packet->senderId);
    delete packet;
}

// Sends a message to only the specified client
void ServerDataHandler::SendMessageTo(const char *data, int dataLength, unsigned int clientUid)
{
    auto packet = new DataPacket();
    packet->data = new char[dataLength];
    packet->dataLength = dataLength;
    std::copy(data, data+dataLength, packet->data);
    packet->senderId = clientUid;
    ProcessAndSendData(packet);
}

// Sends a message to all connected clients
void ServerDataHandler::SendMessageToAll(const char *data, int dataLength)
{
    clientInfoLock.lock();
    for(const ClientInfo &info : connectedClients)
    {
        SendMessageTo(data, dataLength, info.uid);
    }
    clientInfoLock.unlock();
}

// Sends a message to all connected clients excluding the specified client
void ServerDataHandler::SendMessageToAllExcluding(const char *data, int dataLength, unsigned int clientUid)
{
    clientInfoLock.lock();
    for(const ClientInfo &info : connectedClients)
    {
        if(info.uid != clientUid)
        {
            SendMessageTo(data, dataLength, info.uid);
        }
    }
    clientInfoLock.unlock();
}


void ServerDataHandler::ProcessNewClient(ClientInfo info)
{
    clientInfoLock.lock();
    newClients.push(info);
    connectedClients.push_back(info);
    clientInfoLock.unlock();
}

void ServerDataHandler::ProcessDisconnectedClient(unsigned int clientUid)
{
    clientInfoLock.lock();

    for(auto it = connectedClients.begin(); it != connectedClients.end(); ++it) {
        if(it->uid == clientUid)
        {
            disconnectedClients.push(*it);
            connectedClients.erase(it);
            break;
        }
    }
    clientInfoLock.unlock();
}



// Returns true if there are newly connected clients (get with GetNextNewClient)
bool ServerDataHandler::AreNewClients()
{
    clientInfoLock.lock();
    bool reBool = !newClients.empty();
    clientInfoLock.unlock();
    return reBool;
}

// Returns the oldest newly connected client (check with AreNewClients before calling this)
ClientInfo ServerDataHandler::GetNextNewClient()
{
    auto returnClient = newClients.front();
    newClients.pop();
    return returnClient;
}

// Returns true if there are newly disconnected clients
bool ServerDataHandler::AreDisconnectedClients()
{
    clientInfoLock.lock();
    bool reBool = !disconnectedClients.empty();
    clientInfoLock.unlock();
    return reBool;
}

// Returns the oldest newly disconnected client (check with AreDisconnectedClients before calling this()
ClientInfo ServerDataHandler::GetNextDisconnectedClient()
{
    auto returnClient = disconnectedClients.front();
    disconnectedClients.pop();
    return returnClient;
}