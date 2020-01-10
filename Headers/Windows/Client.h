#pragma once
#include <ws2tcpip.h>
#include <atomic>
#include <mutex>
#include <queue>
#include <functional>
#include "DataPacket.h"

class Client
{
public:
    Client() = default;
    ~Client();

    int Start(const std::string& ipv4, int port);
    void Stop();

    void SendMessageToServer(const char* data, unsigned int dataLength);

    std::function<void(DataPacket*)> processPacket;
private:
    void ProcessNetworkEvents();

    SOCKET sock = INVALID_SOCKET;

    std::atomic_bool running;
    std::mutex deleteSafeguard;
};

