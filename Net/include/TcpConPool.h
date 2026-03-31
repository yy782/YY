/**
 * @file TcpConPool.h
 * @brief TCP连接池的定义
 * 
 * 本文件定义了TCP连接池类，用于管理TCP连接对象的生命周期，
 * 实现连接的复用，提高性能。
 */

#ifndef YY_NET_TCP_CON_POOL_H_
#define YY_NET_TCP_CON_POOL_H_

#include "TcpConnection.h"
#include <algorithm>
#include "../Common/locker.h"
using namespace yy::net;
namespace yy 
{
template<typename T>
class ObjectPool;
template<>
class ObjectPool<TcpConnectionPtr>;
/**
 * @brief TCP连接池类型
 */
typedef class ObjectPool<TcpConnectionPtr> TcpConPool;

/**
 * @brief 连接池扩展数量
 */
const int ExpendNum=1000;

/**
 * @brief TCP连接池类
 * 
 * 用于管理TCP连接对象的生命周期，实现连接的复用，提高性能。
 */
template<>
class ObjectPool<TcpConnectionPtr>: noncopyable 
{
public:
    /**
     * @brief 线程ID类型
     */
    typedef Thread::Pid_t Pid_t;
    /**
     * @brief 连接池配置结构体
     */
    struct Config:copyable
    {
        typedef TcpConnection::ServicesMessageCallBack ServicesMessageCallBack;
        typedef TcpConnection::ServicesExceptCallBack ServicesExceptCallBack;
        typedef TcpConnection::ServicesCloseCallBack ServicesCloseCallBack;
        typedef TcpConnection::ServicesErrorCallBack ServicesErrorCallBack;
        typedef TcpConnection::ServicesData ServicesData;        
        /**
         * @brief 事件类型
         */
        Event event_;
        /**
         * @brief 是否启用TCP保活
         */
        bool isTcpAlive_;
        /**
         * @brief 消息回调函数
         */
        ServicesMessageCallBack SmessageCallBack_;
        /**
         * @brief 关闭回调函数
         */
        ServicesCloseCallBack ScloseCallBack_;
        /**
         * @brief 错误回调函数
         */
        ServicesErrorCallBack SerrorCallBack_;
        /**
         * @brief 异常回调函数
         */
        ServicesExceptCallBack SexceptCallBack_;  
        /**
         * @brief 服务数据
         */
        ServicesData data_;
    };
    /**
     * @brief 构造函数
     * 
     * @param num 初始连接数量
     * @param config 连接池配置
     * @param id 连接池ID
     */
    ObjectPool(int num,struct Config& config,int id=-1):
    config_(config),
    id_(id)
    {
        expend(num);
    }
    /**
     * @brief 扩展连接池
     * 
     * @param num 扩展数量
     */
    void expend(int num)
    {
        for(int i=0;i<num;++i)
        {
            auto conn=std::make_shared<TcpConnection>(config_.event_);
            if(config_.isTcpAlive_)conn->setTcpAlive(true);
            conn->setMessageCallBack(config_.SmessageCallBack_);
            conn->setCloseCallBack(config_.ScloseCallBack_);
            conn->setExceptCallBack(config_.SexceptCallBack_);
            conn->setErrorCallBack(config_.SerrorCallBack_);
            conn->date()=config_.data_;
            free_list_.push_back(conn);
        }
    }
    /**
     * @brief 获取连接
     * 
     * @param fd 文件描述符
     * @param addr 对端地址
     * @param loop 事件循环
     * @return TcpConnectionPtr TCP连接对象
     */
    TcpConnectionPtr acquire(int fd,const Address& addr,EventLoop* loop)
    {
        assert(loop->id()==id_);
        assert(fd>0);
        if(free_list_.empty())
        {
            expend(ExpendNum);
        }
        TcpConnectionPtr conn=free_list_.front();
        free_list_.pop_front();
        assert(conn);
        conn->init(fd,addr,loop); 
        return conn;
    }
    /**
     * @brief 释放连接
     * 
     * @param conn 要释放的连接
     */
    void release(TcpConnectionPtr conn)
    {
        assert(conn->loop()->id()==id_);
        assert(conn);
        conn->reset();
        free_list_.push_back(conn);
    }
private:
    /**
     * @brief 连接池配置
     */
    Config config_;// 需外部强行保证
    /**
     * @brief 空闲连接列表
     */
    std::list<TcpConnectionPtr> free_list_;
    /**
     * @brief 连接池ID
     */
    int id_={-1};

};
}

#endif // YY_NET_TCP_CON_POOL_H_