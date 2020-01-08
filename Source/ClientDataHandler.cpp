#include "ClientDataHandler.h"

void ClientDataHandler::ConnectToIP(const std::string& ipv4, int port)
{
    client.processPacket = std::bind(&ClientDataHandler::ProcessPacket, this, std::placeholders::_1);
    client.Start(ipv4, port);
}

void ClientDataHandler::SendMessage(const char *data, int dataLength)
{
    client.SendMessage(data, dataLength);
}

void ClientDataHandler::ProcessPacket(DataPacket* data)
{
    // Here is where the data will be processed (decrypt/uncompress/etc)
    messageLock.lock();
    messages.push(data);
    messageLock.unlock();
}

DataPacket* ClientDataHandler::GetNextMessage()
{
    messageLock.lock();
    auto returnPacket = messages.front();
    messages.pop();
    messageLock.unlock();
    return returnPacket;
}

bool ClientDataHandler::MessagesPending()
{
    messageLock.lock();
    bool reBool = !messages.empty();
    messageLock.unlock();
    return reBool;
}