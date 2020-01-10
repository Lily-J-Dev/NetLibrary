#include "ClientDataHandler.h"
#include "Constants.h"

void ClientDataHandler::ConnectToIP(const std::string& ipv4, int port)
{
    client.processPacket = std::bind(&ClientDataHandler::ProcessPacket, this, std::placeholders::_1);
    client.Start(ipv4, port);
}

void ClientDataHandler::SendPacket(DataPacket* packet)
{
    client.SendMessageToServer(packet->data, packet->dataLength);
    //delete packet;
}

void ClientDataHandler::SendMessageToServer(const char *data, int dataLength)
{
    auto packet = new DataPacket();
    packet->data = new char[dataLength];
    packet->dataLength = dataLength;
    std::copy(data, data+dataLength, packet->data);
    ProcessAndSendData(packet);
}