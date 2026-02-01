/**
 * @file parallel_sort.cpp
 * @brief Parallel merge sort using cpp-threadpool
 */

#include <threadpool/threadpool.hpp>
#include <iostream>
#include <vector>
#include <algorithm>
#include <random>
#include <chrono>

/**
 * @brief Merge two sorted halves
 */
template<typename T>
void merge(std::vector<T>& arr, size_t left, size_t mid, size_t right) {
    std::vector<T> temp(right - left + 1);
    size_t i = left, j = mid + 1, k = 0;
    
    while (i <= mid && j <= right) {
        if (arr[i] <= arr[j]) {
            temp[k++] = arr[i++];
        } else {
            temp[k++] = arr[j++];
        }
    }
    
    while (i <= mid) {
        temp[k++] = arr[i++];
    }
    
    while (j <= right) {
        temp[k++] = arr[j++];
    }
    
    for (size_t i = 0; i < temp.size(); ++i) {
        arr[left + i] = temp[i];
    }
}

/**
 * @brief Sequential merge sort
 */
template<typename T>
void sequential_merge_sort(std::vector<T>& arr, size_t left, size_t right) {
    if (left >= right) return;
    
    size_t mid = left + (right - left) / 2;
    sequential_merge_sort(arr, left, mid);
    sequential_merge_sort(arr, mid + 1, right);
    merge(arr, left, mid, right);
}

/**
 * @brief Parallel merge sort using thread pool
 */
template<typename T>
void parallel_merge_sort(tp::ThreadPool& pool, std::vector<T>& arr, 
                         size_t left, size_t right, size_t threshold = 10000) {
    if (left >= right) return;
    
    // Use sequential sort for small subarrays
    if (right - left < threshold) {
        sequential_merge_sort(arr, left, right);
        return;
    }
    
    size_t mid = left + (right - left) / 2;
    
    // Sort halves in parallel
    auto future_left = pool.submit([&arr, &pool, left, mid, threshold] {
        parallel_merge_sort(pool, arr, left, mid, threshold);
    });
    
    auto future_right = pool.submit([&arr, &pool, mid, right, threshold] {
        parallel_merge_sort(pool, arr, mid + 1, right, threshold);
    });
    
    future_left.wait();
    future_right.wait();
    
    merge(arr, left, mid, right);
}

/**
 * @brief Generate random vector
 */
std::vector<int> generate_random_vector(size_t size) {
    std::vector<int> vec(size);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(0, 1000000);
    
    for (auto& v : vec) {
        v = dist(gen);
    }
    
    return vec;
}

/**
 * @brief Check if vector is sorted
 */
template<typename T>
bool is_sorted(const std::vector<T>& vec) {
    for (size_t i = 1; i < vec.size(); ++i) {
        if (vec[i] < vec[i-1]) return false;
    }
    return true;
}

int main() {
    std::cout << "=== Parallel Merge Sort Demo ===" << std::endl;
    std::cout << std::endl;
    
    const size_t SIZE = 1000000;
    
    std::cout << "Generating " << SIZE << " random integers..." << std::endl;
    auto data_seq = generate_random_vector(SIZE);
    auto data_par = data_seq;  // Copy for parallel sort
    auto data_std = data_seq;  // Copy for std::sort
    
    // Sequential merge sort
    std::cout << "\n1. Sequential merge sort..." << std::endl;
    auto start = std::chrono::high_resolution_clock::now();
    sequential_merge_sort(data_seq, 0, data_seq.size() - 1);
    auto end = std::chrono::high_resolution_clock::now();
    auto seq_time = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "   Time: " << seq_time.count() << " ms" << std::endl;
    std::cout << "   Sorted: " << (is_sorted(data_seq) ? "Yes" : "No") << std::endl;
    
    // Parallel merge sort
    std::cout << "\n2. Parallel merge sort..." << std::endl;
    tp::ThreadPool pool;
    std::cout << "   Using " << pool.size() << " threads" << std::endl;
    
    start = std::chrono::high_resolution_clock::now();
    parallel_merge_sort(pool, data_par, 0, data_par.size() - 1);
    end = std::chrono::high_resolution_clock::now();
    auto par_time = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "   Time: " << par_time.count() << " ms" << std::endl;
    std::cout << "   Sorted: " << (is_sorted(data_par) ? "Yes" : "No") << std::endl;
    
    // std::sort for comparison
    std::cout << "\n3. std::sort (baseline)..." << std::endl;
    start = std::chrono::high_resolution_clock::now();
    std::sort(data_std.begin(), data_std.end());
    end = std::chrono::high_resolution_clock::now();
    auto std_time = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "   Time: " << std_time.count() << " ms" << std::endl;
    
    // Summary
    std::cout << "\n=== Summary ===" << std::endl;
    std::cout << "Sequential: " << seq_time.count() << " ms" << std::endl;
    std::cout << "Parallel:   " << par_time.count() << " ms" << std::endl;
    std::cout << "std::sort:  " << std_time.count() << " ms" << std::endl;
    
    if (seq_time.count() > 0) {
        double speedup = static_cast<double>(seq_time.count()) / par_time.count();
        std::cout << "\nParallel speedup: " << speedup << "x" << std::endl;
    }
    
    return 0;
}
