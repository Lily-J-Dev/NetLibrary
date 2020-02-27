#ifndef CTP_CLIENTCONNECTION_H
#define CTP_CLIENTCONNECTION_H

#include <queue>
#include <map>
#if defined WIN32 || defined _WIN32
#include "NetLib/Windows/Client.h"
#else
#include "NetLib/Linux/Client.h"
#endif
#include "NetworkDevice.h"
#include "ConnectionInfo.h"
#include "Lobby.h"

namespace netlib {
    class ClientConnection : public NetworkDevice {
    public:
        ~ClientConnection() override;
        ClientConnection() = default;

        /// Sends the given data to the server
        void SendMessageToServer(const std::vector<char>& data);
        /// Sends the given data of length (dataLen) to the server
        void SendMessageToServer(const char* data, int dataLen);

        /// Tries to connect to the given ip and port, returning true if successful.
        bool ConnectToIP(const std::string &ipv4, unsigned short port);
        /// Disconnects this client from the server
        void Disconnect();

        /// Returns true if connected to a server
        bool IsRunning(){return client.IsRunning();};

        /// Returns a copy of the connection information for this client
        ConnectionInfo GetConnectionInfo();
        /// Returns the unique ID for this client
        unsigned int GetUID();

        /// Requests a new lobby be created on the server and automatically adds this client to it
        void CreateLobby(std::string lobbyName, int lobbySize);
        /// Requests to join a given lobby
        void JoinLobby(unsigned int lobbyUID);
        /// Requests that a given client is removed from the lobby this client is currently in
        void RemoveFromLobby(unsigned int playerUID);

        /// Returns a copy of the current state of all open lobbies
        std::vector<Lobby> GetAllLobbyInfo();
        /// Returns true if the client is currently in a lobby
        bool IsInLobby();
        /// Returns a copy of the current state of the lobby this client is in (client must be in a lobby for this to work!)
        Lobby GetCurrentLobbyInfo();

        /// Returns a copy of the current member data for this client (client must be in a lobby for this to work!)
        LobbyMember GetMemberInfo();

        /// Requests that the server set the state of this client to ready in the currently active lobby
        void SetReady(bool isReady);
        /// Requests that the server sets the lobby open state to isOpen. Closed lobbies do not appear in GetAllLobbyInfo
        /// and can't be joined by new clients.
        void SetLobbyOpen(bool isOpen);

        /// Sets the name that will be displayed in lobbies
        void SetLobbyName(std::string newName);
    private:
        void ProcessDeviceSpecificEvent(NetworkEvent *event) override;
        void SendPacket(NetworkEvent *event) override;
        void UpdateNetworkStats() override;
        void ProcessDisconnect();
        void TerminateConnection(unsigned int clientUID) override {uid = clientUID;};

        Client client;

        unsigned int uid = 0;
        ConnectionInfo connectionInfo;

        std::mutex clientInfoLock;

        std::chrono::steady_clock::time_point timeOfLastPing = std::chrono::steady_clock::now();
        bool waitingForPing = false;

        std::mutex lobbyLock;
        std::map<unsigned int, Lobby> allLobbies;
        unsigned int activeLobby = 0;
    };
}
#endif //CTP_CLIENTCONNECTION_H