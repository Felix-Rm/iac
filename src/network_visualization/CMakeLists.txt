cmake_minimum_required(VERSION 3.0)
enable_testing()

add_library(network_visualization STATIC json_writer.cpp network_visualization.cpp)

target_include_directories(network_visualization PRIVATE ../)