#include "Client.h"
#include <iostream>
#include <thread>
#include <string>
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
        WSACleanup();
    }
}

int netlib::Client::Start(const std::string& ipv4, unsigned short port)
{
    //std::cout << "Initializing Client..." << std::endl;

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
    if (sock == INVALID_SOCKET)
    {
        std::cerr << "Failed to create socket error code: " << WSAGetLastError() << std::endl;
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
    std::vector<char> data;
    data.resize((MAX_PACKET_SIZE+1)*2);
    unsigned int readPos = 0;
    unsigned int writePos = 0;
    int bytesReceived = 0;

    while(running)
    {
        // Wait for response
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
            running = false;
            processDisconnect();
        }
    }
    safeToExit = true;
}

void netlib::Client::SendMessageToServer(const char* data, char dataLength)
{
    // If there is no data, just return as winsock uses 0-length messages to signal exit.
    if(dataLength <= 0)
        return;
    char* sendData = new char[dataLength+1];
    sendData[0] = dataLength;
    std::copy(data, data + dataLength, sendData+1);
    int sendResult = send(sockCopy, sendData, dataLength+1, 0);
    delete[] sendData;

    if (sendResult == SOCKET_ERROR)
    {
        std::cerr << "Error in sending data error number : " << WSAGetLastError() << std::endl;
    }
}
