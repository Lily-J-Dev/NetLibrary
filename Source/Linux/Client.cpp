#include <iostream>
#include <string>
#include <thread>
#include <cstring>

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
        shutdown(sock, SHUT_RDWR);
        close(sock);
        shutdown(udp, SHUT_RDWR);
        close(udp);
    }
}

bool netlib::Client::Start(const std::string &ipv4, int port)
{
    dataUDP.resize((MAX_PACKET_SIZE+1)*2);
    dataTCP.resize((MAX_PACKET_SIZE+1)*2);
    //std::cout << "Initializing Client..." << std::endl;
    // Create a socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    udp = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if(sock == -1)
    {
        std::cerr << "Failed to create socket: "  << strerror(errno) << std::endl;
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
        std::cerr << "Failed to connect to the server: "  << strerror(errno) << std::endl;
        return false;
    }

    FD_ZERO(&master);

    FD_SET(sock, &master);
    FD_SET(udp, &master);

    si.sin_family = AF_INET;
    si.sin_port = htons(port);
    si.sin_addr.s_addr = inet_addr(ipv4.c_str());

    //std::cout << "Client successfully initilized!" << std::endl;

    sockCopy = sock;
    running = true;
    safeToExit = false;
    std::thread tr(&Client::ProcessNetworkEvents, this);
    tr.detach();

    return true;
}

void netlib::Client::SendMessageToServer(const char *data, char dataLength)
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


    if (sendResult == -1)
    {
        std::cerr << "Error in sending data: " << strerror(errno) <<  std::endl;
    }
}

void netlib::Client::ProcessNetworkEvents()
{
    // Loop to send and receive data
    char buf[netlib::MAX_PACKET_SIZE];

    while(running)
    {
        timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 100000;
        fd_set mCopy = master;
        unsigned int socketCount = select(SOMAXCONN, &mCopy, nullptr, nullptr, &tv);

        if(socketCount > 0)
        {
            if (FD_ISSET(sock, &mCopy))
            {
                HandleMessageEvent(sock, true);
            }
            if (FD_ISSET(udp, &mCopy))
            {
                HandleMessageEvent(udp, false);
            }
        }
    }
    safeToExit = true;
}

void netlib::Client::HandleMessageEvent(int s, bool isTCP)
{
    auto& data = isTCP ? dataTCP : dataUDP;
    auto& writePos =  isTCP ? writePosTCP : writePosUDP;
    auto& bytesReceived =  isTCP ? bytesReceivedTCP : bytesReceivedUDP;
    int offset = isTCP ? 1 : 1 + sizeof(unsigned int);
    int newBytes;

    if(isTCP)
    {
        newBytes = recv(s, data.data() + writePos, MAX_PACKET_SIZE + 1, 0);
        if(newBytes == -1) {

            std::cerr << "Error in receiving data from TCP socket : " << strerror(errno) << std::endl;
        }
    }
    else
    {
        socklen_t len = sizeof(si);
        newBytes = recvfrom(udp, data.data() + writePos, MAX_PACKET_SIZE + 1, 0, (sockaddr *) &si, &len);
        if(newBytes == -1) {

            std::cerr << "Error in receiving data from UDP socket : " << strerror(errno) << std::endl;
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
        processDisconnect();
    }
}

void netlib::Client::SetUID(unsigned int uid)
{
    uidOnServer = uid;
    uidAsChar = reinterpret_cast<char*>(&uidOnServer);
    uidSet = true;
}