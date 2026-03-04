cmake_minimum_required(VERSION 3.10)
project(memory_test LANGUAGES CXX)

file (GLOB TEST "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")



foreach(TEST_FILE ${TEST})
    get_filename_component(TEST_NAME ${TEST_FILE} NAME_WE)
    add_executable(${TEST_NAME} ${TEST_FILE})
    target_link_libraries(${TEST_NAME} memorypool)
    target_compile_options(${TEST_NAME}
        PRIVATE
        ${TEST_WARNING_OPTIONS}
    )

endforeach()

