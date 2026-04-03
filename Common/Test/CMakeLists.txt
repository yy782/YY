cmake_minimum_required(VERSION 3.10)
project(common_test LANGUAGES CXX)

file(GLOB TEST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

foreach(TEST_SOURCE ${TEST_SOURCES})
    get_filename_component(TEST_NAME ${TEST_SOURCE} NAME_WE)    
    add_executable(${TEST_NAME} ${TEST_SOURCE})
    
    target_link_libraries(${TEST_NAME}
        PRIVATE
        common
    )
    
    target_include_directories(${TEST_NAME}
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}
    )
    target_compile_options(${TEST_NAME}
        PRIVATE
        ${WARNING_OPTIONS}
    )
endforeach()