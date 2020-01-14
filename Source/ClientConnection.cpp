#include "ClientConnection.h"
#include "Constants.h"

void ClientConnection::Disconnect()
{
    Stop();
    client.Stop();
}


void ClientConnection::ConnectToIP(const std::string& ipv4, int port)
{
    client.processPacket = std::bind(&ClientConnection::ProcessPacket, this, std::placeholders::_1);
    client.Start(ipv4, port);
    Start();
}

void ClientConnection::SendPacket(DataPacket* data)
{
    client.SendMessageToServer(data->data, data->dataLength);
    delete data;
}

void ClientConnection::SendMessageToServer(const char *data, int dataLength)
{
    auto packet = new DataPacket();
    packet->data = new char[dataLength];
    packet->dataLength = dataLength;
    std::copy(data, data+dataLength, packet->data);
    ProcessAndSendData(packet);
}

void ClientConnection::ProcessDeviceSpecificEvent(DataPacket *data)
{
    if(data->data[0] == (char)MessageType::SET_CLIENT_UID)
    {
        uid = *reinterpret_cast<unsigned int*>(&data->data[1]);
        delete data;
    }
    else if(data->data[0] == (char)MessageType::PING_RESPONSE)
    {
        using clock = std::chrono::steady_clock;
        clientInfoLock.lock();
        connectionInfo.ping = std::chrono::duration_cast<std::chrono::milliseconds>(clock::now() - timeOfLastPing).count();
        waitingForPing = false;
        clientInfoLock.unlock();
        delete data;
    }
}

void ClientConnection::UpdateNetworkStats()
{
    // Handle pings
    using clock = std::chrono::steady_clock;
    clientInfoLock.lock();
    if(!waitingForPing && std::chrono::duration_cast<std::chrono::milliseconds>(clock::now() - timeOfLastPing).count() > PING_FREQUENCY)
    {
        waitingForPing = true;
        auto packet = new DataPacket();
        packet->data = new char[1];
        packet->dataLength = 1;
        packet->data[0] = (char)MessageType::PING_REQUEST;
        outQueueLock.lock();
        outQueue.push(packet);
        outQueueLock.unlock();
        timeOfLastPing = clock::now();
    }
    clientInfoLock.unlock();
}

// Returns a ConnectionInfo struct with information about the current network conditions.
ConnectionInfo ClientConnection::GetConnectionInfo()
{
    clientInfoLock.lock();
    ConnectionInfo returnInfo = connectionInfo;
    clientInfoLock.unlock();
    return returnInfo;
}

// Returns this clients unique id in the network, returns 0 if the uid has not been set yet
unsigned int ClientConnection::GetUID()
{
    clientInfoLock.lock();
    unsigned int returnInfo = uid;
    clientInfoLock.unlock();
    return returnInfo;
}