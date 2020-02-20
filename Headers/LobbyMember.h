#ifndef NETLIB_LOBBYMEMBER_H
#define NETLIB_LOBBYMEMBER_H
#include <string>

namespace netlib
{
    struct LobbyMember
    {
        friend class ClientConnection;
        float ping = 0;
        bool ready = false;
        std::string name = "";
        unsigned int uid = 0;
        unsigned int lobbySlot = 0;


    private:
        static bool Sort(const LobbyMember& a,const LobbyMember& b)
        {
            return a.lobbySlot < b.lobbySlot;
        }
    };
}

#endif //NETLIB_LOBBYMEMBER_H
