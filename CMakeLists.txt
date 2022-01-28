cmake_minimum_required(VERSION 3.0)
enable_testing()

set_property(GLOBAL PROPERTY CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CXX_STANDARD 17)
set(GCC_COVERAGE_COMPILE_FLAGS "-std=17")

if(NOT DEFINED DISABLE_BUILD_ALL)
    set(COMPILE_TESTS ON)
    # set(BUILD_PYTHON_BINDINGS ON)
endif()


add_subdirectory(src)

# Define COMPILE_TESTS to build a test suite for iac
if(COMPILE_TESTS)
add_subdirectory(dependencies/ftest)
add_subdirectory(dependencies/lw_std)
add_subdirectory(tests)
endif()

# Define BUILD_PYTHON_BINDINGS to also build python bindings for iac (requires pybind11)
if(BUILD_PYTHON_BINDINGS)
add_subdirectory(python_bindings)
endif()