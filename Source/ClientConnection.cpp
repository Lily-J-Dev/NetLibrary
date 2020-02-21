#include <iostream>
#include <algorithm>
#include "ClientConnection.h"
#include "Constants.h"

netlib::ClientConnection::ClientConnection()
{

}

netlib::ClientConnection::~ClientConnection()
{

}

void netlib::ClientConnection::Disconnect()
{
    if(client.IsRunning())
    {
        Stop();
        client.Stop();
    }
}


bool netlib::ClientConnection::ConnectToIP(const std::string& ipv4, unsigned short port)
{
    if(client.IsRunning())
        return false;
    std::string ip = ipv4;
    if(ip == "localhost")
        ip = "127.0.0.1";
    client.processPacket = std::bind(&ClientConnection::ProcessPacket, this, std::placeholders::_1);
    client.processDisconnect = std::bind(&ClientConnection::ProcessDisconnect, this);
    if(client.Start(ip, port))
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
    messages.back().eventType = NetworkEvent::EventType::ON_DISCONNECT;
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
    if(data.empty())
        return;
    auto packet = new NetworkEvent();
    packet->data.resize(data.size());
    std::copy(data.data(), data.data() + data.size(), packet->data.data());
    ProcessAndSendData(packet);
}

void netlib::ClientConnection::SendMessageToServer(const char* data, int dataLen)
{
    if(dataLen == 0)
        return;
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
            messages.front().eventType = NetworkEvent::EventType::ON_CONNECT;
            messageLock.unlock();
            delete event;
            break;
        }
        case MessageType::PING_RESPONSE:
        {
            using clock = std::chrono::steady_clock;
            clientInfoLock.lock();
            connectionInfo.ping = static_cast<float>(
                    std::chrono::duration_cast<std::chrono::milliseconds>(clock::now() - timeOfLastPing).count());
            waitingForPing = false;
            clientInfoLock.unlock();
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
            if(allLobbies.count(lobbyID) > 0)
            {
                if(count > allLobbies[lobbyID].clientsInRoom)
                    count = allLobbies[lobbyID].clientsInRoom;
                for (int i = 0; i < count; i++)
                {
                    allLobbies[lobbyID].memberInfo[i].ping = event->ReadData<float>(offset);
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
            unsigned int id = event->ReadData<unsigned int>(1);
            allLobbies[id].lobbyID = id;
            allLobbies[id].maxClientsInRoom = event->ReadData<int>(1 + sizeof(int));
            auto nameLen = event->ReadData<unsigned int>(1 + (sizeof(unsigned int)) * 2);
            allLobbies[id].name = std::string(event->data.data()+1+(sizeof(unsigned int) *3), nameLen);


            lobbyLock.unlock();
            delete event;
            break;
        }
        case MessageType::REMOVE_LOBBY:
        {
            auto lobbyID = event->ReadData<unsigned int>(1);
            lobbyLock.lock();
            if(allLobbies.count(lobbyID) > 0)
                allLobbies.erase(lobbyID);
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
            messages.back().eventType = NetworkEvent::EventType::ON_LOBBY_JOIN;
            messageLock.unlock();
            break;
        }
        case MessageType::SET_LOBBY_SLOT:
        {
            unsigned int lobbyID = *reinterpret_cast<unsigned int*>(&event->data[1]);
            unsigned int playerID = *reinterpret_cast<unsigned int*>(&event->data[1 + sizeof(unsigned int)]);
            unsigned int slot = *reinterpret_cast<unsigned int*>(&event->data[1 + (sizeof(unsigned int)*2)]);
            for(LobbyMember& member : allLobbies[lobbyID].memberInfo)
            {
                if(member.uid == playerID)
                    member.lobbySlot = slot;
            }
            std::sort(allLobbies[lobbyID].memberInfo.begin(),allLobbies[lobbyID].memberInfo.end(), netlib::LobbyMember::Sort);
            delete event;
            break;
        }
        case MessageType::SET_CLIENT_READY: {
            auto lobbyID = event->ReadData<unsigned int>(1);
            auto clientID = event->ReadData<unsigned int>(1 + sizeof(unsigned int));
            auto ready = event->ReadData<bool>(1 + (sizeof(unsigned int) * 2));

            lobbyLock.lock();

            if (allLobbies.count(lobbyID) > 0)
            {
                for (auto &member : allLobbies[lobbyID].memberInfo)
                {
                    if (member.uid == clientID)
                    {
                        member.ready = ready;
                        delete event;
                        lobbyLock.unlock();
                        return;
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
            if(allLobbies.count(lobbyID) > 0)
            {

                allLobbies[lobbyID].clientsInRoom++;
                allLobbies[lobbyID].memberInfo.emplace_back();
                allLobbies[lobbyID].memberInfo.back().uid = clientID;
                allLobbies[lobbyID].memberInfo.back().name = std::string(event->data.data() + 1 + (sizeof(unsigned int)*3), nameLen);
            }
            lobbyLock.unlock();
            delete event;
            break;
        }
        case MessageType::SET_LOBBY_OPEN:
        {
            auto lobbyID = event->ReadData<unsigned int>(1);
            auto open = event->ReadData<bool>(1 + (sizeof(unsigned int) *2));
            lobbyLock.lock();

            // If this client is not in the lobby that has been closed, remove it from the list
            if(!open && activeLobby != lobbyID)
            {
                allLobbies.erase(lobbyID);
            }
            if(activeLobby == lobbyID && allLobbies.count(lobbyID) > 0)
            {
                allLobbies[lobbyID].open = open;
            }
            lobbyLock.unlock();
            delete event;
            break;
        }
        case MessageType::SET_LOBBY_NAME:
        {
            auto lobbyID = event->ReadData<unsigned int>(1);
            auto clientID = event->ReadData<unsigned int>(1+ sizeof(unsigned int));
            auto nameLen = event->ReadData<unsigned int>(1+ (sizeof(unsigned int)*2));
            lobbyLock.lock();
            if(allLobbies.count(lobbyID))
            {
                for(LobbyMember& member : allLobbies[lobbyID].memberInfo)
                {
                    if(member.uid == clientID)
                    {
                        member.name = std::string(event->data.data() + 1 + (sizeof(unsigned int)*3), nameLen);
                    }
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
            if(allLobbies.count(lobbyID) > 0)
            {
                for(auto it = allLobbies[lobbyID].memberInfo.begin(); it != allLobbies[lobbyID].memberInfo.end(); ++it)
                {
                    if(it->uid == clientID)
                    {
                        allLobbies[lobbyID].memberInfo.erase(it);
                        allLobbies[lobbyID].clientsInRoom--;
                        if(clientID == uid)
                        {
                            activeLobby = 0;
                            messageLock.lock();
                            ClearQueue();
                            messages.emplace();
                            messages.back().eventType = NetworkEvent::EventType::REMOVED_FROM_LOBBY;
                            messageLock.unlock();
                        }
                        break;
                    }
                }
                std::sort(allLobbies[lobbyID].memberInfo.begin(),allLobbies[lobbyID].memberInfo.end(), netlib::LobbyMember::Sort);
            }
            lobbyLock.unlock();
            delete event;
            break;
        }
        default:
        {
            delete event;
            break;
        }
    }
}

void netlib::ClientConnection::UpdateNetworkStats()
{
    // Handle pings
    using clock = std::chrono::steady_clock;
    clientInfoLock.lock();
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
    clientInfoLock.unlock();
}

// Returns a ConnectionInfo struct with information about the current network conditions.
netlib::ConnectionInfo netlib::ClientConnection::GetConnectionInfo()
{
    clientInfoLock.lock();
    ConnectionInfo returnInfo = connectionInfo;
    clientInfoLock.unlock();
    return returnInfo;
}

// Returns this clients unique id in the network, returns 0 if the uid has not been set yet
unsigned int netlib::ClientConnection::GetUID()
{
    clientInfoLock.lock();
    unsigned int returnInfo = uid;
    clientInfoLock.unlock();
    return returnInfo;
}

// Returns the currently active lobby
netlib::Lobby netlib::ClientConnection::GetCurrentLobbyInfo()
{
    std::lock_guard<std::mutex> guard(lobbyLock);
    if(allLobbies.count(activeLobby) > 0)
        return allLobbies[activeLobby];
    else
    {
        std::cerr << "WARNING: Calling GetCurrentLobbyInfo when the client is not connected to a lobby! Returning an empty Lobby struct."
                  << std::endl;
        return Lobby();
    }
}

// Returns information about all currently open lobbies
std::vector<netlib::Lobby> netlib::ClientConnection::GetAllLobbyInfo()
{
    std::lock_guard<std::mutex> guard(lobbyLock);
    std::vector<netlib::Lobby> returnList;
    for(auto& pair : allLobbies)
    {
        returnList.push_back(pair.second);
    }
    return returnList;
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
    event->WriteData<unsigned int>(lobbyName.size(), 1 + (sizeof(unsigned int)*2));
    std::copy(lobbyName.data(), lobbyName.data() + lobbyName.size(), event->data.data()+ 1 + (sizeof(unsigned int)*3));
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

bool netlib::ClientConnection::IsInLobby()
{
    std::lock_guard<std::mutex> guard(lobbyLock);
    return activeLobby > 0;
}

void netlib::ClientConnection::SetLobbyOpen(bool isOpen)
{
    if(activeLobby == 0)
        return;
    auto event = new NetworkEvent();
    event->data.resize(MAX_PACKET_SIZE);
    event->data[0] = (char)MessageType::SET_LOBBY_OPEN;
    event->WriteData<unsigned int>(activeLobby, 1);
    event->WriteData<unsigned int>(uid, 1 + sizeof(unsigned int));
    event->WriteData<bool>(isOpen, 1 + (sizeof(unsigned int)*2));
    SendEvent(event);
}

netlib::LobbyMember netlib::ClientConnection::GetMemberInfo()
{
    std::lock_guard<std::mutex> guard(lobbyLock);
    for(LobbyMember& member : allLobbies[activeLobby].memberInfo)
    {
        if(member.uid == uid)
        {
            return member;
        }
    }
    std::cerr << "WARNING: Calling GetMemberInfo when the client is not connected to a lobby! Returning an empty LobbyMember struct." << std::endl;
    return LobbyMember();
}

void netlib::ClientConnection::SetLobbyName(std::string newName)
{
    auto event = new NetworkEvent();
    event->data.resize(MAX_PACKET_SIZE);
    event->data[0] = (char)MessageType::SET_LOBBY_NAME;
    event->WriteData<unsigned int>(activeLobby, 1);
    event->WriteData<unsigned int>(uid, 1 + sizeof(unsigned int));
    event->WriteData<unsigned int>(newName.size(), 1 + (sizeof(unsigned int)*2));
    std::copy(newName.data(), newName.data() + newName.size(), event->data.data()+ 1 + (sizeof(unsigned int)*3));
    SendEvent(event);
}