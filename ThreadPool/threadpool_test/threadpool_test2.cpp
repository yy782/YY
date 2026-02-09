// thread_pool_benchmark.cpp
#include <benchmark/benchmark.h>
#include <atomic>
#include <memory>
#include "../ThreadPool.h"
using namespace yy;
// 1. 简单任务吞吐量测试
static void BM_ThreadPool_SimpleTask(benchmark::State& state) {
    // 获取线程数作为参数
    int thread_count = state.range(0);
    auto& pool = ThreadPool<FSFC>::getInstance(thread_count, thread_count * 2);
    
    for (auto _ : state) {
        std::atomic<int> counter{0};
        int task_count = 10000;
        
        state.PauseTiming();  // 暂停计时（准备阶段不计时）
        // 准备任务
        state.ResumeTiming();  // 恢复计时
        
        // 提交任务
        for (int i = 0; i < task_count; ++i) {
            pool.submit([&counter]() {
                // 简单计算任务
                volatile int sum = 0;
                for (int j = 0; j < 100; ++j) {
                    sum += j * j;
                }
                counter.fetch_add(1, std::memory_order_relaxed);
            });
        }
        
        // 等待所有任务完成
        while (counter.load(std::memory_order_relaxed) < task_count) {
            // 空循环等待
        }
        
        state.SetItemsProcessed(task_count);  // 设置处理的项目数
    }
    
    // 设置复杂度
    state.SetComplexityN(state.range(0));  // 以线程数为复杂度
}

// 注册测试：测试不同线程数
BENCHMARK(BM_ThreadPool_SimpleTask)
    ->RangeMultiplier(2)            // 每次乘以2
    ->Range(1, 16)                  // 从1到16个线程
    ->Unit(benchmark::kMillisecond) // 时间单位：毫秒
    ->UseRealTime()                 // 使用真实时间（包括等待时间）
    ->Threads(1);                    // 用1个线程运行测试

// 2. 测试不同任务大小的性能
static void BM_ThreadPool_TaskSize(benchmark::State& state) {
    auto& pool = ThreadPool<FSFC>::getInstance(4, 8);
    int work_amount = state.range(0);  // 每个任务的工作量
    
    for (auto _ : state) {
        std::atomic<int> counter{0};
        const int task_count = 1000;
        
        for (int i = 0; i < task_count; ++i) {
            pool.submit([&counter, work_amount]() {
                // 根据work_amount决定工作量
                volatile double sum = 0;
                for (int j = 0; j < work_amount; ++j) {
                    sum += std::sqrt(j) * std::log(j + 1);
                }
                counter.fetch_add(1);
            });
        }
        
        while (counter.load() < task_count) {
            std::this_thread::yield();
        }
        
        state.SetItemsProcessed(task_count);
        state.SetComplexityN(work_amount);
    }
}

BENCHMARK(BM_ThreadPool_TaskSize)
    ->RangeMultiplier(10)
    ->Range(10, 1000000)  // 工作量从10到100万次计算
    ->Unit(benchmark::kMicrosecond)
    ->UseRealTime()
    ->Threads(1)
    ->Complexity(benchmark::oN);  // 期望是线性复杂度

// 3. 测试任务调度延迟
static void BM_ThreadPool_SchedulingLatency(benchmark::State& state) {
    auto& pool = ThreadPool<FSFC>::getInstance(state.range(0), state.range(0));
    std::vector<long long> latencies;
    
    for (auto _ : state) {
        auto submit_time = std::chrono::high_resolution_clock::now();
        
        auto fut = pool.submit([submit_time]() {
            auto start_time = std::chrono::high_resolution_clock::now();
            auto latency = std::chrono::duration_cast<std::chrono::nanoseconds>(
                start_time - submit_time);
            return latency.count();
        });
        
        long long latency = fut.get().value.value();
        state.SetIterationTime(latency / 1e9);  // 转换为秒
    }
    
    state.SetLabel(("Threads: " + std::to_string(state.range(0))).c_str());
}

BENCHMARK(BM_ThreadPool_SchedulingLatency)
    ->DenseRange(1, 8, 1)  // 从1到8，步长为1
    ->Unit(benchmark::kNanosecond)
    ->UseManualTime();

// 4. 多线程竞争测试
static void BM_ThreadPool_Contention(benchmark::State& state) {
    auto& pool = ThreadPool<FSFC>::getInstance(4, 4);
    std::atomic<int> shared_counter{0};
    
    for (auto _ : state) {
        const int tasks_per_thread = 1000;
        const int total_tasks = state.threads() * tasks_per_thread;
        std::atomic<int> completed{0};
        
        // 每个测试线程提交任务
        for (int t = 0; t < state.threads(); ++t) {
            std::thread([&pool, &shared_counter, &completed, tasks_per_thread]() {
                for (int i = 0; i < tasks_per_thread; ++i) {
                    pool.submit([&shared_counter, &completed]() {
                        // 访问共享资源（模拟竞争）
                        for (int j = 0; j < 100; ++j) {
                            shared_counter.fetch_add(1, std::memory_order_relaxed);
                        }
                        completed.fetch_add(1);
                    });
                }
            }).detach();
        }
        
        // 等待所有任务完成
        while (completed.load() < total_tasks) {
            std::this_thread::yield();
        }
        
        state.SetItemsProcessed(total_tasks);
    }
}

BENCHMARK(BM_ThreadPool_Contention)
    ->ThreadRange(1, 8)  // 用1-8个测试线程运行
    ->Unit(benchmark::kMillisecond)
    ->UseRealTime();

BENCHMARK_MAIN();