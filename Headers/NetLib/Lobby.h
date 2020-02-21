#ifndef NETLIB_LOBBY_H
#define NETLIB_LOBBY_H

#include <vector>
#include "LobbyMember.h"

namespace netlib
{
    struct Lobby
        {
            // The name of the lobby
            std::string name = "";
            // The unique ID of the lobby
            unsigned int lobbyID = 0;
            // Information about the current lobby member
            std::vector<netlib::LobbyMember> memberInfo;
            // The number of clients in this lobby
            int clientsInRoom = 0;
            // The maximum number of clients allowed in the room
            int maxClientsInRoom = 0;
            // While the room is closed, new clients cannot connect to it
            bool open = true;

    };
}
#endif //NETLIB_LOBBY_H
