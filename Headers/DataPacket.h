#ifndef CTP_DATAPACKET_H
#define CTP_DATAPACKET_H

struct DataPacket
{
    ~DataPacket(){delete[] data;};

    char* data;
    unsigned int dataLength;
    unsigned int senderId;
};

#endif //CTP_DATAPACKET_H