#include <iostream>
#include <thread>
#include "Server.h"

#pragma comment (lib, "ws2_32.lib")

Server::~Server()
{
    Stop();
    deleteSafeguard.lock();
    deleteSafeguard.unlock();
}

void Server::Stop()
{
    running = false;
    for(unsigned int i = 0; i<master.fd_count; i++)
    {
        closesocket(master.fd_array[i]);
    }
    WSACleanup();
}

bool Server::Start(int port)
{
    std::cout << "Initializing Server..." << std::endl;

    // Initialize winsock
    WSADATA wsData;
    WORD ver = MAKEWORD(2, 2);

    int wsResult = WSAStartup(ver, &wsData);
    if (wsResult != 0)
    {
        std::cerr << "Failed to initialize winsock Error code: " << wsResult << std::endl;
        return false;
    }

    // Create a socket
    listening = socket(AF_INET, SOCK_STREAM, 0);
    if (listening == INVALID_SOCKET)
    {
        std::cerr << "Failed to create socket error code: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return false;
    }

    // Bind an ip and port to the socket
    hint.sin_family = AF_INET;
    hint.sin_port = htons(port);
    hint.sin_addr.S_un.S_addr = INADDR_ANY;

    bind(listening, (sockaddr*)&hint, sizeof(hint));

    // Tell winsock the socket is for listening
    listen(listening, SOMAXCONN);

    FD_ZERO(&master);

    FD_SET(listening, &master);

    std::thread tr(&Server::ProcessNetworkEvents, this);
    tr.detach();

    std::cout << "Server Initialized!" << std::endl;
    return true;
}

void Server::ProcessNetworkEvents()
{
    deleteSafeguard.lock();
    running = true;

    while (running)
    {
        fd_set mCopy = master;
        unsigned int socketCount = select(0, &mCopy, nullptr, nullptr, nullptr);

        for (unsigned int i = 0; i < socketCount; i++)
        {
            SOCKET sock = mCopy.fd_array[i];

            if (sock == listening)
            {
                HandleConnectionEvent();
            }
            else
            {
                HandleMessageEvent(sock);
            }
        }
    }
    deleteSafeguard.unlock();
}

void Server::HandleConnectionEvent()
{
    // Accept a new connection
    SOCKET client = accept(listening, nullptr, nullptr);

    // Add the new connection to the master file set
    FD_SET(client, &master);

    // Assign the connection a uid
    ClientInfo newClient;
    if(uidLookup.count(client) > 0)
    {
        std::cout << "Duplicate socket, this needs re-working!" << std::endl;
    }
    newClient.uid = nextUid;
    uidLookup[client] = nextUid;
    nextUid++;

    // Add a quick lookup for the index of this socket in the master set
    indexLookup[newClient.uid] = master.fd_count-1;

    char host[NI_MAXHOST]; // Client's remote name
    char ip[NI_MAXHOST]; // Client ip address
    char service[NI_MAXSERV]; // Port the client is connected on

    ZeroMemory(host, NI_MAXHOST);
    ZeroMemory(service, NI_MAXSERV);

    // Try to get the host name
    if (getnameinfo((sockaddr*)&hint, sizeof(hint), host, NI_MAXHOST, service, NI_MAXSERV, 0) == 0)
    {
        //std::cout << host << " Connected on port " << service << std::endl;
        newClient.name = host;
    }
    //else
    //{
        inet_ntop(AF_INET, &hint.sin_addr, ip, NI_MAXHOST);
        //std::cout << ip << " connected on port " << ntohs(hint.sin_port) << std::endl;
        //char *addr = inet_ntoa(hint.sin_addr);
        //std::cout << addr << " connected on address " << ntohs(hint.sin_port) << std::endl;
    //}

    newClient.ipv4 = ip;
    std::cout << "New client: " << newClient.name << " Connected with ip " << newClient.ipv4 << std::endl;
    processNewClient(newClient);
}

void Server::HandleMessageEvent(const SOCKET& sock)
{
    // Accept a new message
    ZeroMemory(cBuf, MAX_PACKET_SIZE);
    int bytesReceived = recv(sock, cBuf, MAX_PACKET_SIZE, 0);

    if (bytesReceived <= 0)
    {
        closesocket(sock);
        FD_CLR(sock, &master);
        std::cout << "Client Disconnected.";
        // Re-make the lookup table as the indices have changed
        indexLookup.clear();
        processDisconnectedClient(uidLookup[sock]);
        uidLookup.erase(sock);
        for(size_t i = 0; i < master.fd_count; i++)
        {
            indexLookup[uidLookup[master.fd_array[i]]] = i;
        }
    }
    else
    {
        auto packet = new DataPacket();
        packet->data = new char[bytesReceived];
        memcpy(packet->data,cBuf, bytesReceived);
        packet->dataLength = bytesReceived;
        packet->senderId = uidLookup[sock];
        processPacket(packet);
    }
}

void Server::SendMessage(const char* data, int dataLength, unsigned int client)
{
    // If there is no data, just return as winsock uses 0-length messages to signal exit.
    if(dataLength <= 0)
        return;
    send(master.fd_array[indexLookup[client]], data,dataLength, 0);
}