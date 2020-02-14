cmake_minimum_required(VERSION 3.12)
project(netlib)

set(CMAKE_CXX_STANDARD 14)

include_directories(Headers)
include_directories(Source)



IF(WIN32)
    include_directories(Headers/Windows)
    include_directories(Source/Windows)

    add_executable(netlib
            main.cpp
            Source/ClientConnection.cpp Headers/ClientConnection.h Source/ServerConnection.cpp Headers/ServerConnection.h Headers/ClientInfo.h Headers/DataPacket.h Source/TestClient.cpp Headers/TestClient.h Source/TestServer.cpp Headers/TestServer.h Source/Windows/Client.cpp Headers/Windows/Client.h Source/Windows/Server.cpp Headers/Windows/Server.h Headers/Constants.h Source/NetworkDevice.cpp Headers/NetworkDevice.h Headers/MultiPacketContainer.h Headers/ConnectionInfo.h Headers/NetworkEvent.h Headers/Lobby.h Headers/LobbyMember.h)

    #add_executable(netlib_test
    #        main.cpp
    #        Source/TestClient.cpp Headers/TestClient.h Source/TestServer.cpp Headers/TestServer.h Headers/ClientConnection.h Headers/ServerConnection.h)
    target_link_libraries(netlib ws2_32 wsock32)
    #add_library(
    #        netlib SHARED
    #        Headers/Windows/Client.h Source/Windows/Client.cpp Headers/Windows/Server.h Source/Windows/Server.cpp Source/ClientConnection.cpp Headers/ClientConnection.h Source/ServerConnection.cpp Headers/ServerConnection.h Headers/ClientInfo.h Headers/DataPacket.h Headers/Constants.h Source/NetworkDevice.cpp Headers/NetworkDevice.h Headers/ConnectionInfo.h)

ELSEIF(UNIX)
    SET(CMAKE_CXX_FLAGS -pthread)
    include_directories(Headers/Linux)
    include_directories(Source/Linux)
    add_executable(netlib
            main.cpp
            Source/ClientConnection.cpp Headers/ClientConnection.h Source/ServerConnection.cpp Headers/ServerConnection.h Headers/ClientInfo.h Headers/DataPacket.h Source/TestClient.cpp Headers/TestClient.h Source/TestServer.cpp Headers/TestServer.h Source/Linux/Client.cpp Headers/Linux/Client.h Source/Linux/Server.cpp Headers/Linux/Server.h Headers/Constants.h Source/NetworkDevice.cpp Headers/NetworkDevice.h Headers/MultiPacketContainer.h Headers/ConnectionInfo.h Headers/NetworkEvent.h Headers/Lobby.h Headers/LobbyMember.h)
ENDIF()


#set_target_properties(netlib_test PROPERTIES
#        DEBUG_POSTFIX "d"
#        RELEASE_POSTFIX ""
#)

#target_link_libraries(netlib_test netlib.dll)