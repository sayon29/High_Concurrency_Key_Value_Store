cmake_minimum_required(VERSION 3.10)
project(KVStore)

set(CMAKE_CXX_STANDARD 17)

option(USE_LOGGING "Enable server logging" OFF)

set(LOG_LEVEL "NORMAL" CACHE STRING "Level of logging: NORMAL or FULL")
set_property(CACHE LOG_LEVEL PROPERTY STRINGS "NORMAL" "FULL")

find_package(Threads REQUIRED)

add_executable(server
    src/main.cpp
    src/server.cpp
    src/client_handler.cpp
    src/parser.cpp
    src/store.cpp
    src/logger.cpp
    src/event_loop.cpp
)

target_include_directories(server PRIVATE include)
target_link_libraries(server PRIVATE Threads::Threads)

add_executable(benchmark tests/benchmark.cpp)
target_include_directories(benchmark PRIVATE include)
target_link_libraries(benchmark PRIVATE Threads::Threads)

add_executable(shard_benchmark tests/shard_benchmark.cpp)
target_include_directories(shard_benchmark PRIVATE include)
target_link_libraries(shard_benchmark PRIVATE Threads::Threads)

if(USE_LOGGING)
    target_compile_definitions(server PRIVATE ENABLE_LOGGING)
    
    if(LOG_LEVEL STREQUAL "FULL")
        target_compile_definitions(server PRIVATE LOG_FILTER_FULL)
    else()
        target_compile_definitions(server PRIVATE LOG_FILTER_NORMAL)
    endif()
endif()