#include "../net.h"
using namespace yy;
using namespace yy::net;
#include <iostream>
using namespace std;


// ./test111 127.0.0.1 8080
int main(int argc, char* argv[])
{
    int port;
    string ip;
    if(argc<3)
    {
        port=8080;
        ip="127.0.0.1";
    }
    else
    {
        port=atoi(argv[2]);
        ip=argv[1];
    }
    Address addr(ip.c_str(),port);
    cout<<addr.sockaddrToString()<<endl;   
}