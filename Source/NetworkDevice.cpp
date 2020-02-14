#include "NetworkDevice.h"
#include "Constants.h"
#include <cassert>
#include <iostream>
#include <chrono>
#include <thread>
#include <cmath>

netlib::NetworkDevice::NetworkDevice()
{
    // Save the various index offsets so they only have to be calculated once
    offsets[0] = 1;
    offsets[1] = offsets[0] + 1;
    offsets[2] = offsets[1] + sizeof(unsigned int);
    offsets[3] = offsets[2] + sizeof(unsigned int);
    offsets[4] = offsets[3] + sizeof(unsigned int);

    // MAX_PACKET_SIZE is too small if this assert hits
    assert(MAX_PACKET_SIZE > offsets[3] +1);
    maxPacketDataLen = MAX_PACKET_SIZE - 1;
    maxMultiPacketDataLen = MAX_PACKET_SIZE - offsets[4];
}

void netlib::NetworkDevice::Start()
{
    running = true;
    // Clear out the message queue in case a message slipped through while the device was shutting down
    while(!outQueue.empty())
    {
        outQueue.pop();
    }

    std::thread tr(&NetworkDevice::Run, this);
    tr.detach();
}

void netlib::NetworkDevice::Stop()
{
    running = false;
    deleteLock.lock();
    deleteLock.unlock();
    recMultiPackets.clear();
}

void netlib::NetworkDevice::Run()
{
    deleteLock.lock();
    while (running)
    {
        // Send all messages in queue
        outQueueLock.lock();
        while(!outQueue.empty())
        {
            SendPacket(outQueue.front());
            outQueue.pop();
        }

        // Handle the cleanup for any disconnected clients
        while(!disconnectQueue.empty())
        {
            TerminateConnection(disconnectQueue.front());
            // Find any multi packets of this ID
            std::vector<int> foundMulti;
            for(auto& pair : recMultiPackets)
            {
                if(pair.first.first == disconnectQueue.front())
                {
                    foundMulti.push_back(pair.first.second);
                }
            }
            // Clear all multi packets for this client
            for(int& i : foundMulti)
            {
                recMultiPackets.erase(std::pair<int,int>(disconnectQueue.front(),i));
            }
            disconnectQueue.pop();
        }

        outQueueLock.unlock();

        UpdateNetworkStats();

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    deleteLock.unlock();
}

std::queue<netlib::NetworkEvent> netlib::NetworkDevice::GetNetworkEvents()
{
    ClearQueue();
    std::lock_guard<std::mutex> guard(messageLock);
    shouldClearQueue = true;
    return messages;
}


void netlib::NetworkDevice::ProcessAndSendData(NetworkEvent* packet)
{
    if(packet->data.size() > maxPacketDataLen)
    {
        unsigned int startIndex = 0;
        unsigned int totalPackets = (int)ceilf((float)packet->data.size() / (float)maxMultiPacketDataLen);
        auto totalAsChar = reinterpret_cast<const char*>(&totalPackets);
        auto idAsChar = reinterpret_cast<const char*>(&packetID);

        for(unsigned int i = 0; i< totalPackets; i++)
        {
            unsigned int packetSize =  packet->data.size() - startIndex > maxMultiPacketDataLen ? maxMultiPacketDataLen : packet->data.size() - startIndex;

            auto pack = new NetworkEvent();
            pack->data.resize(MAX_PACKET_SIZE);

            pack->data[0] = (char)MessageType::MULTI_USER_MESSAGE;
            pack->data[1] = static_cast<char>(packetSize);
            auto packAsChar = reinterpret_cast<const char*>(&i);

            std::copy(idAsChar, idAsChar + sizeof(unsigned int), pack->data.data()+offsets[1]);
            std::copy(packAsChar, packAsChar + sizeof(unsigned int), pack->data.data()+offsets[2]);
            std::copy(totalAsChar, totalAsChar + sizeof(unsigned int), pack->data.data()+offsets[3]);
            std::copy(packet->data.data() + startIndex, packet->data.data() + startIndex+packetSize, pack->data.data()+offsets[4]);

            pack->senderId = packet->senderId;

            outQueueLock.lock();
            outQueue.push(pack);
            outQueueLock.unlock();

            startIndex += packetSize;
        }
        delete packet;
        packetID++;
    }
    else
    {
        auto pack = new NetworkEvent();
        pack->data.resize(MAX_PACKET_SIZE);
        pack->data[0] = static_cast<char>(MessageType::SINGLE_USER_MESSAGE);
        pack->data[1] = static_cast<char>(packet->data.size());
        pack->senderId = packet->senderId;
        std::copy(packet->data.data(), packet->data.data()+packet->data.size(), pack->data.data()+offsets[1]);

        delete packet;

        outQueueLock.lock();
        outQueue.push(pack);
        outQueueLock.unlock();
    }
}

// Function that gets called by the client/server every time a network message is received.
void netlib::NetworkDevice::ProcessPacket(NetworkEvent* event)
{
    // Here is where the data will be processed (decrypt/uncompress/etc)


    if(event->data[0] == (char)MessageType::SINGLE_USER_MESSAGE)
    {
        messageLock.lock();
        ClearQueue();
        messages.emplace();
        messages.front().data.resize(event->data[1]);
        messages.front().senderId = event->senderId;
        std::copy(event->data.data() + offsets[1], event->data.data() + offsets[1] + event->data[1], messages.front().data.data());
        delete event;

        messageLock.unlock();
    }
    else if(event->data[0] == (char)MessageType::MULTI_USER_MESSAGE)
    {
        auto id = *reinterpret_cast<unsigned int*>(&event->data[offsets[1]]);
        auto num = *reinterpret_cast<unsigned int*>(&event->data[offsets[2]]);
        auto total = *reinterpret_cast<unsigned int*>(&event->data[offsets[3]]);

        auto pack = new NetworkEvent();
        pack->data.resize(event->data[1]);
        pack->senderId = event->senderId;

        std::copy(event->data.data() + offsets[4], event->data.data() + offsets[4] + event->data[1], pack->data.data());
        delete event;

        // If this is the first packet for this id, add it to the map
        using uintPair = std::pair<unsigned int, unsigned int>;
        using mapPair = std::pair<uintPair, MultiPacketContainer>;
        if(recMultiPackets.count(uintPair (pack->senderId, id)) == 0)
        {
            recMultiPackets.insert(mapPair(uintPair(pack->senderId, id), MultiPacketContainer(total)));
        }
        recMultiPackets[uintPair(pack->senderId, id)].packets[num] = pack;
        recMultiPackets[uintPair(pack->senderId, id)].packetCount++;

        // If all packets are present
        if(recMultiPackets[uintPair(pack->senderId, id)].IsComplete())
        {
            ProcessMultiPacket(pack->senderId, id);
        }
    }
    else if(event->data[0] == (char)MessageType::PING_REQUEST)
    {
        event->data[0] = (char)MessageType::PING_RESPONSE;
        outQueueLock.lock();
        outQueue.push(event);
        outQueueLock.unlock();
    }
    else
    {
        ProcessDeviceSpecificEvent(event);
    }
}

// Once every part of a multi-packet message has been received, this method will merge them together and place them on the message queue
void netlib::NetworkDevice::ProcessMultiPacket(unsigned int senderId, unsigned int id)
{
    using uintPair = std::pair<unsigned int, unsigned int>;
    MultiPacketContainer* container = &recMultiPackets[uintPair(senderId, id)];

    unsigned int dataLen = 0;
    for(size_t i = 0; i < container->packets.size(); i++)
    {
        dataLen += container->packets[i]->data.size();
    }

    messageLock.lock();
    ClearQueue();
    messages.emplace();
    messages.front().senderId = container->packets[0]->senderId;
    messages.front().data.resize(dataLen);
    unsigned int index = 0;

    for(size_t i = 0; i < container->packets.size(); i++)
    {
        std::copy(container->packets[i]->data.data(),
                container->packets[i]->data.data() + container->packets[i]->data.size(),
                  messages.front().data.data() + index);
        index += container->packets[i]->data.size();
    }

    recMultiPackets.erase(uintPair(senderId, id));
    messageLock.unlock();
}

// Returns true if there are messages pending
bool netlib::NetworkDevice::MessagesPending()
{
    messageLock.lock();
    bool reBool = !messages.empty();
    messageLock.unlock();
    return reBool;
}

void netlib::NetworkDevice::ClearQueue()
{
    if(shouldClearQueue) {
        while (!messages.empty())
            messages.pop();
    }
    shouldClearQueue = false;
}

void netlib::NetworkDevice::SendEvent(NetworkEvent* event)
{
    outQueueLock.lock();
    outQueue.push(event);
    outQueueLock.unlock();
}

