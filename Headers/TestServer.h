#ifndef CTP_TESTSERVER_H
#define CTP_TESTSERVER_H
#include "ServerConnection.h"

class TestServer
{
public:
    TestServer();
    int Update();

private:
    ServerConnection server;
};


#endif //CTP_TESTSERVER_H
