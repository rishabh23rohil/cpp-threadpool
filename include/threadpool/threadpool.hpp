#pragma once

/**
 * @file threadpool.hpp
 * @brief Modern C++17 Thread Pool Library
 * @author Rishabh Rohil
 * @version 1.0.0
 * 
 * A header-only thread pool implementation featuring:
 * - Work-stealing scheduling
 * - Typed futures for return values
 * - Priority task scheduling
 * - Graceful shutdown
 */

#include <thread>
#include <vector>
#include <queue>
#include <functional>
#include <future>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <memory>
#include <type_traits>
#include <optional>
#include <chrono>

namespace tp {

/**
 * @brief Task wrapper with priority support
 */
class Task {
public:
    Task() = default;
    
    template<typename F>
    explicit Task(F&& func, int priority = 0)
        : func_(std::forward<F>(func))
        , priority_(priority) 
    {}
    
    void operator()() {
        if (func_) {
            func_();
        }
    }
    
    int priority() const noexcept { return priority_; }
    
    explicit operator bool() const noexcept { return static_cast<bool>(func_); }
    
    // Comparison for priority queue (lower priority value = higher priority)
    bool operator<(const Task& other) const noexcept {
        return priority_ > other.priority_; // Reversed for max-heap behavior
    }

private:
    std::function<void()> func_;
    int priority_ = 0;
};

/**
 * @brief Thread-safe task queue with priority support
 */
class TaskQueue {
public:
    TaskQueue() = default;
    
    // Non-copyable
    TaskQueue(const TaskQueue&) = delete;
    TaskQueue& operator=(const TaskQueue&) = delete;
    
    /**
     * @brief Push a task to the queue
     */
    void push(Task task) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            queue_.push(std::move(task));
        }
        cv_.notify_one();
    }
    
    /**
     * @brief Try to pop a task (non-blocking)
     */
    std::optional<Task> try_pop() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (queue_.empty()) {
            return std::nullopt;
        }
        Task task = std::move(const_cast<Task&>(queue_.top()));
        queue_.pop();
        return task;
    }
    
    /**
     * @brief Wait and pop a task (blocking)
     */
    std::optional<Task> wait_pop(std::atomic<bool>& stop_flag) {
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait(lock, [this, &stop_flag] {
            return !queue_.empty() || stop_flag.load(std::memory_order_acquire);
        });
        
        if (queue_.empty()) {
            return std::nullopt;
        }
        
        Task task = std::move(const_cast<Task&>(queue_.top()));
        queue_.pop();
        return task;
    }
    
    /**
     * @brief Get queue size
     */
    size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.size();
    }
    
    /**
     * @brief Check if queue is empty
     */
    bool empty() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.empty();
    }
    
    /**
     * @brief Wake up all waiting threads
     */
    void notify_all() {
        cv_.notify_all();
    }
    
    /**
     * @brief Clear all pending tasks
     */
    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        while (!queue_.empty()) {
            queue_.pop();
        }
    }

private:
    mutable std::mutex mutex_;
    std::condition_variable cv_;
    std::priority_queue<Task> queue_;
};

/**
 * @brief Work-stealing deque for per-thread task storage
 */
class WorkStealingDeque {
public:
    WorkStealingDeque() = default;
    
    /**
     * @brief Push task to front (owner thread)
     */
    void push_front(Task task) {
        std::lock_guard<std::mutex> lock(mutex_);
        deque_.push_front(std::move(task));
    }
    
    /**
     * @brief Pop from front (owner thread)
     */
    std::optional<Task> pop_front() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (deque_.empty()) {
            return std::nullopt;
        }
        Task task = std::move(deque_.front());
        deque_.pop_front();
        return task;
    }
    
    /**
     * @brief Steal from back (other threads)
     */
    std::optional<Task> steal() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (deque_.empty()) {
            return std::nullopt;
        }
        Task task = std::move(deque_.back());
        deque_.pop_back();
        return task;
    }
    
    size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return deque_.size();
    }
    
    bool empty() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return deque_.empty();
    }

private:
    mutable std::mutex mutex_;
    std::deque<Task> deque_;
};

/**
 * @brief Thread pool statistics
 */
struct PoolStats {
    size_t total_tasks_submitted = 0;
    size_t total_tasks_completed = 0;
    size_t total_tasks_stolen = 0;
    std::chrono::nanoseconds total_execution_time{0};
};

/**
 * @brief Modern C++17 Thread Pool with work-stealing
 * 
 * Features:
 * - Configurable number of worker threads
 * - Work-stealing for load balancing
 * - Priority task scheduling
 * - Typed futures for return values
 * - Graceful shutdown
 */
class ThreadPool {
public:
    /**
     * @brief Construct thread pool with specified number of threads
     * @param num_threads Number of worker threads (default: hardware concurrency)
     */
    explicit ThreadPool(size_t num_threads = std::thread::hardware_concurrency())
        : num_threads_(num_threads > 0 ? num_threads : 1)
        , stop_(false)
        , active_tasks_(0)
    {
        local_queues_.reserve(num_threads_);
        workers_.reserve(num_threads_);
        
        for (size_t i = 0; i < num_threads_; ++i) {
            local_queues_.push_back(std::make_unique<WorkStealingDeque>());
        }
        
        for (size_t i = 0; i < num_threads_; ++i) {
            workers_.emplace_back(&ThreadPool::worker_loop, this, i);
        }
    }
    
    /**
     * @brief Destructor - waits for all tasks to complete
     */
    ~ThreadPool() {
        shutdown();
        for (auto& worker : workers_) {
            if (worker.joinable()) {
                worker.join();
            }
        }
    }
    
    // Non-copyable, non-movable
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;
    ThreadPool(ThreadPool&&) = delete;
    ThreadPool& operator=(ThreadPool&&) = delete;
    
    /**
     * @brief Submit a task and get a future for the result
     * @param func Callable to execute
     * @param args Arguments to pass to the callable
     * @return std::future for the result
     */
    template<typename F, typename... Args>
    auto submit(F&& func, Args&&... args) 
        -> std::future<std::invoke_result_t<F, Args...>> 
    {
        return submit_priority(0, std::forward<F>(func), std::forward<Args>(args)...);
    }
    
    /**
     * @brief Submit a task with priority
     * @param priority Priority level (lower = higher priority)
     * @param func Callable to execute
     * @param args Arguments to pass to the callable
     * @return std::future for the result
     */
    template<typename F, typename... Args>
    auto submit_priority(int priority, F&& func, Args&&... args) 
        -> std::future<std::invoke_result_t<F, Args...>> 
    {
        using ReturnType = std::invoke_result_t<F, Args...>;
        
        if (stop_.load(std::memory_order_acquire)) {
            throw std::runtime_error("Cannot submit to stopped thread pool");
        }
        
        auto task_ptr = std::make_shared<std::packaged_task<ReturnType()>>(
            std::bind(std::forward<F>(func), std::forward<Args>(args)...)
        );
        
        std::future<ReturnType> result = task_ptr->get_future();
        
        Task task([task_ptr]() { (*task_ptr)(); }, priority);
        global_queue_.push(std::move(task));
        
        ++stats_.total_tasks_submitted;
        
        return result;
    }
    
    /**
     * @brief Get number of worker threads
     */
    size_t size() const noexcept {
        return num_threads_;
    }
    
    /**
     * @brief Get number of pending tasks
     */
    size_t pending() const noexcept {
        size_t count = global_queue_.size();
        for (const auto& q : local_queues_) {
            count += q->size();
        }
        return count;
    }
    
    /**
     * @brief Get number of actively executing tasks
     */
    size_t active() const noexcept {
        return active_tasks_.load(std::memory_order_relaxed);
    }
    
    /**
     * @brief Wait for all submitted tasks to complete
     */
    void wait() {
        while (pending() > 0 || active() > 0) {
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
    }
    
    /**
     * @brief Stop accepting new tasks and wait for completion
     */
    void shutdown() {
        stop_.store(true, std::memory_order_release);
        global_queue_.notify_all();
    }
    
    /**
     * @brief Stop and cancel all pending tasks
     */
    void shutdown_now() {
        stop_.store(true, std::memory_order_release);
        global_queue_.clear();
        for (auto& q : local_queues_) {
            while (q->pop_front().has_value()) {}
        }
        global_queue_.notify_all();
    }
    
    /**
     * @brief Get pool statistics
     */
    PoolStats stats() const {
        return stats_;
    }

private:
    /**
     * @brief Worker thread main loop
     */
    void worker_loop(size_t worker_id) {
        while (true) {
            std::optional<Task> task;
            
            // 1. Try local queue first
            task = local_queues_[worker_id]->pop_front();
            
            // 2. Try global queue
            if (!task) {
                task = global_queue_.try_pop();
            }
            
            // 3. Try stealing from other workers
            if (!task) {
                task = try_steal(worker_id);
            }
            
            // 4. Wait on global queue
            if (!task) {
                task = global_queue_.wait_pop(stop_);
            }
            
            if (!task) {
                if (stop_.load(std::memory_order_acquire)) {
                    break;
                }
                continue;
            }
            
            // Execute task
            ++active_tasks_;
            auto start = std::chrono::high_resolution_clock::now();
            
            (*task)();
            
            auto end = std::chrono::high_resolution_clock::now();
            stats_.total_execution_time += (end - start);
            ++stats_.total_tasks_completed;
            --active_tasks_;
        }
    }
    
    /**
     * @brief Try to steal a task from another worker
     */
    std::optional<Task> try_steal(size_t worker_id) {
        for (size_t i = 0; i < num_threads_; ++i) {
            size_t victim = (worker_id + i + 1) % num_threads_;
            if (victim == worker_id) continue;
            
            auto task = local_queues_[victim]->steal();
            if (task) {
                ++stats_.total_tasks_stolen;
                return task;
            }
        }
        return std::nullopt;
    }

private:
    size_t num_threads_;
    std::atomic<bool> stop_;
    std::atomic<size_t> active_tasks_;
    
    TaskQueue global_queue_;
    std::vector<std::unique_ptr<WorkStealingDeque>> local_queues_;
    std::vector<std::thread> workers_;
    
    mutable PoolStats stats_;
};

/**
 * @brief Parallel for loop utility
 */
template<typename Func>
void parallel_for(ThreadPool& pool, size_t start, size_t end, Func&& func) {
    std::vector<std::future<void>> futures;
    futures.reserve(end - start);
    
    for (size_t i = start; i < end; ++i) {
        futures.push_back(pool.submit(std::forward<Func>(func), i));
    }
    
    for (auto& f : futures) {
        f.wait();
    }
}

/**
 * @brief Parallel map utility
 */
template<typename Container, typename Func>
auto parallel_map(ThreadPool& pool, const Container& input, Func&& func) {
    using InputType = typename Container::value_type;
    using ResultType = std::invoke_result_t<Func, InputType>;
    
    std::vector<std::future<ResultType>> futures;
    futures.reserve(input.size());
    
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

} // namespace tp
