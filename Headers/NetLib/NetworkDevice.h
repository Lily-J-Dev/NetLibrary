#ifndef CTP_NETWORKDEVICE_H
#define CTP_NETWORKDEVICE_H
#include <queue>
#include <map>
#include <mutex>
#include <atomic>

#include "MultiPacketContainer.h"

namespace netlib {
    class NetworkDevice {
    public:
        NetworkDevice();
        virtual ~NetworkDevice();

        bool MessagesPending();

        std::queue<NetworkEvent> GetNetworkEvents();

    protected:
        void Start();
        void Stop();

        void ProcessAndSendData(NetworkEvent *packet);
        void ProcessSharedEvent(NetworkEvent *event);
        void ProcessMultiPacket(unsigned int senderId, unsigned int id);
        virtual void SendPacket(NetworkEvent *data) = 0;
        virtual void TerminateConnection(unsigned int clientUID) = 0;
        virtual void ProcessDeviceSpecificEvent(NetworkEvent *data) = 0;
        virtual void CheckForResends() = 0;
        virtual void UpdateNetworkStats() = 0;

        void SendEvent(NetworkEvent* event);

        void ClearQueue();

        std::queue<NetworkEvent*> outQueue;
        std::mutex outQueueLock;

        std::mutex messageLock;
        std::queue<NetworkEvent> messages;
        std::queue<unsigned int> disconnectQueue;

        std::atomic_bool safeToExit{true};
    private:
        void Run();

        std::map<std::pair<unsigned int, unsigned int>, MultiPacketContainer> recMultiPackets;
        unsigned int packetID = 0;
        bool shouldClearQueue = false;

        unsigned int offsets[5];
        unsigned int maxPacketDataLen = 0;
        unsigned int maxMultiPacketDataLen = 0;

        std::atomic_bool running{false};
    };
}

#endif //CTP_NETWORKDEVICE_H
