#ifndef CTP_DATAPACKET_H
#define CTP_DATAPACKET_H
#include <vector>

namespace netlib {
    struct DataPacket {
        std::vector<char> data;
        unsigned int senderId;
    };
}
#endif //CTP_DATAPACKET_H