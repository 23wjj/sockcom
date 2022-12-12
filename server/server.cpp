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
typedef pair<int,ip_port> fd_ip_port; // conn_fd  ip  port

pthread_mutex_t mtx;

class server{
public:
    int socket_fd;
    sockaddr_in serveraddr;
    void run();
    server();

private:
    vector<fd_ip_port> cli_list;
    static void* interact_handler(void*);
};

server::server(){
    // create socket
    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(4880);
    // listening to all the clients connected
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    bind(socket_fd, (sockaddr*)&serveraddr, sizeof(serveraddr));
    listen(socket_fd,MaxConnection);
}

void* server::interact_handler(void* connfd){
    // send hello message to the client
    char str[]="hello\n";
    send(*((int*)connfd),str,strlen(str),0);

    // create buffer to store request message from the client
    char buffer[MaxBuffer];
    // create buffer to store reply message to the client
    char msg[MaxBuffer];

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
                // send the answer to the client
                send(*((int*)connfd),msg,strlen(msg),0);
                break;
            }
            case NAME: {

            }
        }
    }

    pthread_mutex_unlock(&mtx);
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
        cli_list.push_back(fd_ip_port(conn_fd, ip_port(inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port))));

        cout<<"[Accept] "<<inet_ntoa(client_addr.sin_addr)<<":"<<ntohs(client_addr.sin_port)<<" connected.\n";

        // create a thread to interact with the client's request
        pthread_t interact_thread;
        pthread_create(&interact_thread, nullptr, interact_handler, &conn_fd);
    }
}

int main(){
    ios::sync_with_stdio(false);
    server serv;
    serv.run();
}