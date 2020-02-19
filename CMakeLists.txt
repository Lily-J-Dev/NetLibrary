cmake_minimum_required(VERSION 3.9.0)

OPTION(STATIC_NETLIB "Builds the library as static" ON)

project(NetLib)
set(netlib_VERSION_MAJOR "0")
set(netlib_VERSION_MINOR "3")
set(netlib_VERSION_PATCH "2")
set(netlib_VERSION_EXTRA "")
set(netlib_VERSION "${netlib_VERSION_MAJOR}.${netlib_VERSION_MINOR}")
set(netlib_VERSION_FULL "${netlib_VERSION}.${netlib_VERSION_PATCH}${netlib_VERSION_EXTRA}")

# Let's give the debug library a postfix
set(CMAKE_DEBUG_POSTFIX d)


set( PROJECT_HEADERS_PUBLIC
        "${CMAKE_CURRENT_LIST_DIR}/Headers/ClientConnection.h"
        "${CMAKE_CURRENT_LIST_DIR}/Headers/ClientInfo.h"
        "${CMAKE_CURRENT_LIST_DIR}/Headers/ConnectionInfo.h"
        "${CMAKE_CURRENT_LIST_DIR}/Headers/ServerConnection.h"
        "${CMAKE_CURRENT_LIST_DIR}/Headers/Lobby.h"
        "${CMAKE_CURRENT_LIST_DIR}/Headers/LobbyMember.h"
        "${CMAKE_CURRENT_LIST_DIR}/Headers/NetworkEvent.h")

set( PROJECT_HEADERS_PRIVATE
        "${CMAKE_CURRENT_LIST_DIR}/Headers/Constants.h"
        "${CMAKE_CURRENT_LIST_DIR}/Headers/MultiPacketContainer.h"
        "${CMAKE_CURRENT_LIST_DIR}/Headers/NetworkDevice.h")


set( PROJECT_SOURCES
        "${CMAKE_CURRENT_LIST_DIR}/Source/ClientConnection.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/Source/NetworkDevice.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/Source/ServerConnection.cpp")

if(UNIX)
    set( PROJECT_SOURCES ${PROJECT_SOURCES}
            "${CMAKE_CURRENT_LIST_DIR}/Source/Linux/Client.cpp"
            "${CMAKE_CURRENT_LIST_DIR}/Source/Linux/Server.cpp")
    set( PROJECT_HEADERS_PRIVATE ${PROJECT_HEADERS_PRIVATE}
            "${CMAKE_CURRENT_LIST_DIR}/Headers/Linux/Client.h"
            "${CMAKE_CURRENT_LIST_DIR}/Headers/Linux/Server.h")
else()
    set( PROJECT_SOURCES ${PROJECT_SOURCES}
            "${CMAKE_CURRENT_LIST_DIR}/Source/Windows/Client.cpp"
            "${CMAKE_CURRENT_LIST_DIR}/Source/Windows/Server.cpp")
    set( PROJECT_HEADERS ${PROJECT_HEADERS_PRIVATE}
            "${CMAKE_CURRENT_LIST_DIR}/Headers/Windows/Client.h"
            "${CMAKE_CURRENT_LIST_DIR}/Headers/Windows/Server.h")
endif()

set ( PROJECT_CONFIG
        CMakeLists.txt
        Readme.md
        .gitattributes .gitignore .gitmodules )

source_group("Headers" FILES ${PROJECT_HEADERS_PRIVATE} ${PROJECT_HEADERS_PUBLIC})
source_group("Sources" FILES ${PROJECT_SOURCES})
source_group("Vendors" FILES ${VENDORS_SOURCES})
source_group("Configs" FILES ${PROJECT_CONFIG})

if(STATIC_NETLIB)
    add_library(
            NetLib STATIC)
else()
   add_library(
           NetLib SHARED)
endif()

target_sources(NetLib
        PUBLIC
        ${PROJECT_HEADERS_PUBLIC}

        PRIVATE
        ${PROJECT_HEADERS_PRIVATE}
        ${PROJECT_SOURCES} )

# Define headers for this library. PUBLIC headers are used for
# compiling the library and added to build paths.
if(WIN32)
    target_include_directories(NetLib
            PUBLIC
            $<INSTALL_INTERFACE:include>
            "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Headers>"
            "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Headers/Windows>"
            PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/Source"
            "${CMAKE_CURRENT_SOURCE_DIR}/Source/Windows"
            SYSTEM
            )
else()
    target_include_directories(NetLib
            PUBLIC
            $<INSTALL_INTERFACE:include>
            "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Headers>"
            "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Headers/Linux>"
            PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/Source"
            "${CMAKE_CURRENT_SOURCE_DIR}/Source/Linux"
            SYSTEM
            )
endif()

# Compiler requirements for this library,
target_compile_features(NetLib PUBLIC cxx_std_14)

# Dependencies on other libraries
if(WIN32)
    target_link_libraries(NetLib PUBLIC ws2_32 wsock32)
else()
    SET(CMAKE_CXX_FLAGS -pthread)
endif()


# This makes the project importable from the build directory

set_target_properties(
        ${PROJECT_NAME} PROPERTIES
        VERSION ${netlib_VERSION_FULL}
        SOVERSION ${netlib_VERSION}
        LINKER_LANGUAGE CXX
        CMAKE_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/build
        CMAKE_PREFIX_PATH ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}
)

if(WIN32)
    include_directories(Headers/Windows)
    include_directories(Source/Windows)
ELSE()
    include_directories(Headers/Linux)
    include_directories(Source/Linux)
endif()
add_executable(netlib_test
        main.cpp
        Source/TestClient.cpp Headers/TestClient.h Source/TestServer.cpp Headers/TestServer.h ${PROJECT_HEADERS})

target_link_libraries(netlib_test NetLib)