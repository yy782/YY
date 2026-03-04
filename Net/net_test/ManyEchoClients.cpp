#include "EchoClient.h"
const size_t Num=1000;

// ./ManyEchoClients
int main()
{
    Address addr=Address(8080,true);
    vector<unique_ptr<EchoClient>> clients;
    clients.reserve(Num*2);
    for(size_t i=0;i<Num;++i)
    {
        clients.push_back(std::make_unique<EchoClient>(addr));
    }
    for(size_t i=0;i<Num;++i)
    {
        clients[i]->connect();
    }
    for(size_t i=0;i<Num;++i)
    {
        for(size_t j=1;j<=100;++j)
        {
            string msg="第"+to_string(i)+"条消息\n";
            clients[i]->send(msg.c_str());            
        }
    }
    for(size_t i=0;i<Num;++i)
    {
        clients[i]->disconnect();
    }

    return 0;
}