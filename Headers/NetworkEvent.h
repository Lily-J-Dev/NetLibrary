#ifndef NETLIB_NETWORKEVENT_H
#define NETLIB_NETWORKEVENT_H
#include <vector>

namespace netlib {
    struct NetworkEvent
    {
        NetworkEvent() = default;

        enum class EventType : int
         {
            // These events are created by the various SendMessageToX methods on client/server, the contents of the
            // message can be found in data.data.
             MESSAGE,
            // On the client this event occurs after ConnectToIP is called and once fully connected to a server.
            // On the server this event occurs when a new client has connected, the client can be found in data.senderID.
             ONCONNECT,
            // On the client this event occurs after the client has disconnected from the server.
            // On the server this event occurs when a client has disconnected, the client can be found in data.senderID.
            ONDISCONNECT,
            // On the client this event occurs when the client successfully joins a lobby
            ONLOBBYJOIN,
            // On the client this event occurs when the client has been removed from its lobby
            REMOVEDFROMLOBBY,
            // On the client this event occurs when the client re-connects to the server and automatically re-joins its
            // previous lobby. Note this will only happen if the lobby is closed.
            ONLOBBYREJOIN,

        };

        EventType eventType = EventType::MESSAGE; // What kind of event is this?
        std::vector<char> data; // Contains the data for MESSAGE events
        unsigned int senderId = 0; // On the server this contains the unique ID for the client that the event was generated for
    };
}
#endif //NETLIB_NETWORKEVENT_H
