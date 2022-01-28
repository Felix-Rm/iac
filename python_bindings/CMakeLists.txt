cmake_minimum_required(VERSION 3.0)
enable_testing()

SET(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-std=c++17 -Wall")

project(iac_py)

# add_compile_definitions(COMPILE_PYTHON_BINDINGS IAC_ENABLE_DEBUG_PRINTING)

find_package(pybind11 REQUIRED)

pybind11_add_module(iac_py python_bindings.cpp)
target_link_libraries(iac_py PRIVATE stdc++fs iac)

find_package(Python COMPONENTS Interpreter Development)
install(TARGETS iac_py DESTINATION ${Python_SITELIB})