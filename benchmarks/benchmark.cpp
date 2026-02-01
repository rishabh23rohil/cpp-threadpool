/**
 * @file benchmark.cpp
 * @brief Performance benchmarks for cpp-threadpool
 */

#include <threadpool/threadpool.hpp>
#include <iostream>
#include <vector>
#include <chrono>
#include <iomanip>
#include <cmath>
#include <numeric>

using Clock = std::chrono::high_resolution_clock;
using Duration = std::chrono::duration<double, std::milli>;

/**
 * @brief Benchmark result
 */
struct BenchmarkResult {
    std::string name;
    size_t num_tasks;
    double total_time_ms;
    double tasks_per_second;
};

/**
 * @brief Print benchmark results
 */
void print_results(const std::vector<BenchmarkResult>& results) {
    std::cout << "\n";
    std::cout << std::left << std::setw(30) << "Benchmark"
              << std::right << std::setw(12) << "Tasks"
              << std::right << std::setw(15) << "Time (ms)"
              << std::right << std::setw(18) << "Throughput"
              << std::endl;
    std::cout << std::string(75, '-') << std::endl;
    
    for (const auto& r : results) {
        std::cout << std::left << std::setw(30) << r.name
                  << std::right << std::setw(12) << r.num_tasks
                  << std::right << std::setw(15) << std::fixed << std::setprecision(2) << r.total_time_ms
                  << std::right << std::setw(15) << std::fixed << std::setprecision(0) << r.tasks_per_second
                  << " tasks/sec"
                  << std::endl;
    }
}

/**
 * @brief Benchmark: Empty tasks
 */
BenchmarkResult benchmark_empty_tasks(tp::ThreadPool& pool, size_t num_tasks) {
    std::vector<std::future<void>> futures;
    futures.reserve(num_tasks);
    
    auto start = Clock::now();
    
    for (size_t i = 0; i < num_tasks; ++i) {
        futures.push_back(pool.submit([] {}));
    }
    
    for (auto& f : futures) {
        f.wait();
    }
    
    auto end = Clock::now();
    Duration elapsed = end - start;
    
    return {
        "Empty tasks",
        num_tasks,
        elapsed.count(),
        (num_tasks / elapsed.count()) * 1000.0
    };
}

/**
 * @brief Benchmark: Light compute tasks
 */
BenchmarkResult benchmark_light_compute(tp::ThreadPool& pool, size_t num_tasks) {
    std::vector<std::future<double>> futures;
    futures.reserve(num_tasks);
    
    auto start = Clock::now();
    
    for (size_t i = 0; i < num_tasks; ++i) {
        futures.push_back(pool.submit([i] {
            double result = 0.0;
            for (int j = 0; j < 100; ++j) {
                result += std::sin(static_cast<double>(i + j));
            }
            return result;
        }));
    }
    
    for (auto& f : futures) {
        f.get();
    }
    
    auto end = Clock::now();
    Duration elapsed = end - start;
    
    return {
        "Light compute (100 sin ops)",
        num_tasks,
        elapsed.count(),
        (num_tasks / elapsed.count()) * 1000.0
    };
}

/**
 * @brief Benchmark: Heavy compute tasks
 */
BenchmarkResult benchmark_heavy_compute(tp::ThreadPool& pool, size_t num_tasks) {
    std::vector<std::future<double>> futures;
    futures.reserve(num_tasks);
    
    auto start = Clock::now();
    
    for (size_t i = 0; i < num_tasks; ++i) {
        futures.push_back(pool.submit([i] {
            double result = 0.0;
            for (int j = 0; j < 10000; ++j) {
                result += std::sin(static_cast<double>(i + j)) * 
                          std::cos(static_cast<double>(i - j));
            }
            return result;
        }));
    }
    
    for (auto& f : futures) {
        f.get();
    }
    
    auto end = Clock::now();
    Duration elapsed = end - start;
    
    return {
        "Heavy compute (10K ops)",
        num_tasks,
        elapsed.count(),
        (num_tasks / elapsed.count()) * 1000.0
    };
}

/**
 * @brief Benchmark: Memory allocation tasks
 */
BenchmarkResult benchmark_memory_alloc(tp::ThreadPool& pool, size_t num_tasks) {
    std::vector<std::future<size_t>> futures;
    futures.reserve(num_tasks);
    
    auto start = Clock::now();
    
    for (size_t i = 0; i < num_tasks; ++i) {
        futures.push_back(pool.submit([i] {
            std::vector<int> vec(1000);
            std::iota(vec.begin(), vec.end(), static_cast<int>(i));
            return std::accumulate(vec.begin(), vec.end(), 0UL);
        }));
    }
    
    for (auto& f : futures) {
        f.get();
    }
    
    auto end = Clock::now();
    Duration elapsed = end - start;
    
    return {
        "Memory alloc (1K ints)",
        num_tasks,
        elapsed.count(),
        (num_tasks / elapsed.count()) * 1000.0
    };
}

/**
 * @brief Benchmark: Mixed workload
 */
BenchmarkResult benchmark_mixed_workload(tp::ThreadPool& pool, size_t num_tasks) {
    std::vector<std::future<double>> futures;
    futures.reserve(num_tasks);
    
    auto start = Clock::now();
    
    for (size_t i = 0; i < num_tasks; ++i) {
        futures.push_back(pool.submit([i] {
            // Mix of operations
            std::vector<double> vec(100);
            for (size_t j = 0; j < vec.size(); ++j) {
                vec[j] = std::sin(static_cast<double>(i + j));
            }
            std::sort(vec.begin(), vec.end());
            return std::accumulate(vec.begin(), vec.end(), 0.0);
        }));
    }
    
    for (auto& f : futures) {
        f.get();
    }
    
    auto end = Clock::now();
    Duration elapsed = end - start;
    
    return {
        "Mixed workload",
        num_tasks,
        elapsed.count(),
        (num_tasks / elapsed.count()) * 1000.0
    };
}

/**
 * @brief Benchmark: Priority tasks
 */
BenchmarkResult benchmark_priority_tasks(tp::ThreadPool& pool, size_t num_tasks) {
    std::vector<std::future<int>> futures;
    futures.reserve(num_tasks);
    
    auto start = Clock::now();
    
    for (size_t i = 0; i < num_tasks; ++i) {
        int priority = static_cast<int>(i % 10);
        futures.push_back(pool.submit_priority(priority, [i] {
            return static_cast<int>(i * i);
        }));
    }
    
    for (auto& f : futures) {
        f.get();
    }
    
    auto end = Clock::now();
    Duration elapsed = end - start;
    
    return {
        "Priority tasks (10 levels)",
        num_tasks,
        elapsed.count(),
        (num_tasks / elapsed.count()) * 1000.0
    };
}

/**
 * @brief Compare single-threaded vs thread pool
 */
void benchmark_scaling() {
    std::cout << "\n=== Scaling Benchmark ===" << std::endl;
    
    const size_t num_tasks = 10000;
    
    // Single-threaded
    auto start = Clock::now();
    double result = 0.0;
    for (size_t i = 0; i < num_tasks; ++i) {
        for (int j = 0; j < 1000; ++j) {
            result += std::sin(static_cast<double>(i + j));
        }
    }
    auto end = Clock::now();
    Duration single_time = end - start;
    
    std::cout << "Single-threaded: " << std::fixed << std::setprecision(2) 
              << single_time.count() << " ms" << std::endl;
    
    // Thread pool with different thread counts
    for (size_t num_threads : {1, 2, 4, 8}) {
        tp::ThreadPool pool(num_threads);
        
        std::vector<std::future<double>> futures;
        futures.reserve(num_tasks);
        
        start = Clock::now();
        
        for (size_t i = 0; i < num_tasks; ++i) {
            futures.push_back(pool.submit([i] {
                double r = 0.0;
                for (int j = 0; j < 1000; ++j) {
                    r += std::sin(static_cast<double>(i + j));
                }
                return r;
            }));
        }
        
        for (auto& f : futures) {
            f.get();
        }
        
        end = Clock::now();
        Duration pool_time = end - start;
        
        double speedup = single_time.count() / pool_time.count();
        
        std::cout << num_threads << " threads: " << std::fixed << std::setprecision(2)
                  << pool_time.count() << " ms (speedup: " << speedup << "x)" << std::endl;
    }
}

int main() {
    std::cout << "=== cpp-threadpool Benchmarks ===" << std::endl;
    std::cout << "Hardware concurrency: " << std::thread::hardware_concurrency() << std::endl;
    
    // Create thread pool with default thread count
    tp::ThreadPool pool;
    std::cout << "Thread pool size: " << pool.size() << std::endl;
    
    // Warm up
    std::cout << "\nWarming up..." << std::endl;
    for (int i = 0; i < 1000; ++i) {
        pool.submit([] {}).wait();
    }
    
    // Run benchmarks
    std::vector<BenchmarkResult> results;
    
    std::cout << "\nRunning benchmarks..." << std::endl;
    
    results.push_back(benchmark_empty_tasks(pool, 100000));
    results.push_back(benchmark_light_compute(pool, 100000));
    results.push_back(benchmark_heavy_compute(pool, 10000));
    results.push_back(benchmark_memory_alloc(pool, 100000));
    results.push_back(benchmark_mixed_workload(pool, 50000));
    results.push_back(benchmark_priority_tasks(pool, 100000));
    
    // Print results
    print_results(results);
    
    // Pool stats
    auto stats = pool.stats();
    std::cout << "\n=== Pool Statistics ===" << std::endl;
    std::cout << "Total tasks submitted: " << stats.total_tasks_submitted << std::endl;
    std::cout << "Total tasks completed: " << stats.total_tasks_completed << std::endl;
    std::cout << "Total tasks stolen: " << stats.total_tasks_stolen << std::endl;
    
    // Scaling benchmark
    benchmark_scaling();
    
    std::cout << "\n=== Benchmarks Complete ===" << std::endl;
    
    return 0;
}
