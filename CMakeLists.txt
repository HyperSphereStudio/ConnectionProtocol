cmake_minimum_required(VERSION 3.28)
project(ConnectionProtocol)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

add_definitions(-DEXPORT_WINDOWS=1)

add_library(ConnectionProtocol SHARED library.cpp
        src/SimpleConnection.hpp)
