cmake_minimum_required(VERSION 3.10)
project(KVStore)

set(CMAKE_CXX_STANDARD 17)

option(USE_LOGGING "Enable server logging" OFF)

set(LOG_LEVEL "NORMAL" CACHE STRING "Level of logging: NORMAL or FULL")
set_property(CACHE LOG_LEVEL PROPERTY STRINGS "NORMAL" "FULL")

add_executable(server
    src/main.cpp
    src/server.cpp
    src/client_handler.cpp
    src/parser.cpp
    src/store.cpp
    src/logger.cpp
)

target_include_directories(server PRIVATE include)

if(USE_LOGGING)
    target_compile_definitions(server PRIVATE ENABLE_LOGGING)
    
    if(LOG_LEVEL STREQUAL "FULL")
        target_compile_definitions(server PRIVATE LOG_FILTER_FULL)
    else()
        target_compile_definitions(server PRIVATE LOG_FILTER_NORMAL)
    endif()
endif()

find_package(Threads REQUIRED)
target_link_libraries(server PRIVATE Threads::Threads)

add_executable(benchmark tests/benchmark.cpp)
target_link_libraries(benchmark PRIVATE Threads::Threads)