#ifndef CTP_CONSTANTS_H
#define CTP_CONSTANTS_H

const int MAX_PACKET_SIZE = 30;

// Every network message will start with a MessageType enum which defines how the rest of the message should be read.
enum class MessageType : char
{
    SINGLE_USER_MESSAGE, // A single packet message from the user - char... (data)
    MULTI_USER_MESSAGE, // A multi packet message from the user - uint(packetID) - uint(packetNum) -> uint(totalPackets) -> char...(data)
    SET_CLIENT_UID, // An internal message from the server to set the clients UID - uint(client uid)
    PING_REQUEST, // A 1 byte Type only packet that needs to respond with a PING_RESPONSE packet immediately
    PING_RESPONSE, // A 1 byte Type only packet that signifies the response of a PING_REQUEST
};

#endif //CTP_CONSTANTS_H
