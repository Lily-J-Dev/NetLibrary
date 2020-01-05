#pragma once
#include <atomic>
#include <mutex>
#include <functional>
#include <map>
#include <sys/types.h>
#include <netdb.h>
#include <forward_list>


#include "DataPacket.h"
#include "ClientInfo.h"

class Server
{
public:
    Server() = default;
    Server(const Server&) = delete;
    Server& operator=(Server const&) = delete;
    ~Server();

    bool Start();
    void Stop();

    void SendMessage(const char* data, int dataLength, unsigned int client);

    std::function<void(DataPacket*)> processPacket;
    std::function<void(ClientInfo)> processNewClient;

private:
    void ProcessNetworkEvents();
    void HandleConnectionEvent();
    void HandleMessageEvent(int sock);

    fd_set master;
    unsigned int listening = 0;
    sockaddr_in hint;
    char cBuf[4096];

    std::atomic_bool running;
    std::mutex* deleteSafeguard;
    std::forward_list<int> sockets;
};