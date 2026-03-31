#ifndef YY_NET_AUTO_CONTEXT_H_
#define YY_NET_AUTO_CONTEXT_H_
#include <unordered_map>
#include <functional>
#include <typeindex>
namespace yy
{
namespace net 
{

/**
 * @file AutoContext.h
 * @brief 自动上下文类的定义
 * 
 * 本文件定义了自动上下文类，用于管理各种类型的上下文对象。
 */

/**
 * @brief 自动上下文类
 * 
 * 用于管理各种类型的上下文对象，自动处理对象的生命周期。
 */
class AutoContext 
{
private:
    /**
     * @brief 上下文节点结构体
     */
    struct ContextNode 
    {
        void* obj;               /**< 对象指针 */
        std::function<void()> deleter;  /**< 删除函数 */
    };
    
    /**
     * @brief 上下文映射
     */
    std::unordered_map<std::type_index,ContextNode> contexts_;
public:
    /**
     * @brief 获取上下文
     * 
     * @tparam T 上下文类型
     * @return T& 上下文引用
     */
    template <typename T>
    T& context() 
    {
        auto it=contexts_.find(typeid(T));
        if (it==contexts_.end()) 
        {
            ContextNode node;
            node.obj=new T();
            node.deleter=[this,p=node.obj](){delete static_cast<T*>(p);};
            auto result=contexts_.emplace(typeid(T),std::move(node));
            it=result.first;
        }
        return *static_cast<T*>(it->second.obj);
    }
    
    /**
     * @brief 析构函数
     * 
     * 自动调用所有上下文对象的删除函数。
     */
    ~AutoContext() 
    {
        for(auto& pair : contexts_) 
        {
            pair.second.deleter();  // 调用对应的删除函数
        }
    }
};    
}    
}
#endif