#ifndef YY_BASETASK_H_
#define YY_BASETASK_H_
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
namespace yy 
{
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
}
#endif 