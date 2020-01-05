#pragma once
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

    bool Start(const std::string& ipv4);
    void Stop();

    void SendMessage(const char* data, int dataLength);

    std::function<void(DataPacket*)> processPacket;
private:
    void ProcessNetworkEvents();

    int sock = -1;
    std::queue<DataPacket> outData;

    std::atomic_bool running;
    std::mutex* deleteSafeguard;
};

