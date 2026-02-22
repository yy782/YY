#pragma once
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
#define LOG_FILE
#include "../Common/Log.h" 
#include "../Common/noncopyable.h"
#include "../Common/TimeStamp.h"
#include "../Common/locker.h"

namespace yy{


enum class TaskStatus{
// DEPENDENCY: 任务正在等待依赖的其他任务完成    
// PENDING: 任务已提交但还没开始执行，在队列中等待
// RUNNING: 任务正在被某个工作线程执行
// COMPLETED: 任务成功执行完成
// FAILED: 任务执行过程中发生异常
    DEPENDENCY,
    PENDING,
    RUNNING,
    COMPLETED,
    FAILED,
};
class BaseTask;


template<typename ResultType>
class TaskResult:public copyable{//封装是否完成,T是int ,string等返回类型
public:
    TaskStatus status;
    std::exception_ptr exception;//任务执行过程中发生异常
    std::optional<ResultType> value;// 任务返回值，使用optional因为可能没有值
    inline TaskResult():status(TaskStatus::PENDING){}
    inline TaskResult(ResultType val):status(TaskStatus::COMPLETED),value(std::move(val)){}
    inline TaskResult(std::exception_ptr e) :status(TaskStatus::FAILED), exception(e) {}
    };
template<>
class TaskResult<void>{
public:
    TaskStatus status;
    std::exception_ptr exception;
    inline TaskResult(): status(TaskStatus::PENDING) {}
    inline TaskResult(std::nullptr_t):status(TaskStatus::COMPLETED){}
    inline explicit TaskResult(std::exception_ptr e):status(TaskStatus::FAILED),exception(e){} 
};




class IThreadPool;




class BaseTask:public copyable,
                public std::enable_shared_from_this<BaseTask>{
protected:
    std::atomic<TaskStatus> status{TaskStatus::PENDING};
    std::atomic<int> unresolved_dependencies{0};//未完成的依赖任务数量
    std::set<std::shared_ptr<BaseTask>> dependents;//依赖此任务的其他任务
    mutable std::mutex task_mutex;//保护依赖关系的互斥锁
    const size_t task_id;
    

    std::function<void()> function_;//任务执行的函数对象
public:
    BaseTask(std::function<void()> func):
    task_id(generate_id()),
    function_(std::move(func))    
    {}
    virtual ~BaseTask()=default;
    void execute(IThreadPool* pool);
    void add_dependency(std::shared_ptr<BaseTask> task);// 添加任务依赖：指定此任务必须等待其他任务完成
    bool is_ready()const{return status.load()==TaskStatus::PENDING;}
    TaskStatus get_status()const{return status.load();}
    size_t get_id()const{return task_id;}


private:
    static size_t generate_id(){
        static std::atomic<size_t> counter{0};
        return  ++counter;
    }
};

#pragma endregion

// template<typename PoolStrategy>
// class MonitorThread;

#pragma region WorkerManager;
class WorkerManager:public noncopyable{
private: 
    IThreadPool* pool_;
    std::atomic<size_t> queue_capacity;//本地队列容量
    struct Worker{
        enum class Status
        {
            Init=1<<0,
            Running=2<<0,
            Stoping=3<<0
        };
        WorkerManager* manager_;
        std::deque<std::shared_ptr<BaseTask>> local_queue;
        mutable std::mutex queue_mutex;
        std::condition_variable cv;
        TimeStamp<LowPrecision> last_active_time;
        const size_t worker_id;
        Thread thread;
        Status status_;
        Worker(WorkerManager* manager,size_t id):
        manager_(manager),
        worker_id(id),
        status_(Status::Init)
        {}
        void run(IThreadPool* pool);
        void stop();
        bool try_push_task(std::shared_ptr<BaseTask> task);
        auto get_last_active_time()const{return last_active_time.get_time_point();}
    }; 
    std::vector<std::unique_ptr<Worker>> workers;
    mutable std::mutex workers_mutex;
    std::shared_ptr<BaseTask> try_steal_task(size_t thief_id);
    friend struct Worker;
    template<typename PoolStrategy>
    friend class MonitorThread;

    friend class IThreadPool;
    Worker* get_worker(size_t id){
        std::lock_guard<std::mutex> lock(workers_mutex);
        if(id>=workers.size())return nullptr;
        return workers[id].get();
    }
    auto& get_mutex()const{
        return workers_mutex;
    }
    void add_worker();
    void remove_worker(size_t worker_id);
public:
    WorkerManager()=delete;
    WorkerManager(IThreadPool* pool):pool_(pool),queue_capacity(2){}
    ~WorkerManager()=default;
    
    
    
    size_t get_size()const{
        return workers.size();
    }
    void set_queue_capacity(size_t n){
        queue_capacity.store(n);
    }
    size_t get_queue_size()const{
        return queue_capacity.load();
    }    
    bool try_add_task(std::shared_ptr<BaseTask> task);

    void shutdown();     
};


#pragma endregion




#pragma region TaskManager;



class TaskManager:public noncopyable{
private:    
    mutable std::mutex tasks_mutex;
    std::unordered_map<size_t,std::shared_ptr<BaseTask>> all_tasks;
    template<typename PoolStrategy>
    friend class MonitorThread;
public:
    TaskManager()=default;
    ~TaskManager()=default;
    void register_task(std::shared_ptr<BaseTask> task);
    void deregister_task(size_t task_id);
    void add_dependency(size_t task_id,size_t dep_task_id);
    void shutdown(){}
};

#pragma endregion

#pragma region GlobalTaskManager;





template<class Strategy>
class GlobalTaskManager:noncopyable{
public:
    typedef typename Strategy::TaskType TaskType;
    GlobalTaskManager()=default;

    void enqueue_task(std::shared_ptr<TaskType> task);

    void dequeue_task(std::shared_ptr<BaseTask>& task);
    bool empty()const;
    size_t size()const;
    void notify_one();
    void notify_all();
    void shutdown(){
        notify_all();
    }
private:
    
    std::priority_queue<std::shared_ptr<TaskType>, std::vector<std::shared_ptr<TaskType>>, Strategy> global_queue;
    mutable std::mutex global_queue_mutex;
    std::condition_variable global_queue_cv;   
    
    template<typename PoolStrategy>
    friend class MonitorThread;
};


#pragma endregion

#pragma region MonitorThread;



template<typename Strategy>
class ThreadPool;

template<class PoolStrategy>
class MonitorThread:noncopyable{
public:     
    MonitorThread(ThreadPool<PoolStrategy>* pool):pool_(pool){}
    void start();
    void stop();
    template<class Stategy_Pattern>
    void set_Strategy(Stategy_Pattern&& pattern);
    void notify_one();
    ~MonitorThread();
private:    

    friend struct Adjust_Worker_Strategy;
    std::atomic<bool> running{true};
    Thread thread;
    std::vector<std::function<void()>> strategies;
    std::mutex strategy_mutex;
    ThreadPool<PoolStrategy>* pool_;
    std::condition_variable cv;
    size_t target_load_factor{70};//目标CPU负载率，百分比    

};


#pragma endregion




#pragma region ThreadPool;




class IThreadPool:noncopyable{
public:
    template<typename PoolStrategy>
    IThreadPool(size_t min_threads,size_t max_threads,ThreadPool<PoolStrategy>* pool);
    ~IThreadPool()=default;
    virtual void shutdown()=0;
    
    void add_dependency(size_t task_id,size_t dep_task_id);
    size_t get_thread_nums()const{
        return workermanager.get_size();
    }
protected:
    const size_t min_threads_;
    const size_t max_threads_;
    std::atomic<bool> running{true};

    TaskManager taskmanager;
    WorkerManager workermanager;


    virtual void dequeue_task(std::shared_ptr<BaseTask>& task)=0;
    virtual void enqueue_task(std::shared_ptr<BaseTask> task)=0;
    virtual void execute_task(std::shared_ptr<BaseTask> task)=0;

    void deregister_task(size_t task_id);
    size_t get_min_threads()const;
    size_t get_max_threads()const;
    


    friend void BaseTask::execute(IThreadPool* pool);
    friend struct WorkerManager;
    friend class BaseTask;
};
template<class Strategy>
class ThreadPool:public IThreadPool{

public:
    static ThreadPool& getInstance(size_t min_threads=std::thread::hardware_concurrency(),size_t max_threads=std::thread::hardware_concurrency()*2){
        static ThreadPool instance(min_threads,max_threads);
        return instance;
    }
    


    template<typename F,typename... Args>
    auto submit(F&& f, Args&&... args)//args是设置任务优先级的参数
    ->std::shared_future<TaskResult<std::invoke_result_t<F>>>;

    template<typename F,typename... Args>
    auto dep_submit(F&& f,int& task_id,Args&&... args)//args是设置任务优先级的参数
    ->std::shared_future<TaskResult<std::invoke_result_t<F>>>;

    void shutdown()override;

    template<typename Strategy_Pattern>
    void set_monitor_strategy(Strategy_Pattern&& pattern);    


private:
    GlobalTaskManager<Strategy> global_tasks;
    MonitorThread<Strategy> monitor;
    
    explicit ThreadPool(size_t min_threads=std::thread::hardware_concurrency(),size_t max_threads=std::thread::hardware_concurrency()*2);    
    ~ThreadPool();

    void execute_task(std::shared_ptr<BaseTask> task)override;
    void enqueue_task(std::shared_ptr<BaseTask> task)override;//有依赖的任务提交回线程池
    void dequeue_task(std::shared_ptr<BaseTask>& task)override;



};

#pragma endregion
  


#pragma region Strategy
struct FsfcTask:public BaseTask{
public:
    FsfcTask(std::function<void()> func):
    BaseTask(func),
    arrival_time(std::chrono::steady_clock::now())
    {}
    auto get_arrival_time()const{return arrival_time;}

    ~FsfcTask()=default;

private:
    const std::chrono::steady_clock::time_point arrival_time;
};  



struct FSFC
{
    typedef FsfcTask TaskType;
    bool operator()(const std::shared_ptr<TaskType>& a, const std::shared_ptr<TaskType>& b) const {
        return a->get_arrival_time() > b->get_arrival_time();
    }
};

class PriorityTask:public BaseTask{
public:
    PriorityTask(std::function<void()> func,int priority):
    BaseTask(func),
    priority_(priority)
    {}
    int get_priority()const{return priority_;}
private:
    const int priority_;           
};


struct Priority
{
    typedef PriorityTask TaskType;
    bool operator()(const std::shared_ptr<TaskType>& a, const std::shared_ptr<TaskType>& b) const {
        return a->get_priority() > b->get_priority();
    }
};

struct Adjust_Worker_Strategy;

#pragma endregion
}

#include "ThreadPool.tpp"

