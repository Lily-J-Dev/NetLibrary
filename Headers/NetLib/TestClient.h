#ifndef CTP_TESTCLIENT_H
#define CTP_TESTCLIENT_H

#include <mutex>
#include <atomic>
#include <iostream>
#include <sstream>
#include "ClientConnection.h"

class TestClient
{

public:
    TestClient();

    int Update();
private:
    void GetInput();

    netlib::ClientConnection client;

    std::atomic_bool inputRunning;
};


#endif //CTP_TESTCLIENT_H
