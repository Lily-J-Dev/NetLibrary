#include "ServerConnection.h"
#include <iterator>

void ServerConnection::Start(int port)
{
    server.processPacket = std::bind(&ServerConnection::ProcessPacket, this, std::placeholders::_1);
    server.processNewClient = std::bind(&ServerConnection::ProcessNewClient, this, std::placeholders::_1);
    server.processDisconnectedClient = std::bind(&ServerConnection::ProcessDisconnectedClient, this, std::placeholders::_1);
    server.Start(port);
}

void ServerConnection::SendPacket(DataPacket* data)
{
    server.SendMessageToClient(data->data, data->dataLength, data->senderId);
    delete data;
}

// Sends a message to only the specified client
void ServerConnection::SendMessageTo(const char *data, int dataLength, unsigned int clientUid)
{
    auto packet = new DataPacket();
    packet->data = new char[dataLength];
    packet->dataLength = dataLength;
    std::copy(data, data+dataLength, packet->data);
    packet->senderId = clientUid;
    ProcessAndSendData(packet);
}

// Sends a message to all connected clients
void ServerConnection::SendMessageToAll(const char *data, int dataLength)
{
    clientInfoLock.lock();
    for(const ClientInfo &info : connectedClients)
    {
        SendMessageTo(data, dataLength, info.uid);
    }
    clientInfoLock.unlock();
}

// Sends a message to all connected clients excluding the specified client
void ServerConnection::SendMessageToAllExcluding(const char *data, int dataLength, unsigned int clientUid)
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


void ServerConnection::ProcessNewClient(ClientInfo info)
{
    clientInfoLock.lock();
    newClients.push(info);
    connectedClients.push_back(info);
    clientInfoLock.unlock();
}

void ServerConnection::ProcessDisconnectedClient(unsigned int clientUid)
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
bool ServerConnection::AreNewClients()
{
    clientInfoLock.lock();
    bool reBool = !newClients.empty();
    clientInfoLock.unlock();
    return reBool;
}

// Returns the oldest newly connected client (check with AreNewClients before calling this)
ClientInfo ServerConnection::GetNextNewClient()
{
    clientInfoLock.lock();
    auto returnClient = newClients.front();
    newClients.pop();
    clientInfoLock.unlock();
    return returnClient;
}

// Returns true if there are newly disconnected clients
bool ServerConnection::AreDisconnectedClients()
{
    clientInfoLock.lock();
    bool reBool = !disconnectedClients.empty();
    clientInfoLock.unlock();
    return reBool;
}

// Returns the oldest newly disconnected client (check with AreDisconnectedClients before calling this()
ClientInfo ServerConnection::GetNextDisconnectedClient()
{
    clientInfoLock.lock();
    auto returnClient = disconnectedClients.front();
    disconnectedClients.pop();
    clientInfoLock.unlock();
    return returnClient;
}