#ifndef CTP_CLIENTINFO_H
#define CTP_CLIENTINFO_H

#include "ConnectionInfo.h"
#include <chrono>

namespace netlib {
    class ServerConnection;

    struct ClientInfo {
        friend class ServerConnection;

        // The ip address of the client
        std::string ipv4 = "";
        // The name of the device that this client connected with
        std::string name = "";
        // The unique ID of this client
        unsigned int uid = 0;
        // The current lobby that this client is in (0 if none)
        unsigned int lobbyID = 0;
        // A struct of network diagnostic info for this connection (currently only ping)
        ConnectionInfo connectionInfo;


    private:
        bool waitingForPing = false;
        std::chrono::steady_clock::time_point timeOfLastPing = std::chrono::steady_clock::now();
    };

}
#endif //CTP_CLIENTINFO_H
