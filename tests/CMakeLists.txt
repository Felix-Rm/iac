cmake_minimum_required(VERSION 3.0)
enable_testing()

project(iac_test_suite)

# set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_executable(iac_test_suite testing.cpp)

target_include_directories(iac_test_suite PRIVATE ../dependencies/)
target_include_directories(iac_test_suite PRIVATE ../src/)

target_link_libraries(iac_test_suite PRIVATE iac ftest)
target_link_libraries(iac_test_suite PRIVATE Threads::Threads)

add_test(iac_test_suite iac_test_suite)