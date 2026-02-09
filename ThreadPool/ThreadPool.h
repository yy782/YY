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
#include "locker.h"

namespace yy{


enum class TaskStatus{
// DEPENDENCY: 任务正在等待依赖的其他任务完成    
// PENDING: 任务已提交但还没开始执行，在队列中等待
// RUNNING: 任务正在被某个工作线程执行
// COMPLETED: 任务成功执行完成
// FAILED: 任务执行过程中发生异常
    //DEPENDENCY,
    PENDING,
    RUNNING,
    COMPLETED,
    FAILED,
};
class BaseTask;


template<typename ResultType>
class TaskResult{//封装是否完成,T是int ,string等返回类型
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




class BaseTask:public std::enable_shared_from_this<BaseTask>{
protected:
    std::atomic<TaskStatus> status{TaskStatus::PENDING};
    std::vector<std::shared_ptr<BaseTask>> dependencies;//依赖的其他任务
    std::set<std::shared_ptr<BaseTask>> dependents;//依赖此任务的其他任务
    mutable std::mutex task_mutex;//保护依赖关系的互斥锁
    const int task_id;
    

    std::function<void()> function;//任务执行的函数对象
public:
    inline BaseTask(std::function<void()> func):
    function(std::move(func)),
    task_id(generate_id())
    {}
    virtual ~BaseTask()=default;
    void execute(IThreadPool* pool){function();}
    void add_dependency(std::shared_ptr<BaseTask> task);// 添加任务依赖：指定此任务必须等待其他任务完成
    //bool is_ready()const{return status.load()==TaskStatus::PENDING;}
    bool is_ready()const;//检查任务是否所有依赖都已完成，准备好执行了
    TaskStatus get_status()const{return status.load();}
    int get_id()const{return task_id;}

    void notify_dependents(IThreadPool* pool);//将依赖完成的任务返回到线程池的任务队列里
private:
    static int generate_id(){
        static std::atomic<size_t> counter{0};
        return  ++counter;
    }
};





// template<typename ResultType>
// class Task:public BaseTask{
// private:

//     std::function<ResultType()> function;
//     std::promise<TaskResult<ResultType>> promise;
//     std::shared_future<TaskResult<ResultType>> future;
// public:
//     inline Task(std::function<ResultType()> func):
//     BaseTask(),
//     function(std::move(func))
//     {future=promise.get_future().share();}
//     void execute(IThreadPool* pool)override; //执行任务
//     inline std::shared_future<TaskResult<ResultType>> get_future()const{return future;}
// };


#pragma endregion



#pragma region WorkerManager;
class WorkerManager{
private:
    std::atomic<size_t> queue_capacity;//本地队列容量
    IThreadPool* pool;
    struct Worker{
        WorkerManager* manager;
        std::deque<std::shared_ptr<BaseTask>> local_queue;
        mutable std::mutex queue_mutex;
        std::condition_variable cv;
        std::atomic<size_t> tasks_processed{0};//已处理的任务计数
        const size_t worker_id;
        interruptible_thread thread;
        Worker(WorkerManager* manager,size_t id):manager(manager),worker_id(id){}
        void run(IThreadPool* pool);
        bool try_push_task(std::shared_ptr<BaseTask> task);
    }; 
    std::vector<std::unique_ptr<Worker>> workers;
    mutable std::mutex workers_mutex;
    std::shared_ptr<BaseTask> try_steal_task(size_t thief_id);
    friend struct Worker;
public:
    WorkerManager()=delete;
    WorkerManager(IThreadPool* pool):pool(pool),queue_capacity(2){}
    ~WorkerManager()=default;
    void add_worker();
    void remove_worker(size_t worker_id);
    size_t get_tasks_processed(size_t id)const;
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
    size_t find_least_active_worker();
    void shutdown();     
};


#pragma endregion




#pragma region TaskManager;



class TaskManager{
private:    
    mutable std::mutex tasks_mutex;
    std::unordered_map<int,std::shared_ptr<BaseTask>> all_tasks;
public:
    TaskManager()=default;
    ~TaskManager()=default;
    void register_task(std::shared_ptr<BaseTask> task);
    void deregister_task(int task_id);
    void add_dependency(int task_id,int dep_task_id);
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
};


#pragma endregion

#pragma region MonitorThread;



template<typename Strategy>
class ThreadPool;

template<class PoolStrategy>
class MonitorThread{
private:
    friend struct Adjust_Worker_Strategy;
private:    
    interruptible_thread thread;
    std::vector<std::function<void()>> strategies;
    std::mutex strategy_mutex;
    ThreadPool<PoolStrategy>* pool;
    std::condition_variable cv;
private:
    std::atomic<size_t> active_tasks{0};
    struct Statistics{//atomic不可拷贝
        // Statistics: 负责只读展示统计快照,给监控线程监视
        size_t total_threads;
        size_t active_threads;
        size_t pending_tasks;//全局队列中等待的任务数
        size_t running_tasks;
        double avg_tasks_per_thread;//每个线程平均的任务数
    };
    size_t target_load_factor{70};//目标CPU负载率，百分比
public:
    void active_task_increment();
public:     
    MonitorThread(ThreadPool<PoolStrategy>* pool):pool(pool){}
    void start();
    void stop();
    template<class Stategy_Pattern>
    void set_Strategy(Stategy_Pattern&& pattern);
    void notify_one();
    ~MonitorThread();
private:    
    bool is_valid()const;
    struct Statistics get_statistics()const;
    size_t get_worker_count()const;
    size_t get_active_task_count()const;

    void add_worker()const;
    void remove_worker(size_t worker_id)const;
    size_t get_target_load_factor()const;
    size_t find_least_active_worker()const;
    size_t get_min_threads()const;
    size_t get_max_threads()const;

};


#pragma endregion




#pragma region ThreadPool;


class IThreadPool{
public:
    template<typename PoolStrategy>
    IThreadPool(size_t min_threads,size_t max_threads,ThreadPool<PoolStrategy>* pool);
    ~IThreadPool()=default;
    virtual void shutdown()=0;
    
    void add_dependency(int task_id,int dep_task_id);
    size_t get_thread_nums()const{
        return workermanager.get_size();
    }
protected:
    const size_t min_threads;
    const size_t max_threads;
    std::atomic<bool> running{true};

    TaskManager taskmanager;
    WorkerManager workermanager;


    virtual void dequeue_task(std::shared_ptr<BaseTask>& task)=0;
    virtual void enqueue_task(std::shared_ptr<BaseTask> task)=0;
    virtual void execute_task(std::shared_ptr<BaseTask> task)=0;

    void deregister_task(int task_id);
    size_t get_min_threads()const;
    size_t get_max_threads()const;
    


    friend void BaseTask::notify_dependents(IThreadPool* pool);
    friend struct WorkerManager;
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

    //监控线程需要的接口
    void add_worker();
    void remove_worker(size_t worker_id);
    size_t get_worker_count()const;
    size_t get_global_task_size()const;
    size_t get_tasks_processed(int id)const;

    size_t find_least_active_worker();


};

#pragma endregion


// inline void deregister_pool(){
//     IThreadPool::shutdown();
// }


// #pragma region Task.tpp

 
// #pragma endregion



// #pragma region ThreadPool.tpp;



   


// #pragma endregion

// #pragma region BaseTask.cpp;




// #pragma endregion    


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

struct Adjust_Worker_Strategy{
public:
    template<typename MonitorType>
    void operator()(MonitorType* monitor){
        std::this_thread::sleep_for(std::chrono::seconds(2));
        if(monitor->is_valid()==false)return;
        auto stats=monitor->get_statistics();
        double load_factor=(static_cast<double>(stats.running_tasks/stats.total_threads)*100);//(运行的任务数（运行的线程）/总线程)*100
        if(load_factor>monitor->get_target_load_factor()&&monitor->get_worker_count()<monitor->get_max_threads()){
            monitor->add_worker();
            LOG_THREAD_INFO("[Moniter]Added worker thread. Total:"<<monitor->get_worker_count()
            <<",Load:"<<load_factor<<"%");
            
        }else if(load_factor<monitor->get_target_load_factor()/2&&monitor->get_worker_count()>monitor->get_min_threads()){
            size_t least_active=monitor->find_least_active_worker();
            if(least_active>=monitor->get_min_threads()){
                monitor->remove_worker(least_active);
                LOG_THREAD_INFO("[Monitor] Removed worker thread. Total: " << monitor->get_worker_count() 
                            << ", Load: " << load_factor << "%");                   
            }
        }            
    }    
};
#pragma endregion
}

#include "ThreadPool.tpp"

