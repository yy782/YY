#include "ThreadPool.h"
namespace yy{

#pragma region Task;
    
void BaseTask::add_dependency(std::shared_ptr<BaseTask> task){
    {
        std::lock_guard<std::mutex> lock(task_mutex);
        dependencies.push_back(task);        
    }
    {
        std::lock_guard<std::mutex> lock(task->task_mutex);
        task->dependents.insert(shared_from_this());
    }
}    
// PENDING: 任务已提交但还没开始执行，在队列中等待
// RUNNING: 任务正在被某个工作线程执行
// COMPLETED: 任务成功执行完成
// FAILED: 任务执行过程中发生异常
bool BaseTask::is_ready()const{
    std::lock_guard<std::mutex> lock(task_mutex);
    for(const auto& dep:dependencies){
        if(dep->get_status()!=TaskStatus::COMPLETED){
            return false;
        }
    }
    return true;
}

void BaseTask::notify_dependents(IThreadPool* pool) {
std::vector<std::shared_ptr<BaseTask>> ready_dependents;

    {
        std::lock_guard<std::mutex> lock(task_mutex);
        for (auto& dependent : dependents) {
            if (dependent->is_ready()) {
                ready_dependents.push_back(dependent);//安全，只提交一次
            }
        }
    }
    
    // 将就绪的依赖任务提交回线程池
    for (auto& task : ready_dependents) {
        pool->enqueue_task(task);
    }
}

#pragma endregion


#pragma region TaskManager;

void TaskManager::register_task(std::shared_ptr<BaseTask> task){
    std::lock_guard<std::mutex> lock(tasks_mutex);
    all_tasks[task->get_id()]=task;
}

void TaskManager::deregister_task(int task_id){
    std::lock_guard<std::mutex> lock(tasks_mutex);
    all_tasks.erase(task_id);
}

void TaskManager::add_dependency(int task_id,int dep_task_id){ 
    std::lock_guard<std::mutex> lock(tasks_mutex);
    auto task=all_tasks[task_id];
    auto dep_task=all_tasks[dep_task_id];
    task->add_dependency(dep_task);
}

#pragma endregion


#pragma region WorkerManager;
void WorkerManager::Worker::run(IThreadPool* pool){
    LOG_THREAD_INFO("[线程:"<<worker_id<<"]"<<"开始执行任务");

    // auto ptr=dynamic_cast<ThreadPool<FSFC>*>(pool);
    // assert(ptr!=nullptr&&"pool指针未成功转换FSFC对象");


    try{
       
        while(true){
            thread.interruption_point();
            std::shared_ptr<BaseTask> task=nullptr;
            {
                std::unique_lock<std::mutex> lock(queue_mutex);
                if(!local_queue.empty()){
                    task=local_queue.front();
                    local_queue.pop_front();
                }
            }

            if(!task){
                    pool->dequeue_task(task);
                
            }
            if(!task){
                task=manager->try_steal_task(worker_id);
            }
            if(!task){
                std::unique_lock<std::mutex> lock(queue_mutex);
                cv.wait_for(lock,std::chrono::microseconds(100),[this](){
                    return !local_queue.empty();
                });
                continue;
            }
            tasks_processed++;      
            pool->execute_task(task);
            pool->deregister_task(task->get_id());
        }
             
    }catch(const thread_interrupted&){
        LOG_THREAD_INFO("[worker "<<worker_id<<"] 收到中断信号");
    }    
}
bool WorkerManager::Worker::try_push_task(std::shared_ptr<BaseTask> task){
    if(local_queue.size()>=manager->get_queue_size())return false;
    {
        std::lock_guard<std::mutex> lock(queue_mutex);
        local_queue.emplace_back(std::move(task));        
    }
    cv.notify_one();
    return true;
}
std::shared_ptr<BaseTask> WorkerManager::try_steal_task(size_t thief_id){
    std::lock_guard<std::mutex> lk(workers_mutex);
    for(size_t i=0;i<workers.size();++i){
        if(i==thief_id)continue;
        auto& victim=workers[i];
        std::unique_lock<std::mutex> lock(victim->queue_mutex,std::try_to_lock);
        if(lock.owns_lock()&&!victim->local_queue.empty()){//使用owns_lock保持更小程度的锁竞争
            auto task=victim->local_queue.back();//工作线程从队头取任务，窃取线程从队尾窃取
            victim->local_queue.pop_back();
            if(task){
                return task;
            }
        }
         
    } 
    return nullptr;//窃取失败 
}
size_t WorkerManager::find_least_active_worker(){
    size_t min_tasks=std::numeric_limits<size_t>::max();//将 min_tasks 初始化为 size_t 类型的最大值。
    size_t worker_id=0;
    for(size_t i=pool->get_min_threads();i<get_size();++i){
        size_t tasks=get_tasks_processed(i);
        if(tasks<min_tasks){
            min_tasks=tasks;
            worker_id=i;
        }
    }
    return worker_id;
}

size_t WorkerManager::get_tasks_processed(size_t id)const{
    std::lock_guard<std::mutex> lock(workers_mutex);
    if(id>workers.size())return 0;
    return workers[id]->tasks_processed.load();
    
}
bool WorkerManager::try_add_task(std::shared_ptr<BaseTask> task){
    std::lock_guard<std::mutex> lock(workers_mutex);
    for(size_t i=0;i<workers.size();++i){
        if(workers[i]->try_push_task(task)){
            return true;
        }
    }
    return false;
}   
void WorkerManager::add_worker()
{
    Worker *worker_ptr=nullptr;
    {
        size_t worker_id=0;
        std::lock_guard<std::mutex> lock(workers_mutex);
        worker_id=workers.size();
        for(size_t i=0;i<workers.size();i++){
            if(workers[i]==nullptr){worker_id=i;}
        }
        auto worker=std::make_unique<Worker>(this,worker_id);
        worker_ptr=worker.get();
        if(worker_id==workers.size()){
            workers.emplace_back(std::move(worker));

        }else{
            workers[worker_id]=std::move(worker);

        }     
    }
    if(worker_ptr==nullptr)return;
    worker_ptr->thread=[worker_ptr,this](){
        std::this_thread::sleep_for(std::chrono::milliseconds(10));//////////////////////////////////
        LOG_THREAD_DEBUG("[Worker "<<worker_ptr->worker_id<<"] 启动成功");
        worker_ptr->run(pool);
    };
   
}

void WorkerManager::remove_worker(size_t worker_id){
    if(worker_id>=workers.size())return;
    std::unique_ptr<Worker> worker=std::move(workers[worker_id]);
    {
        worker->thread.interrupt();
        worker->cv.notify_all();

    }
    if(worker->thread.joinable()){
        worker->thread.join();
    }
    {
        std::lock_guard<std::mutex> lock(worker->queue_mutex);
        for(auto& task:worker->local_queue){
           pool->enqueue_task(task); 
        }
    }
}

void WorkerManager::shutdown(){
    for(auto& worker:workers){
        worker->thread.interrupt();
    }
    for(auto& worker:workers){
        if(worker->thread.joinable())worker->thread.join();
    }
}

#pragma endregion

#pragma region ThreadPool


void IThreadPool::add_dependency(int task_id,int dep_task_id){
    taskmanager.add_dependency(task_id,dep_task_id);
}

void IThreadPool::deregister_task(int task_id){
    taskmanager.deregister_task(task_id);
}

size_t IThreadPool::get_min_threads()const{
    return min_threads;
}

size_t IThreadPool::get_max_threads()const{
    return max_threads;
}




#pragma endregion


#pragma region MonitorThread






#pragma endregion

}

