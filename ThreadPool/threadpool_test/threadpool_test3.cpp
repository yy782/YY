#include "../ThreadPool.h"
using namespace yy;
int main()
{
    auto& pool1=ThreadPool<FSFC>::getInstance(2,4);
    auto& pool2=ThreadPool<Priority>::getInstance(2,4);


    auto task=[](){
        volatile int sum=0;
        for(int i=0;i<1000000;++i){
            sum+=i;
        }
    };
    std::vector<std::shared_future<yy::TaskResult<void>>> results;
    for(int i=0;i<10;++i)
    {
        results.push_back(pool1.submit([task](){
            task();
            std::cout<<"Task executed by pool1"<<std::endl;
        }));
        results.push_back(pool1.submit([task](){
            task();
            std::cout<<"Important Task executed by pool1"<<std::endl;
        }));

        results.push_back(pool2.submit([task](){
            task();
            std::cout<<"Task executed by pool2"<<std::endl;
        },4));
        results.push_back(pool2.submit([task](){
            task();
            std::cout<<"Important Task executed by pool2"<<std::endl;
        },1));        
    }
    for(auto& r:results)
    {
        r.wait();
    }
    return 0;
}