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
        case MessageType::UPDATE_PEER_CONNECTION_INFO:
        {
            int offset = 1;
            auto lobbyID = event->ReadData<unsigned int>(offset);
            offset += sizeof(unsigned int);
            auto count = event->ReadData<int>(1 + sizeof(unsigned int));
            offset += sizeof(int);

            lobbyLock.lock();
            for(auto& lobby : allLobbies)
            {
                if(lobby.lobbyID != lobbyID)
                    continue;
                if(count > lobby.clientsInRoom)
                    count = lobby.clientsInRoom;
                for (int i = 0; i < count; i++)
                {
                    lobby.memberInfo[i].ping = event->ReadData<float>(offset);
                    offset += sizeof(float);
                }
            }
            lobbyLock.unlock();
            delete event;
            break;
        }
        case MessageType::ADD_NEW_LOBBY:
        {
            lobbyLock.lock();
            allLobbies.emplace_back();
            allLobbies.back().lobbyID = event->ReadData<unsigned int>(1);
            allLobbies.back().maxClientsInRoom = event->ReadData<int>(1 + sizeof(int));
            auto nameLen = event->ReadData<unsigned int>(1 + (sizeof(unsigned int)) * 2);
            allLobbies.back().name = std::string(event->data.data()+1+(sizeof(unsigned int) *3), nameLen);


            lobbyLock.unlock();
            delete event;
            break;
        }
        case MessageType::REMOVE_LOBBY:
        {
            auto lobbyID = event->ReadData<unsigned int>(1);
            lobbyLock.lock();
            for(auto it = allLobbies.begin(); it != allLobbies.end(); ++it)
            {
                if(it->lobbyID == lobbyID)
                {
                    allLobbies.erase(it);
                    break;
                }
            }
            lobbyLock.unlock();
            delete event;
            break;
        }
        case MessageType::SET_ACTIVE_LOBBY:
        {
            lobbyLock.lock();
            activeLobby = *reinterpret_cast<unsigned int*>(&event->data[1]);
            lobbyLock.unlock();
            delete event;

            messageLock.lock();
            ClearQueue();
            messages.emplace();
            messages.back().eventType = NetworkEvent::EventType::ONLOBBYJOIN;
            messageLock.unlock();
            break;
        }
        case MessageType::SET_CLIENT_READY:
        {
            auto lobbyID = event->ReadData<unsigned int>(1);
            auto clientID = event->ReadData<unsigned int>(1+ sizeof(unsigned int));
            auto ready = event->ReadData<bool>(1 + (sizeof(unsigned int) *2));

            lobbyLock.lock();
            for(auto& lobby : allLobbies)
            {
                if(lobby.lobbyID == lobbyID)
                {
                    for(auto& member : lobby.memberInfo)
                    {
                        if(member.uid == clientID)
                        {
                            member.ready = ready;
                            delete event;
                            lobbyLock.unlock();
                            return;
                        }
                    }
                }
            }
            lobbyLock.unlock();
            delete event;
            break;
        }
        case MessageType::NEW_LOBBY_CLIENT:
        {
            auto lobbyID = event->ReadData<unsigned int>(1);
            auto clientID = event->ReadData<unsigned int>(1 + sizeof(unsigned int));
            auto nameLen = event->ReadData<unsigned int>(1 + (sizeof(unsigned int)*2));
            lobbyLock.lock();
            for(Lobby& lobby : allLobbies)
            {
                if(lobby.lobbyID == lobbyID)
                {
                    lobby.clientsInRoom++;
                    lobby.memberInfo.emplace_back();
                    lobby.memberInfo.back().uid = clientID;
                    lobby.memberInfo.back().name = std::string(event->data.data() + 1 + (sizeof(unsigned int)*3), nameLen);
                }
            }
            lobbyLock.unlock();
            delete event;
            break;
        }
        case MessageType::LOBBY_CLIENT_LEFT:
        {
            auto lobbyID = event->ReadData<unsigned int>(1);
            auto clientID = event->ReadData<unsigned int>(1 + sizeof(unsigned int));
            lobbyLock.lock();
            for(auto& lobby : allLobbies)
            {
                if(lobby.lobbyID == lobbyID)
                {
                    for(auto it = lobby.memberInfo.begin(); it != lobby.memberInfo.end(); ++it)
                    {
                        if(it->uid == clientID)
                        {
                            lobby.memberInfo.erase(it);
                            lobby.clientsInRoom--;
                            if(clientID == uid)
                            {
                                activeLobby = 0;
                                messageLock.lock();
                                ClearQueue();
                                messages.emplace();
                                messages.back().eventType = NetworkEvent::EventType::REMOVEDFROMLOBBY;
                                messageLock.unlock();
                            }
                            break;
                        }
                    }
                }
            }
            lobbyLock.unlock();
            delete event;
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

// Returns the currently active lobby
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

// Returns information about all currently open lobbies
std::vector<netlib::Lobby> netlib::ClientConnection::GetAllLobbyInfo()
{
    std::lock_guard<std::mutex> guard(lobbyLock);
    return allLobbies;
}

// Creates a new lobby and adds this client to it, does nothing if the client is already in a lobby.
void netlib::ClientConnection::CreateLobby(std::string lobbyName, int lobbySize)
{
    if(activeLobby != 0)
        return;
    if(lobbySize < 1)
        lobbySize = 1;
    if(lobbyName.size() > MAX_PACKET_SIZE-10)
        lobbyName.resize(MAX_PACKET_SIZE-10);
    auto event = new NetworkEvent();
    event->data.resize(MAX_PACKET_SIZE);
    event->data[0] = (char)MessageType::REQUEST_NEW_LOBBY;
    event->WriteData<int>(lobbySize, 1 + sizeof(int));
    event->WriteData<unsigned int>(lobbyName.size()+1, 1 + (sizeof(unsigned int)*2));
    std::copy(lobbyName.data(), lobbyName.data() + lobbyName.size()+1, event->data.data()+ 1 + (sizeof(unsigned int)*3));
    SendEvent(event);
}

// Adds this client to the given lobby.
void netlib::ClientConnection::JoinLobby(unsigned int lobbyUID)
{
    auto event = new NetworkEvent();
    event->data.resize(MAX_PACKET_SIZE);
    event->data[0] = (char)MessageType::JOIN_LOBBY;
    event->WriteData(lobbyUID, 1);
    SendEvent(event);
}

// Removes the given player from this clients lobby (can be self)
void netlib::ClientConnection::RemoveFromLobby(unsigned int playerUID)
{
    auto event = new NetworkEvent();
    event->data.resize(MAX_PACKET_SIZE);
    event->data[0] = (char)MessageType::REMOVE_FROM_LOBBY;
    event->WriteData(playerUID, 1);
    event->WriteData(activeLobby, 1 + sizeof(unsigned int));
    SendEvent(event);
}

void netlib::ClientConnection::SetReady(bool isReady)
{
    if(activeLobby == 0)
        return;
    auto event = new NetworkEvent();
    event->data.resize(MAX_PACKET_SIZE);
    event->data[0] = (char)MessageType::SET_CLIENT_READY;
    event->WriteData<unsigned int>(activeLobby, 1);
    event->WriteData<unsigned int>(uid, 1 + sizeof(unsigned int));
    event->WriteData<bool>(isReady, 1 + (sizeof(unsigned int)*2));
    SendEvent(event);
}