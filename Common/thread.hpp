#ifndef __SYLAR_THREAD_H_
#define __SYLAR_THREAD_H_

#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>
#include <functional>
#include <iostream>
#include <memory>
#include <thread>
#include <yy/Common/noncopyable.h>
#include "utils.hpp"
namespace monsoon {

class Thread;
inline static thread_local Thread *cur_thread = nullptr;
inline static thread_local std::string cur_thread_name = "UNKNOW";

class Thread : yy::noncopyable {
public:
    typedef std::shared_ptr<Thread> ptr;
    typedef pid_t Pid_t;
    typedef std::function<void()> Functor;

    Thread(const std::string &name = "UNKNOW") : name_(name) {
        CondPanic(name_.size() < 16);
    }
    Thread(Functor cb, const std::string &name = "UNKNOW") : cb_(cb), name_(name) {
        CondPanic(name_.size() < 16);
        int rt = pthread_create(&thread_, nullptr, &Thread::run, this);
        if (rt) {
            std::cout << "pthread_create error,name:" << name_ << std::endl;
            throw std::logic_error("pthread_create");
        }
    }

    ~Thread() {
        if (thread_) {
            pthread_detach(thread_);
        }
    }

    // 非静态函数全部留在类里面不动
    void join() {
        if (thread_) {
            int rt = pthread_join(thread_, nullptr);
            if (rt) {
                std::cout << "pthread_join error,name:" << name_ << std::endl;
                throw std::logic_error("pthread_join");
            }
            thread_ = 0;
        }
    }

    void detach() {
        if (thread_) {
            int rt = pthread_detach(thread_);
            if (rt) {
                std::cout << "pthread_detach error,name:" << name_ << std::endl;
                throw std::logic_error("pthread_detach");
            }
            thread_ = 0;
        }
    }

    bool joinable() {
        return thread_ != 0;
    }

    const std::string &Name() const {
        return name_;
    }

    void run(Functor cb) {
        cb_ = cb;
        int rt = pthread_create(&thread_, nullptr, &Thread::run, this);
        if (rt) {
            std::cout << "pthread_create error,name:" << name_ << std::endl;
            throw std::logic_error("pthread_create");
        }
    }
  //static const std::string &GetName() { return cur_thread_name; }
  // static void SetName(const std::string &name){
  //   if (name.empty()) {
  //     return;
  //   }
  //   if (cur_thread) {
  //     cur_thread->name_ = name;
  //   }
  //   cur_thread_name = name;
  // }
    static Thread *GetThis();
    static bool isSelf(const Pid_t &pid) noexcept;
    static Pid_t getId() noexcept;
    static void *run(void *args);

private:
    Thread(const Thread &&) = delete;

    Pid_t id_;
    pthread_t thread_ = 0;
    Functor cb_;
    std::string name_;
};

inline Thread *Thread::GetThis() {
    return cur_thread;
}

inline bool Thread::isSelf(const Pid_t &pid) noexcept {
    return pid == gettid();
}

inline Thread::Pid_t Thread::getId() noexcept {
    return gettid();
}

inline void *Thread::run(void *args) {
    Thread *thread = static_cast<Thread *>(args);
    cur_thread = thread;
    cur_thread_name = thread->name_;
    thread->id_ = Thread::getId();
    pthread_setname_np(pthread_self(), thread->name_.substr(0, 15).c_str());
    Functor cb;
    cb.swap(thread->cb_);
    cb();
    return nullptr;
}

}  // namespace monsoon


#endif