#ifndef CTP_SERVERCONNECTION_H
#define CTP_SERVERCONNECTION_H

#ifdef WIN32
#include "NetLib/Windows/Server.h"
#else
#include "NetLib/Linux/Server.h"
#endif
#include "NetworkDevice.h"
#include "Lobby.h"

namespace netlib {
    class ServerConnection : public NetworkDevice {
    public:

        ServerConnection() = default;

        ~ServerConnection();

        /// Starts the server up
        void Start(unsigned short port);
        /// Removes a given client from the server
        void DisconnectClient(unsigned int clientUID);
        /// Returns true if the server is running
        bool IsRunning(){return server.IsRunning();};

        /// Sends the given data to the target client (clientUID).
        void SendMessageTo(const std::vector<char>& data, unsigned int clientUID);
        /// Sends the given data of length (dataLen) to the target client (clientUID).
        void SendMessageTo(const char* data, int dataLen, unsigned int clientUID);

        /// Sends the given data to all clients. Can specify to only send to a target lobby (lobbyFilter)
        void SendMessageToAll(const std::vector<char>& data, unsigned int lobbyFilter = 0);
        /// Sends the given data of length (dataLen) to all clients. Can specify to only send to a target lobby (lobbyFilter)
        void SendMessageToAll(const char* data, int dataLen, unsigned int lobbyFilter = 0);

        /// Sends the given data to all clients excluding the target client (clientUID). Can specify to only send to a target lobby (lobbyFilter)
        void SendMessageToAllExcluding(const std::vector<char>& data, unsigned int clientUID, unsigned int lobbyFilter = 0);
        /// Sends the given data of length (dataLen) to all clients excluding the target client (clientUID). Can specify to only send to a target lobby (lobbyFilter)
        void SendMessageToAllExcluding(const char* data, int dataLen, unsigned int clientUID, unsigned int lobbyFilter = 0);

        /// Returns a copy of the connection information for a given client
        ClientInfo GetClientInfo(unsigned int clientUID);
        /// Returns a copy of the connection information for all clients
        std::vector<ClientInfo> GetAllClients();

        /// Returns a copy of the current state of all lobbies
        std::vector<Lobby> GetAllLobbies();
        /// Returns a copy of the current state for a given lobby
        Lobby GetLobby(unsigned int lobbyID);
        /// Removes a given client from a given lobby
        void RemoveClientFromLobby(unsigned int clientID, unsigned int lobbyID);
    private:
        void SendPacket(NetworkEvent *event) override;
        void ProcessNewClient(ClientInfo info);
        void ProcessDisconnectedClient(unsigned int clientUID);
        void ProcessDeviceSpecificEvent(NetworkEvent *event) override;
        void UpdateNetworkStats() override;
        void TerminateConnection(unsigned int clientUID) override;

        void CreateNewLobby(NetworkEvent* event);
        void AddClientToLobby(unsigned int client, unsigned int lobby, bool forceSlot = false, unsigned int slot = 0);
        void AddOpenLobby(unsigned int lobbyID, unsigned int playerUID, bool sendToAll = false);
        void SendEventToAll(NetworkEvent* event);

        Server server;
        std::map<unsigned int, ClientInfo> connectedClients;

        std::mutex clientInfoLock;

        // Lobby
        unsigned int lobbyUID = 1;
        std::mutex lobbyLock;
        std::map<unsigned int, Lobby> lobbies;
        std::map<unsigned int, std::vector<std::pair<unsigned int,std::string>>> disconnectedMembers;
    };
}

#endif //CTP_SERVERCONNECTION_H
