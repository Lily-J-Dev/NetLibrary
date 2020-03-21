#include <thread>
#include "NetLib/TestClient.h"

TestClient::TestClient()
{
    std::cout << "Enter server IP: ";
    std::string input = "";
    std::getline(std::cin, input);
    std::getline(std::cin, input);
    if(input == "localhost" || input == "")
        input = "127.0.0.1";
    client.ConnectToIP(input, 24000);
}

int TestClient::Update()
{
    auto events = client.GetNetworkEvents();
    while(!events.empty())
    {
        switch (events.front().eventType)
        {
            case netlib::NetworkEvent::EventType::ON_CONNECT:
            {
                std::cout << "Connected to the server!" << std::endl;
                std::thread tr(&TestClient::GetInput, this);
                tr.detach();
                break;
            }
            case netlib::NetworkEvent::EventType::MESSAGE:
            {
                auto test = events.front();
                std::cout << events.front().data.data() << std::endl;
                break;
            }
            case netlib::NetworkEvent::EventType::ON_DISCONNECT:
            {
                std::cout << "Remotely disconnected from server." << std::endl;
                inputRunning = false;
                break;
            }
            case netlib::NetworkEvent::EventType::ON_LOBBY_JOIN:
            {
                std::cout << "Joined new Lobby: " << client.GetCurrentLobbyInfo().name << std::endl;
                break;
            }
            case netlib::NetworkEvent::EventType::REMOVED_FROM_LOBBY:
            {
                std::cout << "Removed from lobby." << std::endl;
                break;
            }
            default:
            {
                break;
            }
        }
        events.pop();
    }
    return inputRunning ? 0 : -1;

}

void TestClient::GetInput()
{
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    inputRunning = true;
    std::string input = "";
    while(inputRunning)
    {
        std::cout << "";
        std::getline(std::cin, input);
        if(input == "#getlobbies")
        {
            std::cout << "Current open lobbies:" << std::endl;
            auto lobbies = client.GetAllLobbyInfo();
            for(auto& lobby : lobbies)
            {
                std::cout << "Name: " << lobby.name << " ID: " << lobby.lobbyID
                          << " (" << lobby.clientsInRoom << "/" << lobby.maxClientsInRoom << ")" <<  std::endl;
                std::cout << "Clients: " << std::endl;
                for(auto& member : lobby.memberInfo)
                {
                    std::cout << "[" << member.lobbySlot << "] " << "\t Name: " << member.name << " ID: " << member.uid << " (" << member.ping << " ms)" << (member.ready ? "(READY)" : "(NOT READY)") << std::endl;
                }
            }
            std::cout << std::endl;
        }
        else if(input == "#x")
        {
            inputRunning = false;
            return;
        }
        else if(input == "#createlobby")
        {
            std::cout << "Enter New Lobby Name: ";
            std::getline(std::cin, input);
            std:: string lobbySize;
            std::cout << "Enter Lobby size: ";
            std::getline(std::cin, lobbySize);
            client.CreateLobby(input, std::atoi(lobbySize.c_str()));
        }
        else if(input == "#joinlobby")
        {
            std::cout << "Enter New Lobby Number to Join: ";
            std::getline(std::cin, input);
            client.JoinLobby(std::atoi(input.data()));
        }
        else if(input == "#kick")
        {
            std::cout << "Enter client id to remove: ";
            std::getline(std::cin, input);
            client.RemoveFromLobby(std::atoi(input.data()));
        }
        else if(input == "#!ready")
        {
            client.SetReady(true);
        }
        else if(input == "#ready")
        {
            client.SetReady(false);
        }
        else if(input == "#closelobby")
        {
            client.SetLobbyOpen(false);
        }
        else if(input == "#openlobby")
        {
            client.SetLobbyOpen(true);
        }
        else if(input == "#setname")
        {
            std::cout << "Enter new name: ";
            std::getline(std::cin, input);
            client.SetLobbyName(input);
        }
        else
        {
            input = std::to_string(client.GetUID()) + ": " + input;
            client.SendMessageToServer(input.c_str(), input.size() + 1);
        }
    }

}