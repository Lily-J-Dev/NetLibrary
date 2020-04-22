#include <iostream>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <string>
#include <thread>
#include <algorithm>
#include "Server.h"

netlib::Server::~Server()
{
    if(running)
    {
        Stop();
        delete fdLock;
    }
}

bool netlib::Server::Start(int port)
{
    dataTCP.resize((MAX_PACKET_SIZE+1)*2);
    dataUDP.resize((MAX_PACKET_SIZE+1)*2);
    fdLock = new std::mutex();
    // Create a socket
    listening = socket(AF_INET, SOCK_STREAM, 0);
    udp = socket(AF_INET, SOCK_DGRAM, 0);
    if(listening == -1)
    {
        std::cerr << "Failed to create socket" << std::endl;
        return false;
    }

    // Bind the socket to an ip and port
    hint.sin_family = AF_INET;
    hint.sin_port = htons(port);
    inet_pton(AF_INET, "0.0.0.0", &hint.sin_addr);


    if(bind(listening, (sockaddr*)&hint, sizeof(hint)) == -1)
    {
        std::cerr << "Failed to bind TCP socket to IP or Port" << std::endl;
        std::cerr << gai_strerror(errno) << std::endl;
        return false;
    }

    if(bind(udp, (sockaddr*)&hint, sizeof(hint)) == -1)
    {
        std::cerr << "Failed to bind UDP socket to IP or Port" << std::endl;
        std::cerr << gai_strerror(errno) << std::endl;
        return false;
    }

    // Set the socket as a listening
    if(listen(listening, SOMAXCONN) == -1)
    {
        std::cerr << "Socket failed to start listening" << std::endl;;
    }

    FD_ZERO(&master);

    FD_SET(listening, &master);
    FD_SET(udp, &master);

    running = true;
    safeToExit = false;
    std::thread tr(&Server::ProcessNetworkEvents, this);
    tr.detach();

    //std::cout << "Server Initialized!"  << std::endl;
    return true;
}

void netlib::Server::Stop()
{
    if(running)
    {
        running = false;
        while(!safeToExit){};

        shutdown(listening, SHUT_RDWR);
        close(listening);
        shutdown(udp, SHUT_RDWR);
        close(udp);
        for (auto const &socket : sockets)
        {
            shutdown(socket, SHUT_RDWR);
            close(socket);
        }
        sockets.clear();
    }
}

void netlib::Server::ProcessNetworkEvents()
{
    while(running) {
        fdLock->lock();
        fd_set mCopy = master;
        fdLock->unlock();

        timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 100000;
        unsigned int socketCount = select(SOMAXCONN, &mCopy, nullptr, nullptr, &tv);


        if (FD_ISSET(listening, &mCopy))
        {
            HandleConnectionEvent();
        }
        else
        {
            if(FD_ISSET(udp, &mCopy))
            {
                HandleMessageEvent(udp, false);
            }
            fdLock->lock();
            for (auto const &socket : sockets)
            {
                if (FD_ISSET(socket, &mCopy))
                {
                    fdLock->unlock();
                    HandleMessageEvent(socket, true);
                    fdLock->lock();
                    break;
                }
            }
        }
        fdLock->unlock();
    }
    safeToExit = true;
}

void netlib::Server::HandleConnectionEvent()
{
    sockaddr_in client;
    client.sin_family = AF_INET;
    socklen_t clientSize;
    char host[NI_MAXHOST];
    char svc[NI_MAXSERV];

    int clientSocket =
            accept(listening, (sockaddr*)&client, &clientSize);
    if(clientSocket == -1)
    {
        std::cerr << "Client failed to connect" << std::endl;;
    }

    fdLock->lock();
    // Add new connection to master set
    FD_SET(clientSocket, &master);
    sockets.push_front(clientSocket);
    fdLock->unlock();

    // Assign the connection a uid
    ClientInfo newClient;
    // Since unix sockets are represented as an int, we can just use those directly for the uid
    newClient.uid = clientSocket;
    pendingAddr.push_back(clientSocket);

    memset(host, 0, NI_MAXHOST);
    memset(svc, 0, NI_MAXSERV);

    int result = getnameinfo((sockaddr*)&client,
                             sizeof(client),
                             host,
                             NI_MAXHOST,
                             svc,
                             NI_MAXSERV,
                             0);


    newClient.ipv4 = inet_ntoa(client.sin_addr);

    if(result == 0)
    {
        newClient.name = host;
    }
    else
    {
        std::cerr << gai_strerror(result) << std::endl;
        inet_ntop(AF_INET, &client.sin_addr, host, NI_MAXHOST);

        newClient.name = host;
        newClient.ipv4 = ntohs(client.sin_port);
    }

    //std::cout << "New Client: " << newClient.name << " Connected with ip " << newClient.ipv4 << std::endl;
    processNewClient(newClient);
}

void netlib::Server::HandleMessageEvent(int sock, bool isTCP)
{
    // Accept a new message
    auto& data = isTCP ? dataTCP : dataUDP;
    auto& writePos =  isTCP ? writePosTCP : writePosUDP;
    auto& bytesReceived =  isTCP ? bytesReceivedTCP : bytesReceivedUDP;
    int offset = isTCP ? 1 : 1 + sizeof(unsigned int);
    struct sockaddr_in si;
    int newBytes;
    unsigned int clientID;
    if(isTCP)
    {
        newBytes = recv(sock, data.data() + writePos, MAX_PACKET_SIZE+1, 0);
    }
    else
    {
        socklen_t sockLen = sizeof(si);
        newBytes = recvfrom(udp, data.data() + writePos, MAX_PACKET_SIZE + 1, 0, (struct sockaddr *)&si,&sockLen);
        if(newBytes == 0)
            return;
    }

    if (newBytes > 0)
    {
        bytesReceived += newBytes;
        // First byte of a packet tells us the length of the remaining data
        while(bytesReceived > 0 && data[readPos] + offset <= bytesReceived)
        {
            if(isTCP)
            {
                clientID = sock;
            }
            else
            {
                clientID = *reinterpret_cast<unsigned int*>(&data[readPos]);
                for(auto it = pendingAddr.begin(); it != pendingAddr.end(); it++)
                {
                    if(*it == clientID)
                    {
                        addrLookup[clientID] = si;
                        pendingAddr.erase(it);
                        break;
                    }
                }
                readPos += sizeof(unsigned int);
                bytesReceived -= sizeof(unsigned int);
            }
            auto packet = new NetworkEvent();
            packet->data.resize(data[readPos]);
            std::copy(data.data() + readPos + 1, data.data() + readPos + data[readPos], packet->data.data());
            bytesReceived -= data[readPos] + 1;
            readPos += data[readPos] + 1;
            packet->senderId = clientID;
            processPacket(packet);
        }

        if(bytesReceived != 0)
        {
            if(isTCP)
            {
                std::copy(data.data() + readPos, data.data() + readPos + data[readPos] + 1, data.data());
            }
            else
            {
                std::copy(data.data() + readPos, data.data() + readPos + data[readPos] + 1 + sizeof(unsigned int), data.data());
            }
            writePos = bytesReceived;
        }
        else
        {
            writePos = 0;
            for(int i = 0; i < data.size(); i++)
                data[i] = 0;
        }
        readPos = 0;
    }
    else if(isTCP) {
        std::cerr << strerror(errno) << std::endl;
        fdLock->lock();
        shutdown(sock, SHUT_RDWR);
        close(sock);
        FD_CLR(sock, &master);

        processDisconnectedClient(sock);

        fdLock->unlock();
    }

}

void netlib::Server::SendMessageToClient(const char *data, char dataLength, unsigned int client, bool sendTCP)
{
    if(dataLength <= 0)
        return;
    fdLock->lock();
    int wsResult;

    if(!sendTCP && std::count(pendingAddr.begin(), pendingAddr.end(), client) == 0)
    {
        char* sendData = new char[dataLength+1+sizeof(unsigned int)];
        sendData[sizeof(unsigned int)] = dataLength;
        char* clientAsChar = reinterpret_cast<char*>(&client);
        std::copy(clientAsChar, clientAsChar + sizeof(unsigned int), sendData);
        std::copy(data, data + dataLength, sendData+1+ sizeof(unsigned int));
        wsResult = sendto(udp, sendData,dataLength + 1 + sizeof(unsigned int), 0, (sockaddr*)&addrLookup[client], sizeof(addrLookup[client]));
        delete[] sendData;
    }
    else
    {
        char *sendData = new char[dataLength + 1];
        sendData[0] = dataLength;
        std::copy(data, data + dataLength, sendData + 1);
        wsResult = send(client, sendData,dataLength+1, 0);
        delete[] sendData;
    }

    if (wsResult == -1)
    {
        std::cerr << "Failed to send message." << std::endl;
    }
    fdLock->unlock();
}

void netlib::Server::DisconnectClient(unsigned int client)
{
    shutdown(client, SHUT_RDWR);
    close(client);
    FD_CLR(client, &master);
    sockets.remove(client);
    processDisconnectedClient(client);
}
