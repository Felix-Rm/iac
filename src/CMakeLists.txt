cmake_minimum_required(VERSION 3.0)
enable_testing()

project(iac)

set(viz_site_dir "${CMAKE_INSTALL_PREFIX}/include/iac/src/network_visualization/site/")
message("VISUALIZATION_SITE_DIRECTORY=${viz_site_dir}")

add_compile_definitions( VISUALIZATION_SITE_DIRECTORY="${viz_site_dir}")


# add_compile_definitions(IAC_DISABLE_STD)
# add_compile_definitions(IAC_DISABLE_LOG_WITH_LINE_NUMBERS)

set(cpp_files
    "buffer_rw.cpp"
    "package.cpp"
    "local_node.cpp"
    "local_endpoint.cpp"
    "logging.cpp"
    "network.cpp"
)

add_subdirectory(transport_routes/)
add_subdirectory(network_visualization/)


add_library(iac STATIC ${cpp_files})
target_link_libraries(iac transport_routes network_visualization)

target_include_directories(iac PRIVATE ./)
target_include_directories(iac PRIVATE ../dependencies/)

install(TARGETS iac LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(DIRECTORY "${CMAKE_SOURCE_DIR}/" #source directory
        DESTINATION "include/iac" #target directory
        FILES_MATCHING #install only matched files
        PATTERN "*.hpp" #select header files
        PATTERN "**/site/*" 
        PATTERN "build" EXCLUDE
        PATTERN ".*" EXCLUDE
        PATTERN "**CMakeFiles**" EXCLUDE)

