#include <threadpool/threadpool.hpp>
#include <gtest/gtest.h>
#include <string>
#include <exception>

class FuturesTest : public ::testing::Test {
protected:
    tp::ThreadPool pool{4};
};

TEST_F(FuturesTest, FutureReturnsInt) {
    auto future = pool.submit([] {
        return 42;
    });
    
    EXPECT_EQ(future.get(), 42);
}

TEST_F(FuturesTest, FutureReturnsString) {
    auto future = pool.submit([] {
        return std::string("Hello, ThreadPool!");
    });
    
    EXPECT_EQ(future.get(), "Hello, ThreadPool!");
}

TEST_F(FuturesTest, FutureReturnsVector) {
    auto future = pool.submit([] {
        return std::vector<int>{1, 2, 3, 4, 5};
    });
    
    auto result = future.get();
    EXPECT_EQ(result.size(), 5);
    EXPECT_EQ(result[0], 1);
    EXPECT_EQ(result[4], 5);
}

TEST_F(FuturesTest, FutureWithArguments) {
    auto future = pool.submit([](int a, int b, int c) {
        return a * b + c;
    }, 3, 4, 5);
    
    EXPECT_EQ(future.get(), 17);
}

TEST_F(FuturesTest, FutureWithStringArguments) {
    auto future = pool.submit([](const std::string& prefix, int num) {
        return prefix + std::to_string(num);
    }, std::string("Value: "), 42);
    
    EXPECT_EQ(future.get(), "Value: 42");
}

TEST_F(FuturesTest, FuturePropagatesException) {
    auto future = pool.submit([] {
        throw std::runtime_error("Test exception");
        return 0;
    });
    
    EXPECT_THROW(future.get(), std::runtime_error);
}

TEST_F(FuturesTest, MultipleFuturesComplete) {
    std::vector<std::future<int>> futures;
    
    for (int i = 0; i < 10; ++i) {
        futures.push_back(pool.submit([i] {
            return i * i;
        }));
    }
    
    for (int i = 0; i < 10; ++i) {
        EXPECT_EQ(futures[i].get(), i * i);
    }
}

TEST_F(FuturesTest, FutureChaining) {
    auto future1 = pool.submit([] {
        return 10;
    });
    
    int value1 = future1.get();
    
    auto future2 = pool.submit([value1] {
        return value1 * 2;
    });
    
    EXPECT_EQ(future2.get(), 20);
}

TEST_F(FuturesTest, VoidFutureCompletes) {
    std::atomic<bool> executed{false};
    
    auto future = pool.submit([&executed] {
        executed = true;
    });
    
    future.wait();
    EXPECT_TRUE(executed);
}

TEST_F(FuturesTest, FutureWaitFor) {
    auto future = pool.submit([] {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        return 42;
    });
    
    // Should timeout
    auto status = future.wait_for(std::chrono::milliseconds(10));
    EXPECT_EQ(status, std::future_status::timeout);
    
    // Should complete
    status = future.wait_for(std::chrono::milliseconds(100));
    EXPECT_EQ(status, std::future_status::ready);
    
    EXPECT_EQ(future.get(), 42);
}

int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
