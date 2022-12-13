//
// Created by User on 2022/9/16.
//
#include <iostream>
#include <vector>
#include <algorithm>
#include <string>
#include <cstring>
#include <cstdio>
#include <sys/msg.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <arpa/inet.h>
#include "../include/message.h"
#include "time.h"
#include <unistd.h>

#define MaxConnection 10
#define MaxBuffer 0x100
using namespace  std;

typedef pair<string ,int> ip_port;
typedef pair<int,ip_port> fd_ip_port; // sock_fd  ip  port

pthread_mutex_t mtx;

class server{
public:
    int socket_fd;
    sockaddr_in serveraddr;
    void run();
    server();
    ~server();

private:
    static void* interact_handler(void*);
};

vector<fd_ip_port> cli_list;

server::server(){
    cout<<"[Info] create socket for server\n";
    // create socket
    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(4880);
    // listening to all the clients connected
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    // bind the listening port to the socket handler
    bind(socket_fd, (sockaddr*)&serveraddr, sizeof(serveraddr));
    // set the length of waiting queue to MaxConnection
    listen(socket_fd,MaxConnection);
}

server::~server()
{
    close(socket_fd);
}

// thread to send and recv information with the client through socket handler
void* server::interact_handler(void* connfd){
    // send hello message to the client
    char str[]="thello\n";
    str[0]=GREET;
    send(*((int*)connfd),str,strlen(str),0);

    // create buffer to store request message from the client
    char buffer[MaxBuffer];
    // create buffer to store reply message to the client
    char msg[MaxBuffer];

    // call receive to recv data from client
    while(1){
        // clear the buffer
        memset(msg,0,MaxBuffer);
        memset(buffer,0,MaxBuffer);

        recv(*((int*)connfd), buffer, MaxBuffer, 0);

        pthread_mutex_lock(&mtx);
        char msg_type = buffer[0];

        switch(msg_type){
            case TIME: {
                // get the time of the server
                time_t t;
                time(&t);
                cout<<"[Request] "<<*(int*)connfd<<" get the time\n";
                // construct the reply message
                msg[0]=TIME;
                sprintf(msg+strlen(msg), "%ld", t);
                // send the reply to the client
                send(*((int*)connfd),msg,strlen(msg),0);
                break;
            }
            case NAME: {
                // get host name
                msg[0]=NAME;
                gethostname(msg+strlen(msg),sizeof(msg)-sizeof(char));
                cout<<"[Request] "<<*(int*)connfd<<" get the host name\n";
                // send the reply to the client
                send(*((int*)connfd),msg,strlen(msg),0);
                break;
            }
            case CLILIST: {
                // get the client list
                msg[0]=CLILIST;
                cout<<"[Request] "<<*(int*)connfd<<" get the client list\n";
                for(auto ite=cli_list.begin();ite!=cli_list.end();ite++){
                    // get the ip
                    sprintf(msg+strlen(msg),"%s",ite->second.first.c_str());
                    sprintf(msg+strlen(msg),"|");
                    // get the port
                    sprintf(msg+strlen(msg),"%d",ite->second.second);
                    // end one client info
                    sprintf(msg+strlen(msg),"#");
                }
                // send the reply to the client
                send(*((int*)connfd),msg,strlen(msg),0);
                break;
            }
            case SEND: {
                // copy the received info except the msg_type
                string recvmsg(buffer+1);
                // check the IP and port
                string ip=recvmsg.substr(0,recvmsg.find("#"));
                int port=atoi(recvmsg.substr(recvmsg.find("#")+1,recvmsg.find("$")-recvmsg.find("#")-1).c_str());
                string msgcont=recvmsg.substr(recvmsg.find("$")+1);
                cout<<"[Request] "<<*(int*)connfd<<" send message to client "<<ip<<":"<<port<<"\n";
                int sock_fd=-1; // to store the target client's socket
                // find the target client in the client_list
                cout<<ip<<":"<<port<<endl;
                for(auto ite=cli_list.begin();ite!=cli_list.end();ite++){
                    if(ite->second.first==ip&&ite->second.second==port){
                        sock_fd=ite->first;
                        break;
                    }
                }
                msg[0]=SEND;
                // if cannot find the target client
                if(sock_fd==-1){
                    cout<<"[Error] target client not in the client list!\n";
                    // send the error back to the client
                    sprintf(msg+strlen(msg),"Fail.\n");
                    send(*((int*)connfd),msg,strlen(msg),0);
                    break;
                }
                // retransmit the data to the target client
                char send_msg[MaxBuffer]={0};
                send_msg[0]=SEND;
                sprintf(send_msg+strlen(send_msg),"%s",msgcont.c_str());
                send(sock_fd,send_msg,strlen(send_msg),0);
                // send information to the source client
                sprintf(msg+strlen(msg),"%s","Success.\n");
                send(*((int*)connfd),msg,strlen(msg),0);
            }
            default:
                break;
        }
        pthread_mutex_unlock(&mtx);
    }

}

void server::run(){
    cout<<"[Info] server is running...\n";
    cout<<"[Info] listening to the client...\n";
    // waiting for clients to connect
    while(1){
        // catch the client to connected
        sockaddr_in client_addr;
        socklen_t cliaddr_len=sizeof(client_addr);

        // accept connection fd
        int conn_fd;
        conn_fd=accept(socket_fd,(sockaddr*)&client_addr,&cliaddr_len);
        // accept error
        if(conn_fd<0){
            cout<<"[Error] accept socket error!\n";
            continue;
        }
        // add the client to the list
        cli_list.push_back(fd_ip_port(conn_fd, ip_port(inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port))));

        cout<<"[Accept] "<<inet_ntoa(client_addr.sin_addr)<<":"<<ntohs(client_addr.sin_port)<<" connected.\n";

        // create a thread to interact with the client's request
        pthread_t interact_thread;
        // pass the socket handler to the child thread
        pthread_create(&interact_thread, nullptr, interact_handler, &conn_fd);
    }
}

int main(){
    // ban the storing buffer for input and output
    // ios::sync_with_stdio(false);
    // initialize a server
    cout<<"[Info] initialize a server\n";
    server serv;
    serv.run();
}