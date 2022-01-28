cmake_minimum_required(VERSION 3.0)

project(iac)

set(viz_site_dir "${CMAKE_INSTALL_PREFIX}/include/iac/network_visualization/site")
message("VISUALIZATION_SITE_DIRECTORY=${viz_site_dir}")
add_compile_definitions( VISUALIZATION_SITE_DIRECTORY="${viz_site_dir}")


# add_compile_definitions(IAC_DISABLE_STD)
# add_compile_definitions(IAC_DISABLE_LOG_WITH_LINE_NUMBERS)

set(cpp_files
    "endpoint.cpp"
    "buffer_rw.cpp"
    "node.cpp"
    "package.cpp"
    "logging.cpp"
    "network_visualization/network_visualization.cpp"
    "transport_routes/transport_route.cpp"
    "transport_routes/socket_transport_route.cpp"
    "transport_routes/loopback_transport_route.cpp")

add_library(iac ${cpp_files})

target_include_directories(iac PRIVATE ./)
target_include_directories(iac PRIVATE ../dependencies/)

target_link_libraries(iac PRIVATE stdc++fs)

install(TARGETS iac LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(DIRECTORY "${CMAKE_SOURCE_DIR}/" #source directory
        DESTINATION "include/iac" #target directory
        FILES_MATCHING #install only matched files
        PATTERN "*.hpp" #select header files
        PATTERN "**/site/*" PATTERN "build" EXCLUDE
        PATTERN ".*" EXCLUDE
        PATTERN "**CMakeFiles**" EXCLUDE)

