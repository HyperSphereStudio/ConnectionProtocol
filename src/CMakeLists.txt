cmake_minimum_required(VERSION 3.2)
project(simplecommunicationencoder)

set(CMAKE_CXX_STANDARD 17)
set(PROJECT_BINARY_DIR ${BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${BINARY_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${BINARY_DIR})
message(PROJECT_BINARY_DIR="${PROJECT_BINARY_DIR}")

add_library(simplecommunicationencoder SHARED library.cpp
        SimpleConnection.hpp)