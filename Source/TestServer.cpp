#include "TestServer.h"
#include <iostream>

TestServer::TestServer()
{
    server.Start(24000);
}

int TestServer::Update()
{
    while (server.MessagesPending())
    {
        auto packet = server.GetNextMessage();
        server.SendMessageToAllExcluding(packet->data, packet->dataLength, packet->senderId);
    }

    return 0;
}