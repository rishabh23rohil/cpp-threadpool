/**
 * @file basic_usage.cpp
 * @brief Basic usage examples for cpp-threadpool
 */

#include <threadpool/threadpool.hpp>
#include <iostream>
#include <vector>
#include <cmath>

int main() {
    std::cout << "=== cpp-threadpool Basic Usage ===" << std::endl;
    std::cout << std::endl;
    
    // Example 1: Create a thread pool
    std::cout << "1. Creating thread pool..." << std::endl;
    tp::ThreadPool pool;
    std::cout << "   Pool created with " << pool.size() << " threads" << std::endl;
    std::cout << std::endl;
    
    // Example 2: Submit a simple task
    std::cout << "2. Submitting a void task..." << std::endl;
    auto future1 = pool.submit([] {
        std::cout << "   Hello from thread pool!" << std::endl;
    });
    future1.wait();
    std::cout << std::endl;
    
    // Example 3: Get a return value
    std::cout << "3. Getting return value from task..." << std::endl;
    auto future2 = pool.submit([] {
        return 42;
    });
    std::cout << "   Result: " << future2.get() << std::endl;
    std::cout << std::endl;
    
    // Example 4: Pass arguments to tasks
    std::cout << "4. Passing arguments to tasks..." << std::endl;
    auto future3 = pool.submit([](int a, int b) {
        return a * b;
    }, 7, 6);
    std::cout << "   7 * 6 = " << future3.get() << std::endl;
    std::cout << std::endl;
    
    // Example 5: Submit multiple tasks
    std::cout << "5. Computing squares of 1-10 in parallel..." << std::endl;
    std::vector<std::future<int>> futures;
    for (int i = 1; i <= 10; ++i) {
        futures.push_back(pool.submit([](int n) {
            return n * n;
        }, i));
    }
    std::cout << "   Results: ";
    for (auto& f : futures) {
        std::cout << f.get() << " ";
    }
    std::cout << std::endl;
    std::cout << std::endl;
    
    // Example 6: Using parallel_for
    std::cout << "6. Using parallel_for..." << std::endl;
    std::vector<double> results(10);
    tp::parallel_for(pool, 0, 10, [&results](size_t i) {
        results[i] = std::sqrt(static_cast<double>(i));
    });
    std::cout << "   Square roots: ";
    for (const auto& r : results) {
        std::cout << r << " ";
    }
    std::cout << std::endl;
    std::cout << std::endl;
    
    // Example 7: Using parallel_map
    std::cout << "7. Using parallel_map..." << std::endl;
    std::vector<int> input = {1, 2, 3, 4, 5};
    auto mapped = tp::parallel_map(pool, input, [](int x) {
        return x * x * x;  // Cube
    });
    std::cout << "   Cubes: ";
    for (const auto& m : mapped) {
        std::cout << m << " ";
    }
    std::cout << std::endl;
    std::cout << std::endl;
    
    // Example 8: Priority tasks
    std::cout << "8. Priority task scheduling..." << std::endl;
    std::atomic<int> order{0};
    std::vector<std::pair<int, int>> execution_order;
    std::mutex order_mutex;
    
    // Block the pool first
    std::promise<void> blocker;
    pool.submit([&blocker] { blocker.get_future().wait(); });
    
    // Submit with different priorities
    pool.submit_priority(10, [&] {
        std::lock_guard<std::mutex> lock(order_mutex);
        execution_order.push_back({10, order++});
    });
    pool.submit_priority(1, [&] {
        std::lock_guard<std::mutex> lock(order_mutex);
        execution_order.push_back({1, order++});
    });
    pool.submit_priority(5, [&] {
        std::lock_guard<std::mutex> lock(order_mutex);
        execution_order.push_back({5, order++});
    });
    
    blocker.set_value();
    pool.wait();
    
    std::cout << "   Execution order (priority -> order):" << std::endl;
    for (const auto& [prio, ord] : execution_order) {
        std::cout << "     Priority " << prio << " executed at position " << ord << std::endl;
    }
    std::cout << std::endl;
    
    // Example 9: Pool statistics
    std::cout << "9. Pool statistics..." << std::endl;
    auto stats = pool.stats();
    std::cout << "   Tasks submitted: " << stats.total_tasks_submitted << std::endl;
    std::cout << "   Tasks completed: " << stats.total_tasks_completed << std::endl;
    std::cout << "   Tasks stolen: " << stats.total_tasks_stolen << std::endl;
    std::cout << std::endl;
    
    std::cout << "=== Examples Complete ===" << std::endl;
    
    return 0;
}
