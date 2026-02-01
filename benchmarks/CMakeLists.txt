# Benchmarks CMakeLists.txt

add_executable(benchmark benchmark.cpp)
target_link_libraries(benchmark PRIVATE threadpool)

# Enable optimizations for benchmarks
target_compile_options(benchmark PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-O3 -march=native>
    $<$<CXX_COMPILER_ID:MSVC>:/O2>
)
