#ifndef YY_THREADPOOL_H_
#define YY_THREADPOOL_H_
#include <exception>
#include <memory>
#include <atomic>
#include <vector>
#include <set>
#include <mutex>
#include <string>
#include <chrono>
#include <functional>
#include <future>
#include <thread>
#include <optional>
#include <deque>
#include <condition_variable>
#include <queue>
#include <shared_mutex>
#include <utility>
#include <random>
#include <iostream>
#include <map>
#include <any>
#include <assert.h>
#include "../Common/LogFilter.h" 
#include "../Common/noncopyable.h"
#include "../Common/TimeStamp.h"
#include "../Common/locker.h"
#include "../Common/copyable.h"

namespace yy {

// 前向声明
template<typename Strategy>
class ThreadPool;

// 枚举类定义
enum class TaskStatus {
    DEPENDENCY,
    PENDING,
    RUNNING,
    COMPLETED,
    FAILED,
};

// TaskResult 模板特化
template<typename ResultType>
class TaskResult : public copyable {
public:
    TaskStatus status;
    std::exception_ptr exception;
    std::optional<ResultType> value;
    
    inline TaskResult() : status(TaskStatus::PENDING) {}
    inline TaskResult(ResultType val) : status(TaskStatus::COMPLETED), value(std::move(val)) {}
    inline TaskResult(std::exception_ptr e) : status(TaskStatus::FAILED), exception(e) {}
};

template<>
class TaskResult<void> : public copyable {
public:
    TaskStatus status;
    std::exception_ptr exception;
    
    inline TaskResult() : status(TaskStatus::PENDING) {}
    inline TaskResult(std::nullptr_t) : status(TaskStatus::COMPLETED) {}
    inline explicit TaskResult(std::exception_ptr e) : status(TaskStatus::FAILED), exception(e) {}
};

// BaseTask 定义
class BaseTask : public copyable, public std::enable_shared_from_this<BaseTask> {
protected:
    std::atomic<TaskStatus> status{TaskStatus::PENDING};
    std::atomic<int> unresolved_dependencies{0};
    std::set<std::shared_ptr<BaseTask>> dependents;
    mutable std::mutex task_mutex;
    const size_t task_id;
    std::function<void()> function_;

public:
    explicit BaseTask(std::function<void()> func);
    virtual ~BaseTask() = default;
    
    template<typename Strategy>
    void execute(ThreadPool<Strategy>* pool);
    
    void add_dependency(std::shared_ptr<BaseTask> task);
    bool is_ready() const { return status.load() == TaskStatus::PENDING; }
    TaskStatus get_status() const { return status.load(); }
    size_t get_id() const { return task_id; }

private:
    static size_t generate_id();
};

// ThreadPool 主类
template<class Strategy>
class ThreadPool : public noncopyable {
public:
    // 构造函数和析构函数
    explicit ThreadPool(size_t min_threads = std::thread::hardware_concurrency(),
                        size_t max_threads = std::thread::hardware_concurrency() * 2);
    ~ThreadPool();
    
    // 单例模式
    static ThreadPool& getInstance(size_t min_threads = std::thread::hardware_concurrency(),
                                   size_t max_threads = std::thread::hardware_concurrency() * 2);
    
    // 任务提交接口
    template<typename F, typename... Args>
    auto submit(F&& f, Args&&... args)
        -> std::shared_future<TaskResult<std::invoke_result_t<F>>>;
    
    template<typename F, typename... Args>
    auto dep_submit(F&& f, int& task_id, Args&&... args)
        -> std::shared_future<TaskResult<std::invoke_result_t<F>>>;
    
    // 控制接口
    void shutdown();
    void add_dependency(size_t task_id, size_t dep_task_id);
    size_t get_thread_nums() const;
    
    template<typename Strategy_Pattern>
    void set_monitor_strategy(Strategy_Pattern&& pattern);
    
private:
    // ==================== WorkerManager 组件 ====================
    struct Worker {
        enum class Status {
            Init = 1 << 0,
            Running = 2 << 0,
            Stoping = 3 << 0
        };
        
        ThreadPool* manager_;
        std::deque<std::shared_ptr<BaseTask>> local_queue;
        mutable std::mutex queue_mutex;
        std::condition_variable cv;
        TimeStamp<LowPrecision> last_active_time;
        const size_t worker_id;
        Thread thread;
        Status status_;
        
        Worker(ThreadPool* manager, size_t id);
        void run();
        void stop();
        bool try_push_task(std::shared_ptr<BaseTask> task);
        auto get_last_active_time() const { return last_active_time.get_time_point(); }
    };
    
    std::vector<std::unique_ptr<Worker>> workers;
    mutable std::mutex workers_mutex;
    std::atomic<size_t> queue_capacity;
    
    void add_worker();
    void remove_worker(size_t worker_id);
    std::shared_ptr<BaseTask> try_steal_task(size_t thief_id);
    bool try_add_task(std::shared_ptr<BaseTask> task);
    void shutdown_workers();
    
    // ==================== TaskManager 组件 ====================
    struct TaskManager {
        mutable std::mutex tasks_mutex;
        std::unordered_map<size_t, std::shared_ptr<BaseTask>> all_tasks;
        
        void register_task(std::shared_ptr<BaseTask> task);
        void deregister_task(size_t task_id);
        void add_dependency(size_t task_id, size_t dep_task_id);
        void shutdown() {}
    } task_manager;
    
    // ==================== GlobalTaskManager 组件 ====================
    struct GlobalTaskManager {
        typedef typename Strategy::TaskType TaskType;
        
        std::priority_queue<std::shared_ptr<TaskType>, 
                            std::vector<std::shared_ptr<TaskType>>, 
                            Strategy> global_queue;
        mutable std::mutex global_queue_mutex;
        std::condition_variable global_queue_cv;
        
        void enqueue_task(std::shared_ptr<TaskType> task);
        void dequeue_task(std::shared_ptr<BaseTask>& task);
        bool empty() const;
        size_t size() const;
        void notify_one();
        void notify_all();
        void shutdown() { notify_all(); }
    } global_tasks;
    
    // ==================== MonitorThread 组件 ====================
    class MonitorThread {
    public:
        MonitorThread(ThreadPool* pool);
        void start();
        void stop();
        template<class Strategy_Pattern>
        void set_strategy(Strategy_Pattern&& pattern);
        void notify_one();
        ~MonitorThread();
        
    private:
        friend struct Adjust_Worker_Strategy;
        
        std::atomic<bool> running{true};
        Thread thread;
        std::vector<std::function<void()>> strategies;
        std::mutex strategy_mutex;
        ThreadPool* pool_;
        std::condition_variable cv;
        size_t target_load_factor{70};
    } monitor;
    
    // ==================== 核心任务处理函数 ====================
    void execute_task(std::shared_ptr<BaseTask> task);
    void enqueue_task(std::shared_ptr<BaseTask> task);
    void dequeue_task(std::shared_ptr<BaseTask>& task);
    void deregister_task(size_t task_id);
    
    // ==================== 线程池配置 ====================
    const size_t min_threads_;
    const size_t max_threads_;
    std::atomic<bool> running{true};
    
    // 友元声明
    template<typename S>
    friend void BaseTask::execute(ThreadPool<S>* pool);
    friend class MonitorThread;
    friend struct Adjust_Worker_Strategy;
};

// 监控策略
struct Adjust_Worker_Strategy {
    template<typename MonitorType>
    void operator()(MonitorType* monitor);
};

} // namespace yy

#include "ThreadPool.tpp"

#endif // THREAD_POOL_H

