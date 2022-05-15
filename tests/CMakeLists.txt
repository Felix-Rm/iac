cmake_minimum_required(VERSION 3.0)
enable_testing()

set (CMAKE_CXX_STANDARD 17)
set_property(GLOBAL PROPERTY CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Threads REQUIRED)

macro(IAC_TEST target_name iac_link_name compile_defs)
    add_executable(${target_name} testing.cpp)

    target_include_directories(${target_name} PRIVATE ../dependencies/)
    target_include_directories(${target_name} PRIVATE ../src/)

    target_link_libraries(${target_name} PRIVATE ${iac_link_name} ftest)
    target_link_libraries(${target_name} PRIVATE Threads::Threads)


    target_compile_definitions(${target_name} PRIVATE ${compile_defs})

    target_compile_options(${target_name} PRIVATE -Wall)

    target_compile_options(${target_name} PRIVATE -fsanitize=address)
    target_link_options(${target_name} PRIVATE -lasan)

    add_test(${target_name} ${target_name})
endmacro()

IAC_TEST(iac_test_suite iac "")
IAC_TEST(iac_test_suite_lwstd iac_lwstd "IAC_USE_LWSTD")