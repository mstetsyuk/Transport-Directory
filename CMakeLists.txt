cmake_minimum_required(VERSION 3.17)
project(route_manager)

set(CMAKE_CXX_STANDARD 20)

add_executable(route_manager
        main.cpp
        route_manager.cpp
        json.cpp
        Bus.cpp
        Coordinates.cpp
        Stop.cpp
        Response.cpp)