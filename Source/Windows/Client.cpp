#include "Client.h"
#include <iostream>
#include <thread>
#include <string>
#include <inaddr.h>
#include "NetLib/Constants.h"

#pragma comment (lib, "Ws2_32.lib")

netlib::Client::~Client()
{
    if(running)
        Stop();
}

void netlib::Client::Stop()
{
    if(running)
    {
        running = false;
        while(!safeToExit);
        closesocket(sock);
        closesocket(udp);
        WSACleanup();
    }
}

int netlib::Client::Start(const std::string& ipv4, unsigned short port)
{
    //std::cout << "Initializing Client..." << std::endl;
    dataUDP.resize((MAX_PACKET_SIZE+1)*2);
    dataTCP.resize((MAX_PACKET_SIZE+1)*2);
    // Initilise winsock
    WSAData data;
    WORD ver = MAKEWORD(2, 2);
    int wsResult = WSAStartup(ver, &data);

    if (wsResult != 0)
    {
        std::cerr << "Failed to initialize winsock error code: " << wsResult;
        return false;
    }

    // Create socket

    sock = socket(AF_INET, SOCK_STREAM, 0);
    udp = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if(udp == INVALID_SOCKET)
    {
        std::cerr << "Failed to create UDP socket error code: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return false;
    }
    if (sock == INVALID_SOCKET)
    {
        std::cerr << "Failed to create TCP socket error code: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return false;
    }

    // Fill in hint struct
    //sockaddr_in hint;
    //hint.sin_family = AF_INET;
    //hint.sin_port = htons(port);

    //inet_pton(AF_INET, ipv4.c_str(), &hint.sin_addr);

    struct addrinfo *info;
    struct addrinfo *node;
    struct addrinfo hints;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    int result = getaddrinfo(ipv4.c_str(), std::to_string(port).c_str(), &hints, &info);

    if(result != 0)
    {
        std::cerr << "Failed to get address info: " << WSAGetLastError() << std::endl;
        closesocket(sock);
        WSACleanup();
        return false;
    }


    // Connect to server
    int connResult = SOCKET_ERROR;
    for(node = info; node != NULL; node = node->ai_next)
    {
        connResult = connect(sock, node->ai_addr, node->ai_addrlen);
        if(connResult != SOCKET_ERROR)
            break;
    }

    if (connResult == SOCKET_ERROR)
    {
        std::cerr << "Failed to connect to server error number: " << WSAGetLastError() << std::endl;
        closesocket(sock);
        WSACleanup();
        return false;
    }

    freeaddrinfo(info);

    //int connResult = connect(sock, (sockaddr*)&hint, sizeof(hint));
    //if (connResult == SOCKET_ERROR)
    //{
    //    std::cerr << "Failed to connect to server error number: " << WSAGetLastError() << std::endl;
    //    closesocket(sock);
    //    WSACleanup();
    //    return -1;
    //}

    //std::cout << "Client successfully initialised!" << std::endl;

    FD_ZERO(&master);

    FD_SET(sock, &master);
    FD_SET(udp, &master);

    si.sin_family = AF_INET;
    si.sin_port = htons(port);
    si.sin_addr.S_un.S_addr = inet_addr(ipv4.c_str());

    // Create a copy to avoid threading issues when using the socket
    sockCopy = sock;
    running = true;
    safeToExit = false;
    std::thread tr(&Client::ProcessNetworkEvents, this);
    tr.detach();

    return true;
}


void netlib::Client::ProcessNetworkEvents()
{
    // Loop to send and receive data

    while(running)
    {
        TIMEVAL tv;
        tv.tv_sec = 0;
        tv.tv_usec = 100000;
        fd_set mCopy = master;
        unsigned int socketCount = select(0, &mCopy, nullptr, nullptr, &tv);

        for (unsigned int i = 0; i < socketCount; i++)
        {
            SOCKET s = mCopy.fd_array[i];
            if(s == sock)
            {
                HandleMessageEvent(s, true);
            }
            else if(s == udp)
            {
                HandleMessageEvent(s, false);
            }
            else std::cout << "Should never print" << std::endl;
        }
    }
    safeToExit = true;
}

void netlib::Client::HandleMessageEvent(const SOCKET& s, bool isTCP)
{
    auto& data = isTCP ? dataTCP : dataUDP;
    auto& writePos =  isTCP ? writePosTCP : writePosUDP;
    auto& bytesReceived =  isTCP ? bytesReceivedTCP : bytesReceivedUDP;
    int offset = isTCP ? 1 : 1 + sizeof(unsigned int);
    int newBytes;

    unsigned int clientID;
    if(isTCP)
    {
        newBytes = recv(s, data.data() + writePos, MAX_PACKET_SIZE + 1, 0);
    }
    else
    {
        int len = sizeof(si);
        newBytes = recvfrom(udp, data.data() + writePos, MAX_PACKET_SIZE + 1, 0, (sockaddr *) &si, &len);
        if(newBytes == SOCKET_ERROR) {
            if (WSAGetLastError() == 10054)
            {
                running = false;
                processDisconnect();
                return;
            }
            std::cerr << "Error in receiving data from UDP socket : " << WSAGetLastError() << std::endl;
        }
        if(newBytes <= 0)
            return;
    }
    if (newBytes > 0)
    {
        bytesReceived += newBytes;
        // First byte of a packet tells us the length of the remaining data
        while (bytesReceived > 0 && data[readPos] + offset <= bytesReceived) {
            if(!isTCP)
            {
                if(uidOnServer == *reinterpret_cast<unsigned int*>(&data[readPos]))
                {
                    readPos += sizeof(unsigned int);
                    bytesReceived -= sizeof(unsigned int);
                }
                else
                {
                    readPos += sizeof(unsigned int);
                    bytesReceived -= data[readPos] + 1 + sizeof(unsigned int);
                    readPos += data[readPos] + 1;
                    continue;
                }
            }
            auto packet = new NetworkEvent();
            packet->data.resize(data[readPos]);
            std::copy(data.data() + readPos + 1, data.data() + readPos + data[readPos],
                      packet->data.data());
            bytesReceived -= data[readPos] + 1;
            readPos += data[readPos] + 1;
            processPacket(packet);
        }

        if (bytesReceived != 0) {
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
    else
    {
        running = false;
        processDisconnect();
    }
}

void netlib::Client::SendMessageToServer(const char* data, char dataLength)
{
    // If there is no data, just return as winsock uses 0-length messages to signal exit.
    if(dataLength <= 0)
        return;

    int sendResult;
    if(uidSet)
    {
        char* sendData = new char[dataLength+1+sizeof(unsigned int)];
        sendData[sizeof(unsigned int)] = dataLength;
        std::copy(uidAsChar, uidAsChar + sizeof(unsigned int), sendData);
        std::copy(data, data + dataLength, sendData+1+ sizeof(unsigned int));
        sendResult = sendto(udp, sendData, dataLength + 1 + sizeof(unsigned int), 0, (sockaddr *) &si, sizeof(si));
        delete[] sendData;
    }
    else
    {
        char *sendData = new char[dataLength + 1];
        sendData[0] = dataLength;
        std::copy(data, data + dataLength, sendData + 1);
        sendResult = send(sockCopy, sendData, dataLength + 1, 0);
        delete[] sendData;
    }


    if (sendResult == SOCKET_ERROR)
    {
        std::cerr << "Error in sending data error number : " << WSAGetLastError() << std::endl;
    }
}

void netlib::Client::SetUID(unsigned int uid)
{
    uidOnServer = uid;
    uidAsChar = reinterpret_cast<char*>(&uidOnServer);
    uidSet = true;
}