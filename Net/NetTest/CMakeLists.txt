cmake_minimum_required(VERSION 3.10)
project(net_test LANGUAGES CXX)

find_package(Protobuf REQUIRED)

file(GLOB PROTO_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.proto")

set(PROTO_GEN_DIR ${CMAKE_CURRENT_BINARY_DIR}/proto_gen)
file(MAKE_DIRECTORY ${PROTO_GEN_DIR})
protobuf_generate(
    LANGUAGE cpp
    OUT_VAR PROTO_GEN_SRCS
    PROTOS ${PROTO_FILES}
    PROTOC_OUT_DIR ${PROTO_GEN_DIR}
)

if(PROTO_GEN_SRCS)
    add_library(proto_objects OBJECT ${PROTO_GEN_SRCS})
    target_include_directories(proto_objects 
        PRIVATE 
            ${PROTO_GEN_DIR}
            ${Protobuf_INCLUDE_DIRS}
    )
    target_link_libraries(proto_objects 
        PUBLIC 
            ${Protobuf_LIBRARIES}
    )
endif()

file(GLOB TEST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")


foreach(TEST_SOURCE ${TEST_SOURCES})
    get_filename_component(TEST_NAME ${TEST_SOURCE} NAME_WE)
    
    add_executable(${TEST_NAME} ${TEST_SOURCE})
    
    if(TARGET proto_objects)
        target_link_libraries(${TEST_NAME}
            PRIVATE
                net
                threadpool
                proto_objects
        )
    else()
        target_link_libraries(${TEST_NAME}
            PRIVATE
                net
                threadpool
                ${Protobuf_LIBRARIES}
        )
    endif()
    
    target_include_directories(${TEST_NAME}
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}
            ${PROTO_GEN_DIR}
            ${Protobuf_INCLUDE_DIRS}
    )
    
    
    target_compile_options(${TEST_NAME}
        PRIVATE
        ${WARNING_OPTIONS}
    )

    
endforeach()
