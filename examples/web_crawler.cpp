/**
 * @file web_crawler.cpp
 * @brief Simulated web crawler using cpp-threadpool
 * 
 * This example demonstrates:
 * - Producer-consumer pattern
 * - Dynamic task submission
 * - Task with varying durations
 */

#include <threadpool/threadpool.hpp>
#include <iostream>
#include <vector>
#include <string>
#include <set>
#include <mutex>
#include <random>
#include <chrono>

// Simulated URL database
class URLDatabase {
public:
    URLDatabase() {
        // Seed some initial URLs
        urls_ = {
            "https://example.com",
            "https://example.com/page1",
            "https://example.com/page2",
            "https://example.com/about",
            "https://example.com/contact",
            "https://blog.example.com",
            "https://blog.example.com/post1",
            "https://blog.example.com/post2",
            "https://shop.example.com",
            "https://shop.example.com/products"
        };
        
        // Generate more URLs
        for (int i = 0; i < 50; ++i) {
            urls_.push_back("https://example.com/page" + std::to_string(i));
        }
    }
    
    // Get random links from a "page"
    std::vector<std::string> get_links(const std::string& url) {
        std::vector<std::string> links;
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> count_dist(0, 3);
        std::uniform_int_distribution<> url_dist(0, static_cast<int>(urls_.size() - 1));
        
        int num_links = count_dist(gen);
        for (int i = 0; i < num_links; ++i) {
            links.push_back(urls_[url_dist(gen)]);
        }
        
        return links;
    }
    
private:
    std::vector<std::string> urls_;
};

// Web crawler
class WebCrawler {
public:
    WebCrawler(tp::ThreadPool& pool, int max_depth = 3)
        : pool_(pool)
        , max_depth_(max_depth)
    {}
    
    void start(const std::string& seed_url) {
        std::cout << "Starting crawl from: " << seed_url << std::endl;
        std::cout << "Max depth: " << max_depth_ << std::endl;
        std::cout << std::endl;
        
        crawl(seed_url, 0);
    }
    
    void wait_complete() {
        pool_.wait();
    }
    
    void print_stats() {
        std::lock_guard<std::mutex> lock(mutex_);
        std::cout << "\n=== Crawl Statistics ===" << std::endl;
        std::cout << "URLs visited: " << visited_.size() << std::endl;
        std::cout << "Total tasks submitted: " << tasks_submitted_ << std::endl;
    }
    
private:
    void crawl(const std::string& url, int depth) {
        // Check if already visited
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (visited_.count(url) > 0 || depth > max_depth_) {
                return;
            }
            visited_.insert(url);
            ++tasks_submitted_;
        }
        
        // Submit crawl task
        pool_.submit([this, url, depth] {
            // Simulate network delay
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> delay_dist(10, 100);
            std::this_thread::sleep_for(std::chrono::milliseconds(delay_dist(gen)));
            
            // Log progress
            {
                std::lock_guard<std::mutex> lock(mutex_);
                std::cout << "[Depth " << depth << "] Crawled: " << url << std::endl;
            }
            
            // Get links and crawl them
            auto links = db_.get_links(url);
            for (const auto& link : links) {
                crawl(link, depth + 1);
            }
        });
    }
    
private:
    tp::ThreadPool& pool_;
    URLDatabase db_;
    int max_depth_;
    
    std::mutex mutex_;
    std::set<std::string> visited_;
    int tasks_submitted_ = 0;
};

int main() {
    std::cout << "=== Simulated Web Crawler Demo ===" << std::endl;
    std::cout << std::endl;
    
    // Create thread pool
    tp::ThreadPool pool(4);
    std::cout << "Thread pool created with " << pool.size() << " workers" << std::endl;
    std::cout << std::endl;
    
    // Create and run crawler
    WebCrawler crawler(pool, 2);
    
    auto start = std::chrono::high_resolution_clock::now();
    
    crawler.start("https://example.com");
    crawler.wait_complete();
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    crawler.print_stats();
    std::cout << "Total time: " << duration.count() << " ms" << std::endl;
    
    // Pool stats
    auto stats = pool.stats();
    std::cout << "\n=== Pool Statistics ===" << std::endl;
    std::cout << "Tasks completed: " << stats.total_tasks_completed << std::endl;
    std::cout << "Tasks stolen: " << stats.total_tasks_stolen << std::endl;
    
    return 0;
}
