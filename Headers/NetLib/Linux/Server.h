#pragma once
#include <atomic>
#include <mutex>
#include <functional>
#include <map>
#include <sys/types.h>
#include <netdb.h>
#include <forward_list>

#include "NetLib/Constants.h"
#include "NetLib/NetworkEvent.h"
#include "NetLib/ClientInfo.h"

namespace netlib
{
class Server
{
public:
    Server() = default;
    Server(const Server&) = delete;
    Server& operator=(Server const&) = delete;
    ~Server();

    bool Start(int port);
    void Stop();

    void SendMessageToClient(const char* data, char dataLength, unsigned int client, bool sendTCP);
    void DisconnectClient(unsigned int client);

    std::function<void(netlib::NetworkEvent*)> processPacket;
    std::function<void(netlib::ClientInfo)> processNewClient;
    std::function<void(unsigned int)> processDisconnectedClient;

    bool IsRunning(){return running;};

private:
    void ProcessNetworkEvents();
    void HandleConnectionEvent();
    void HandleMessageEvent(int sock, bool isTCP);

    fd_set master;
    int udp = 0;
    int listening = 0;
    sockaddr_in hint;
    char cBuf[netlib::MAX_PACKET_SIZE];

    std::atomic_bool running{false};
    std::atomic_bool safeToExit{true};
    std::mutex* fdLock;
    std::forward_list<int> sockets;

    unsigned int readPos = 0;

    std::map<unsigned int, struct sockaddr_in> addrLookup;
    std::vector<unsigned int> pendingAddr;

    int bytesReceivedTCP = 0;
    int bytesReceivedUDP = 0;
    std::vector<char> dataTCP;
    unsigned int writePosTCP = 0;
    std::vector<char> dataUDP;
    unsigned int writePosUDP = 0;
};
}