cmake_minimum_required(VERSION 3.0)
enable_testing()

add_library(transport_routes STATIC transport_route.cpp socket_transport_route.cpp loopback_transport_route.cpp)