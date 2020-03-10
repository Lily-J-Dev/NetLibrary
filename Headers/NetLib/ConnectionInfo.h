#ifndef CTP_CONNECTIONINFO_H
#define CTP_CONNECTIONINFO_H

namespace netlib {
    struct ConnectionInfo {
        float ping = 100; // in ms
        //float packetLoss = 0; // As a percent TODO:: Add this if/when the library changes to/adds UDP
        //float outBandwidth = 0; // in MB/s    TODO:: ADD
        //float inBandwidth = 0; // In MB/s     TODO:: ADD
    };
}
#endif //CTP_CONNECTIONINFO_H
