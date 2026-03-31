#ifndef LOCKER_H
#define LOCKER_H
#include <exception>
#include <pthread.h>
#include <atomic>
#include <semaphore.h>
#include <stdexcept>
#include <exception>
#include <thread>
#include <condition_variable>
#include <future>
#include <functional>
#include "noncopyable.h"
#include <queue>
#include <assert.h>
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








class Thread:noncopyable 
{    
public:
    typedef std::thread::id Pid_t;
    typedef std::function<void()> Functor;
    Thread()=default;
    //void setFunctor(Functor cb){functor_=std::move(cb);}
    ~Thread(){ // @note 这是最后的保证，但是尽量在类的构析函数调用join()接口，而不是等他自然构析
        assert(!joinable()); // 要求上层必须自己join,清理资源 
    }
    void run(Functor cb){
        thread_=std::thread([func=std::move(cb)](){try{
            func();
        }
        catch(const std::exception& e)
        {
            assert(e.what());
        }});
    }   
    bool joinable()const noexcept{
        return thread_.joinable();
    }
    void join(){
        
        thread_.join();
       
        }
    void detach(){thread_.detach();}
    static bool isSelf(const Pid_t& pid) noexcept{
        return pid==std::this_thread::get_id();
    }
    Pid_t static getId() noexcept{return std::this_thread::get_id();}
private:
    std::thread thread_;
};

class FairMutex {
public:
    void lock() {
        std::unique_lock<std::mutex> lock(mtx_);
        if (!locked_) {
            locked_ = true;
            return;
        }
        // 自己入队，等待唤醒
        waiters_.emplace();
        waiters_.back().wait(lock);
    }

    void unlock() {
        std::lock_guard<std::mutex> lock(mtx_);
        if (waiters_.empty()) {
            locked_ = false;
            return;
        }
        // 唤醒队首（先阻塞的线程）
        waiters_.front().notify_one();
        waiters_.pop();
    }
    
private:
    std::mutex mtx_;
    bool locked_{false};
    std::queue<std::condition_variable> waiters_;
};
}


#endif