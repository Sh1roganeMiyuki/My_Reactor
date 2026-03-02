cmake_minimum_required(VERSION 3.0)
project(My_Reactor)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(${PROJECT_SOURCE_DIR}/include)

file(GLOB SOURCES "${PROJECT_SOURCE_DIR}/src/*.cpp")

add_executable(http_server example/main.cpp ${SOURCES})

target_link_libraries(http_server pthread)

message(STATUS "Found sources: ${SOURCES}")