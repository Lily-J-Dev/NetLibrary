#ifndef NETLIB_LOBBYMEMBER_H
#define NETLIB_LOBBYMEMBER_H
#include <string>

namespace netlib
{
    struct LobbyMember
    {
        float ping = 0;
        bool ready = false;
        std::string name = "";
        unsigned int uid = 0;
    };
}

#endif //NETLIB_LOBBYMEMBER_H
