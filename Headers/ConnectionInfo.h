#ifndef CTP_CONNECTIONINFO_H
#define CTP_CONNECTIONINFO_H

struct ConnectionInfo
{
    float ping = 0; // in ms
    //float packetLoss = 0; // As a percent TODO:: Add this if/when the library changes to/adds UDP
    float outBandwidth = 0; // in MB/s
    float inBandwidth = 0; // In MB/s
};

#endif //CTP_CONNECTIONINFO_H
