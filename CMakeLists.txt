cmake_minimum_required(VERSION 3.16)
project(matching-engine VERSION 0.0.0)

include(cmake/prelude.cmake)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(MATCHING_ENGINE_BUILD_TESTS "Build tests" ON)
option(MATCHING_ENGINE_RUN_TESTS_AFTER_BUILD "Run tests after build" ON)
option(MATCHING_ENGINE_ENABLE_SANITIZERS "Enable sanitizers" ON)

find_package(asio REQUIRED)
find_package(fmt REQUIRED)

# the matching-engine
# ~~~
add_library(matching-engine INTERFACE)
target_include_directories(matching-engine INTERFACE source)
target_link_libraries(matching-engine INTERFACE asio::asio fmt::fmt)
# ~~~

# the main executable
# ~~~
add_executable(main source/main.cpp)
target_include_directories(main PRIVATE source)
target_link_libraries(main PRIVATE asio::asio fmt::fmt)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(main PRIVATE -Wall -Wextra -Wconversion)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_compile_options(main PRIVATE /W4)
endif()

# sanitizer (only on GCC or Clang)
if(
    CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang"
    AND MATCHING_ENGINE_ENABLE_SANITIZERS
)
    target_compile_options(main PRIVATE -fsanitize=address,leak,undefined)
    target_link_options(main PRIVATE -fsanitize=address,leak,undefined)
endif()
# ~~~

if(MATCHING_ENGINE_BUILD_TESTS)
    add_subdirectory(test)
endif()
