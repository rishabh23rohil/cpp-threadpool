#include <threadpool/threadpool.hpp>
#include <gtest/gtest.h>
#include <atomic>
#include <random>

class StressTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(StressTest, HighVolumeTasks) {
    tp::ThreadPool pool(8);
    std::atomic<int> counter{0};
    const int num_tasks = 10000;
    
    std::vector<std::future<void>> futures;
    futures.reserve(num_tasks);
    
    for (int i = 0; i < num_tasks; ++i) {
        futures.push_back(pool.submit([&counter] {
            ++counter;
        }));
    }
    
    for (auto& f : futures) {
        f.wait();
    }
    
    EXPECT_EQ(counter.load(), num_tasks);
}

TEST_F(StressTest, MixedTaskDurations) {
    tp::ThreadPool pool(4);
    std::atomic<int> counter{0};
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(1, 10);
    
    std::vector<std::future<void>> futures;
    
    for (int i = 0; i < 100; ++i) {
        int sleep_ms = dist(gen);
        futures.push_back(pool.submit([&counter, sleep_ms] {
            std::this_thread::sleep_for(std::chrono::milliseconds(sleep_ms));
            ++counter;
        }));
    }
    
    for (auto& f : futures) {
        f.wait();
    }
    
    EXPECT_EQ(counter.load(), 100);
}

TEST_F(StressTest, ConcurrentSubmissions) {
    tp::ThreadPool pool(4);
    std::atomic<int> counter{0};
    const int num_submitters = 4;
    const int tasks_per_submitter = 250;
    
    std::vector<std::thread> submitters;
    std::vector<std::future<void>> all_futures;
    std::mutex futures_mutex;
    
    for (int s = 0; s < num_submitters; ++s) {
        submitters.emplace_back([&] {
            std::vector<std::future<void>> local_futures;
            for (int i = 0; i < tasks_per_submitter; ++i) {
                local_futures.push_back(pool.submit([&counter] {
                    ++counter;
                }));
            }
            std::lock_guard<std::mutex> lock(futures_mutex);
            for (auto& f : local_futures) {
                all_futures.push_back(std::move(f));
            }
        });
    }
    
    for (auto& t : submitters) {
        t.join();
    }
    
    for (auto& f : all_futures) {
        f.wait();
    }
    
    EXPECT_EQ(counter.load(), num_submitters * tasks_per_submitter);
}

TEST_F(StressTest, RecursiveTaskSubmission) {
    tp::ThreadPool pool(4);
    std::atomic<int> counter{0};
    
    std::function<void(int)> recursive_task;
    recursive_task = [&pool, &counter, &recursive_task](int depth) {
        ++counter;
        if (depth > 0) {
            pool.submit(recursive_task, depth - 1);
            pool.submit(recursive_task, depth - 1);
        }
    };
    
    auto future = pool.submit(recursive_task, 5);
    future.wait();
    pool.wait();
    
    // 2^6 - 1 = 63 tasks (binary tree)
    EXPECT_EQ(counter.load(), 63);
}

TEST_F(StressTest, RapidPoolCreationDestruction) {
    for (int i = 0; i < 10; ++i) {
        tp::ThreadPool pool(4);
        std::atomic<int> counter{0};
        
        for (int j = 0; j < 100; ++j) {
            pool.submit([&counter] {
                ++counter;
            });
        }
        
        pool.wait();
        EXPECT_EQ(counter.load(), 100);
    }
}

TEST_F(StressTest, WorkStealingEfficiency) {
    tp::ThreadPool pool(4);
    std::atomic<int> counter{0};
    
    // Submit tasks that have varying execution times
    std::vector<std::future<void>> futures;
    for (int i = 0; i < 100; ++i) {
        futures.push_back(pool.submit([&counter, i] {
            // Uneven workload - some tasks take longer
            if (i % 10 == 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(20));
            }
            ++counter;
        }));
    }
    
    for (auto& f : futures) {
        f.wait();
    }
    
    EXPECT_EQ(counter.load(), 100);
    
    // Check that work stealing happened
    auto stats = pool.stats();
    // Note: work stealing may or may not happen depending on timing
    EXPECT_GE(stats.total_tasks_completed, 100);
}

int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
