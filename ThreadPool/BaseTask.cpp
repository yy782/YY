#include "ThreadPool.h"
namespace yy 
{
// ==================== BaseTask 实现 ====================

size_t BaseTask::generate_id() {
    static std::atomic<size_t> counter{0};
    return ++counter;
}

BaseTask::BaseTask(std::function<void()> func)
    : task_id(generate_id())
    , function_(std::move(func)) {}

void BaseTask::add_dependency(std::shared_ptr<BaseTask> task) {
    {
        std::lock_guard<std::mutex> lock(task_mutex);
        unresolved_dependencies++;
    }
    {
        std::lock_guard<std::mutex> lock(task->task_mutex);
        task->dependents.insert(shared_from_this());
    }
}   
}