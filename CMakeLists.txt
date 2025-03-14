cmake_minimum_required(VERSION 3.28)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "D:/Download/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
endif()

project(ayr-cpp)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_PREFIX_PATH "D:/Download/vcpkg/installed/x64-windows" CACHE STRING "")
find_package(Boost REQUIRED)

include_directories(${PROJECT_SOURCE_DIR}/include)

file(GLOB TEST_SOURCES ${PROJECT_SOURCE_DIR}/test/*.cpp)
foreach(TEST_SOURCE ${TEST_SOURCES})
    get_filename_component(TEST_EXE ${TEST_SOURCE} NAME_WE)
    add_executable(${TEST_EXE} ${TEST_SOURCE})
    target_include_directories(${TEST_EXE} PUBLIC ${PROJECT_SOURCE_DIR}/include)
    target_link_libraries(${TEST_EXE} Boost::boost)
endforeach()