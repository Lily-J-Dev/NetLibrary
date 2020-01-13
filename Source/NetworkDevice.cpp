#include "NetworkDevice.h"
#include "Constants.h"
#include <cassert>
#include <iostream>

NetworkDevice::NetworkDevice()
{
    // Save the various index offsets so they only have to be calculated once
    offsets[0] = 1;
    offsets[1] = 1+ sizeof(unsigned int);
    offsets[2] = offsets[1] + sizeof(unsigned int);
    offsets[3] = offsets[2] + sizeof(unsigned int);

    // MAX_PACKET_SIZE is too small if this assert hits
    assert(MAX_PACKET_SIZE > offsets[3] +1);
    maxPacketDataLen = MAX_PACKET_SIZE - 1;
    maxMultiPacketDataLen = MAX_PACKET_SIZE - offsets[3];
}

void NetworkDevice::ProcessAndSendData(DataPacket* packet)
{
    if(packet->dataLength > maxPacketDataLen)
    {
        unsigned int startIndex = 0;
        unsigned int totalPackets = (int)ceilf((float)packet->dataLength / (float)maxMultiPacketDataLen);
        auto totalAsChar = reinterpret_cast<const char*>(&totalPackets);
        auto idAsChar = reinterpret_cast<const char*>(&packetID);

        for(unsigned int i = 0; i< totalPackets; i++)
        {
            unsigned int packetSize =  packet->dataLength > maxMultiPacketDataLen ? maxMultiPacketDataLen : packet->dataLength;
            unsigned int totalPacketSize = packetSize + offsets[3];
            char* nData = new char[totalPacketSize];

            nData[0] = (char)MessageType::MULTI_USER_MESSAGE;
            auto packAsChar = reinterpret_cast<const char*>(&i);

            std::copy(idAsChar, idAsChar + sizeof(unsigned int), nData+offsets[0]);
            std::copy(packAsChar, packAsChar + sizeof(unsigned int), nData+offsets[1]);
            std::copy(totalAsChar, totalAsChar + sizeof(unsigned int), nData+offsets[2]);
            std::copy(packet->data + startIndex, packet->data+startIndex+packetSize, nData+offsets[3]);

            auto pack = new DataPacket();
            pack->dataLength = totalPacketSize;
            pack->data = nData;
            pack->senderId = packet->senderId;

            SendPacket(pack);

            packet->dataLength -= packetSize;
            startIndex += packetSize;
        }
        delete packet;
        packetID++;
    }
    else
    {
        char* nData = new char[packet->dataLength+offsets[0]];
        nData[0] = (char)MessageType::SINGLE_USER_MESSAGE;
        std::copy(packet->data, packet->data+packet->dataLength, nData+offsets[0]);

        delete[] packet->data;
        packet->data = nData;
        packet->dataLength += offsets[0];
        SendPacket(packet);
    }
}

// Function that gets called by the client every time a network message is received.
void NetworkDevice::ProcessPacket(DataPacket* data)
{
    // Here is where the data will be processed (decrypt/uncompress/etc)


    if(data->data[0] == (char)MessageType::SINGLE_USER_MESSAGE)
    {
        char* nData = new char[data->dataLength - offsets[0]];
        std::copy(data->data + offsets[0], data->data + data->dataLength, nData);
        delete[] data->data;
        data->data = nData;
        data->dataLength -= offsets[0];

        messageLock.lock();
        messages.push(data);
        messageLock.unlock();

    }
    else if(data->data[0] == (char)MessageType::MULTI_USER_MESSAGE)
    {
        auto id = *reinterpret_cast<unsigned int*>(&data->data[offsets[0]]);
        auto num = *reinterpret_cast<unsigned int*>(&data->data[offsets[1]]);
        auto total = *reinterpret_cast<unsigned int*>(&data->data[offsets[2]]);

        char* nData =  new char[data->dataLength-offsets[3]];
        std::copy(data->data + offsets[3], data->data + data->dataLength, nData);
        delete[] data->data;

        data->data = nData;
        data->dataLength = data->dataLength-offsets[3];


        // If this is the first packet for this id, add it to the map
        using uintPair = std::pair<unsigned int, unsigned int>;
        using mapPair = std::pair<uintPair, MultiPacketContainer>;
        if(recMultiPackets.count(uintPair (data->senderId, id)) == 0)
        {
            recMultiPackets.insert(mapPair(uintPair(data->senderId, id), MultiPacketContainer(total)));
        }
        recMultiPackets[uintPair(data->senderId, id)].packets[num] = data;
        recMultiPackets[uintPair(data->senderId, id)].packetCount++;

        // If all packets are present
        if(recMultiPackets[uintPair(data->senderId, id)].IsComplete())
        {
            ProcessMultiPacket(data->senderId, id);
        }
    }
    else if(data->data[0] == (char)MessageType::PING_REQUEST)
    {
        data->data[0] = (char)MessageType::PING_RESPONSE;
        SendPacket(data);
    }
    else
    {
        ProcessDeviceSpecificEvent(data);
    }
}

// Once every part of a multi-packet message has been received, this method will merge them together and place them on the message queue
void NetworkDevice::ProcessMultiPacket(unsigned int senderId, unsigned int id)
{
    using uintPair = std::pair<unsigned int, unsigned int>;
    MultiPacketContainer* container = &recMultiPackets[uintPair(senderId, id)];

    unsigned int dataLen = 0;
    for(int i = 0; i < container->packets.size(); i++)
    {
        dataLen += container->packets[i]->dataLength;
    }

    auto packet = new DataPacket();
    packet->senderId = container->packets[0]->senderId;
    packet->dataLength = dataLen;
    packet->data = new char[dataLen];
    unsigned int index = 0;

    for(int i = 0; i < container->packets.size(); i++)
    {
        std::copy(container->packets[i]->data,
                container->packets[i]->data + container->packets[i]->dataLength,
                packet->data+index);
        index += container->packets[i]->dataLength;
    }

    recMultiPackets.erase(uintPair(senderId, id));

    messageLock.lock();
    messages.push(packet);
    messageLock.unlock();
}

// Returns true if there are messages pending (get with GetNextMessage)
bool NetworkDevice::MessagesPending()
{
    messageLock.lock();
    bool reBool = !messages.empty();
    messageLock.unlock();
    return reBool;
}

// Returns the oldest message in the queue. Data returned from this method is no longer used by the library
// so the user must manage its memory (ie delete it when no longer being used.)
DataPacket* NetworkDevice::GetNextMessage()
{
    messageLock.lock();
    if(messages.empty())
    {
        messageLock.unlock();
        return nullptr;
    }
    auto returnPacket = messages.front();
    messages.pop();
    messageLock.unlock();
    return returnPacket;
}