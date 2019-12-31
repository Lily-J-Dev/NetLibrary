#ifndef CTP_TESTSERVER_H
#define CTP_TESTSERVER_H
#include "ServerDataHandler.h"

class TestServer
{
public:
    TestServer();
    int Update();

private:
    ServerDataHandler server;
};


#endif //CTP_TESTSERVER_H
