cmake_minimum_required(VERSION 3.28)

project(ayr-cpp)

set(CMAKE_CXX_STANDARD 20)

include_directories(${PROJECT_SOURCE_DIR}/include)

add_executable(ayr ${PROJECT_SOURCE_DIR}/test/test_main.cpp)