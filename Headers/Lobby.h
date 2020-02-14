#ifndef NETLIB_LOBBY_H
#define NETLIB_LOBBY_H

#include <vector>
#include "LobbyMember.h"

namespace netlib
{
    struct Lobby
        {
        std::string name = "";
        unsigned int lobbyID = 0;
        std::vector<netlib::LobbyMember> memberInfo;
        int clientsInRoom = 0;
        int maxClientsInRoom = 0;
        bool open = true;
    };
}
#endif //NETLIB_LOBBY_H
