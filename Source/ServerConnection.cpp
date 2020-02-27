#include "NetLib/ServerConnection.h"
#include <iterator>
#include <iostream>

netlib::ServerConnection::~ServerConnection()
{
    while(!safeToExit);
}

void netlib::ServerConnection::Start(unsigned short port)
{
    if(server.IsRunning())
        return;
    server.processPacket = std::bind(&ServerConnection::ProcessPacket, this, std::placeholders::_1);
    server.processNewClient = std::bind(&ServerConnection::ProcessNewClient, this, std::placeholders::_1);
    server.processDisconnectedClient = std::bind(&ServerConnection::ProcessDisconnectedClient, this, std::placeholders::_1);
    server.Start(port);
    NetworkDevice::Start();
}

void netlib::ServerConnection::SendPacket(NetworkEvent* event)
{
    server.SendMessageToClient(event->data.data(), event->data.size(), event->senderId);
    delete event;
}

// Sends a message to only the specified client
void netlib::ServerConnection::SendMessageTo(const std::vector<char>& data, unsigned int clientUid)
{
    if(data.empty())
        return;
    auto packet = new NetworkEvent();
    packet->data.resize(data.size());
    std::copy(data.data(), data.data() + data.size(), packet->data.data());
    packet->senderId = clientUid;
    ProcessAndSendData(packet);
}

// Sends a message to only the specified client
void netlib::ServerConnection::SendMessageTo(const char* data, int dataLen, unsigned int clientUID)
{
    if(dataLen == 0)
        return;
    auto packet = new NetworkEvent();
    packet->data.resize(dataLen);
    std::copy(data, data + dataLen, packet->data.data());
    packet->senderId = clientUID;
    ProcessAndSendData(packet);
}

// Sends a message to all connected clients
void netlib::ServerConnection::SendMessageToAll(const std::vector<char>& data, unsigned int lobbyFilter)
{
    clientInfoLock.lock();
    for(const auto &info : connectedClients)
    {
        if(info.second.lobbyID == lobbyFilter)
            SendMessageTo(data, info.second.uid);
    }
    clientInfoLock.unlock();
}

void netlib::ServerConnection::SendMessageToAll(const char* data, int dataLen, unsigned int lobbyFilter)
{
    clientInfoLock.lock();
    for(const auto &info : connectedClients)
    {
        if(info.second.lobbyID == lobbyFilter)
            SendMessageTo(data, dataLen, info.second.uid);
    }
    clientInfoLock.unlock();
}

// Sends a message to all connected clients excluding the specified client
void netlib::ServerConnection::SendMessageToAllExcluding(const std::vector<char>& data, unsigned int clientUID, unsigned int lobbyFilter)
{
    clientInfoLock.lock();
    for(const auto &info : connectedClients)
    {
        if(info.first == clientUID)
            continue;
        if(info.second.lobbyID == lobbyFilter)
            SendMessageTo(data, info.first);
    }
    clientInfoLock.unlock();
}

void netlib::ServerConnection::SendMessageToAllExcluding(const char* data, int dataLen, unsigned int clientUID, unsigned int lobbyFilter)
{
    clientInfoLock.lock();
    for(const auto &info : connectedClients)
    {
        if(info.first == clientUID)
            continue;
        if(info.second.lobbyID == lobbyFilter)
            SendMessageTo(data, dataLen,  info.first);
    }
    clientInfoLock.unlock();
}

void netlib::ServerConnection::ProcessDeviceSpecificEvent(NetworkEvent *event)
{
    switch ((MessageType)event->data[0])
    {
        case MessageType ::PING_RESPONSE:
        {
            using clock = std::chrono::steady_clock;
            clientInfoLock.lock();

            connectedClients[event->senderId].connectionInfo.ping = static_cast<float>(
                    std::chrono::duration_cast<std::chrono::milliseconds>
                            (clock::now() - connectedClients[event->senderId].timeOfLastPing).count());
            connectedClients[event->senderId].waitingForPing = false;

            lobbyLock.lock();
            if(connectedClients[event->senderId].lobbyID != 0)
            {
                for (auto &member : lobbies[connectedClients[event->senderId].lobbyID].memberInfo)
                {
                    if (member.uid == event->senderId)
                    {
                        member.ping = connectedClients[event->senderId].connectionInfo.ping;
                    }
                }
            }
            lobbyLock.unlock();

            delete event;
            clientInfoLock.unlock();
            break;
        }
        case MessageType::REQUEST_NEW_LOBBY:
        {
            CreateNewLobby(event);
            break;
        }
        case MessageType::JOIN_LOBBY:
        {
            auto lobbyId = event->ReadData<unsigned int>(1);
            if(lobbies.count(lobbyId) == 0 || lobbies[lobbyId].clientsInRoom == lobbies[lobbyId].maxClientsInRoom)
                return;
            AddClientToLobby(event->senderId, lobbyId);
            delete event;
            break;
        }
        case MessageType::REMOVE_FROM_LOBBY:
        {
            auto clientID = event->ReadData<unsigned int>(1);
            auto lobbyID = event->ReadData<unsigned int>(1+sizeof(unsigned int));
            RemoveClientFromLobby(clientID, lobbyID);
            delete event;
            break;
        }
        case MessageType::SET_CLIENT_READY:
        {
            auto lobbyID = event->ReadData<unsigned int>(1);
            auto clientID = event->ReadData<unsigned int>(1+ sizeof(unsigned int));
            auto ready = event->ReadData<bool>(1 + (sizeof(unsigned int) *2));
            lobbyLock.lock();
            for(auto& member : lobbies[lobbyID].memberInfo)
            {
                if(member.uid == clientID)
                {
                    member.ready = ready;
                    break;
                }
            }
            lobbyLock.unlock();
            SendEventToAll(event);
            break;
        }
        case MessageType::SET_LOBBY_OPEN:
        {
            auto lobbyID = event->ReadData<unsigned int>(1);
            auto clientID = event->ReadData<unsigned int>(1+ sizeof(unsigned int));
            auto open = event->ReadData<bool>(1 + (sizeof(unsigned int) *2));
            lobbyLock.lock();
            lobbies[lobbyID].open = open;
            lobbyLock.unlock();
            SendEventToAll(event);

            if(open)
            {
                if(disconnectedMembers.count(lobbyID) > 0)
                    disconnectedMembers.erase(lobbyID);
                lobbyLock.lock();
                AddOpenLobby(lobbyID, 0, true);
                lobbyLock.unlock();
            }
            break;
        }
        case MessageType::SET_LOBBY_NAME:
        {
            auto lobbyID = event->ReadData<unsigned int>(1);
            auto clientID = event->ReadData<unsigned int>(1+ sizeof(unsigned int));
            auto nameLen = event->ReadData<unsigned int>(1+ (sizeof(unsigned int)*2));
            clientInfoLock.lock();
            connectedClients[clientID].name = std::string(event->data.data() + 1 + (sizeof(unsigned int)*3), nameLen);
            clientInfoLock.unlock();
            if(lobbyID != 0)
            {
                SendEventToAll(event);
            }
            else
            {
                delete event;
            }
            break;
        }
        default:
        {
            delete event;
            break;
        }
    }
}

void netlib::ServerConnection::CreateNewLobby(NetworkEvent* event)
{
    // Create the new lobby
    lobbyLock.lock();
    lobbies[lobbyUID].lobbyID = lobbyUID;

    lobbies[lobbyUID].maxClientsInRoom = event->ReadData<int>(1 + sizeof(int));
    auto nameLen = event->ReadData<unsigned int>(1+ (sizeof(unsigned int)*2));
    lobbies[lobbyUID].name = std::string(event->data.data() + 1 + (sizeof(unsigned int)*3), nameLen);


    lobbyLock.unlock();
    unsigned int senderID = event->senderId;

    // Signal to all clients there is a new lobby
    event->WriteData<unsigned int>(lobbyUID, 1);
    event->data[0] = (char)MessageType::ADD_NEW_LOBBY;
    SendEventToAll(event);

    // Connect the sending client to this new lobby
    AddClientToLobby(senderID, lobbyUID);
    lobbyUID++;
}

void netlib::ServerConnection::AddClientToLobby(unsigned int client, unsigned int lobby, bool forceSlot, unsigned int slot)
{
    auto event = new NetworkEvent();
    event->senderId = client;
    event->data.resize(MAX_PACKET_SIZE);
    event->data[0] = (char)MessageType::SET_ACTIVE_LOBBY;
    event->WriteData<unsigned int>(lobby, 1);
    SendEvent(event);

    // Tell all other clients a new client has joined a lobby
    event = new NetworkEvent();
    event->data.resize(MAX_PACKET_SIZE);
    event->data[0] = (char)MessageType::NEW_LOBBY_CLIENT;
    event->WriteData<unsigned int>(lobby, 1);
    event->WriteData<unsigned int>(client, 1 + sizeof(unsigned int));

    clientInfoLock.lock();
    connectedClients[client].lobbyID = lobby;
    // Add the client into the lobby onto the server
    lobbyLock.lock();
    lobbies[lobby].clientsInRoom++;
    lobbies[lobby].memberInfo.emplace_back();
    lobbies[lobby].memberInfo.back().name = connectedClients[client].name;
    lobbies[lobby].memberInfo.back().uid = client;
    lobbyLock.unlock();

    unsigned int nameLen = connectedClients[client].name.size();
    event->WriteData<unsigned int>(nameLen, 1 + (sizeof(unsigned int) *2));

    std::copy(connectedClients[client].name.data(),
              connectedClients[client].name.data() +  nameLen,
              event->data.data() + 1 + (sizeof(unsigned int) *3));
    clientInfoLock.unlock();
    SendEventToAll(event);

    event = new NetworkEvent();
    event->senderId = client;
    event->data.resize(MAX_PACKET_SIZE);
    event->data[0] = (char)MessageType::SET_LOBBY_SLOT;
    lobbyLock.lock();
    event->WriteData<unsigned int>(lobby, 1);
    event->WriteData<unsigned int>(client, 1 + sizeof(unsigned int));
    if(forceSlot)
    {
        event->WriteData<unsigned int>(slot, 1 + (sizeof(unsigned int) * 2));
        lobbies[lobby].memberInfo.back().lobbySlot = slot;
    }
    else
    {
        unsigned int nextSlot = 0;
        bool slotFound = false;
        bool duplicate;
        while(!slotFound)
        {
            duplicate = false;
            for (LobbyMember &member : lobbies[lobby].memberInfo)
            {
                if (member.lobbySlot == nextSlot)
                {
                    duplicate = true;
                    break;
                }
            }
            if(!duplicate)
                slotFound = true;
            else
                nextSlot++;
        }

        event->WriteData<unsigned int>(nextSlot, 1 + (sizeof(unsigned int) * 2));
        lobbies[lobby].memberInfo.back().lobbySlot = nextSlot;

    }

    lobbyLock.unlock();
    SendEventToAll(event);
}

std::vector<netlib::Lobby> netlib::ServerConnection::GetAllLobbies()
{
    std::vector<Lobby> returnVec;
    lobbyLock.lock();
    for(auto& Lobby : lobbies)
    {
        returnVec.push_back(Lobby.second);
    }
    lobbyLock.unlock();
    return returnVec;
}

netlib::Lobby netlib::ServerConnection::GetLobby(unsigned int lobbyID)
{
    std::lock_guard<std::mutex> guard(lobbyLock);
    if(lobbies.count(lobbyID) > 0)
        return lobbies[lobbyID];
    else
    {
        std::cerr << "WARNING: TRYING TO GET LOBBY THAT DOES NOT EXIST! RETURNING AN EMPTY LOBBY STRUCT." << std::endl;
        return Lobby();
    }
}

void netlib::ServerConnection::RemoveClientFromLobby(unsigned int clientID, unsigned int lobbyID)
{
    clientInfoLock.lock();
    connectedClients[clientID].lobbyID = 0;
    clientInfoLock.unlock();
    lobbyLock.lock();
    if(lobbies.count(lobbyID) == 0)
    {
        lobbyLock.unlock();
        return;
    }

    for(auto it = lobbies[lobbyID].memberInfo.begin(); it != lobbies[lobbyID].memberInfo.end(); ++it)
    {
        if(it->uid == clientID)
        {
            lobbies[lobbyID].memberInfo.erase(it);
            lobbies[lobbyID].clientsInRoom--;
            auto event = new NetworkEvent();
            event->data.resize(MAX_PACKET_SIZE);
            event->data[0] = (char)MessageType::LOBBY_CLIENT_LEFT;
            event->WriteData(lobbyID, 1);
            event->WriteData(clientID, 1 + sizeof(unsigned int));
            SendEventToAll(event);
            break;
        }
    }
    if(lobbies[lobbyID].clientsInRoom == 0)
    {
        lobbies.erase(lobbyID);
        if(disconnectedMembers.count(lobbyID) > 0)
            disconnectedMembers.erase(lobbyID);
        auto event = new NetworkEvent();
        event->data.resize(MAX_PACKET_SIZE);
        event->data[0] = (char)MessageType::REMOVE_LOBBY;
        event->WriteData(lobbyID, 1);
        SendEventToAll(event);
    }
    lobbyLock.unlock();
}

void netlib::ServerConnection::SendEventToAll(netlib::NetworkEvent* event)
{
    clientInfoLock.lock();
    for(auto& client : connectedClients)
    {
        auto copy = new netlib::NetworkEvent();
        *copy = *event;
        copy->senderId = client.first;
        SendEvent(copy);
    }
    clientInfoLock.unlock();
    delete event;
}

void netlib::ServerConnection::ProcessNewClient(ClientInfo info)
{
    clientInfoLock.lock();
    connectedClients[info.uid] = info;
    clientInfoLock.unlock();

    messageLock.lock();
    ClearQueue();
    messages.emplace();
    messages.front().senderId = info.uid;
    messages.front().eventType = NetworkEvent::EventType::ON_CONNECT;
    messageLock.unlock();

    auto packet = new NetworkEvent();
    packet->data.resize(MAX_PACKET_SIZE);
    packet->data[0] = (char)MessageType::SET_CLIENT_UID;
    packet->senderId = info.uid;
    packet->WriteData<unsigned int>(info.uid, 1);

    SendEvent(packet);

    // Tell the new client of all active lobbies
    lobbyLock.lock();
    for(auto& lobby : lobbies)
    {
        if(lobby.second.open)
            AddOpenLobby(lobby.first, info.uid);
    }

    // Check if this client should re-join a lobby it disconnected from
    for(auto& lobby : disconnectedMembers)
    {
        for(auto it = lobby.second.begin(); it != lobby.second.end(); it++)
        {
            if(it->second == info.ipv4)
            {
                if(lobbies.count(lobby.first) > 0)
                {
                    lobbyLock.unlock();
                    AddOpenLobby(lobby.first, info.uid, false);
                    AddClientToLobby(info.uid, lobby.first, true, it->first);
                    lobbyLock.lock();
                    lobby.second.erase(it);
                    if (lobby.second.empty())
                        disconnectedMembers.erase(lobby.first);
                    lobbyLock.unlock();
                    return;
                }
            }
        }
    }
    lobbyLock.unlock();
}


void netlib::ServerConnection::AddOpenLobby(unsigned int lobbyID, unsigned int clientUID, bool sendToAll)
{
    Lobby& lobby = lobbies[lobbyID];
    auto event = new NetworkEvent();
    event->data.resize(MAX_PACKET_SIZE);

    event->WriteData<unsigned int>(lobbyID, 1);
    event->WriteData<unsigned int>(lobby.maxClientsInRoom, 1 + sizeof(unsigned int));
    event->WriteData<unsigned int>(lobby.name.size(), 1 + (sizeof(unsigned int)*2));

    std::copy(lobby.name.data(), lobby.name.data() + lobby.name.size(), event->data.data() + 1 + (sizeof(unsigned int)*3));
    event->data[0] = (char)MessageType::ADD_NEW_LOBBY;
    event->senderId = clientUID;
    if(sendToAll)
        SendEventToAll(event);
    else
         SendEvent(event);
    for(auto& member : lobby.memberInfo)
    {
        clientInfoLock.lock();
        event = new NetworkEvent();
        event->data.resize(MAX_PACKET_SIZE);
        event->data[0] = (char) MessageType::NEW_LOBBY_CLIENT;
        event->WriteData<unsigned int>(lobbyID, 1);
        event->WriteData<unsigned int>(member.uid, 1 + sizeof(unsigned int));

        unsigned int nameLen = connectedClients[member.uid].name.size();
        event->WriteData<unsigned int>(nameLen, 1 + (sizeof(unsigned int) * 2));

        std::copy(connectedClients[member.uid].name.data(),
                  connectedClients[member.uid].name.data() + nameLen,
                  event->data.data() + 1 + (sizeof(unsigned int) * 3));

        event->senderId = clientUID;
        if(sendToAll)
        {
            clientInfoLock.unlock();
            SendEventToAll(event);
            clientInfoLock.lock();
        }
        else
            SendEvent(event);

        event = new NetworkEvent();
        event->data.resize(MAX_PACKET_SIZE);
        event->data[0] = (char) MessageType::SET_CLIENT_READY;
        event->WriteData<unsigned int>(lobbyID, 1);
        event->WriteData<unsigned int>(member.uid, 1 + sizeof(unsigned int));
        event->WriteData<bool>(member.ready, 1 + (sizeof(unsigned int) * 2));
        event->senderId = clientUID;
        if(sendToAll)
        {
            clientInfoLock.unlock();
            SendEventToAll(event);
            clientInfoLock.lock();
        }
        else
            SendEvent(event);
        clientInfoLock.unlock();

        event = new NetworkEvent();
        event->senderId = clientUID;
        event->data.resize(MAX_PACKET_SIZE);
        event->data[0] = (char)MessageType::SET_LOBBY_SLOT;
        event->WriteData<unsigned int>(lobby.lobbyID, 1);
        event->WriteData<unsigned int>(member.uid, 1 + sizeof(unsigned int));
        event->WriteData<unsigned int>(member.lobbySlot, 1 + (sizeof(unsigned int)*2));

        if(sendToAll)
            SendEventToAll(event);
        else
            SendEvent(event);
    }
}

void netlib::ServerConnection::ProcessDisconnectedClient(unsigned int clientUID)
{
    clientInfoLock.lock();
    unsigned int lobby = connectedClients[clientUID].lobbyID;
    clientInfoLock.unlock();
    if(lobby > 0)
    {
        lobbyLock.lock();
        if(lobbies.count(lobby) > 0)
        {
            for(auto& member : lobbies[lobby].memberInfo)
            {
                if(member.uid == clientUID)
                {
                    if(!lobbies[lobby].open && lobbies.count(lobby) > 0)
                        disconnectedMembers[lobby].push_back(std::pair<unsigned int, std::string>(member.lobbySlot, GetClientInfo(clientUID).ipv4));
                    lobbyLock.unlock();
                    RemoveClientFromLobby(clientUID, lobby);
                    lobbyLock.lock();

                    break;
                }
            }
        }
        lobbyLock.unlock();
    }
    clientInfoLock.lock();
    connectedClients.erase(clientUID);
    clientInfoLock.unlock();

    messageLock.lock();
    ClearQueue();
    messages.emplace();
    messages.front().senderId = clientUID;
    messages.front().eventType = NetworkEvent::EventType::ON_DISCONNECT;
    messageLock.unlock();
}

void netlib::ServerConnection::UpdateNetworkStats()
{
    // Handle pings
    using clock = std::chrono::steady_clock;
    clientInfoLock.lock();
    lobbyLock.lock();
    for(auto& client : connectedClients) {
        if (!client.second.waitingForPing &&
            std::chrono::duration_cast<std::chrono::milliseconds>(clock::now() - client.second.timeOfLastPing).count() >
            PING_FREQUENCY)
        {
            client.second.waitingForPing = true;
            auto packet = new NetworkEvent();
            packet->data.resize(MAX_PACKET_SIZE);
            packet->data[0] = (char) MessageType::PING_REQUEST;
            packet->senderId = client.second.uid;
            outQueueLock.lock();
            outQueue.push(packet);
            outQueueLock.unlock();
            client.second.timeOfLastPing = clock::now();
            // If this client is the top member of a lobby, use this time to refresh all the ping values for the lobby
            if(lobbies.count(client.second.lobbyID) > 0 && lobbies[client.second.lobbyID].memberInfo[0].uid == client.first)
            {
                auto event = new NetworkEvent();
                event->data.resize(MAX_PACKET_SIZE);
                event->data[0] = (char)MessageType::UPDATE_PEER_CONNECTION_INFO;
                int offset = 1;
                event->WriteData<unsigned int>(client.second.lobbyID, offset);
                offset += sizeof(int);
                event->WriteData<int>(lobbies[client.second.lobbyID].clientsInRoom, offset);
                offset += sizeof(int);

                for(auto& member : lobbies[client.second.lobbyID].memberInfo)
                {
                    event->WriteData<float>(member.ping, offset);
                    offset += sizeof(float);
                }
                for(auto& client : connectedClients)
                {
                    auto copy = new netlib::NetworkEvent();
                    *copy = *event;
                    copy->senderId = client.first;
                    SendEvent(copy);
                }

                delete event;
            }

        }
    }
    lobbyLock.unlock();
    clientInfoLock.unlock();
}

netlib::ClientInfo netlib::ServerConnection::GetClientInfo(unsigned int clientUID)
{
    std::lock_guard<std::mutex> guard(clientInfoLock);
    if(connectedClients.count(clientUID) > 0)
        return connectedClients[clientUID];
    else
    {
        ClientInfo info;
        return info;
    }
}

std::vector<netlib::ClientInfo> netlib::ServerConnection::GetAllClients()
{
    clientInfoLock.lock();
    std::vector<ClientInfo> returnVec;
    for(auto pair : connectedClients)
    {
        returnVec.emplace_back(pair.second);
    }
    clientInfoLock.unlock();
    return returnVec;
}

void netlib::ServerConnection::DisconnectClient(unsigned int clientUID)
{
    outQueueLock.lock();
    disconnectQueue.push(clientUID);
    outQueueLock.unlock();
}

// Gets called after the message queue has been processed
void netlib::ServerConnection::TerminateConnection(unsigned int clientUID)
{
    server.DisconnectClient(clientUID);
}


