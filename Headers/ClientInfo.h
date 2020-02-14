#ifndef CTP_CLIENTINFO_H
#define CTP_CLIENTINFO_H

#include "ConnectionInfo.h"
#include <chrono>

namespace netlib {
    class ServerConnection;

    struct ClientInfo {
        friend class ServerConnection;

        std::string ipv4 = "";
        std::string name = "";
        unsigned int uid = 0;
        ConnectionInfo connectionInfo;

    private:
        bool waitingForPing = false;
        std::chrono::steady_clock::time_point timeOfLastPing = std::chrono::steady_clock::now();
    };


#endif //CTP_CLIENTINFO_H
}