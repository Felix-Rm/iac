cmake_minimum_required(VERSION 2.8)

add_library(transport_routes STATIC
    "transport_route.cpp"
    "socket_transport_route.cpp"
    "loopback_transport_route.cpp")