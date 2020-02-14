#include <iostream>
#include "ClientConnection.h"
#include "Constants.h"

netlib::ClientConnection::ClientConnection()
{
    clientInfoLock = new std::mutex();
}

netlib::ClientConnection::~ClientConnection()
{
    delete clientInfoLock;
}

void netlib::ClientConnection::Disconnect()
{
    Stop();
    client.Stop();
}


bool netlib::ClientConnection::ConnectToIP(const std::string& ipv4, int port)
{
    client.processPacket = std::bind(&ClientConnection::ProcessPacket, this, std::placeholders::_1);
    client.processDisconnect = std::bind(&ClientConnection::ProcessDisconnect, this);
    if(client.Start(ipv4, port))
    {
        Start();
        return true;
    }
    return false;
}

void netlib::ClientConnection::ProcessDisconnect()
{
    messageLock.lock();
    ClearQueue();
    messages.emplace();
    messages.back().eventType = NetworkEvent::EventType::ONDISCONNECT;
    messageLock.unlock();
    Disconnect();
}

void netlib::ClientConnection::SendPacket(NetworkEvent* event)
{
    client.SendMessageToServer(event->data.data(), event->data.size());
    delete event;
}

void netlib::ClientConnection::SendMessageToServer(const std::vector<char>& data)
{
    auto packet = new NetworkEvent();
    packet->data.resize(data.size());
    std::copy(data.data(), data.data() + data.size(), packet->data.data());
    ProcessAndSendData(packet);
}

void netlib::ClientConnection::SendMessageToServer(const char* data, int dataLen)
{
    auto packet = new NetworkEvent();
    packet->data.resize(dataLen);
    std::copy(data, data + dataLen, packet->data.data());
    ProcessAndSendData(packet);
}

void netlib::ClientConnection::ProcessDeviceSpecificEvent(NetworkEvent *event)
{
    switch ((MessageType)event->data[0])
    {
        case MessageType::SET_CLIENT_UID:
        {
            uid = *reinterpret_cast<unsigned int*>(&event->data[1]);
            messageLock.lock();
            ClearQueue();
            messages.emplace();
            messages.front().eventType = NetworkEvent::EventType::ONCONNECT;
            messageLock.unlock();
            delete event;
            break;
        }
        case MessageType::PING_RESPONSE:
        {
            using clock = std::chrono::steady_clock;
            clientInfoLock->lock();
            connectionInfo.ping = static_cast<float>(
                    std::chrono::duration_cast<std::chrono::milliseconds>(clock::now() - timeOfLastPing).count());
            waitingForPing = false;
            clientInfoLock->unlock();
            delete event;
            break;
        }
        case MessageType::ADD_NEW_LOBBY:
        {
            lobbyLock.lock();
            allLobbies.emplace_back();
            allLobbies.back().lobbyID = *reinterpret_cast<unsigned int*>(&event->data[1]);

            unsigned int nameLen = *reinterpret_cast<unsigned int*>(&event->data[1 + sizeof(unsigned int)]);;
            allLobbies.back().name = std::string(event->data.data()+1+(sizeof(unsigned int) *2), nameLen);

            auto test = allLobbies.back();
            std::cout << test.name << std::endl;
            lobbyLock.unlock();
            delete event;
            break;
        }
        case MessageType::SET_ACTIVE_LOBBY:
        {
            lobbyLock.lock();
            activeLobby = *reinterpret_cast<unsigned int*>(&event->data[1 + sizeof(unsigned int)]);
            lobbyLock.unlock();
            delete event;

            messageLock.lock();
            ClearQueue();
            messages.emplace();
            messages.back().eventType = NetworkEvent::EventType::ONLOBBYJOIN;
            messageLock.unlock();
            break;
        }

        default:
        {
            break;
        }
    }
}

void netlib::ClientConnection::UpdateNetworkStats()
{
    // Handle pings
    using clock = std::chrono::steady_clock;
    clientInfoLock->lock();
    if(!waitingForPing && std::chrono::duration_cast<std::chrono::milliseconds>(clock::now() - timeOfLastPing).count() > PING_FREQUENCY)
    {
        waitingForPing = true;
        auto packet = new NetworkEvent();
        packet->data.resize(MAX_PACKET_SIZE);
        packet->data[0] = (char)MessageType::PING_REQUEST;
        outQueueLock.lock();
        outQueue.push(packet);
        outQueueLock.unlock();
        timeOfLastPing = clock::now();
    }
    clientInfoLock->unlock();
}

// Returns a ConnectionInfo struct with information about the current network conditions.
netlib::ConnectionInfo netlib::ClientConnection::GetConnectionInfo()
{
    clientInfoLock->lock();
    ConnectionInfo returnInfo = connectionInfo;
    clientInfoLock->unlock();
    return returnInfo;
}

// Returns this clients unique id in the network, returns 0 if the uid has not been set yet
unsigned int netlib::ClientConnection::GetUID()
{
    clientInfoLock->lock();
    unsigned int returnInfo = uid;
    clientInfoLock->unlock();
    return returnInfo;
}

/// Returns the currently active lobby
netlib::Lobby netlib::ClientConnection::GetCurrentLobbyInfo()
{
    std::lock_guard<std::mutex> guard(lobbyLock);
    for(Lobby& lobby : allLobbies)
    {
        if(lobby.lobbyID == activeLobby)
            return lobby;
    }
    std::cerr << "WARNING: Calling GetCurrentLobbyInfo when the client is not connected to a lobby! Returning an empty Lobby struct." << std::endl;
    return Lobby();
}

std::vector<netlib::Lobby> netlib::ClientConnection::GetAllLobbyInfo()
{
    std::lock_guard<std::mutex> guard(lobbyLock);
    return allLobbies;
}

void netlib::ClientConnection::CreateLobby(std::string lobbyName)
{
    if(lobbyName.size() > MAX_PACKET_SIZE-10)
        lobbyName.resize(MAX_PACKET_SIZE-10);
    auto event = new NetworkEvent();
    event->data.resize(MAX_PACKET_SIZE);
    event->data[0] = (char)MessageType::REQUEST_NEW_LOBBY;

    unsigned int nameSize = lobbyName.size()+1;
    auto sizeAsChar = reinterpret_cast<const char*>(&nameSize);

    std::copy(sizeAsChar, sizeAsChar + sizeof(unsigned int), event->data.data() + 1 + sizeof(unsigned int));
    std::copy(sizeAsChar, sizeAsChar + sizeof(unsigned int), event->data.data() + 1);
    std::copy(lobbyName.data(), lobbyName.data() + lobbyName.size()+1, event->data.data()+ 1 + (sizeof(unsigned int)*2));
    SendEvent(event);
}

void netlib::ClientConnection::JoinLobby(unsigned int lobbyUID)
{
    auto event = new NetworkEvent();
    event->data.resize(MAX_PACKET_SIZE);
    event->data[0] = (char)MessageType::JOIN_LOBBY;
    auto idAsChar = reinterpret_cast<char*>(&lobbyUID);
    std::copy(idAsChar, idAsChar + sizeof(unsigned int), event->data.data()+1);
    SendEvent(event);
}