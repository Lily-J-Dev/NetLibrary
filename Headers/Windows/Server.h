#pragma once
#include <ws2tcpip.h>
#include <atomic>
#include <mutex>
#include <functional>
#include <map>
#include "DataPacket.h"
#include "ClientInfo.h"

class Server
{
public:
    Server() = default;
    ~Server();

    bool Start();
    void Stop();

    void SendMessage(const char* data, int dataLength, unsigned int client);

    std::function<void(DataPacket*)> processPacket;
    std::function<void(ClientInfo)> processNewClient;

private:
    void ProcessNetworkEvents();
    void HandleConnectionEvent();
    void HandleMessageEvent(const SOCKET& sock, unsigned int id);

    fd_set master;
    SOCKET listening = INVALID_SOCKET;
    sockaddr_in hint;
    char cBuf[4096];
    std::map<SOCKET, unsigned int> uidLookup; // Get the UID of the given socket
    std::map<unsigned int, size_t> indexLookup; // Gets the index in the master fd_set of the socket of a given ID
    unsigned int nextUid = 0;

    std::atomic_bool running;
    std::mutex deleteSafeguard;


};