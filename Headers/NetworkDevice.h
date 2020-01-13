#ifndef CTP_NETWORKDEVICE_H
#define CTP_NETWORKDEVICE_H
#include <queue>
#include <map>
#include <mutex>

#include "MultiPacketContainer.h"

class NetworkDevice {
public:
    NetworkDevice();

    bool MessagesPending();
    DataPacket* GetNextMessage();

protected:
    void ProcessAndSendData(DataPacket* packet);
    void ProcessPacket(DataPacket* data);
    void ProcessMultiPacket(unsigned int senderId, unsigned int id);

    virtual void SendPacket(DataPacket* data) = 0;
    virtual void ProcessDeviceSpecificEvent(DataPacket* data) = 0;
private:
    std::map<std::pair<unsigned int, unsigned int>, MultiPacketContainer> recMultiPackets;
    unsigned int packetID = 0;
    std::queue<DataPacket*> messages;

    std::mutex messageLock;
    unsigned int offsets[4];

    unsigned int maxPacketDataLen = 0;
    unsigned int maxMultiPacketDataLen = 0;
};


#endif //CTP_NETWORKDEVICE_H
