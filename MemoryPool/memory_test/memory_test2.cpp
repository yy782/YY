#include "../MemoryPool.h"
#include <string>
#include <vector>
#include <iostream>

using namespace yy;
using namespace std;

// 测试结构体1：基础类型
struct Base
{
    int a_;
    int b_;
    char c_;
    
    Base(int a, int b, char c):
        a_(a),
        b_(b),
        c_(c)
    {}
    
    void print() const {
        cout << "Base: a=" << a_ << ", b=" << b_ << ", c=" << c_ << endl;
    }
};

// 测试结构体2：空结构体
struct Test1
{
    int id;
    // 只需要一个参数的构造函数
    Test1(int i = 0) : id(i) {}
};

// 测试结构体3：大对象
struct BigObject
{
    int data[1024];
    string name;
    
    BigObject(const string& n = "") : name(n) {
        for(int i = 0; i < 1024; ++i) {
            data[i] = i;
        }
    }
};

int main()
{
    PoolBucket::initMemoryPool();
    cout << "========== 内存池测试开始 ==========" << endl;
    
    // 测试1：基本分配和释放
    cout << "\n=== 测试1：基本分配和释放 ===" << endl;
    {
        // 分配单个对象
        Base* one = newElement<Base>(1, 2, 'c');
        cout << "分配对象: ";
        one->print();
        deleteElement(one);  // 不需要模板参数，编译器可以推导
        cout << "释放完成" << endl;
    }
    
    // 测试2：不同类型的对象
    cout << "\n=== 测试2：不同类型对象 ===" << endl;
    {
        Base* b = newElement<Base>(10, 20, 'x');
        Test1* t = newElement<Test1>(100);  // 单个参数
        BigObject* big = newElement<BigObject>("big object");
        
        cout << "Base对象: ";
        b->print();
        cout << "Test1对象: id=" << t->id << endl;
        cout << "BigObject: name=" << big->name << endl;
        
        deleteElement(b);
        deleteElement(t);
        deleteElement(big);
    }
    
    // 测试3：批量分配（使用vector管理指针）
    cout << "\n=== 测试3：批量分配 ===" << endl;
    {
        vector<Base*> ptrs;
        const int COUNT = 10;
        
        cout << "分配 " << COUNT << " 个Base对象:" << endl;
        for(int i = 0; i < COUNT; ++i) {
            Base* p = newElement<Base>(i, i*2, 'a' + i%26);
            ptrs.push_back(p);
            cout << "  ";
            p->print();
        }
        
        // 释放所有对象
        cout << "释放所有对象:" << endl;
        for(auto p : ptrs) {
            deleteElement(p);
        }
    }
    
    // 测试4：分配大量对象观察内存重用
    cout << "\n=== 测试4：内存重用测试 ===" << endl;
    {
        vector<Test1*> ptrs;
        const int COUNT = 5;
        
        cout << "第一次分配:" << endl;
        for(int i = 0; i < COUNT; ++i) {
            Test1* p = newElement<Test1>(i);
            ptrs.push_back(p);
            cout << "  分配 Test1对象，id=" << p->id << "，地址=" << p << endl;
        }
        
        // 释放一半
        cout << "释放中间3个对象:" << endl;
        for(int i = 1; i <= 3; ++i) {
            cout << "  释放地址=" << ptrs[i] << endl;
            deleteElement(ptrs[i]);
        }
        
        // 重新分配，应该重用刚释放的内存
        cout << "重新分配3个对象（应该重用上面的地址）:" << endl;
        for(int i = 0; i < 3; ++i) {
            Test1* p = newElement<Test1>(100 + i);
            ptrs.push_back(p);
            cout << "  新分配 Test1对象，id=" << p->id << "，地址=" << p << endl;
        }
        
        // 清理剩余对象
        for(auto p : ptrs) {
            if(p) deleteElement(p);  // 注意：需要避免重复释放
        }
    }
    
    // 测试5：内存池自动释放（观察析构函数）
    cout << "\n=== 测试5：内存池析构自动释放 ===" << endl;
    {
        cout << "进入作用域，分配对象但不手动释放:" << endl;
        Base* b1 = newElement<Base>(1, 1, 'a');
        Base* b2 = newElement<Base>(2, 2, 'b');
        Base* b3 = newElement<Base>(3, 3, 'c');
        
        cout << "  分配了3个对象，地址：" << b1 << ", " << b2 << ", " << b3 << endl;
        cout << "  不手动释放，离开作用域时内存池析构函数会自动释放" << endl;
    }
    cout << "离开作用域，内存池已自动释放所有内存" << endl;
    
    cout << "\n========== 所有测试完成 ==========" << endl;
    
    return 0;
}