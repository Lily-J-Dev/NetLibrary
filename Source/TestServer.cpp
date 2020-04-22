#include "NetLib/TestServer.h"
#include "NetLib/ClientConnection.h"
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
                testDataIterations++;
                if(testDataIterations < 1000) {
                    auto timeForSend = std::chrono::duration_cast<std::chrono::milliseconds>(
                            clock::now() - timeOfSend).count();
                    timeOfSend = clock::now();
                    testData.resize(102400);
                    server.SendMessageTo(testData, event.senderId);
                    totalTime += timeForSend;
                    break;
                }
                else
                {
                    std::cout << "Over 1,000 iterations, the average time to send 100kb of data was: " << totalTime / 10000 << "ms" << std::endl;
                }
                std::cout << "Message from client " << event.senderId << " :" << event.data.data() << std::endl;
                ClientInfo info = server.GetClientInfo(event.senderId);
                server.SendMessageToAllExcluding(event.data, event.senderId, info.lobbyID);
                if(event.data.size() > 2 && event.data[3] == 'x')
                    return -1;
                break;
            }
            case NetworkEvent::EventType::ON_CONNECT:
            {
                std::cout << "New Client " + server.GetClientInfo(event.senderId).name + " connected on IP: " << server.GetClientInfo(event.senderId).ipv4 << " ID: " << event.senderId << std::endl;
                // Test send 1GB of data and time how long it takes to arrive;
                timeOfSend = clock::now();
                testData.resize(102400);
                server.SendMessageTo(testData, event.senderId);
                break;
            }
            case NetworkEvent::EventType::ON_DISCONNECT:
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