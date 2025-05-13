cmake_minimum_required(VERSION 3.28)

project(ayr-cpp)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(${PROJECT_SOURCE_DIR}/include)
file(GLOB TEST_SOURCES ${PROJECT_SOURCE_DIR}/test/*.cpp)
file(GLOB SRC_SOURCES ${PROJECT_SOURCE_DIR}/include/*.cpp)

foreach(TEST_SOURCE ${TEST_SOURCES})
    get_filename_component(TEST_EXE ${TEST_SOURCE} NAME_WE)
    add_executable(${TEST_EXE} ${TEST_SOURCE} ${SRC_SOURCES})
    target_include_directories(${TEST_EXE} PUBLIC ${PROJECT_SOURCE_DIR}/include)
endforeach()