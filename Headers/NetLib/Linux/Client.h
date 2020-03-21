#pragma once
#include <atomic>
#include <mutex>
#include <queue>
#include <functional>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "NetLib/NetworkEvent.h"
namespace netlib
{
class Client
{
public:
    Client() = default;
    ~Client();

    bool Start(const std::string& ipv4, int port);
    void Stop();

    void SendMessageToServer(const char* data, char dataLength);

    bool IsRunning(){return running;};
    void SetUID(unsigned int uid);

    std::function<void(netlib::NetworkEvent*)> processPacket;
    std::function<void()> processDisconnect;
private:
    void ProcessNetworkEvents();
    void HandleMessageEvent(int s, bool isTCP);

    struct sockaddr_in si;
    fd_set master;
    bool uidSet = false;
    unsigned int uidOnServer;
    char* uidAsChar = nullptr;

    int sock = -1;
    int udp = -1;
    int sockCopy = -1;

    std::atomic_bool running{false};
    std::atomic_bool safeToExit{true};

    unsigned int readPos = 0;

    int bytesReceivedTCP = 0;
    int bytesReceivedUDP = 0;
    std::vector<char> dataTCP;
    unsigned int writePosTCP = 0;
    std::vector<char> dataUDP;
    unsigned int writePosUDP = 0;
};
}
