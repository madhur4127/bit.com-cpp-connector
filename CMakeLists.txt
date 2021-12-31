cmake_minimum_required(VERSION 3.21)
project(assignment)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_BUILD_TYPE Debug)

add_subdirectory(fmt)
find_package(Boost 1.78 REQUIRED)
find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED)

include(FetchContent)
FetchContent_Declare(
        simdjson
        GIT_REPOSITORY https://github.com/simdjson/simdjson.git
        GIT_TAG master
        SOURCE_DIR ${CMAKE_SOURCE_DIR}/x_deps/simdjson
)

FetchContent_GetProperties(simdjson)
if (NOT simdjson_POPULATED)
    FetchContent_Populate(simdjson)
    option(SIMDJSON_JUST_LIBRARY "" ON)
    option(SIMDJSON_ENABLE_THREADS "Link with thread support" OFF)
    option(SIMDJSON_BUILD_STATIC "" ON)

    add_subdirectory(${simdjson_SOURCE_DIR} ${simdjson_BINARY_DIR})
endif ()

#add_compile_options(-fsanitize=address,undefined -g -fno-omit-frame-pointer)
#add_link_options(-fsanitize=address,undefined)

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    add_compile_options(-stdlib=libc++ -fcoroutines-ts)
    add_link_options(-stdlib=libc++)
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    add_compile_options(-fcoroutines)
else ()
    message(FATAL_ERROR "compiler not supported")
endif ()

add_executable(assignment
        main.cpp
        connector.cc)
target_link_libraries(assignment PUBLIC
        ${Boost_LIBRARIES}
        ${CMAKE_THREAD_LIBS_INIT}
        ${OPENSSL_LIBRARIES}
        fmt::fmt-header-only
        simdjson)
target_precompile_headers(assignment
        PUBLIC
        config.h)

