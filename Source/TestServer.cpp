#include "TestServer.h"
#include <iostream>
#include <string>

TestServer::TestServer()
{
    server.Start(24000);
}

int TestServer::Update()
{
    using namespace netlib;
    auto events = server.GetNetworkEvents();
    while(!events.empty())
    {
        NetworkEvent& event = events.front();
        switch (events.front().eventType)
        {
            case NetworkEvent::EventType::MESSAGE:
            {
                server.SendMessageToAllExcluding(event.data, event.senderId);
                break;
            }
            case NetworkEvent::EventType::ONCONNECT:
            {
                std::cout << "New Client " + server.GetClientInfo(event.senderId).name + " connected on IP: " << server.GetClientInfo(event.senderId).ipv4 << " ID: " << event.senderId << std::endl;
                break;
            }
            case NetworkEvent::EventType::ONDISCONNECT:
            {
                std::cout << "Client " + std::to_string(event.senderId) + " disconnected." << std::endl;
                break;
            }
            default:
            {
                break;
            }
        }
        events.pop();
    }
    return 0;
}