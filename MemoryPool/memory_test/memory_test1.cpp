#include "../MemoryPool.h"
#include <string>
#include <vector>

using namespace yy;
using namespace std;
struct Base
{
    int a_;
    int b_;
    char c_;
    Base(int a,int b,char c):
    a_(a),
    b_(b),
    c_(c)
    {}
};

struct Test1
{
    
};



int main()
{
    Base* one=newElement<Base>(1,2,'c');
    deleteElement<Base>(one);
    return 0;
}