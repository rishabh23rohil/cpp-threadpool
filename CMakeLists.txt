cmake_minimum_required(VERSION 3.16)
project(cpp-threadpool 
    VERSION 1.0.0
    DESCRIPTION "Modern C++17 thread pool library"
    LANGUAGES CXX
)

# C++17 required
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Options
option(THREADPOOL_BUILD_TESTS "Build unit tests" ON)
option(THREADPOOL_BUILD_EXAMPLES "Build examples" ON)
option(THREADPOOL_BUILD_BENCHMARKS "Build benchmarks" ON)

# Header-only library target
add_library(threadpool INTERFACE)
add_library(tp::threadpool ALIAS threadpool)

target_include_directories(threadpool INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_compile_features(threadpool INTERFACE cxx_std_17)

# Find threads
find_package(Threads REQUIRED)
target_link_libraries(threadpool INTERFACE Threads::Threads)

# Tests
if(THREADPOOL_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# Examples
if(THREADPOOL_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# Benchmarks
if(THREADPOOL_BUILD_BENCHMARKS)
    add_subdirectory(benchmarks)
endif()

# Installation
include(GNUInstallDirs)
install(TARGETS threadpool
    EXPORT threadpool-targets
)

install(DIRECTORY include/threadpool
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(EXPORT threadpool-targets
    FILE threadpool-config.cmake
    NAMESPACE tp::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/threadpool
)
