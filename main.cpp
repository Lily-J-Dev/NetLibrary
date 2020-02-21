#include "NetLib/TestServer.h"
#include "NetLib/TestClient.h"
#include <chrono>
#include <thread>
#include <mutex>

int main()
{
    std::string input;
    std::cin >> input;

    if (input == "s")
    {
        TestServer server;
        while (server.Update() == 0)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
    else if (input == "c")
    {
        TestClient client;
        while (client.Update() == 0)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }


    return 0;
}

