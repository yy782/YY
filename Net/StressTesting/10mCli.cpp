// #include "../TcpClient.h"
// using namespace yy;
// using namespace yy::net;



// LTimerPtr ConnCreateAliveTimer(TcpConnectionPtr conn,EventLoop* loop)
// {
//         auto timer=std::make_shared<LTimer>([](){}, 10s, BaseTimer::FOREVER);
//         loop->runTimer<LowPrecision>([](){
            
//         },10min,BaseTimer::FOREVER);    
// }




// int main(int argc, char* argv[])
// {
//     Address addr(8080,true);
//     int MaxConNum=500;
//     int blockSize = 4096;
//     if(argc>1)
//     {
//         MaxConNum=std::atoi(argv[1]);
//         blockSize=std::atoi(argv[2]);
//     }
//     int pid = 1;
//     const int OneproFdNum=static_cast<int>(1e6);
//     int processes=MaxConNum/OneproFdNum;
//     for (int i = 0; i < processes; i++) {
//         pid = fork();
//         if (pid == 0) {  // a child process, break
//             sleep(1);
//             break;
//         }
//     }
//     if (pid == 0) {  // child process
//         char *buf = new char[blockSize];
        
//     }

        

//     return 0;
// }
int main()
{
    return 0;
}