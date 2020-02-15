#ifndef CTP_CLIENTCONNECTION_H
#define CTP_CLIENTCONNECTION_H

#include <queue>
#include <map>
#include "Client.h"
#include "NetworkDevice.h"
#include "ConnectionInfo.h"
#include "Lobby.h"

namespace netlib {
    class ClientConnection : public NetworkDevice {
    public:
        ClientConnection();

        ~ClientConnection();

        void SendMessageToServer(const std::vector<char>& data);
        void SendMessageToServer(const char* data, int dataLen);

        // Tries to connect to the given ip and port, returning true if successful.
        bool ConnectToIP(const std::string &ipv4, int port);
        void Disconnect();

        ConnectionInfo GetConnectionInfo();
        unsigned int GetUID();

        void CreateLobby(std::string lobbyName, int lobbySize);
        void JoinLobby(unsigned int lobbyUID);
        void RemoveFromLobby(unsigned int playerUID);

        std::vector<Lobby> GetAllLobbyInfo();
        Lobby GetCurrentLobbyInfo();

        void SetReady(bool isReady);

    private:
        void ProcessDeviceSpecificEvent(NetworkEvent *event) override;
        void SendPacket(NetworkEvent *event) override;
        void UpdateNetworkStats() override;
        void ProcessDisconnect();

        Client client;

        unsigned int uid = 0;
        ConnectionInfo connectionInfo;

        std::mutex *clientInfoLock = nullptr;

        std::chrono::steady_clock::time_point timeOfLastPing = std::chrono::steady_clock::now();
        bool waitingForPing = false;

        std::mutex lobbyLock;
        std::vector<Lobby> allLobbies;
        unsigned int activeLobby = 0;
    };
}
#endif //CTP_CLIENTCONNECTION_H