cmake_minimum_required(VERSION 3.12)
project(CTP)

set(CMAKE_CXX_STANDARD 14)

include_directories(Headers)
include_directories(Source)
include_directories(Headers/Windows)
include_directories(Source/Windows)


IF(WIN32)

    add_executable(CTP
            main.cpp
            Headers/Windows/Client.h Source/Windows/Client.cpp Headers/Windows/Server.h Source/Windows/Server.cpp Source/ClientDataHandler.cpp Headers/ClientDataHandler.h Source/ServerDataHandler.cpp Headers/ServerDataHandler.h Headers/ClientInfo.h Headers/DataPacket.h Source/TestClient.cpp Headers/TestClient.h Source/TestServer.cpp Headers/TestServer.h)
    target_link_libraries(CTP ws2_32 wsock32)


ENDIF()


