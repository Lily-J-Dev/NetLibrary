cmake_minimum_required(VERSION 3.9.0)

project(NetLib)
set(netlib_VERSION_MAJOR "0")
set(netlib_VERSION_MINOR "3")
set(netlib_VERSION_PATCH "2")
set(netlib_VERSION_EXTRA "")
set(netlib_VERSION "${netlib_VERSION_MAJOR}.${netlib_VERSION_MINOR}")
set(netlib_VERSION_FULL "${netlib_VERSION}.${netlib_VERSION_PATCH}${netlib_VERSION_EXTRA}")

# Let's give the debug library a postfix
set(CMAKE_DEBUG_POSTFIX d)

# Setup the libs that the game engine are reliant on
#list(APPEND LINK_LIBS "")
#add_subdirectory(Libs)

set( PROJECT_HEADERS
        "${CMAKE_CURRENT_LIST_DIR}/Headers/Windows/Client.h"
        "${CMAKE_CURRENT_LIST_DIR}/Headers/Windows/Server.h"
        "${CMAKE_CURRENT_LIST_DIR}/Headers/ClientConnection.h"
        "${CMAKE_CURRENT_LIST_DIR}/Headers/ClientInfo.h"
        "${CMAKE_CURRENT_LIST_DIR}/Headers/ConnectionInfo.h"
        "${CMAKE_CURRENT_LIST_DIR}/Headers/Constants.h"
        "${CMAKE_CURRENT_LIST_DIR}/Headers/MultiPacketContainer.h"
        "${CMAKE_CURRENT_LIST_DIR}/Headers/NetworkDevice.h"
        "${CMAKE_CURRENT_LIST_DIR}/Headers/ServerConnection.h"
        "${CMAKE_CURRENT_LIST_DIR}/Headers/Lobby.h"
        "${CMAKE_CURRENT_LIST_DIR}/Headers/LobbyMember.h"
        "${CMAKE_CURRENT_LIST_DIR}/Headers/NetworkEvent.h")


set( PROJECT_SOURCES
        "${CMAKE_CURRENT_LIST_DIR}/Source/Windows/Client.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/Source/Windows/Server.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/Source/ClientConnection.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/Source/NetworkDevice.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/Source/ServerConnection.cpp")

set ( PROJECT_CONFIG
        CMakeLists.txt
        Readme.md
        .gitattributes .gitignore .gitmodules )

source_group("Headers" FILES ${PROJECT_HEADERS})
source_group("Sources" FILES ${PROJECT_SOURCES})
source_group("Vendors" FILES ${VENDORS_SOURCES})
source_group("Configs" FILES ${PROJECT_CONFIG})

#add_definitions(
#        -DGLFW_INCLUDE_NONE
#        -DPROJECT_SOURCE_DIR="${PROJECT_SOURCE_DIR}")

#if(ENGINE_SHARED_LIB)
    add_library(
            NetLib SHARED Source/ServerConnection.cpp)
#else()
#   add_library(
#           NetLibDLL STATIC Source/ServerConnection.cpp )
#endif()

target_sources(NetLib
        PUBLIC
        ${PROJECT_HEADERS}

        PRIVATE
        ${PROJECT_SOURCES} )

# Define headers for this library. PUBLIC headers are used for
# compiling the library and added to build paths. 
target_include_directories(NetLib
        PUBLIC
        $<INSTALL_INTERFACE:include>
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Headers>"
        #"$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Libs/PhysFS++/include>"
        #"$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Libs/PhysFS/src>"
        PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/Source"
        "${CMAKE_CURRENT_SOURCE_DIR}/Source/Windows"
        SYSTEM
        #"${CMAKE_CURRENT_SOURCE_DIR}/Libs/GLEW/2.1.0/include"
        #"${CMAKE_CURRENT_SOURCE_DIR}/Libs/GLFW/include"
        #"${CMAKE_CURRENT_SOURCE_DIR}/Libs/GLM"
        #"${CMAKE_CURRENT_SOURCE_DIR}/Libs/Freetype2/include"
        #"${CMAKE_CURRENT_SOURCE_DIR}/Libs/STB"
        )

# Compiler requirements for this library,
target_compile_features(NetLib PUBLIC cxx_std_14)

# Preprocessor requirements
#if(PHYSFS_SHARED_LIB)
#    target_compile_definitions(GameEngine PRIVATE GLM_FORCE_CTOR_INIT=1 PHYSFS_EXPORT_LIB)
#else()
#    target_compile_definitions(GameEngine PRIVATE GLM_FORCE_CTOR_INIT=1 PHYSFS_CPP_STATIC)
#endif()

# Dependencies on other libraries
target_link_libraries(NetLib ws2_32 wsock32)
#target_link_libraries(NetLib PUBLIC ${LINK_LIBS})

# This makes the project importable from the build directory
# export(TARGETS GameEngine glfw freetype glew_s physfs++ physfs-static FILE GameEngineConfig.cmake)

set_target_properties(
        ${PROJECT_NAME} PROPERTIES
        VERSION ${netlib_VERSION_FULL}
        SOVERSION ${netlib_VERSION}
        LINKER_LANGUAGE CXX
        CMAKE_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/build
        CMAKE_PREFIX_PATH ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}
)

#add_dependencies(GameEngine Freetype2)
#add_dependencies(GameEngine PhysFS++)

#include_directories(Headers/Windows)
#include_directories(Source/Windows)
add_executable(netlib_test
        main.cpp
        Source/TestClient.cpp Headers/TestClient.h Source/TestServer.cpp Headers/TestServer.h ${PROJECT_HEADERS})

target_link_libraries(netlib_test NetLib)