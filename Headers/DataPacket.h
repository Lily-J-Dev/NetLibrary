#ifndef CTP_DATAPACKET_H
#define CTP_DATAPACKET_H

struct DataPacket
{
    char* data;
    int dataLength;
    unsigned int senderId;
};

#endif //CTP_DATAPACKET_H