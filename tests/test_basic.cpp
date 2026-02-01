#include <threadpool/threadpool.hpp>
#include <gtest/gtest.h>
#include <atomic>
#include <vector>

class ThreadPoolTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(ThreadPoolTest, Construction) {
    tp::ThreadPool pool(4);
    EXPECT_EQ(pool.size(), 4);
    EXPECT_EQ(pool.pending(), 0);
    EXPECT_EQ(pool.active(), 0);
}

TEST_F(ThreadPoolTest, DefaultConstruction) {
    tp::ThreadPool pool;
    EXPECT_GT(pool.size(), 0);
}

TEST_F(ThreadPoolTest, SubmitVoidTask) {
    tp::ThreadPool pool(2);
    std::atomic<bool> executed{false};
    
    auto future = pool.submit([&executed] {
        executed = true;
    });
    
    future.wait();
    EXPECT_TRUE(executed);
}

TEST_F(ThreadPoolTest, SubmitTaskWithReturn) {
    tp::ThreadPool pool(2);
    
    auto future = pool.submit([] {
        return 42;
    });
    
    EXPECT_EQ(future.get(), 42);
}

TEST_F(ThreadPoolTest, SubmitTaskWithArguments) {
    tp::ThreadPool pool(2);
    
    auto future = pool.submit([](int a, int b) {
        return a + b;
    }, 10, 20);
    
    EXPECT_EQ(future.get(), 30);
}

TEST_F(ThreadPoolTest, MultipleTasksExecute) {
    tp::ThreadPool pool(4);
    std::atomic<int> counter{0};
    
    std::vector<std::future<void>> futures;
    for (int i = 0; i < 100; ++i) {
        futures.push_back(pool.submit([&counter] {
            ++counter;
        }));
    }
    
    for (auto& f : futures) {
        f.wait();
    }
    
    EXPECT_EQ(counter.load(), 100);
}

TEST_F(ThreadPoolTest, TasksRunInParallel) {
    tp::ThreadPool pool(4);
    std::atomic<int> concurrent{0};
    std::atomic<int> max_concurrent{0};
    
    std::vector<std::future<void>> futures;
    for (int i = 0; i < 8; ++i) {
        futures.push_back(pool.submit([&concurrent, &max_concurrent] {
            int current = ++concurrent;
            int expected = max_concurrent.load();
            while (current > expected && 
                   !max_concurrent.compare_exchange_weak(expected, current)) {
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            --concurrent;
        }));
    }
    
    for (auto& f : futures) {
        f.wait();
    }
    
    EXPECT_GT(max_concurrent.load(), 1);
}

TEST_F(ThreadPoolTest, WaitForTasks) {
    tp::ThreadPool pool(2);
    std::atomic<int> counter{0};
    
    for (int i = 0; i < 10; ++i) {
        pool.submit([&counter] {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            ++counter;
        });
    }
    
    pool.wait();
    EXPECT_EQ(counter.load(), 10);
}

TEST_F(ThreadPoolTest, ShutdownStopsAcceptingTasks) {
    tp::ThreadPool pool(2);
    
    pool.shutdown();
    
    EXPECT_THROW(pool.submit([] {}), std::runtime_error);
}

TEST_F(ThreadPoolTest, PriorityTasksExecuteFirst) {
    tp::ThreadPool pool(1);  // Single thread for deterministic order
    std::vector<int> execution_order;
    std::mutex order_mutex;
    
    // Block the worker
    std::promise<void> blocker;
    pool.submit([&blocker] {
        blocker.get_future().wait();
    });
    
    // Submit tasks with different priorities
    for (int i = 0; i < 5; ++i) {
        pool.submit_priority(10 - i, [i, &execution_order, &order_mutex] {
            std::lock_guard<std::mutex> lock(order_mutex);
            execution_order.push_back(i);
        });
    }
    
    // Unblock
    blocker.set_value();
    pool.wait();
    
    // Higher priority (lower value) should execute first
    ASSERT_EQ(execution_order.size(), 5);
    EXPECT_EQ(execution_order[0], 4);  // Priority 6
    EXPECT_EQ(execution_order[4], 0);  // Priority 10
}

int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
