#include "../Headers/ServerConnection.h"
#include <iterator>
#include <iostream>

netlib::ServerConnection::ServerConnection()
{

}

netlib::ServerConnection::~ServerConnection()
{

}

void netlib::ServerConnection::Start(int port)
{
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
    auto packet = new NetworkEvent();
    packet->data.resize(data.size());
    std::copy(data.data(), data.data() + data.size(), packet->data.data());
    packet->senderId = clientUid;
    ProcessAndSendData(packet);
}

// Sends a message to only the specified client
void netlib::ServerConnection::SendMessageTo(const char* data, int dataLen, unsigned int clientUID)
{
    auto packet = new NetworkEvent();
    packet->data.resize(dataLen);
    std::copy(data, data + dataLen, packet->data.data());
    packet->senderId = clientUID;
    ProcessAndSendData(packet);
}

// Sends a message to all connected clients
void netlib::ServerConnection::SendMessageToAll(const std::vector<char>& data)
{
    clientInfoLock.lock();
    for(const auto &info : connectedClients)
    {
        SendMessageTo(data, info.second.uid);
    }
    clientInfoLock.unlock();
}

void netlib::ServerConnection::SendMessageToAll(const char* data, int dataLen)
{
    clientInfoLock.lock();
    for(const auto &info : connectedClients)
    {
        SendMessageTo(data, dataLen, info.second.uid);
    }
    clientInfoLock.unlock();
}

// Sends a message to all connected clients excluding the specified client
void netlib::ServerConnection::SendMessageToAllExcluding(const std::vector<char>& data, unsigned int clientUID)
{
    clientInfoLock.lock();
    for(const auto &info : connectedClients)
    {
        if(info.first == clientUID)
            continue;
        SendMessageTo(data, info.first);
    }
    clientInfoLock.unlock();
}

void netlib::ServerConnection::SendMessageToAllExcluding(const char* data, int dataLen, unsigned int clientUID)
{
    clientInfoLock.lock();
    for(const auto &info : connectedClients)
    {
        if(info.first == clientUID)
            continue;
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

            delete event;
            clientInfoLock.unlock();
            break;
        }
        case MessageType::REQUEST_NEW_LOBBY:
        {
            lobbyLock.lock();
            lobbyLock.unlock();
            CreateNewLobby(event);
            break;
        }
        case MessageType::JOIN_LOBBY:
        {
            unsigned int lobbyId = *reinterpret_cast<unsigned int*>(event->data.data()+1);
            AddPlayerToLobby(event->senderId, lobbyId);
            delete event;
            break;
        }
        case MessageType::REMOVE_FROM_LOBBY:
        {

            break;
        }
        default:
        {
            break;
        }
    }
}

void netlib::ServerConnection::CreateNewLobby(NetworkEvent* event)
{
    // Create the new lobby
    lobbyLock.lock();
    lobbies[lobbyUID].lobbyID = lobbyUID;

    unsigned int nameLen = *reinterpret_cast<unsigned int*>(&event->data[1+ sizeof(unsigned int)]);
    lobbies[lobbyUID].name = std::string(event->data.data() + 1 + (sizeof(unsigned int)*2), nameLen);


    lobbyLock.unlock();
    unsigned int senderID = event->senderId;

    // Signal to all clients there is a new lobby
    auto idAsChar = reinterpret_cast<char*>(&lobbyUID);
    std::copy(idAsChar, idAsChar + sizeof(unsigned int), event->data.data()+1);
    event->data[0] = (char)MessageType::ADD_NEW_LOBBY;
    SendEventToAll(event);

    // Connect the sending client to this new lobby
    AddPlayerToLobby(senderID, lobbyUID);
    lobbyUID++;
}

void netlib::ServerConnection::AddPlayerToLobby(unsigned int player, unsigned int lobby)
{
    auto event = new NetworkEvent();
    event->senderId = player;
    event->data.resize(MAX_PACKET_SIZE);
    event->data[0] = (char)MessageType::SET_ACTIVE_LOBBY;
    event->WriteData<unsigned int>(lobbyUID, 1);
    SendEvent(event);

    // Tell all other clients a new client has joined a lobby
    event = new NetworkEvent();
    event->data.resize(MAX_PACKET_SIZE);
    event->data[0] = (char)MessageType::NEW_LOBBY_CLIENT;
    event->WriteData<unsigned int>(lobby, 1);
    event->WriteData<unsigned int>(player, 1 + sizeof(unsigned int));

    clientInfoLock.lock();
    // Add the player into the lobby onto the server
    lobbyLock.lock();
    lobbies[lobby].clientsInRoom++;
    lobbies[lobby].memberInfo.emplace_back();
    lobbies[lobby].memberInfo.back().name = connectedClients[player].name;
    lobbies[lobby].memberInfo.back().uid = player;

    unsigned int nameLen = connectedClients[player].name.size() + 1;
    event->WriteData<unsigned int>(nameLen, 1 + (sizeof(unsigned int) *2));

    std::copy(connectedClients[player].name.data(),
              connectedClients[player].name.data() +  nameLen,
              event->data.data() + 1 + (sizeof(unsigned int) *3));
    clientInfoLock.unlock();
    SendEventToAll(event);
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
    messages.front().eventType = NetworkEvent::EventType::ONCONNECT;
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
        auto event = new NetworkEvent();
        event->data.resize(MAX_PACKET_SIZE);

        event->WriteData<unsigned int>(lobby.first, 1);
        event->WriteData<unsigned int>(lobby.second.name.size() + 1, 1 + sizeof(unsigned int));

        std::copy(lobby.second.name.data(), lobby.second.name.data() + lobby.second.name.size(), event->data.data() + 1 + (sizeof(unsigned int)*2));
        event->data[0] = (char)MessageType::ADD_NEW_LOBBY;
        event->senderId = info.uid;
        SendEvent(event);
    }
    lobbyLock.unlock();


}

void netlib::ServerConnection::ProcessDisconnectedClient(unsigned int clientUID) {
    clientInfoLock.lock();
    connectedClients.erase(clientUID);
    clientInfoLock.unlock();

    messageLock.lock();
    ClearQueue();
    messages.emplace();
    messages.front().senderId = clientUID;
    messages.front().eventType = NetworkEvent::EventType::ONDISCONNECT;
    messageLock.unlock();


}

void netlib::ServerConnection::UpdateNetworkStats()
{
    // Handle pings
    using clock = std::chrono::steady_clock;
    clientInfoLock.lock();
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
        }
    }
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

