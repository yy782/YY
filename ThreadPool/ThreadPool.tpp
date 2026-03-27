#include <cassert>
#include <cstddef>
#include "ThreadPool.h"
namespace yy {



template<typename Strategy>
inline void BaseTask::execute(ThreadPool<Strategy>* pool) {
    function_();
    for (auto& dep : dependents) {
        assert(dep->get_status() == TaskStatus::DEPENDENCY && "依赖的任务状态必须是DEPENDENCY");
        dep->unresolved_dependencies--;
        if (dep->unresolved_dependencies.load() == 0) {
            dep->status.store(TaskStatus::PENDING);
            pool->enqueue_task(dep);
        }
    }
}

// ==================== ThreadPool::TaskManager 实现 ====================

template<class Strategy>
inline void ThreadPool<Strategy>::TaskManager::register_task(std::shared_ptr<BaseTask> task) {
    std::lock_guard<std::mutex> lock(tasks_mutex);
    all_tasks[task->get_id()] = task;
}

template<class Strategy>
inline void ThreadPool<Strategy>::TaskManager::deregister_task(size_t task_id) {
    std::lock_guard<std::mutex> lock(tasks_mutex);
    all_tasks.erase(task_id);
}

template<class Strategy>
inline void ThreadPool<Strategy>::TaskManager::add_dependency(size_t task_id, size_t dep_task_id) {
    std::lock_guard<std::mutex> lock(tasks_mutex);
    auto task = all_tasks[task_id];
    auto dep_task = all_tasks[dep_task_id];
    task->add_dependency(dep_task);
}

// ==================== ThreadPool::GlobalTaskManager 实现 ====================

template<class Strategy>
inline void ThreadPool<Strategy>::GlobalTaskManager::enqueue_task(std::shared_ptr<typename Strategy::TaskType> task) {
    std::lock_guard<std::mutex> lock(global_queue_mutex);
    global_queue.push(task);
}

template<class Strategy>
inline void ThreadPool<Strategy>::GlobalTaskManager::dequeue_task(std::shared_ptr<BaseTask>& task) {
    std::lock_guard<std::mutex> lock(global_queue_mutex);
    if (!global_queue.empty()) {
        task = global_queue.top();
        global_queue.pop();
    }
}

template<class Strategy>
inline bool ThreadPool<Strategy>::GlobalTaskManager::empty() const {
    return global_queue.empty();
}

template<class Strategy>
inline size_t ThreadPool<Strategy>::GlobalTaskManager::size() const {
    return global_queue.size();
}

template<class Strategy>
inline void ThreadPool<Strategy>::GlobalTaskManager::notify_one() {
    global_queue_cv.notify_one();
}

template<class Strategy>
inline void ThreadPool<Strategy>::GlobalTaskManager::notify_all() {
    global_queue_cv.notify_all();
}

// ==================== ThreadPool::Worker 实现 ====================

template<class Strategy>
ThreadPool<Strategy>::Worker::Worker(ThreadPool* manager, size_t id)
    : manager_(manager)
    , worker_id(id)
    , status_(Status::Init) {}

template<class Strategy>
void ThreadPool<Strategy>::Worker::run() {
    EXCLUDE_BEFORE_COMPILATION(
        LOG_THREAD_INFO("[线程:" << worker_id << "]" << "开始执行任务");
    )
    while (true) {
        if (status_ == Status::Stoping) break;
        
        std::shared_ptr<BaseTask> task = nullptr;
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            if (!local_queue.empty()) {
                task = local_queue.front();
                local_queue.pop_front();
            }
        }
        
        if (!task) {
            manager_->dequeue_task(task);
        }
        
        if (!task) {
            task = manager_->try_steal_task(worker_id);
        }
        
        if (!task) {
            std::unique_lock<std::mutex> lock(queue_mutex);
            cv.wait_for(lock, std::chrono::microseconds(100), [this]() {
                return !local_queue.empty();
            });
            continue;
        }
        
        last_active_time.flush();
        manager_->execute_task(task);
        manager_->deregister_task(task->get_id());
    }
}

template<class Strategy>
void ThreadPool<Strategy>::Worker::stop() {
    status_ = Status::Stoping;
}

template<class Strategy>
bool ThreadPool<Strategy>::Worker::try_push_task(std::shared_ptr<BaseTask> task) {
    if (local_queue.size() >= manager_->queue_capacity.load()) return false;
    {
        std::lock_guard<std::mutex> lock(queue_mutex);
        local_queue.emplace_back(std::move(task));
    }
    cv.notify_one();
    return true;
}

// ==================== ThreadPool 核心实现 ====================

template<class Strategy>
ThreadPool<Strategy>::ThreadPool(size_t min_threads, size_t max_threads)
    : min_threads_(std::max(min_threads, size_t(1)))
    , max_threads_(std::max(max_threads, min_threads))
    , queue_capacity(2)
    , task_manager()
    , global_tasks()
    , monitor(this)
    , running(true) {
    
    for (size_t i = 0; i < min_threads_; ++i) {
        add_worker();
        EXCLUDE_BEFORE_COMPILATION(
            LOG_THREAD_INFO("工作线程:" << "[" << i << "]" << "添加成功");
        )
    }

}

template<class Strategy>
ThreadPool<Strategy>::~ThreadPool() {
    shutdown();
}

template<class Strategy>
ThreadPool<Strategy>& ThreadPool<Strategy>::getInstance(size_t min_threads, size_t max_threads) {
    static ThreadPool instance(min_threads, max_threads);
    return instance;
}

template<class Strategy>
void ThreadPool<Strategy>::add_worker() {
    Worker* worker_ptr = nullptr;
    {
        std::lock_guard<std::mutex> lock(workers_mutex);
        size_t worker_id = workers.size();
        for (size_t i = 0; i < workers.size(); ++i) {
            if (workers[i] == nullptr) {
                worker_id = i;
                break;
            }
        }
        
        auto worker = std::make_unique<Worker>(this, worker_id);
        worker_ptr = worker.get();
        
        if (worker_id == workers.size()) {
            workers.emplace_back(std::move(worker));
        } else {
            workers[worker_id] = std::move(worker);
        }
    }
    
    if (worker_ptr == nullptr) return;
    
    worker_ptr->thread.run([worker_ptr, this]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        worker_ptr->run();
    });
}

template<class Strategy>
void ThreadPool<Strategy>::remove_worker(size_t worker_id) {
    if (worker_id >= workers.size()) return;
    
    std::unique_ptr<Worker> worker = std::move(workers[worker_id]);
    {
        worker->stop();
        worker->cv.notify_all();
    }
    
    if (worker->thread.joinable()) {
        worker->thread.join();
    }
    
    {
        std::lock_guard<std::mutex> lock(worker->queue_mutex);
        for (auto& task : worker->local_queue) {
            enqueue_task(task);
        }
    }
}

template<class Strategy>
std::shared_ptr<BaseTask> ThreadPool<Strategy>::try_steal_task(size_t thief_id) {
    std::lock_guard<std::mutex> lock(workers_mutex);
    
    for (size_t i = 0; i < workers.size(); ++i) {
        if (i == thief_id) continue;
        
        auto& victim = workers[i];
        if (!victim) continue;
        
        std::unique_lock<std::mutex> lk(victim->queue_mutex, std::try_to_lock);
        if (lk.owns_lock() && !victim->local_queue.empty()) {
            auto task = victim->local_queue.back();
            victim->local_queue.pop_back();
            if (task) {
                return task;
            }
        }
    }
    return nullptr;
}

template<class Strategy>
bool ThreadPool<Strategy>::try_add_task(std::shared_ptr<BaseTask> task) {
    std::lock_guard<std::mutex> lock(workers_mutex);
    for (auto& worker : workers) {
        if (worker && worker->try_push_task(task)) {
            return true;
        }
    }
    return false;
}

template<class Strategy>
void ThreadPool<Strategy>::shutdown_workers() {
    for (auto& worker : workers) {
        if (worker) {
            worker->stop();
        }
    }
    for (auto& worker : workers) {
        if (worker && worker->thread.joinable()) {
            worker->thread.join();
        }
    }
}

template<class Strategy>
void ThreadPool<Strategy>::execute_task(std::shared_ptr<BaseTask> task) {
    task->execute(this);
}

template<class Strategy>
void ThreadPool<Strategy>::enqueue_task(std::shared_ptr<BaseTask> task) {
    auto t = std::static_pointer_cast<typename Strategy::TaskType>(task);
    global_tasks.enqueue_task(t);
}

template<class Strategy>
void ThreadPool<Strategy>::dequeue_task(std::shared_ptr<BaseTask>& task) {
    global_tasks.dequeue_task(task);
}

template<class Strategy>
void ThreadPool<Strategy>::deregister_task(size_t task_id) {
    task_manager.deregister_task(task_id);
}

template<class Strategy>
void ThreadPool<Strategy>::add_dependency(size_t task_id, size_t dep_task_id) {
    task_manager.add_dependency(task_id, dep_task_id);
}

template<class Strategy>
size_t ThreadPool<Strategy>::get_thread_nums() const {
    return workers.size();
}

template<class Strategy>
template<typename F, typename... Args>
auto ThreadPool<Strategy>::submit(F&& f, Args&&... args)
    -> std::shared_future<TaskResult<std::invoke_result_t<F>>> {
    
    using ResultType = std::invoke_result_t<F>;
    using TaskType = typename Strategy::TaskType;
    
    auto promise = std::make_shared<std::promise<TaskResult<ResultType>>>();
    std::shared_future<TaskResult<ResultType>> future = promise->get_future().share();
    
    auto task_function = [func = std::forward<F>(f), promise = std::move(promise)]() mutable {
        try {
            if constexpr (std::is_same_v<ResultType, void>) {
                func();
                promise->set_value(TaskResult<void>(nullptr));
            } else {
                ResultType result = func();
                promise->set_value(TaskResult<ResultType>(std::move(result)));
            }
        } catch (...) {
            std::exception_ptr eptr = std::current_exception();
            promise->set_value(TaskResult<ResultType>(eptr));
        }
    };
    
    auto task = std::make_shared<TaskType>(task_function, std::forward<Args>(args)...);
    task_manager.register_task(task);
    
    if (task->is_ready()) {
        if (!try_add_task(task)) {
            global_tasks.enqueue_task(task);
        }
    }
    
    return future;
}

template<class Strategy>
template<typename F, typename... Args>
auto ThreadPool<Strategy>::dep_submit(F&& f, int& task_id, Args&&... args)
    -> std::shared_future<TaskResult<std::invoke_result_t<F>>> {
    
    using ResultType = std::invoke_result_t<F>;
    using TaskType = typename Strategy::TaskType;
    
    auto promise = std::make_shared<std::promise<TaskResult<ResultType>>>();
    std::shared_future<TaskResult<ResultType>> future = promise->get_future().share();
    
    auto task_function = [func = std::forward<F>(f), promise = std::move(promise)]() mutable {
        try {
            if constexpr (std::is_same_v<ResultType, void>) {
                func();
                promise->set_value(TaskResult<void>(nullptr));
            } else {
                ResultType result = func();
                promise->set_value(TaskResult<ResultType>(std::move(result)));
            }
        } catch (...) {
            std::exception_ptr eptr = std::current_exception();
            promise->set_value(TaskResult<ResultType>(eptr));
        }
    };
    
    auto task = std::make_shared<TaskType>(task_function, std::forward<Args>(args)...);
    task_id = task->get_id();
    task->status.store(TaskStatus::DEPENDENCY);
    task_manager.register_task(task);
    
    if (task->is_ready()) {
        if (!try_add_task(task)) {
            global_tasks.enqueue_task(task);
        }
    }
    
    return future;
}

template<class Strategy>
void ThreadPool<Strategy>::shutdown() {
    EXCLUDE_BEFORE_COMPILATION(
        LOG_THREAD_DEBUG("线程池关闭中...");
    )
    running = false;
    monitor.stop();
    shutdown_workers();
    global_tasks.shutdown();
    task_manager.shutdown();
    EXCLUDE_BEFORE_COMPILATION(
        LOG_THREAD_INFO("线程池成功关闭");
    )
}

template<class Strategy>
template<typename Strategy_Pattern>
void ThreadPool<Strategy>::set_monitor_strategy(Strategy_Pattern&& pattern) {
    monitor.set_strategy(std::forward<Strategy_Pattern>(pattern));
}
template<class Strategy>
void ThreadPool<Strategy>::monitorRun()
{
    monitor.start();
    EXCLUDE_BEFORE_COMPILATION(
        LOG_THREAD_INFO("监控线程启动成功");
        LOG_THREAD_INFO("线程池成功启动");
    )
}
// ==================== ThreadPool::MonitorThread 实现 ====================

template<class Strategy>
ThreadPool<Strategy>::MonitorThread::MonitorThread(ThreadPool* pool)
    : pool_(pool) {}

template<class Strategy>
void ThreadPool<Strategy>::MonitorThread::start() {
    thread.run([this]() mutable {
        while (running) {
            if (strategies.empty()) {
                std::unique_lock<std::mutex> lock(strategy_mutex);
                cv.wait_for(lock, std::chrono::microseconds(100), [this]() {
                    return !strategies.empty();
                });
                continue;
            }
            std::lock_guard<std::mutex> lock(strategy_mutex);
            for (auto& strategy : strategies) {
                strategy();
            }
        }
    });
}

template<class Strategy>
void ThreadPool<Strategy>::MonitorThread::stop() {
    if (thread.joinable()) {
        running = false;
        thread.join();
    }
}

template<class Strategy>
template<class Strategy_Pattern>
void ThreadPool<Strategy>::MonitorThread::set_strategy(Strategy_Pattern&& pattern) {
    std::lock_guard<std::mutex> lock(strategy_mutex);
    auto bound_strategy = std::bind(std::forward<Strategy_Pattern>(pattern), this);
    strategies.push_back(bound_strategy);
    notify_one();
}

template<class Strategy>
void ThreadPool<Strategy>::MonitorThread::notify_one() {
    cv.notify_one();
}

template<class Strategy>
ThreadPool<Strategy>::MonitorThread::~MonitorThread() {
    stop();
}

// ==================== Adjust_Worker_Strategy 实现 ====================

template<typename MonitorType>
void Adjust_Worker_Strategy::operator()(MonitorType* monitor) {
    assert(monitor->pool_ && "监控线程未正确初始化");
    auto& pool = *monitor->pool_;
    
    std::unique_lock<std::mutex> lock(pool.workers_mutex);
    size_t worker_count = pool.workers.size();
    TimeStamp<LowPrecision> now;
    
    for (size_t i = 0; i < worker_count; ++i) {
        auto worker = pool.workers[i].get();
        if (worker) {
            auto idle_time = now - worker->get_last_active_time();
            if (idle_time > 10 && worker_count > pool.min_threads_) {
                pool.remove_worker(i);
                EXCLUDE_BEFORE_COMPILATION(
                    LOG_THREAD_INFO("移除空闲工作线程:" << "[" << i << "]");
                )
            }
        }
    }
    lock.unlock();
    
    size_t global_task_size = pool.global_tasks.size();
    if (global_task_size > 100 && worker_count < pool.max_threads_) {
        pool.add_worker();
        EXCLUDE_BEFORE_COMPILATION(
            LOG_THREAD_INFO("添加工作线程");
        )
    }
}

}
