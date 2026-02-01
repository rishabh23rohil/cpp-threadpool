# cpp-threadpool

[![C++17](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](https://en.cppreference.com/w/cpp/17)
[![CMake](https://img.shields.io/badge/CMake-3.16+-green.svg)](https://cmake.org/)
[![License](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![Header Only](https://img.shields.io/badge/Header-Only-orange.svg)]()

A modern, header-only C++17 thread pool library featuring work-stealing scheduling, typed futures, and lock-free task queues.

---

## Features

- **Header-only** - Just include and use, no linking required
- **Modern C++17** - Uses `std::invoke`, fold expressions, `if constexpr`
- **Typed Futures** - Get return values from async tasks via `std::future<T>`
- **Work Stealing** - Automatic load balancing across worker threads
- **Priority Scheduling** - Submit urgent vs background tasks
- **Graceful Shutdown** - Wait for all tasks or cancel pending work
- **Zero Dependencies** - Only standard library required

---

## Quick Start

```cpp
#include <threadpool/threadpool.hpp>
#include <iostream>

int main() {
    // Create pool with hardware concurrency
    tp::ThreadPool pool;
    
    // Submit a task and get future
    auto future = pool.submit([] {
        return 42;
    });
    
    std::cout << "Result: " << future.get() << std::endl;
    
    // Submit multiple tasks
    std::vector<std::future<int>> futures;
    for (int i = 0; i < 100; ++i) {
        futures.push_back(pool.submit([i] {
            return i * i;
        }));
    }
    
    // Collect results
    for (auto& f : futures) {
        std::cout << f.get() << " ";
    }
    
    return 0;
}
```

---

## Installation

### Option 1: Copy Headers

Copy the `include/threadpool` directory to your project.

### Option 2: CMake FetchContent

```cmake
include(FetchContent)
FetchContent_Declare(
    cpp-threadpool
    GIT_REPOSITORY https://github.com/rishabh23rohil/cpp-threadpool.git
    GIT_TAG main
)
FetchContent_MakeAvailable(cpp-threadpool)

target_link_libraries(your_target PRIVATE threadpool)
```

### Option 3: CMake Subdirectory

```cmake
add_subdirectory(cpp-threadpool)
target_link_libraries(your_target PRIVATE threadpool)
```

---

## API Reference

### ThreadPool

```cpp
namespace tp {

class ThreadPool {
public:
    // Constructors
    explicit ThreadPool(size_t num_threads = std::thread::hardware_concurrency());
    ~ThreadPool();
    
    // Non-copyable, non-movable
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;
    
    // Submit a callable and get a future for the result
    template<typename F, typename... Args>
    auto submit(F&& f, Args&&... args) -> std::future<std::invoke_result_t<F, Args...>>;
    
    // Submit with priority (lower = higher priority)
    template<typename F, typename... Args>
    auto submit_priority(int priority, F&& f, Args&&... args) 
        -> std::future<std::invoke_result_t<F, Args...>>;
    
    // Pool management
    size_t size() const noexcept;           // Number of worker threads
    size_t pending() const noexcept;        // Tasks waiting in queue
    size_t active() const noexcept;         // Currently executing tasks
    
    void wait();                            // Wait for all tasks to complete
    void shutdown();                        // Stop accepting new tasks
    void shutdown_now();                    // Cancel pending tasks
};

} // namespace tp
```

---

## Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                        ThreadPool                            │
├─────────────────────────────────────────────────────────────┤
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐         │
│  │  Worker 1   │  │  Worker 2   │  │  Worker N   │         │
│  │ ┌─────────┐ │  │ ┌─────────┐ │  │ ┌─────────┐ │         │
│  │ │Local Q  │ │  │ │Local Q  │ │  │ │Local Q  │ │         │
│  │ └─────────┘ │  │ └─────────┘ │  │ └─────────┘ │         │
│  └──────┬──────┘  └──────┬──────┘  └──────┬──────┘         │
│         │                │                │                 │
│         └────────────────┼────────────────┘                 │
│                    Work Stealing                            │
│  ┌─────────────────────────────────────────────────────┐   │
│  │                  Global Task Queue                   │   │
│  │    [Task] [Task] [Task] [Task] [Task] [Task]        │   │
│  └─────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────┘
```

---

## Performance

Benchmarked on Apple M1 Pro (8 cores):

| Benchmark | Tasks | Time | Throughput |
|-----------|-------|------|------------|
| Empty tasks | 1M | 0.48s | 2.08M tasks/sec |
| Light compute | 1M | 1.2s | 833K tasks/sec |
| Heavy compute | 100K | 2.1s | 47.6K tasks/sec |

---

## Examples

### Parallel Map

```cpp
template<typename Container, typename Func>
auto parallel_map(tp::ThreadPool& pool, const Container& input, Func&& func) {
    using ResultType = std::invoke_result_t<Func, typename Container::value_type>;
    std::vector<std::future<ResultType>> futures;
    
    for (const auto& item : input) {
        futures.push_back(pool.submit(func, item));
    }
    
    std::vector<ResultType> results;
    results.reserve(futures.size());
    for (auto& f : futures) {
        results.push_back(f.get());
    }
    return results;
}
```

### Parallel For

```cpp
void parallel_for(tp::ThreadPool& pool, size_t start, size_t end, 
                  std::function<void(size_t)> func) {
    std::vector<std::future<void>> futures;
    for (size_t i = start; i < end; ++i) {
        futures.push_back(pool.submit(func, i));
    }
    for (auto& f : futures) {
        f.wait();
    }
}
```

---

## Building

```bash
mkdir build && cd build
cmake ..
cmake --build .

# Run tests
ctest --output-on-failure

# Run benchmarks
./benchmarks/benchmark
```

---

## Requirements

- C++17 compatible compiler (GCC 7+, Clang 5+, MSVC 2017+)
- CMake 3.16+ (for building tests/examples)

---

## License

MIT License - see [LICENSE](LICENSE) for details.

---

## Author

**Rishabh Rohil**

[![GitHub](https://img.shields.io/badge/GitHub-rishabh23rohil-black?style=flat&logo=github)](https://github.com/rishabh23rohil)
