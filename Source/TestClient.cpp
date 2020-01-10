#include <thread>
#include "TestClient.h"

TestClient::TestClient()
{
    client.ConnectToIP("127.0.0.1", 24000);
    std::thread tr(&TestClient::GetInput, this);
    tr.detach();
}

int TestClient::Update()
{
    while(client.MessagesPending())
    {
        auto packet = client.GetNextMessage();
        std::cout << packet->senderId  << ": " << std::string(packet->data, packet->dataLength) << std::endl;
    }
    return 0;
}

void TestClient::GetInput()
{
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    inputRunning = true;
    while(inputRunning)
    {
        std::string input = "";
        std::getline(std::cin, input);
        client.SendMessageToServer(input.c_str(), input.size()+1);
    }
}