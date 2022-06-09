cmake_minimum_required(VERSION 3.0)
enable_testing()

set (CMAKE_CXX_STANDARD 17)

set(viz_site_dir "${CMAKE_INSTALL_PREFIX}/include/iac/network_visualization/site/")
message("Set VISUALIZATION_SITE_DIRECTORY to: ${viz_site_dir}")

set(iac_common_compile_defs VISUALIZATION_SITE_DIRECTORY="${viz_site_dir}")
set(iac_common_compile_defs ${iac_common_compile_defs} IAC_DISABLE_LOG_WITH_LINE_NUMBERS)
message("Libs are build with: ${iac_common_compile_defs}")

macro(IAC_BUILD target_name compile_defs)
    set(cpp_files
        "buffer_rw.cpp"
        "package.cpp"
        "local_endpoint.cpp"
        "local_node.cpp"
        "local_node_api.cpp"
        "local_node_state_handling.cpp"
        "local_node_package_handling.cpp"
        "local_transport_route.cpp"
        "logging.cpp"
        "network.cpp"
        "connection_types/connection.cpp"
        "connection_types/socket_connection.cpp"
        "connection_types/loopback_connection.cpp"
        "network_visualization/json_writer.cpp"
        "network_visualization/network_visualization.cpp"
    )

    add_library(${target_name} STATIC ${cpp_files})

    target_include_directories(${target_name} PRIVATE ./)
    target_include_directories(${target_name} PRIVATE ../dependencies/)

    target_compile_definitions(${target_name} PRIVATE ${compile_defs})
endmacro()

IAC_BUILD(iac "${iac_common_compile_defs}")
IAC_BUILD(iac_lwstd "${iac_common_compile_defs} -DIAC_USE_LWSTD")


install(TARGETS iac iac_lwstd LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

install(DIRECTORY "${CMAKE_SOURCE_DIR}/src/" #source directory
        DESTINATION "include/iac" #target directory
        FILES_MATCHING #install only matched files
        PATTERN "*.hpp" #select header files
        PATTERN "**/site/*" 
        PATTERN "build" EXCLUDE
        PATTERN ".*" EXCLUDE
        PATTERN "**CMakeFiles**" EXCLUDE)

