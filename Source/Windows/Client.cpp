#include "Windows/Client.h"
#include <iostream>
#include <thread>
#include <string>
#include "Constants.h"

#pragma comment (lib, "Ws2_32.lib")

Client::~Client()
{
    Stop();
    deleteSafeguard.lock();
    deleteSafeguard.unlock();
}

void Client::Stop()
{
    running = false;
    deleteSafeguard.lock();
    deleteSafeguard.unlock();
    closesocket(sock);
    WSACleanup();
}

int Client::Start(const std::string& ipv4, int port)
{
    std::cout << "Initializing Client..." << std::endl;

    // Initilise winsock
    WSAData data;
    WORD ver = MAKEWORD(2, 2);
    int wsResult = WSAStartup(ver, &data);

    if (wsResult != 0)
    {
        std::cerr << "Failed to initialize winsock error code: " << wsResult;
        return -1;
    }

    // Create socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET)
    {
        std::cerr << "Failed to create socket error code: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return -2;
    }

    // Fill in hint struct
    sockaddr_in hint;
    hint.sin_family = AF_INET;
    hint.sin_port = htons(port);
    inet_pton(AF_INET, ipv4.c_str(), &hint.sin_addr);

    // Connect to server
    int connResult = connect(sock, (sockaddr*)&hint, sizeof(hint));
    if (connResult == SOCKET_ERROR)
    {
        std::cerr << "Failed to connect to server error number: " << WSAGetLastError() << std::endl;
        closesocket(sock);
        WSACleanup();
        return -1;
    }

    std::cout << "Client successfully initilized!" << std::endl;

    std::thread tr(&Client::ProcessNetworkEvents, this);
    tr.detach();

    return true;
}


void Client::ProcessNetworkEvents()
{
    deleteSafeguard.lock();
    running = true;
    // Loop to send and receive data
    char buf[MAX_PACKET_SIZE];

    while(running)
    {
        // Wait for response
        ZeroMemory(buf, MAX_PACKET_SIZE);
        int bytesReceived = recv(sock, buf, MAX_PACKET_SIZE, 0);

        if (bytesReceived > 0)
        {
            auto packet = new DataPacket();
            packet->data = new char[bytesReceived];
            memcpy(packet->data,buf, bytesReceived);
            packet->dataLength = bytesReceived;
            processPacket(packet);
        }
    }

    deleteSafeguard.unlock();
}

void Client::SendMessageToServer(const char* data, unsigned int dataLength)
{
    // If there is no data, just return as winsock uses 0-length messages to signal exit.
    if(dataLength <= 0)
        return;
    int sendResult = send(sock, data, dataLength, 0);
    if (sendResult == SOCKET_ERROR)
    {
        std::cerr << "Error in sending data error number : " << WSAGetLastError() << std::endl;
    }
}
