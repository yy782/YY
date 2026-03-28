cmake_minimum_required(VERSION 3.10)

project(yy LANGUAGES CXX)
# make -j4 2> error.txt
# @learn 
    # project必须在cmake_minimum_required之后,其他文件之前
#
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找 Boost（需要 1.83+）
find_package(Boost 1.83 REQUIRED)

# 如果 find_package 找不到，可以手动指定路径
# set(BOOST_ROOT /usr/local)
# find_package(Boost 1.83 REQUIRED)

#cmake -DCMAKE_BUILD_TYPE=Release ..
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# @learn
    #CMAKE_ARCHIVE_OUTPUT_DIRECTORY是归档文件输出目录，(静态库文件)
    #CMAKE_LIBRARY_OUTPUT_DIRECTORY是动态库文件输出目录
    #CMAKE_RUNTIME_OUTPUT_DIRECTORY是可执行文件输出目录
#

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(WARNING_OPTIONS
        -g -O0
        -std=c++17
        -DNEXCLUDE_BEFORE_COMPILATION
        -rdynamic
        -march=native
        -Wall
        -Wextra                 #额外警告
        -Wpedantic              #严格遵守C++标准
        -Werror                 #将警告视为错误
        -Wno-unknown-pragmas    #需要关闭的警告
        -Wshadow                #变量遮掩警告
        -Wconversion            #类型转换警告
        -Wunused-parameter     #未使用参数警告
        -Wold-style-cast
        -Woverloaded-virtual
        -Wpointer-arith
        -Wwrite-strings
    )
else()
    set(WARNING_OPTIONS
        -O3                     # 生产优化
        -std=c++17
        -march=native
        -DNEXCLUDE_BEFORE_COMPILATION
        -DNDEBUG
        -Wall
        -Wextra
        -Wno-error              # 警告不升级为错误
        -Wno-shadow
        -Wno-conversion    
    )
endif()    

add_subdirectory(Common)
add_subdirectory(MemoryPool)
add_subdirectory(ThreadPool)
add_subdirectory(Net)

# @learn 
    #add_subdirectory多目录需要拆开调用
#

# 添加 Boost 包含目录到所有子项目
# 方法1：通过目标属性（推荐）
target_compile_options(common PRIVATE ${WARNING_OPTIONS})
target_compile_options(memorypool PRIVATE ${WARNING_OPTIONS})
target_compile_options(threadpool PRIVATE ${WARNING_OPTIONS})
target_compile_options(net PRIVATE ${WARNING_OPTIONS})

# 为每个子目标添加 Boost 包含目录
target_include_directories(common PRIVATE ${Boost_INCLUDE_DIRS})
target_include_directories(memorypool PRIVATE ${Boost_INCLUDE_DIRS})
target_include_directories(threadpool PRIVATE ${Boost_INCLUDE_DIRS})
target_include_directories(net PRIVATE ${Boost_INCLUDE_DIRS})

# 如果某些库需要链接 Boost 库（如 boost_system, boost_thread）
# 取消下面的注释
# target_link_libraries(common PRIVATE ${Boost_LIBRARIES})
# target_link_libraries(net PRIVATE ${Boost_LIBRARIES})

# 创建接口库
add_library(yy INTERFACE)

# 为接口库添加 Boost 依赖
target_include_directories(yy INTERFACE ${Boost_INCLUDE_DIRS})
target_link_libraries(yy INTERFACE ${Boost_LIBRARIES})

target_link_libraries(yy 
    INTERFACE 
        common 
        memorypool
        net
        threadpool
)