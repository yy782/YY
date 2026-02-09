#include "../ThreadPool.h"

using namespace yy;

// 全局原子计数器：验证任务执行完整性（线程安全）
std::atomic<size_t> g_task_count = 0;
// 测试配置：可根据机器性能调整（线程数越多、任务数越多，越易复现问题）
const size_t TEST_THREAD_NUM = 10;    // 测试线程数（并发submit的线程数）
const size_t TASKS_PER_THREAD = 10000;// 每个测试线程提交的任务数

// 测试任务：仅对原子计数器+1（无IO、无耗时，纯CPU操作，放大竞态）
void test_task() {
    g_task_count.fetch_add(1, std::memory_order_relaxed);
}

// 测试线程函数：单个线程提交大量任务
void submit_tasks(ThreadPool<FSFC>& pool,std::vector<std::shared_future<TaskResult<void>>>& futures,std::mutex& mtx) {
    for (size_t i = 0; i < TASKS_PER_THREAD; ++i) {
        auto fut=pool.submit(test_task); // 调用你的线程池submit方法
        std::lock_guard<std::mutex> lock(mtx);
        futures.push_back(std::move(fut));
    }
}

int main() {
    std::cout << "开始测试：多线程并发submit任务..." << std::endl;
    std::cout << "测试配置：" << TEST_THREAD_NUM << "个线程，每个线程提交" << TASKS_PER_THREAD << "个任务" << std::endl;
    std::cout << "预期总任务数：" << TEST_THREAD_NUM * TASKS_PER_THREAD << std::endl;

    // 1. 初始化线程池（根据你的实现调整参数，比如4个工作线程）
    auto& pool=ThreadPool<FSFC>::getInstance(4,8); // 最小4线程，最大8线程（根据需要调整）

    std::this_thread::sleep_for(std::chrono::seconds(1));
    // 2. 重置原子计数器
    g_task_count.store(0, std::memory_order_relaxed);

    // 3. 创建测试线程，并发submit任务
    std::vector<std::thread> test_threads;
    std::vector<std::shared_future<TaskResult<void>>> futures;
    std::mutex mtx;
    for (size_t i = 0; i < TEST_THREAD_NUM; ++i) {
        test_threads.emplace_back(submit_tasks, std::ref(pool),std::ref(futures),std::ref(mtx));
    }

    // 4. 等待所有测试线程完成submit（任务可能还在线程池中执行）
    for (auto& t : test_threads) {
        t.join();
    }
    std::cout << "所有任务已提交，等待线程池执行完毕..." << std::endl;

    // 5. 等待线程池所有任务执行完毕（关键！需给你的线程池实现「等待所有任务完成」的方法）
    // 【注意】需要你给ThreadPool加一个wait()方法，实现逻辑：
    //   - 锁住任务队列，判断队列是否为空；
    //   - 结合条件变量，等待所有工作线程完成当前任务，队列无剩余任务；

    // 6. 停止线程池（避免资源泄漏）
    for(size_t i = 0; i < TEST_THREAD_NUM*TASKS_PER_THREAD; ++i){
        if(i==TEST_THREAD_NUM*TASKS_PER_THREAD/2){
            std::cout<<"已等待一半任务完成..."<<std::endl;
        }
        if(i==TEST_THREAD_NUM*TASKS_PER_THREAD/8){
            std::cout<<"已等待八分之一任务完成..."<<std::endl;
        }
        futures[i].wait();
    }
    pool.shutdown();

    // 7. 验证结果
    size_t actual_count = g_task_count.load(std::memory_order_relaxed);
    size_t expected_count = TEST_THREAD_NUM * TASKS_PER_THREAD;
    std::cout << "实际执行任务数：" << actual_count << std::endl;
    if (actual_count == expected_count) {
        std::cout << "✅ 测试通过：多线程submit任务无丢失、无重复！" << std::endl;
    } else {
        std::cout << "❌ 测试失败：任务数不匹配，存在丢失/重复！" << std::endl;
        std::cout << "差异值：" << abs((long long)actual_count - (long long)expected_count) << std::endl;
    }

    return 0;
}