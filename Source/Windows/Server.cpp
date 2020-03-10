#include <iostream>
#include <thread>
#include "Server.h"

#pragma comment (lib, "ws2_32.lib")

netlib::Server::~Server()
{
    Stop();
}

void netlib::Server::Stop()
{
    if(running)
    {
        running = false;

        while(!safeToExit);
        for (unsigned int i = 0; i < master.fd_count; i++) {
            closesocket(master.fd_array[i]);
        }
        WSACleanup();
    }
}

bool netlib::Server::Start(unsigned short port)
{
    data.resize((MAX_PACKET_SIZE+1)*2);
    //std::cout << "Initializing Server..." << std::endl;

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
    udp = socket(AF_INET, SOCK_DGRAM, 0);
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
    FD_SET(udp, &master);

    running = true;
    safeToExit = false;
    std::thread tr(&Server::ProcessNetworkEvents, this);
    tr.detach();

    //std::cout << "Server Initialized!" << std::endl;
    return true;
}

void netlib::Server::ProcessNetworkEvents()
{
    while (running)
    {
        fdLock.lock();
        fd_set mCopy = master;
        fdLock.unlock();

        TIMEVAL tv;
        tv.tv_sec = 0;
        tv.tv_usec = 100000;
        unsigned int socketCount = select(0, &mCopy, nullptr, nullptr, &tv);

        for (unsigned int i = 0; i < socketCount; i++)
        {
            SOCKET sock = mCopy.fd_array[i];

            if (sock == listening)
            {
                HandleConnectionEvent();
            }
            else if(sock == udp)
            {

            }
            else
            {
                HandleMessageEvent(sock);
            }
        }
    }

    safeToExit = true;
}

void netlib::Server::HandleConnectionEvent()
{
    // Accept a new connection
    struct sockaddr_in inaddr;
    socklen_t socklen = sizeof(sockaddr);
    SOCKET client = accept(listening, (struct sockaddr *)&inaddr, &socklen);

    // Add the new connection to the master file set
    fdLock.lock();
    FD_SET(client, &master);

    // Assign the connection a uid
    ClientInfo newClient;
    newClient.uid = nextUid;
    uidLookup[client] = nextUid;
    nextUid++;

    // Add a quick lookup for the index of this socket in the master set
    indexLookup[newClient.uid] = master.fd_count-1;
    fdLock.unlock();

    char host[NI_MAXHOST]; // Client's remote name
    char service[NI_MAXSERV]; // Port the client is connected on

    ZeroMemory(host, NI_MAXHOST);
    ZeroMemory(service, NI_MAXSERV);

    // Try to get the host name
    if (getnameinfo((sockaddr*)&inaddr, sizeof(inaddr), host, NI_MAXHOST, service, NI_MAXSERV, 0) == 0)
    {
        //std::cout << host << " Connected on port " << service << std::endl;
        newClient.name = host;
    }

    //char ip[INET_ADDRSTRLEN];
    //inet_ntop(hint.sin_family,(sockaddr*)&hint,ip, INET_ADDRSTRLEN );
    //newClient.ipv4 = ip;
    newClient.ipv4 = inet_ntoa(inaddr.sin_addr);

    //std::cout << "New client: " << newClient.name << " Connected with ip " << newClient.ipv4 << std::endl;
    processNewClient(newClient);
}

void netlib::Server::HandleMessageEvent(const SOCKET& sock)
{
    // Accept a new message
    int newBytes = recv(sock, data.data() + writePos, MAX_PACKET_SIZE+1, 0);

    if (newBytes > 0)
    {
        bytesReceived += newBytes;
        // First byte of a packet tells us the length of the remaining data
        while(data[readPos] < bytesReceived)
        {
            auto packet = new NetworkEvent();
            packet->data.resize(data[readPos]);
            std::copy(data.data() + readPos + 1, data.data() + readPos + data[readPos], packet->data.data());
            bytesReceived -= data[readPos] + 1;
            readPos += data[readPos] + 1;
            packet->senderId = uidLookup[sock];
            processPacket(packet);
        }

        if(bytesReceived != 0)
        {
            std::copy(data.data() + readPos, data.data() + readPos + data[readPos], data.data());
            writePos = data[0];
        }
        else writePos = 0;
        readPos = 0;
    }
    else
    {
        fdLock.lock();
        closesocket(sock);
        FD_CLR(sock, &master);
        //std::cout << "Client " << uidLookup[sock] << " has disconnected." << std::endl;
        // Re-make the lookup table as the indices have changed
        indexLookup.clear();
        processDisconnectedClient(uidLookup[sock]);
        uidLookup.erase(sock);
        for(size_t i = 0; i < master.fd_count; i++)
        {
            indexLookup[uidLookup[master.fd_array[i]]] = i;
        }
        fdLock.unlock();
    }
}

void netlib::Server::SendMessageToClient(const char* data, char dataLength, unsigned int client)
{
    if(dataLength <= 0)
        return;
    fdLock.lock();
    char* sendData = new char[dataLength+1];
    sendData[0] = dataLength;
    std::copy(data, data + dataLength, sendData+1);
    int wsResult = send(master.fd_array[indexLookup[client]], sendData,dataLength+1, 0);
    delete[] sendData;

    if (wsResult == SOCKET_ERROR)
    {
        //std::cerr << "Failed to send message error code: " << WSAGetLastError() << std::endl;
    }
    fdLock.unlock();
}
void netlib::Server::DisconnectClient(unsigned int client)
{
    SOCKET sock = master.fd_array[indexLookup[client]];
    fdLock.lock();
    closesocket(sock);
    FD_CLR(sock, &master);

    indexLookup.clear();
    processDisconnectedClient(uidLookup[sock]);
    uidLookup.erase(sock);
    for(size_t i = 0; i < master.fd_count; i++)
    {
        indexLookup[uidLookup[master.fd_array[i]]] = i;
    }
    fdLock.unlock();
}
