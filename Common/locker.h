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
#include <functional>
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








class Thread {

    
public:
    typedef std::function<void()> Functor;
    Thread()=default;
    //void setFunctor(Functor cb){functor_=std::move(cb);}
    ~Thread(){
        if(joinable()){
            join();
        }
    }
    void run(Functor cb){
        try{
            cb();
        }
        catch(...)
        {

        }
    }   
    bool joinable()const noexcept{
        return internal_thread.joinable();
    }
    void join(){internal_thread.join();}
    void detach(){internal_thread.detach();}
private:
    std::thread internal_thread;
};
}


#endif