#ifndef CTP_CONSTANTS_H
#define CTP_CONSTANTS_H
namespace netlib {
    const int MAX_PACKET_SIZE = 100;
    const int MAX_PACKET_SIZE_INTERNAL = MAX_PACKET_SIZE + 1 + sizeof(unsigned int);
    const float PING_FREQUENCY = 100;
    const int MAX_RESEND_DELAY = 500; // The longest a client/server will wait before re-sending a packet, a high number here could cause high loss connections to run extremely slowly
    const float RESEND_DELAY_MOD = 1.3f; // This number multiplied by current ping is how long a device will wait for a packet receipt before resending

    // MessageType is for internal use only
    // Every network message will start with a MessageType enum which defines how the rest of the message should be read.
    enum class MessageType : char {
        DEFAULT, // Messages of this type will be ignored
        PACKET_RECEIPT, // Send each time a device receives a message - uint(packetID)
        SINGLE_USER_MESSAGE, // A single packet message from the user - char... (data)
        MULTI_USER_MESSAGE, // A multi packet message from the user - uint(packetID) - uint(packetNum) -> uint(totalPackets) -> char...(data)
        SET_CLIENT_UID, // An internal message from the server to set the clients UID - uint(client uid)
        SET_LOBBY_SLOT, // Sets the slot for a client in a lobby - uint(lobbyID) -> uint(playerID) -> uint(slot)
        PING_REQUEST, // A 1 byte type only packet that needs to respond with a PING_RESPONSE packet immediately
        PING_RESPONSE, // A 1 byte type only packet that signifies the response of a PING_REQUEST

        // Events a client can send to the server
        REQUEST_NEW_LOBBY, // Signals for the server to create a new lobby and add this client to it - uint(blank) -> int(roomSize) -> uint(size of name) -> char...(lobby name)
        REMOVE_FROM_LOBBY, // Tells the server to remove the given client from its current lobby - uint(clientUID)
        JOIN_LOBBY, // Tells the server to add this client to the chosen lobby - uint(lobby uid)
        SET_LOBBY_NAME, // Tells the server to update this players name - uint(lobbyID) -> uint(playerID) -> uint(size of name) -> char...(name)

        // Events the server can send to clients
        SET_ACTIVE_LOBBY, // Tells the client to set its active lobby - uint(lobbyUID)
        NEW_LOBBY_CLIENT, // Tells the client that a new client has joined a lobby - uint(lobby uid) -> uint(new client uid) -> uint(size of name) ->  char...(client name)
        LOBBY_CLIENT_LEFT, // Tells the client that another client has left a lobby - uint(lobby uid) -> uint(left client uid)
        UPDATE_PEER_CONNECTION_INFO, // Tells the client to update its current lobby peerInfo connection values uint(lobby uid) -> int(infoCount) -> float(ping) -- repeat per infoCount
        SET_CLIENT_READY, // Tells the client that a client has set been set to active/inactive uint(lobby uid) -> uint(client uid) -> bool(isReady)
        SET_LOBBY_OPEN, // Requests that the server sets this lobby open state - uint(lobby uid) -> bool(lobby open)
        ADD_NEW_LOBBY, // Tells the client there is a new lobby open - uint(lobby uid) -> int(roomSize) -> uint(size of name) ->  char...(lobby name)
        REMOVE_LOBBY, // Tells the client to close a lobby - uint(lobby uid)


    };
}
#endif //CTP_CONSTANTS_H
