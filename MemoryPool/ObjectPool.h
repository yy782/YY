#include "MemoryPool.h"
#include "../Common/noncopyable.h"
#include <list>
namespace yy 
{
template<typename T>
class ObjectPool : noncopyable {
public:
    /**
     * @brief 构造函数
     * @param capacity 初始容量（预分配数量）
     */
    explicit ObjectPool(size_t capacity = 0): 
    allocated_count_(0) 
    {
        assert(capacity);
        expand(capacity);
        
    }
    
    /**
     * @brief 析构函数，释放所有对象
     */
    ~ObjectPool() {
        clear();
    }
    
    /**
     * @brief 从池中获取一个对象
     * @param args 构造参数
     * @return 对象指针，失败返回 nullptr
     */
    template<typename... Args>
    T* acquire(Args&&... args) {
        T* obj = nullptr;
        
        if (!free_list_.empty()) {
            // 有空闲对象，直接复用
            obj = free_list_.front();
            free_list_.pop_front();
            
            // 在原地构造（placement new）
            new (obj) T(std::forward<Args>(args)...);
        } else {
            // 没有空闲对象，创建新对象
            obj = newElement<T>(std::forward<Args>(args)...);
            if (obj) {
                allocated_count_++;
            }
        }
        
        if (obj) {
            // 记录活跃对象（用于统计和调试）
            active_set_.insert(obj);
        }
        
        return obj;
    }
    
    /**
     * @brief 归还对象到池中
     * @param obj 要归还的对象指针
     */
    void release(T* obj) {
        if (!obj) return;
        
        // 从活跃集合中移除
        auto it = active_set_.find(obj);
        if (it != active_set_.end()) {
            active_set_.erase(it);
        }
        
        // 调用析构函数，但不释放内存
        obj->~T();
        
        // 放回空闲队列
        free_list_.push_back(obj);
    }
    
    /**
     * @brief 预分配指定数量的对象
     * @param count 要预分配的数量
     */
    void expand(size_t count) {
        for (size_t i = 0; i < count; ++i) {
            T* obj = newElement<T>();
            if (obj) {
                free_list_.push_back(obj);
                allocated_count_++;
            } else {
                break;
            }
        }
    }
    
    /**
     * @brief 收缩空闲池，释放多余的空闲对象
     * @param reserve 保留的空闲对象数量
     */
    void shrink(size_t reserve = 0) {
        while (free_list_.size() > reserve) {
            T* obj = free_list_.front();
            free_list_.pop_front();
            deleteElement(obj);
            allocated_count_--;
        }
    }
    
    /**
     * @brief 清空整个对象池（释放所有对象）
     */
    void clear() {
        // 释放所有空闲对象
        for (T* obj : free_list_) {
            deleteElement(obj);
        }
        free_list_.clear();
        
        // 释放所有活跃对象（注意：这通常不应该发生，说明有内存泄漏）
        for (T* obj : active_set_) {
            deleteElement(obj);
        }
        active_set_.clear();
        
        allocated_count_ = 0;
    }
    
    /**
     * @brief 获取空闲对象数量
     */
    size_t idleCount() const { return free_list_.size(); }
    
    /**
     * @brief 获取活跃对象数量
     */
    size_t activeCount() const { return active_set_.size(); }
    
    /**
     * @brief 获取总分配对象数量
     */
    size_t allocatedCount() const { return allocated_count_; }
    
    /**
     * @brief 打印统计信息
     */
    void printStats() const {
        printf("ObjectPool<%s> stats:\n", typeid(T).name());
        printf("  idle: %zu\n", idleCount());
        printf("  active: %zu\n", activeCount());
        printf("  allocated: %zu\n", allocatedCount());
    }
    
private:
    std::list<T*> free_list_;           // 空闲对象队列
    std::unordered_set<T*> active_set_; // 活跃对象集合（用于统计）
    size_t allocated_count_;            // 总分配数量
};
    
}
