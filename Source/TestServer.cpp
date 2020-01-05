#include "TestServer.h"
#include <iostream>

TestServer::TestServer()
{
    server.Start();
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