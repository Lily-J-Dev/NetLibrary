#pragma once
#include <ws2tcpip.h>
#include <atomic>
#include <mutex>
#include <functional>
#include <map>
#include <vector>
#include "NetLib/NetworkEvent.h"
#include "NetLib/ClientInfo.h"
#include "NetLib/Constants.h"
namespace netlib {
    class Server {
    public:
        Server() = default;

        ~Server();

        bool Start(unsigned short port);

        void Stop();

        void SendMessageToClient(const char *data, char dataLength, unsigned int client, bool sendTCP);
        void DisconnectClient(unsigned int client);

        bool IsRunning(){return running;};

        std::function<void(NetworkEvent*)> processPacket;
        std::function<void(ClientInfo)> processNewClient;
        std::function<void(unsigned int)> processDisconnectedClient;

    private:
        void ProcessNetworkEvents();

        void HandleConnectionEvent();

        void HandleMessageEvent(const SOCKET &sock, bool isTCP);

        fd_set master;
        SOCKET listening = INVALID_SOCKET;
        SOCKET udp = INVALID_SOCKET;
        sockaddr_in hint;

        std::map<SOCKET, unsigned int> uidLookup; // Get the UID of the given socket
        std::map<unsigned int, size_t> indexLookup; // Gets the index in the master fd_set of the socket of a given ID
        std::map<unsigned int, struct sockaddr_in> addrLookup;
        std::vector<unsigned int> pendingAddr;
        unsigned int nextUid = 1;

        std::vector<char> data;
        unsigned int readPos = 0;
        unsigned int writePos = 0;
        int bytesReceived = 0;

        std::atomic_bool running{false};
        std::atomic_bool safeToExit{true};
        std::mutex fdLock;
    };
}