# Tests CMakeLists.txt
include(FetchContent)

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
)

FetchContent_MakeAvailable(googletest)

# Test executables
add_executable(test_basic test_basic.cpp)
target_link_libraries(test_basic PRIVATE threadpool GTest::gtest_main)

add_executable(test_futures test_futures.cpp)
target_link_libraries(test_futures PRIVATE threadpool GTest::gtest_main)

add_executable(test_stress test_stress.cpp)
target_link_libraries(test_stress PRIVATE threadpool GTest::gtest_main)

# Register tests
include(GoogleTest)
gtest_discover_tests(test_basic)
gtest_discover_tests(test_futures)
gtest_discover_tests(test_stress)
