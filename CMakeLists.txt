cmake_minimum_required(VERSION 3.28)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(ayr-cpp LANGUAGES CXX)
    include(CTest)
    option(AYR_ENABLE_COVERAGE "Build tests with GCC/Clang coverage instrumentation" OFF)
endif()

# https需要使用openssl
find_package(OpenSSL REQUIRED)

add_library(ayr INTERFACE)
add_library(ayr::ayr ALIAS ayr)
target_include_directories(ayr INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(ayr INTERFACE cxx_std_20)

if (MSVC)
    target_compile_options(ayr INTERFACE /Zc:preprocessor)
endif()

# 给库加上 OpenSSL
target_link_libraries(ayr INTERFACE OpenSSL::SSL OpenSSL::Crypto)

if (WIN32)
    # TlsLayer通过Windows CryptoAPI读取系统受信任根证书存储。
    target_link_libraries(ayr INTERFACE Crypt32)
endif()

if (CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR AND BUILD_TESTING)
    
    message(STATUS "Building tests")

    file(GLOB_RECURSE TEST_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/test/*.cpp)

    foreach(TEST_SOURCE ${TEST_SOURCES})
        get_filename_component(TEST_EXE ${TEST_SOURCE} NAME_WE)
        add_executable(${TEST_EXE} ${TEST_SOURCE})
        target_link_libraries(${TEST_EXE} PRIVATE ayr)
        if (AYR_ENABLE_COVERAGE AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
            target_compile_options(${TEST_EXE} PRIVATE -O0 -g --coverage)
            target_link_options(${TEST_EXE} PRIVATE --coverage)
        endif()
        
        add_test(NAME ${TEST_EXE} COMMAND ${TEST_EXE})
        
    endforeach()
endif()
