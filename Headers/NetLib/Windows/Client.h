#pragma once
#include <ws2tcpip.h>
#include <atomic>
#include <mutex>
#include <queue>
#include <functional>
#include "NetLib/NetworkEvent.h"

namespace netlib {

    class Client {
    public:
        Client() = default;

        ~Client();

        int Start(const std::string &ipv4, unsigned short port);

        void Stop();

        void SendMessageToServer(const char *data, char dataLength);

        bool IsRunning(){return running;};

        std::function<void(NetworkEvent*)> processPacket;
        std::function<void()> processDisconnect;
    private:
        void ProcessNetworkEvents();
        void HandleMessageEvent(const SOCKET& s);

        fd_set master;

        SOCKET sock = INVALID_SOCKET;
        SOCKET udp = INVALID_SOCKET;
        SOCKET sockCopy = INVALID_SOCKET;

        std::atomic_bool safeToExit{true};
        std::atomic_bool running{false};

        std::vector<char> data;
        unsigned int readPos = 0;
        unsigned int writePos = 0;
        int bytesReceived = 0;
    };
}
