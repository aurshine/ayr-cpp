cmake_minimum_required(VERSION 3.28)

project(ayr-cpp)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(CTest)

option(AYR_ENABLE_COVERAGE "Build tests with GCC/Clang coverage instrumentation" OFF)

# https需要使用openssl
find_package(OpenSSL REQUIRED)
find_package(fmt QUIET)

add_library(ayr INTERFACE)
target_include_directories(ayr INTERFACE ${PROJECT_SOURCE_DIR}/include)

if (MSVC)
    target_compile_options(ayr INTERFACE /Zc:preprocessor)
endif()

# 给库加上 OpenSSL
target_link_libraries(ayr INTERFACE OpenSSL::SSL OpenSSL::Crypto)

if (WIN32)
    # TlsLayer通过Windows CryptoAPI读取系统受信任根证书存储。
    target_link_libraries(ayr INTERFACE Crypt32)
endif()

if (fmt_FOUND)
    target_link_libraries(ayr INTERFACE fmt::fmt)
    target_compile_definitions(ayr INTERFACE AYR_USE_FMT=1)
else()
    target_compile_definitions(ayr INTERFACE AYR_USE_FMT=0)
endif()

if (CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME AND BUILD_TESTING)
    
    message(STATUS "Building tests")

    file(GLOB TEST_SOURCES CONFIGURE_DEPENDS ${PROJECT_SOURCE_DIR}/test/*.cpp)

    foreach(TEST_SOURCE ${TEST_SOURCES})
        get_filename_component(TEST_EXE ${TEST_SOURCE} NAME_WE)
        add_executable(${TEST_EXE} ${TEST_SOURCE})
        target_link_libraries(${TEST_EXE} PRIVATE ayr)
        target_include_directories(${TEST_EXE} PRIVATE ${PROJECT_SOURCE_DIR}/include)

        if (AYR_ENABLE_COVERAGE AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
            target_compile_options(${TEST_EXE} PRIVATE -O0 -g --coverage)
            target_link_options(${TEST_EXE} PRIVATE --coverage)
        endif()
        
        add_test(NAME ${TEST_EXE} COMMAND ${TEST_EXE})
        
    endforeach()
endif()
