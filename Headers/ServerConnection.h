#ifndef CTP_SERVERCONNECTION_H
#define CTP_SERVERCONNECTION_H

#include "Windows/Server.h"
#include "NetworkDevice.h"
#include "Lobby.h"

#ifdef netlib_test_EXPORTS
/*Enabled as "export" while compiling the dll project*/
    #define DLLEXPORT __declspec(dllexport)
#else
/*Enabled as "import" in the Client side for using already created dll file*/
#define DLLEXPORT __declspec(dllimport)
#endif

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
    private:
        void SendPacket(NetworkEvent *event) override;
        void ProcessNewClient(ClientInfo info);
        void ProcessDisconnectedClient(unsigned int clientUID);
        void ProcessDeviceSpecificEvent(NetworkEvent *event) override;
        void UpdateNetworkStats() override;
        void TerminateConnection(unsigned int clientUID) override;

        void CreateNewLobby(NetworkEvent* event);
        void AddPlayerToLobby(unsigned int player, unsigned int lobby);
        void SendEventToAll(NetworkEvent* event);

        Server server;
        std::map<int, ClientInfo> connectedClients;

        std::mutex clientInfoLock;

        // Lobby
        unsigned int lobbyUID = 0;
        std::recursive_mutex lobbyLock;
        std::map<unsigned int, Lobby> lobbies;
    };
}

#endif //CTP_SERVERCONNECTION_H
