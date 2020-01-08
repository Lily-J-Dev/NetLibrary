#include "ServerDataHandler.h"

void ServerDataHandler::Start()
{
    server.processPacket = std::bind(&ServerDataHandler::ProcessPacket, this,  std::placeholders::_1);
    server.processNewClient = std::bind(&ServerDataHandler::ProcessNewClient, this,  std::placeholders::_1);
    server.processDisconnectedClient = std::bind(&ServerDataHandler::ProcessDisconnectedClient, this,  std::placeholders::_1);
    server.Start();
}

void ServerDataHandler::ProcessAndSendData(const char *data, int dataLength, unsigned int clientUid)
{

}

// Sends a message to only the specified client
void ServerDataHandler::SendMessageTo(const char *data, int dataLength, unsigned int clientUid)
{
    server.SendMessage(data, dataLength, clientUid);
}

// Sends a message to all connected clients
void ServerDataHandler::SendMessageToAll(const char *data, int dataLength)
{
    clientInfoLock.lock();
    for(const ClientInfo &info : connectedClients)
    {
        server.SendMessage(data, dataLength, info.uid);
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
            server.SendMessage(data, dataLength, info.uid);
        }
    }
    clientInfoLock.unlock();
}

// Function that gets called by the client every time a network message is received.
void ServerDataHandler::ProcessPacket(DataPacket* data)
{
    // Here is where the data will be processed (decrypt/uncompress/etc)
    messageLock.lock();
    messages.push(data);
    messageLock.unlock();
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

// Returns true if there are messages pending (get with GetNextMessage)
bool ServerDataHandler::MessagesPending()
{
    messageLock.lock();
    bool reBool = !messages.empty();
    messageLock.unlock();
    return reBool;
}

// Returns the oldest message in the queue. Data returned from this method is no longer used by the library
// so the user must manage its memory (ie delete it when no longer being used.)
DataPacket* ServerDataHandler::GetNextMessage()
{
    messageLock.lock();
    if(messages.empty())
    {
        messageLock.unlock();
        return nullptr;
    }
    auto returnPacket = messages.front();
    messages.pop();
    messageLock.unlock();
    return returnPacket;
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