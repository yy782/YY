#ifndef LOCKER_H
#define LOCKER_H
#include <exception>
#include <pthread.h>
#include <atomic>
#include <semaphore.h>
#include <stdexcept>
#include <thread>
#include <condition_variable>
#include <future>
namespace yy{
class sem{
public:
    sem(){
        if(sem_init(&m_sem,0,0)!=0){
            throw std::runtime_error("sem init error");
        }
    }
    ~sem(){
        sem_destroy(&m_sem);
    }
    bool wait(){
        return sem_wait(&m_sem)==0;
    }
    bool post(){
        return sem_post(&m_sem)==0;
    }
private:
    sem_t m_sem;    
};

class locker{
public:
    locker(){
        if(pthread_mutex_init(&m_mutex,NULL)!=0){
            throw std::runtime_error("mutex init error");
        }
    }
    ~locker(){
        pthread_mutex_destroy(&m_mutex);
    }
    bool lock(){
        return pthread_mutex_lock(&m_mutex)==0;
    }
    bool unlock(){
        return pthread_mutex_unlock(&m_mutex)==0;
    }
    pthread_mutex_t* get(){
        return &m_mutex;
    }
private:
    pthread_mutex_t m_mutex;
};

class cond{
public:
    cond(){
        if(pthread_mutex_init(&m_mutex,NULL)!=0){
            throw std::runtime_error("cond mutex init error");
        }
        if(pthread_cond_init(&m_cond,NULL)!=0){
            pthread_mutex_destroy(&m_mutex);
            throw std::runtime_error("cond init error");
        }
    }
    ~cond(){
        pthread_mutex_destroy(&m_mutex);
        pthread_cond_destroy(&m_cond);
    }
    bool wait(){
        int ret=0;
        pthread_mutex_lock(&m_mutex);
        ret=pthread_cond_wait(&m_cond,&m_mutex);
        pthread_mutex_unlock(&m_mutex);
        return ret==0;
    }
    bool signal(){
        return pthread_cond_signal(&m_cond)==0;
    }
    bool broadcast(){
        return pthread_cond_broadcast(&m_cond)==0;
    }
    pthread_cond_t* get(){
        return &m_cond;
    }
private:
    pthread_mutex_t m_mutex;
    pthread_cond_t m_cond;
};





class interrupt_flag{
private:
    std::atomic<bool> flag_{false};
    std::mutex mutex2_;
    std::condition_variable* cond2_{nullptr};
public:
    interrupt_flag()=default;
    ~interrupt_flag()=default;
    void set(){
        flag_.store(true, std::memory_order_release);
        std::lock_guard<std::mutex> lock(mutex2_);
        if(cond2_){
            cond2_->notify_all();
        }        
    }
    bool is_set()const{
         return flag_.load(std::memory_order_acquire);
    }
    void set_condition_variable(std::condition_variable& cv){
            std::lock_guard<std::mutex> lock(mutex2_);
            cond2_ = &cv;
    }

    void clear_condition_variable(){
        std::lock_guard<std::mutex> lock(mutex2_);
        cond2_ = nullptr;
    }
};
struct thread_interrupted:std::runtime_error{thread_interrupted():std::runtime_error("Thread interrupted"){}};



class interruptible_thread {
private:
    std::thread internal_thread;
    interrupt_flag this_thread_interrupt_flag;
public:
    template<typename FunctionType>
    interruptible_thread(FunctionType&& f){
        internal_thread = std::thread([this, f = std::forward<FunctionType>(f)]() mutable {
            
            f();
        });
    }
    template<typename FunctionType>
    interruptible_thread& operator=(FunctionType&& f){
        internal_thread=std::thread([this, f = std::forward<FunctionType>(f)]() mutable {
            f();
        });  
        return *this;
    }
    interruptible_thread()=default;
    void interrupt(){
        if(internal_thread.joinable()){
            this_thread_interrupt_flag.set();            
        }
    }
    void interruption_point(){
        if(this_thread_interrupt_flag.is_set()){
            throw thread_interrupted();
        }
    }    
    bool joinable()const noexcept{
        return internal_thread.joinable();
    }
    void join(){internal_thread.join();}
    void detach(){internal_thread.detach();}

    ~interruptible_thread(){
        if(joinable()){
            join();
        }
    }
};
}


#endif