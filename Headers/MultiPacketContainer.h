#ifndef CTP_MULTIPACKETCONTAINER_H
#define CTP_MULTIPACKETCONTAINER_H

#include "DataPacket.h"
#include <vector>

struct MultiPacketContainer
{
    MultiPacketContainer() = default;
    explicit MultiPacketContainer(int size){packets.resize(size);};
    ~MultiPacketContainer(){for(DataPacket* d : packets) delete d;};
    bool IsComplete(){return packets.size() == packetCount;};

    int packetCount = 0;
    std::vector<DataPacket*> packets;
};

#endif //CTP_MULTIPACKETCONTAINER_H
