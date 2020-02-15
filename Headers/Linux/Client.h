#pragma once
#include <atomic>
#include <mutex>
#include <queue>
#include <functional>
#include "NetworkEvent.h"
namespace netlib
{
class Client
{
public:
    Client() = default;
    ~Client();

    bool Start(const std::string& ipv4, int port);
    void Stop();

    void SendMessageToServer(const char* data, int dataLength);

    std::function<void(netlib::NetworkEvent*)> processPacket;
    std::function<void()> processDisconnect;
private:
    void ProcessNetworkEvents();

    int sock = -1;
    int sockCopy = -1;

    std::atomic_bool running;
    std::mutex* deleteSafeguard;
};
}
