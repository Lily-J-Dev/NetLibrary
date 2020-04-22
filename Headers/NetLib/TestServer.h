#ifndef CTP_TESTSERVER_H
#define CTP_TESTSERVER_H
#include "ServerConnection.h"

class TestServer
{
public:
    TestServer();
    int Update();

private:
    netlib::ServerConnection server;

    using clock = std::chrono::steady_clock;
    using time = clock::time_point;
    std::forward_list<std::pair<netlib::NetworkEvent*, time>> sentPackets;

    time timeOfSend = clock::now();
    std::vector<char> testData;
    int testDataIterations = 0;
    int totalTime = 0;
};


#endif //CTP_TESTSERVER_H
