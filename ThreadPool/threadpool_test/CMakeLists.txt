cmake_minimum_required(VERSION 3.10)
project(threadpool_test)

set(BENCHMARK_ENABLE_GTEST_TESTS OFF CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)



add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/google-benchmark)

file(GLOB SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

foreach(SOURCE ${SOURCES})
    get_filename_component(TEST_NAME ${SOURCE} NAME_WE)
    add_executable(${TEST_NAME} ${SOURCE})
    target_link_libraries(${TEST_NAME} 
    threadpool
    benchmark::benchmark
    pthread
    )
    add_dependencies(${TEST_NAME} threadpool)
endforeach()
