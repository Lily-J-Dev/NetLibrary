#ifndef CTP_SERVERCONNECTION_H
#define CTP_SERVERCONNECTION_H

#include "Server.h"
#include "NetworkDevice.h"
#include "Lobby.h"

namespace netlib {
    class ServerConnection : public NetworkDevice {
    public:
        ServerConnection();

        ~ServerConnection();

        void Start(int port);
        void DisconnectClient(unsigned int clientUID);

        void SendMessageTo(const std::vector<char>& data, unsigned int clientUID);
        void SendMessageTo(const char* data, int dataLen, unsigned int clientUID);

        void SendMessageToAll(const std::vector<char>& data);
        void SendMessageToAll(const char* data, int dataLen);

        void SendMessageToAllExcluding(const std::vector<char>& data, unsigned int clientUID);
        void SendMessageToAllExcluding(const char* data, int dataLen, unsigned int clientUID);

        ClientInfo GetClientInfo(unsigned int clientUID); // Gets the connection information for a given client id
        std::vector<ClientInfo> GetAllClients();

        std::vector<Lobby> GetAllLobbies();
        Lobby GetLobby(unsigned int lobbyID);
        void RemoveClientFromLobby(unsigned int clientID, unsigned int lobbyID);
    private:
        void SendPacket(NetworkEvent *event) override;
        void ProcessNewClient(ClientInfo info);
        void ProcessDisconnectedClient(unsigned int clientUID);
        void ProcessDeviceSpecificEvent(NetworkEvent *event) override;
        void UpdateNetworkStats() override;
        void TerminateConnection(unsigned int clientUID) override;

        void CreateNewLobby(NetworkEvent* event);
        void AddClientToLobby(unsigned int client, unsigned int lobby);
        void SendEventToAll(NetworkEvent* event);

        Server server;
        std::map<int, ClientInfo> connectedClients;

        std::mutex clientInfoLock;

        // Lobby
        unsigned int lobbyUID = 1;
        std::mutex lobbyLock;
        std::map<unsigned int, Lobby> lobbies;
    };
}

#endif //CTP_SERVERCONNECTION_H
