#include <iostream>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <string>
#include <thread>
#include "Server.h"

Server::~Server()
{
    Stop();
    deleteSafeguard->lock();
    deleteSafeguard->unlock();
}

bool Server::Start()
{
    deleteSafeguard = new std::mutex();
    // Create a socket
    int listening = socket(AF_INET, SOCK_STREAM, 0);
    if(listening == -1)
    {
        std::cerr << "Failed to create socket" << std::endl;;
        return false;
    }

    // Bind the socket to an ip and port
    hint.sin_family = AF_INET;
    hint.sin_port = htons(54000);
    inet_pton(AF_INET, "0.0.0.0", &hint.sin_addr);


    if(bind(listening, (sockaddr*)&hint, sizeof(hint)) == -1)
    {
        std::cerr << "Failed to bind socket to IP or Port" << std::endl;;
        return false;
    }

    // Set the socket as a listening
    if(listen(listening, SOMAXCONN) == -1)
    {
        std::cerr << "Socket failed to start listening" << std::endl;;
    }

    FD_ZERO(&master);

    FD_SET(listening, &master);

    std::thread tr(&Server::ProcessNetworkEvents, this);

    tr.detach();

    std::cout << "Server Initialized!"  << std::endl;
    return true;
}

void Server::Stop()
{
    running = false;
    close(listening);
}

void Server::ProcessNetworkEvents()
{
    deleteSafeguard->lock();
    running = true;

    while(running)
    {
        fd_set mCopy = master;
        unsigned int socketCount = select(0, &mCopy, nullptr, nullptr, nullptr);

        for(unsigned int i = 0; i < socketCount; i++)
        {
            int sock = mCopy.fds_bits[i];

            if(sock == listening)
            {
                HandleConnectionEvent();
            }
            else
            {
                HandleMessageEvent(sock, i);
            }
        }
    }
}

void Server::HandleConnectionEvent()
{
    sockaddr_in client;
    socklen_t clientSize;
    char host[NI_MAXHOST];
    char svc[NI_MAXSERV];

    int clientSocket =
            accept(listening, (sockaddr*)&client, &clientSize);
    if(clientSocket == -1)
    {
        std::cerr << "Client failed to connect" << std::endl;;
    }

    // Add new connection to master set
    FD_SET(clientSocket, &master);

    // Assign the connection a uid
    ClientInfo newClient;
    // Since unix sockets are represented as an int, we can just use those directly for the uid
    newClient.uid = clientSocket;

    memset(host, 0, NI_MAXHOST);
    memset(svc, 0, NI_MAXSERV);

    int result = getnameinfo((sockaddr*)&client,
                             sizeof(client),
                             host,
                             NI_MAXHOST,
                             svc,
                             NI_MAXSERV,
                             0);

    if(result)
    {
        newClient.name = host;
        newClient.ipv4 = svc;
    }
    else
    {
        inet_ntop(AF_INET, &client.sin_addr, host, NI_MAXHOST);

        newClient.name = host;
        newClient.ipv4 = ntohs(client.sin_port);
    }

    std::cout << "New Client: " << newClient.name << " Connected with ip " << newClient.ipv4 << std::endl;
    processNewClient(newClient);
}

void Server::HandleMessageEvent(int sock, unsigned int id) {
    memset(cBuf, 0, 4096);

    int bytesReceived = recv(sock, cBuf, 4096, 0);
    if (bytesReceived <= 0)
    {
        close(sock);
        FD_CLR(sock, &master);
        std::cout << "Client disconnected" << std::endl;
    }
    else
    {
        auto packet = new DataPacket();
        packet->data = new char[bytesReceived];
        memcpy(packet->data, cBuf, bytesReceived);
        packet->dataLength = bytesReceived;
        packet->senderId = sock;
        processPacket(packet);
    }
}

void Server::SendMessage(const char *data, int dataLength, unsigned int client)
{
    if(dataLength > 0)
    {
        send(client, data, dataLength, 0);
    }
}