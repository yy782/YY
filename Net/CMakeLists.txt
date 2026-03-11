cmake_minimum_required(VERSION 3.10)
project(net)

find_package(Protobuf REQUIRED)

file(GLOB SOURSE ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

add_library(net ${SOURSE})

target_include_directories(net 
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${Protobuf_INCLUDE_DIRS}
)

target_link_libraries(net
    PUBLIC
        common
        ${Protobuf_LIBRARIES} 
)

add_dependencies(net common)
add_subdirectory(NetTest)
add_subdirectory(Poller)
add_subdirectory(protobuf)
add_subdirectory(UDP)
add_subdirectory(HTTP)