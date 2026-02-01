# Examples CMakeLists.txt

add_executable(basic_usage basic_usage.cpp)
target_link_libraries(basic_usage PRIVATE threadpool)

add_executable(parallel_sort parallel_sort.cpp)
target_link_libraries(parallel_sort PRIVATE threadpool)

add_executable(web_crawler web_crawler.cpp)
target_link_libraries(web_crawler PRIVATE threadpool)
