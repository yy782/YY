#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <iostream>
using namespace std;

//./EchoClient
#define CHECK_ERROR(ret, msg) do{ \
    if(ret == -1) { perror(msg); exit(EXIT_FAILURE); } \
}while(0)

int main(int argc,char* argv[]){    
    

    string str="127.0.0.1";
    if(argc== 2) {
        str=argv[1];
    }   

    int cli_fd = socket(AF_INET, SOCK_STREAM, 0);
    CHECK_ERROR(cli_fd, "socket create failed");



    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(8080);  
    ssize_t ret = inet_pton(AF_INET,str.c_str(), &serv_addr.sin_addr);
    CHECK_ERROR(ret, "inet_pton failed");
    ret = connect(cli_fd, reinterpret_cast<struct sockaddr*>(&serv_addr), sizeof(serv_addr));
    CHECK_ERROR(ret, "connect failed");
    string line;
    while(std::getline(cin,line))
    {
        line += "\n";
        ret = send(cli_fd, line.c_str(), static_cast<size_t>(line.size()), 0);
        cout<<"发送: "<<line<<endl;
        CHECK_ERROR(ret, "send failed");
        char buf[1024];
        ret = recv(cli_fd, buf, static_cast<size_t>(sizeof(buf)), 0);
        buf[ret] = '\0';
        CHECK_ERROR(ret, "recv failed");
        cout <<"接收: "<<buf<< endl;
    }
                
    close(cli_fd);
    return 0;
}