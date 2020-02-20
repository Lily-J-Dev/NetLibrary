#ifndef NETLIB_NETWORKEVENT_H
#define NETLIB_NETWORKEVENT_H
#include <vector>

namespace netlib {
    struct NetworkEvent
    {
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


        };

        EventType eventType = EventType::MESSAGE; // What kind of event is this?
        std::vector<char> data; // Contains the data for MESSAGE events
        unsigned int senderId = 0; // On the server this contains the unique ID for the client that the event was generated for

        template <class T>
        void WriteData(T newData, int writePos)
        {
            char* dataAsChar = reinterpret_cast<char*>(&newData);
            std::copy(dataAsChar, dataAsChar + sizeof(T), data.data() + writePos);
        };

        template <class T>
        T ReadData(int readPos)
        {
            return *reinterpret_cast<T*>(data.data() + readPos);
        };
    };
}
#endif //NETLIB_NETWORKEVENT_H
