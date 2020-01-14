#include "TestServer.h"
#include <iostream>

TestServer::TestServer()
{
    server.Start(24000);
}

int TestServer::Update()
{
    while(server.AreNewClients())
    {
        // Do something based on new client
        server.GetNextNewClient();
    }

    while(server.AreDisconnectedClients())
    {
        // Do something based on disconnected client
        server.GetNextDisconnectedClient();
    }

    while (server.MessagesPending())
    {
        auto packet = server.GetNextMessage();
        server.SendMessageToAllExcluding(packet->data, packet->dataLength, packet->senderId);
        delete packet;
    }

    return 0;
}