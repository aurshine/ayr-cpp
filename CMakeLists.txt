cmake_minimum_required(VERSION 3.28)

project(ayr-cpp)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(ayr INTERFACE)
target_include_directories(ayr INTERFACE ${PROJECT_SOURCE_DIR}/include)

if (CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    
    message(STATUS "Building tests")

    file(GLOB TEST_SOURCES ${PROJECT_SOURCE_DIR}/test/*.cpp)

    foreach(TEST_SOURCE ${TEST_SOURCES})
        get_filename_component(TEST_EXE ${TEST_SOURCE} NAME_WE)
        add_executable(${TEST_EXE} ${TEST_SOURCE})
        target_link_libraries(${TEST_EXE} PRIVATE ayr)
        target_include_directories(${TEST_EXE} PUBLIC ${PROJECT_SOURCE_DIR}/include)
    endforeach()
endif()