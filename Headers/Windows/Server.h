#pragma once
#include <ws2tcpip.h>
#include <atomic>
#include <mutex>
#include <functional>
#include <map>
#include "NetworkEvent.h"
#include "ClientInfo.h"
#include "Constants.h"
namespace netlib {
    class Server {
    public:
        Server() = default;

        ~Server();

        bool Start(int port);

        void Stop();

        void SendMessageToClient(const char *data, int dataLength, unsigned int client);
        void DisconnectClient(unsigned int client);

        std::function<void(NetworkEvent*)> processPacket;
        std::function<void(ClientInfo)> processNewClient;
        std::function<void(unsigned int)> processDisconnectedClient;

    private:
        void ProcessNetworkEvents();

        void HandleConnectionEvent();

        void HandleMessageEvent(const SOCKET &sock);

        fd_set master;
        SOCKET listening = INVALID_SOCKET;
        sockaddr_in hint;
        char cBuf[MAX_PACKET_SIZE];
        std::map<SOCKET, unsigned int> uidLookup; // Get the UID of the given socket
        std::map<unsigned int, size_t> indexLookup; // Gets the index in the master fd_set of the socket of a given ID
        unsigned int nextUid = 1;

        std::atomic_bool running;
        std::mutex fdLock;
        std::mutex deleteLock;


    };
}