cmake_minimum_required(VERSION 3.10)
project(net)

file(GLOB SOURSE ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

add_library(net ${SOURSE})

target_include_directories(net 
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
    )

target_link_libraries(net
    PUBLIC
    common
)
add_dependencies(net common)      