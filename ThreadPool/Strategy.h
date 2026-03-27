#ifndef YY_STRATEGY_H_
#define YY_STRATEGY_H_
#include "BaseTask.h"
namespace yy 
{
// ==================== 优先级任务定义 ====================

struct FsfcTask : public BaseTask {
public:
    FsfcTask(std::function<void()> func) :
        BaseTask(func),
        arrival_time(std::chrono::steady_clock::now())
    {}
    
    auto get_arrival_time() const { return arrival_time; }
    
    ~FsfcTask() = default;

private:
    const std::chrono::steady_clock::time_point arrival_time;
};

struct FSFC {
    typedef FsfcTask TaskType;
    bool operator()(const std::shared_ptr<TaskType>& a, const std::shared_ptr<TaskType>& b) const {
        return a->get_arrival_time() > b->get_arrival_time();
    }
};

class PriorityTask : public BaseTask {
public:
    PriorityTask(std::function<void()> func, int priority) :
        BaseTask(func),
        priority_(priority)
    {}
    
    int get_priority() const { return priority_; }
    
private:
    const int priority_;
};

struct Priority {
    typedef PriorityTask TaskType;
    bool operator()(const std::shared_ptr<TaskType>& a, const std::shared_ptr<TaskType>& b) const {
        return a->get_priority() > b->get_priority();
    }
};

struct Adjust_Worker_Strategy;    
}
#endif 