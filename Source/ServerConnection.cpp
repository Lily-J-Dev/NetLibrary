#include "ServerConnection.h"
#include <iterator>
#include <iostream>

void ServerConnection::Start(int port)
{
    server.processPacket = std::bind(&ServerConnection::ProcessPacket, this, std::placeholders::_1);
    server.processNewClient = std::bind(&ServerConnection::ProcessNewClient, this, std::placeholders::_1);
    server.processDisconnectedClient = std::bind(&ServerConnection::ProcessDisconnectedClient, this, std::placeholders::_1);
    server.Start(port);
    NetworkDevice::Start();
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

void ServerConnection::ProcessDeviceSpecificEvent(DataPacket *data)
{
    if(data->data[0] == (char)MessageType::PING_RESPONSE)
    {
        using clock = std::chrono::steady_clock;
        clientInfoLock.lock();
        for(ClientInfo& client : connectedClients)
        {
            if(client.uid == data->senderId)
            {
                client.connectionInfo.ping = std::chrono::duration_cast<std::chrono::milliseconds>(clock::now() - client.timeOfLastPing).count();
                client.waitingForPing = false;
                break;
            }
        }
        delete data;
        clientInfoLock.unlock();
    }
}

void ServerConnection::ProcessNewClient(ClientInfo info)
{
    clientInfoLock.lock();
    newClients.push(info);
    connectedClients.push_back(info);
    clientInfoLock.unlock();

    auto packet = new DataPacket();
    packet->dataLength = 1+ sizeof(unsigned int);
    packet->data = new char[packet->dataLength];
    packet->data[0] = (char)MessageType::SET_CLIENT_UID;
    packet->senderId = info.uid;
    auto uidAsChar = reinterpret_cast<const char*>(&info.uid);
    std::copy(uidAsChar, uidAsChar + sizeof(unsigned int), packet->data+1);

    outQueueLock.lock();
    outQueue.push(packet);
    outQueueLock.unlock();
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

void ServerConnection::UpdateNetworkStats()
{
    // Handle pings
    using clock = std::chrono::steady_clock;
    clientInfoLock.lock();
    for(ClientInfo& client : connectedClients) {
        if (!client.waitingForPing &&
            std::chrono::duration_cast<std::chrono::milliseconds>(clock::now() - client.timeOfLastPing).count() >
            PING_FREQUENCY) {
            client.waitingForPing = true;
            auto packet = new DataPacket();
            packet->data = new char[1];
            packet->dataLength = 1;
            packet->data[0] = (char) MessageType::PING_REQUEST;
            packet->senderId = client.uid;
            outQueueLock.lock();
            outQueue.push(packet);
            outQueueLock.unlock();
            client.timeOfLastPing = clock::now();
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