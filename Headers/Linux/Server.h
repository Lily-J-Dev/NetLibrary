#pragma once
#include <atomic>
#include <mutex>
#include <functional>
#include <map>
#include <sys/types.h>
#include <netdb.h>

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
    void HandleMessageEvent(int sock, unsigned int id);

    fd_set master;
    int listening = -1;
    sockaddr_in hint;
    char cBuf[4096];

    std::atomic_bool running;
    std::mutex* deleteSafeguard;


};