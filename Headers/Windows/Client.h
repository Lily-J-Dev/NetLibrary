#pragma once
#include <ws2tcpip.h>
#include <atomic>
#include <mutex>
#include <queue>
#include <functional>
#include "NetworkEvent.h"

namespace netlib {

    class Client {
    public:
        Client() = default;

        ~Client();

        int Start(const std::string &ipv4, int port);

        void Stop();

        void SendMessageToServer(const char *data, int dataLength);

        std::function<void(NetworkEvent*)> processPacket;
        std::function<void()> processDisconnect;
    private:
        void ProcessNetworkEvents();

        SOCKET sock = INVALID_SOCKET;
        SOCKET sockCopy = INVALID_SOCKET;

        std::atomic_bool running;
        std::mutex deleteGuard;
    };
}
