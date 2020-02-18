#ifndef CTP_MULTIPACKETCONTAINER_H
#define CTP_MULTIPACKETCONTAINER_H

#include "NetworkEvent.h"
#include <vector>
namespace netlib {
    struct MultiPacketContainer {
        MultiPacketContainer() = default;

        explicit MultiPacketContainer(int size) { packets.resize(size); };

        ~MultiPacketContainer() { for (NetworkEvent *d : packets) delete d; };

        bool IsComplete() { return packets.size() == packetCount; };

        unsigned int packetCount = 0;
        std::vector<NetworkEvent *> packets;
    };
}
#endif //CTP_MULTIPACKETCONTAINER_H
