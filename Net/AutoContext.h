#ifndef YY_NET_AUTO_CONTEXT_H_
#define YY_NET_AUTO_CONTEXT_H_
#include <unordered_map>
#include <functional>
#include <typeindex>
namespace yy
{
namespace net 
{
class AutoContext 
{
private:
    struct ContextNode 
    {
        void* obj;
        std::function<void()> deleter;
    };
    
    std::unordered_map<std::type_index, ContextNode> contexts_;
public:
    template <typename T>
    T& context() 
    {
        auto it=contexts_.find(typeid(T));
        if (it==contexts_.end()) {
            ContextNode node;
            node.obj=new T();
            node.deleter=[this,p=node.obj](){delete static_cast<T*>(p);};
            auto result=contexts_.emplace(typeid(T),std::move(node));
            it=result.first;
        }
        return *static_cast<T*>(it->second.obj);
    }
    
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