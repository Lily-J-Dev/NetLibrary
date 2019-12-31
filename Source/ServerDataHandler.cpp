#include "ServerDataHandler.h"

void ServerDataHandler::Start()
{
    server.processPacket = std::bind(&ServerDataHandler::ProcessPacket, this,  std::placeholders::_1);
    server.processNewClient = std::bind(&ServerDataHandler::ProcessNewClient, this,  std::placeholders::_1);
    server.Start();
}

void ServerDataHandler::SendMessageTo(const char *data, int dataLength, unsigned int clientUid)
{
    server.SendMessage(data, dataLength, clientUid);
}

void ServerDataHandler::SendMessageToAll(const char *data, int dataLength)
{
    clientInfoLock.lock();
    for(const ClientInfo &info : connectedClients)
    {
        server.SendMessage(data, dataLength, info.uid);
    }
    clientInfoLock.unlock();
}

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

DataPacket* ServerDataHandler::GetNextMessage()
{
    messageLock.lock();
    auto returnPacket = messages.front();
    messages.pop();
    messageLock.unlock();
    return returnPacket;
}

bool ServerDataHandler::MessagesPending()
{
    messageLock.lock();
    bool reBool = !messages.empty();
    messageLock.unlock();
    return reBool;
}

void ServerDataHandler::ProcessNewClient(ClientInfo info)
{
    clientInfoLock.lock();
    connectedClients.push_back(info);
    clientInfoLock.unlock();
}