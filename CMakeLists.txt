cmake_minimum_required(VERSION 3.0)
enable_testing()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

#SET(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-std=c++17 -g -Wall -fsanitize=address")

#SET(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-std=c++17 -O0 -Wall -fsanitize=address")
#SET(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-std=c++17 -O0 -Wall")
SET(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-std=c++17 -Wall")

project(iac)
include_directories("./")

set(viz_site_dir "${CMAKE_INSTALL_PREFIX}/include/iac/network_visualization/site")
message("VISUALIZATION_SITE_DIRECTORY=${viz_site_dir}")

# add_compile_definitions(IAC_DISABLE_STD)
# add_compile_definitions(IAC_DISABLE_LOG_WITH_LINE_NUMBERS)
add_compile_definitions( VISUALIZATION_SITE_DIRECTORY="${viz_site_dir}")

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
target_link_libraries(iac PRIVATE stdc++fs)

install(TARGETS iac LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(DIRECTORY "${CMAKE_SOURCE_DIR}/" #source directory
        DESTINATION "include/iac" #target directory
        FILES_MATCHING #install only matched files
        PATTERN "*.hpp" #select header files
        PATTERN "**/site/*" PATTERN "build" EXCLUDE
        PATTERN ".*" EXCLUDE
        PATTERN "**CMakeFiles**" EXCLUDE)

project(iac_test_suite)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_executable(iac_test_suite tests/testing.cpp tests/test_logging.cpp)
target_link_libraries(iac_test_suite PRIVATE iac)
target_link_libraries(iac_test_suite PRIVATE Threads::Threads)

add_test(test_suite iac_test_suite)

if(COMPILE_PYTHON_BINDINGS)
project(iac_py)

# add_compile_definitions(COMPILE_PYTHON_BINDINGS IAC_ENABLE_DEBUG_PRINTING)

find_package(pybind11 REQUIRED)

pybind11_add_module(iac_py python_bindings.cpp ${cpp_files})
target_link_libraries(iac_py PRIVATE stdc++fs)
#target_link_libraries(iac_py PRIVATE iac)

find_package(Python COMPONENTS Interpreter Development)
install(TARGETS iac_py DESTINATION ${Python_SITELIB})
endif()