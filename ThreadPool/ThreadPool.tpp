namespace yy{





#pragma region GlobalTaskManager;

template<class Strategy>
void GlobalTaskManager<Strategy>::enqueue_task(std::shared_ptr<typename Strategy::TaskType> task)
{
    std::lock_guard<std::mutex> lock(global_queue_mutex);
     global_queue.push(task);
}

template<class Strategy>
void GlobalTaskManager<Strategy>::dequeue_task(std::shared_ptr<BaseTask>& task)
{
    std::lock_guard<std::mutex> lock(global_queue_mutex);
    if(!global_queue.empty())
    {
        task = global_queue.top();
        global_queue.pop();
        
    }
}

template<class Strategy>
bool GlobalTaskManager<Strategy>::empty()const{
    return global_queue.empty();
}
template<class Strategy>
size_t GlobalTaskManager<Strategy>::size()const{
    return global_queue.size();
}
template<class Strategy>
void GlobalTaskManager<Strategy>::notify_one(){
    global_queue_cv.notify_one();
}
template<class Strategy>
void GlobalTaskManager<Strategy>::notify_all(){
    global_queue_cv.notify_all();
}

#pragma endregion

#pragma region ThreadPool


template<typename PoolStrategy>
IThreadPool::IThreadPool(size_t min_threads,size_t max_threads,ThreadPool<PoolStrategy>* pool):
min_threads(std::max(min_threads,size_t(1))),
max_threads(std::max(max_threads,min_threads)),
taskmanager(),
workermanager(pool)
{
    LOG_THREAD_DEBUG("线程池启动");
    for(size_t i=0;i<min_threads;++i){
        workermanager.add_worker();
        LOG_THREAD_INFO("工作线程:"<<"["<<i<<"]"<<"添加成功");
    }
        
}


template<class Strategy>
ThreadPool<Strategy>::ThreadPool(size_t min_threads,size_t max_threads):
IThreadPool(min_threads,max_threads,this),
global_tasks(),
monitor(this)
{
 monitor.start();
LOG_THREAD_INFO("监控线程启动成功");
LOG_THREAD_INFO("线程池成功启动"); 
}

template<class Strategy>
ThreadPool<Strategy>::~ThreadPool(){
    shutdown();
}
template<class Strategy>
void ThreadPool<Strategy>::execute_task(std::shared_ptr<BaseTask> task){
    monitor.active_task_increment();
    task->execute(this);

}
template<class Strategy>
void ThreadPool<Strategy>::dequeue_task(std::shared_ptr<BaseTask>& task){
    global_tasks.dequeue_task(task);
}
template<class Strategy>
void ThreadPool<Strategy>::enqueue_task(std::shared_ptr<BaseTask> task){
    auto t=std::static_pointer_cast<typename Strategy::TaskType>(task);
    global_tasks.enqueue_task(t);
}

template<class Strategy>
void ThreadPool<Strategy>::add_worker(){
    workermanager.add_worker();     
}

template<class Strategy>
void ThreadPool<Strategy>::remove_worker(size_t worker_id){
    workermanager.remove_worker(worker_id);
}
template<class Strategy>
size_t ThreadPool<Strategy>::get_worker_count()const{
    return workermanager.get_size();
}
template<class Strategy>
size_t ThreadPool<Strategy>::get_global_task_size()const{
    return global_tasks.size();
}
template<class Strategy>
size_t ThreadPool<Strategy>::get_tasks_processed(int id)const{
    return workermanager.get_tasks_processed(id);
}
template<class Strategy>
size_t ThreadPool<Strategy>::find_least_active_worker(){
    return workermanager.find_least_active_worker();
}

template<class Strategy>
template<typename Strategy_Pattern>
void ThreadPool<Strategy>::set_monitor_strategy(Strategy_Pattern&& pattern){
    monitor.set_Strategy(std::forward<Strategy_Pattern>(pattern));
}


template<typename Strategy>
template<typename F,typename... Args>
auto ThreadPool<Strategy>::submit(F&& f,Args&&... args)
->std::shared_future<TaskResult<std::invoke_result_t<F>>>{    
    using ResultType=std::invoke_result_t<F>;    
    using TaskType=typename Strategy::TaskType;
    
    std::shared_ptr<std::promise<TaskResult<ResultType>>> promise=std::make_shared<std::promise<TaskResult<ResultType>>>();
    //这里选择用shared_ptr包装是因为promise是不可拷贝的，导致task_function不可拷贝，那么违反了function对象必须可拷贝的要求
    std::shared_future<TaskResult<ResultType>> future=promise->get_future().share();
    auto task_function=[func=std::forward<F>(f),promise=std::move(promise)]()mutable{
        try{
            if constexpr (std::is_same_v<ResultType,void>){ 
                func();
                promise->set_value(TaskResult<void>(nullptr));
            }else{
                ResultType result=func();
                promise->set_value(TaskResult<ResultType>(std::move(result)));
            }
        }catch(...){
            std::exception_ptr eptr=std::current_exception();
            promise->set_value(TaskResult<ResultType>(eptr));
        }
    };
    std::shared_ptr<TaskType> task=std::make_shared<TaskType>(task_function,std::forward<Args>(args)...);
    taskmanager.register_task(task);
    if(task->is_ready()){
        if(!workermanager.try_add_task(task)){
            global_tasks.enqueue_task(task);
        }    
    }    
    return future;
}

template<class Strategy>
void ThreadPool<Strategy>::shutdown(){
    LOG_THREAD_DEBUG("线程池关闭中...");
    running=false;
    monitor.stop();
    workermanager.shutdown();
    global_tasks.shutdown();
    taskmanager.shutdown();
    LOG_THREAD_INFO("线程池成功关闭");
}

#pragma endregion


#pragma region MonitorThread

template<class PoolStrategy>
void MonitorThread<PoolStrategy>::start(){
    thread=([this]()mutable{
        try{
            while(true){
                thread.interruption_point();
                if(strategies.empty()){
                    std::unique_lock<std::mutex> lock(strategy_mutex);
                    cv.wait_for(lock,std::chrono::microseconds(100),[this](){return !strategies.empty();});//这里会虚假唤醒
                    continue;
                }
                std::lock_guard<std::mutex> lock(strategy_mutex);
                for(auto& strategy:strategies){
                    thread.interruption_point();
                    strategy();
                }
            }            
        }catch(thread_interrupted&){
            LOG_THREAD_DEBUG("线程池监控线程中断");
        }
    });
}

template<class PoolStrategy>
void MonitorThread<PoolStrategy>::stop(){
    if(thread.joinable()){
        thread.interrupt();
        thread.join(); 
    }
}
template<class PoolStrategy>
template<class Strategy_Pattern>
void MonitorThread<PoolStrategy>::set_Strategy(Strategy_Pattern&& pattern){
    strategy_mutex.lock();
    auto bound_strategy=std::bind(std::forward<Strategy_Pattern>(pattern),this);
    strategies.push_back(bound_strategy);
    strategy_mutex.unlock();
    notify_one();
}

template<class PoolStrategy>
void MonitorThread<PoolStrategy>::notify_one(){
    cv.notify_one();
}
template<class PoolStrategy>
void MonitorThread<PoolStrategy>::active_task_increment(){
    active_tasks.fetch_add(1);
}
template<class PoolStrategy>
MonitorThread<PoolStrategy>::~MonitorThread(){
    
    stop();
}
template<class PoolStrategy>
bool MonitorThread<PoolStrategy>::is_valid()const{
    return pool!=nullptr;
}

template<class PoolStrategy>
typename MonitorThread<PoolStrategy>::Statistics
MonitorThread<PoolStrategy>::get_statistics()const{
    using Statistics=typename MonitorThread<PoolStrategy>::Statistics;
    Statistics stats{};
    stats.total_threads=get_worker_count();
    stats.active_threads=get_active_task_count();
    stats.pending_tasks=pool->get_global_task_size();
    stats.running_tasks = get_active_task_count();
    auto worker_size=get_worker_count();
    stats.avg_tasks_per_thread = worker_size > 0 ? 
        static_cast<double>(stats.pending_tasks + stats.running_tasks) / worker_size : 0.0;
    
    return stats;        
}
template<class PoolStrategy>
size_t MonitorThread<PoolStrategy>::get_worker_count()const{

    return pool->get_worker_count();
}
template<class PoolStrategy>
size_t MonitorThread<PoolStrategy>::get_active_task_count()const{
    return active_tasks.load();
}
template<class PoolStrategy>
size_t MonitorThread<PoolStrategy>::get_target_load_factor()const{
    return target_load_factor;
}
template<class PoolStrategy>
size_t MonitorThread<PoolStrategy>::get_min_threads()const{
    return pool->get_min_threads();
}

template<class PoolStrategy>
size_t MonitorThread<PoolStrategy>::get_max_threads()const{
    return pool->get_max_threads();
}


template<class PoolStrategy>
size_t MonitorThread<PoolStrategy>::find_least_active_worker()const{
    return pool->find_least_active_worker();
}



#pragma endregion

}
