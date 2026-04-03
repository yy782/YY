#include <iostream>
#include <thread>
#include <vector>
#include <atomic>
#include <random>
#include <chrono>
#include <cassert>
#include <set>

#include "../../Common/LockFreeCurcularQueue.h"
#include <algorithm>
using namespace yy;
//./CurcularQueueTest

#include <iostream>
#include <thread>
#include <vector>
#include <atomic>
#include <random>
#include <chrono>
#include <cassert>
#include <iomanip>
#include <memory>

// 假设你的 LockFreeCurcularQueue 类在这里定义
// ... 你的代码 ...

// 高并发测试：多生产者多消费者
void testHighConcurrency() {
    std::cout << "========== High Concurrency Test ==========" << std::endl;
    
    const size_t queue_size = 4096;
    const int producer_count = 8;
    const int consumer_count = 8;
    const int items_per_producer = 100000;
    const int total_items = producer_count * items_per_producer;
    
    LockFreeCurcularQueue<int> queue(queue_size);
    std::atomic<int> produced_count{0};
    std::atomic<int> consumed_count{0};
    std::atomic<bool> stop_consumer{false};
    
    // 用于验证数据完整性的数组
    std::unique_ptr<std::atomic<int>[]> received_count(new std::atomic<int>[total_items]);
    for (int i = 0; i < total_items; ++i) {
        received_count[i] = 0;
    }
    
    // 生产者线程
    auto producer = [&](int producer_id) {
        int base = producer_id * items_per_producer;
        for (int i = 0; i < items_per_producer; ++i) {
            int value = base + i;
            while (!queue.enqueue(value)) {
                // 队列满时自旋
            }
            produced_count++;
        }
    };
    
    // 消费者线程
    auto consumer = [&]() {
        int value;
        while (!stop_consumer.load() || consumed_count < total_items) {
            if (queue.dequeue(value)) {
                if (value >= 0 && value < total_items) {
                    received_count[value]++;
                    consumed_count++;
                } else {
                    std::cout << "ERROR: Invalid value: " << value << std::endl;
                }
            } else if (!stop_consumer.load()) {
                std::this_thread::yield();
            }
        }
    };
    
    auto start = std::chrono::high_resolution_clock::now();
    
    // 启动消费者线程
    std::vector<std::thread> consumers;
    for (int i = 0; i < consumer_count; ++i) {
        consumers.emplace_back(consumer);
    }
    
    // 启动生产者线程
    std::vector<std::thread> producers;
    for (int i = 0; i < producer_count; ++i) {
        producers.emplace_back(producer, i);
    }
    
    // 等待所有生产者完成
    for (auto& t : producers) {
        t.join();
    }
    
    // 通知消费者可以停止
    stop_consumer = true;
    
    // 等待所有消费者完成
    for (auto& t : consumers) {
        t.join();
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // 验证结果
    std::cout << "Produced: " << produced_count.load() << std::endl;
    std::cout << "Consumed: " << consumed_count.load() << std::endl;
    std::cout << "Time: " << duration.count() << " ms" << std::endl;
    
    double throughput = static_cast<double>(total_items) * 1000.0 / static_cast<double>(duration.count());
    std::cout << "Throughput: " << std::fixed << std::setprecision(2) << throughput << " items/sec" << std::endl;
    
    // 验证数据完整性
    int missing = 0;
    int duplicated = 0;
    for (int i = 0; i < total_items; ++i) {
        int count = received_count[i].load();
        if (count == 0) {
            missing++;
        } else if (count > 1) {
            duplicated += (count - 1);
        }
    }
    
    std::cout << "Missing items: " << missing << std::endl;
    std::cout << "Duplicated items: " << duplicated << std::endl;
    
    assert(produced_count == total_items);
    assert(consumed_count == total_items);
    assert(missing == 0);
    assert(duplicated == 0);
    
    std::cout << "✓ High concurrency test passed!" << std::endl;
}

// 极限压力测试
void testExtremePressure() {
    std::cout << "\n========== Extreme Pressure Test ==========" << std::endl;
    
    const size_t queue_size = 1024;
    const int producer_count = 16;
    const int consumer_count = 16;
    const int items_per_producer = 50000;
    const int total_items = producer_count * items_per_producer;
    
    LockFreeCurcularQueue<int> queue(queue_size);
    std::atomic<int> produced_count{0};
    std::atomic<int> consumed_count{0};
    
    // 使用简单的计数器验证
    std::unique_ptr<std::atomic<int>[]> counter(new std::atomic<int>[total_items]);
    for (int i = 0; i < total_items; ++i) {
        counter[i] = 0;
    }
    
    auto producer = [&](int id) {
        int start = id * items_per_producer;
        for (int i = 0; i < items_per_producer; ++i) {
            int val = start + i;
            while (!queue.enqueue(val)) {
                // 忙等待
            }
            produced_count++;
        }
    };
    
    auto consumer = [&]() {
        int val;
        while (consumed_count < total_items) {
            if (queue.dequeue(val)) {
                if (val >= 0 && val < total_items) {
                    counter[val]++;
                    consumed_count++;
                }
            } else {
                std::this_thread::yield();
            }
        }
    };
    
    auto start = std::chrono::high_resolution_clock::now();
    
    std::vector<std::thread> producers, consumers;
    for (int i = 0; i < producer_count; ++i) {
        producers.emplace_back(producer, i);
    }
    for (int i = 0; i < consumer_count; ++i) {
        consumers.emplace_back(consumer);
    }
    
    for (auto& t : producers) t.join();
    for (auto& t : consumers) t.join();
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // 验证
    int errors = 0;
    for (int i = 0; i < total_items; ++i) {
        int c = counter[i].load();
        if (c != 1) {
            errors++;
            if (c == 0) {
                std::cout << "Missing item: " << i << std::endl;
            } else if (c > 1) {
                std::cout << "Duplicate item: " << i << " count=" << c << std::endl;
            }
        }
    }
    
    double throughput = static_cast<double>(total_items) * 1000.0 / static_cast<double>(duration.count());
    
    std::cout << "Produced: " << produced_count.load() << std::endl;
    std::cout << "Consumed: " << consumed_count.load() << std::endl;
    std::cout << "Time: " << duration.count() << " ms" << std::endl;
    std::cout << "Errors (missing/duplicate): " << errors << std::endl;
    std::cout << "Throughput: " << std::fixed << std::setprecision(2) << throughput << " items/sec" << std::endl;
    
    assert(produced_count == total_items);
    assert(consumed_count == total_items);
    assert(errors == 0);
    
    std::cout << "✓ Extreme pressure test passed!" << std::endl;
}

// 长时间稳定性测试
void testLongRunningStability() {
    std::cout << "\n========== Long Running Stability Test ==========" << std::endl;
    
    const size_t queue_size = 2048;
    const int thread_count = 8;
    const int duration_seconds = 5;
    
    LockFreeCurcularQueue<int> queue(queue_size);
    std::atomic<bool> stop{false};
    std::atomic<long long> total_operations{0};
    std::atomic<int> enqueue_success{0};
    std::atomic<int> dequeue_success{0};
    
    // 混合工作线程（既生产又消费）
    auto worker = [&](int) {  // 忽略未使用的参数
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> op_dis(0, 1);
        std::uniform_int_distribution<> value_dis(1, 10000);
        
        while (!stop.load()) {
            if (op_dis(gen) == 0) {
                // 入队操作
                if (queue.enqueue(value_dis(gen))) {
                    enqueue_success++;
                }
            } else {
                // 出队操作
                int val;
                if (queue.dequeue(val)) {
                    dequeue_success++;
                }
            }
            total_operations++;
        }
    };
    
    std::vector<std::thread> threads;
    for (int i = 0; i < thread_count; ++i) {
        threads.emplace_back(worker, i);
    }
    
    std::cout << "Running for " << duration_seconds << " seconds..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(duration_seconds));
    stop = true;
    
    for (auto& t : threads) {
        t.join();
    }
    
    std::cout << "Total operations: " << total_operations.load() << std::endl;
    std::cout << "Enqueue success: " << enqueue_success.load() << std::endl;
    std::cout << "Dequeue success: " << dequeue_success.load() << std::endl;
    std::cout << "Queue final size: " << (enqueue_success.load() - dequeue_success.load()) << std::endl;
    
    // 验证队列状态一致性
    int final_size = 0;
    int val;
    while (queue.dequeue(val)) {
        final_size++;
    }
    
    std::cout << "Actual queue size: " << final_size << std::endl;
    assert(final_size == (enqueue_success.load() - dequeue_success.load()));
    
    std::cout << "✓ Long running stability test passed!" << std::endl;
}

int main() {
    std::cout << "Lock-Free Circular Queue Thread Safety Tests\n";
    std::cout << "============================================\n";
    
    try {
        testHighConcurrency();
        testExtremePressure();
        testLongRunningStability();
        
        std::cout << "\n============================================" << std::endl;
        std::cout << "All high concurrency tests passed!" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Test failed: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}