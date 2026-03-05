#include "EchoClient.h"
// ./EchoClient
int main(int argc,char* argv[])
{    string port;
    if(argc!=2)
    {
        port="8080";
    }
    else
    {
        port=argv[1];
    }
    Address serverAddr("127.0.0.1",std::stoi(port));
    cout<<serverAddr.sockaddrToString()<<endl;
    EchoClient client(serverAddr);
    client.connect();
    std::string line;
    while(getline(cin,line))
    {
        line+='\n';
        client.send(line);
    }
    return 0;
}