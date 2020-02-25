#include <iostream>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <string>
#include <thread>

#include "Client.h"
#include "NetLib/Constants.h"

netlib::Client::~Client()
{
    if(running)
    {
        Stop();
    }
}

void netlib::Client::Stop()
{
    if(running)
    {
        running = false;
        while(!safeToExit);
        close(sock);
    }
}

bool netlib::Client::Start(const std::string &ipv4, int port)
{
    //std::cout << "Initializing Client..." << std::endl;
    // Create a socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if(sock == -1)
    {
        std::cerr << "Failed to create socket" << std::endl;
        return false;
    }

    // Create hint structure
    sockaddr_in hint;
    hint.sin_family = AF_INET;
    hint.sin_port = htons(port);
    inet_pton(AF_INET, ipv4.c_str(), &hint.sin_addr);

    // Connect to the server
    int conResult = connect(sock, (sockaddr*)&hint, sizeof(hint));
    if(conResult == -1)
    {
        std::cerr << "Failed to connect to the server" << std::endl;
        return false;
    }

    //std::cout << "Client successfully initilized!" << std::endl;

    sockCopy = sock;
    running = true;
    safeToExit = false;
    std::thread tr(&Client::ProcessNetworkEvents, this);
    tr.detach();

    return true;
}

void netlib::Client::SendMessageToServer(const char *data, int dataLength)
{
    // If there is no data dont send it
    if(dataLength > 0)
    {
        int sendResult = send(sockCopy, data, dataLength, 0);
        if (sendResult == -1)
        {
            std::cerr << "Error in sending data" << std::endl;
        }
    }
}

void netlib::Client::ProcessNetworkEvents()
{
    // Loop to send and receive data
    char buf[netlib::MAX_PACKET_SIZE];

    while(running)
    {
        // Wait for response
        memset(buf,0, MAX_PACKET_SIZE);
        int bytesReceived = recv(sock, buf, MAX_PACKET_SIZE, 0);

        if (bytesReceived > 0)
        {
            auto packet = new NetworkEvent();
            packet->data.resize(bytesReceived);
            memcpy(packet->data.data(),buf, bytesReceived);
            processPacket(packet);
            // Echo response to console
            //std::cout << std::string(buf, 0, bytesReceived) << std::endl;
        }
        else
        {
            processDisconnect();
        }
    }
    safeToExit = true;
}