#include "../net.h"
using namespace std;
using namespace yy;
using namespace yy::net;
// ./AddressTest
int main()
{

    Address addr(8080,true);
    cout<<addr.sockaddrToString()<<endl;
    Address addr2("127.0.0.1",8080);
    cout<<addr2.sockaddrToString()<<endl;
}