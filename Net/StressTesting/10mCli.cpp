#include "../TcpClient.h"
using namespace yy;
using namespace yy::net;

int main(int argc, char* argv[])
{
    Address addr(8080,true);
    int MaxConNum=500;
    if(argc>1)
    {
        MaxConNum=std::atoi(argv[1]);
    }
    int pid = 1;
    int processes=MaxConNum/(static_cast<int>(1e6));
    for (int i = 0; i < processes; i++) {
        pid = fork();
        if (pid == 0) {  // a child process, break
            sleep(1);
            break;
        }
    }    

    return 0;
}