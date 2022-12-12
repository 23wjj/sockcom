//
// Created by User on 2022/9/16.
//

#include <iostream>
#include <cstdlib>
#include <vector>
#include <string>
#include <cstring>
#include <algorithm>
#include <sys/msg.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ipc.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "../include/message.h"
#include <pthread.h>
#include <time.h>
#include <unistd.h>

#define RESET "\033[0m"
#define BLACK "\033[30m"  /* Black */
#define RED "\033[31m"    /* Red */
#define GREEN "\033[32m"  /* Green */
#define YELLOW "\033[33m" /* Yellow */
#define BLUE "\033[34m"   /* Blue */
#define PURPLE "\033[35m" /* Purple */
#define CYAN "\033[36m"   /* Cyan */
#define WHITE "\033[37m"  /* White */

#define NORMAL (std::cout << RESET)
#define INFO (std::cout << GREEN)
#define WARN (std::cout << YELLOW)
#define ERROR (std::cout << RED)
#define DEBUG (std::cout << CYAN)
#define END (std::endl)
#define REND "\033[0m" << std::endl

#define MAXBUFFER 0x100

using namespace  std;

pthread_mutex_t mtx;

class client{
public:
    int socket_fd; // socket handler
    int connection;
    sockaddr_in serverAddr; // local information
    pthread_t interact_thread;
    client();
    void client_help();
    void client_connect(string);
    void quit();
    void get_time();
    void get_name();
    void get_cli();
    void send_message(int);
    void exitcli();
private:
    int msgqid; // identifier of the message queue
    static void* interact_handler(void*); // entry function for handling connection
};

client::client(){
    // initialize the socket handler
    socket_fd=-1;
    connection=0;

    // initialize the parameters of message queue for IPC
    key_t ipc_key = ftok("./",2022);
    msgqid=msgget(ipc_key, IPC_CREAT|0666);
}

void* client::interact_handler(void* sockfd) {
    MSG message;
    key_t ipc_key = ftok("./",2022);
    int msqid=msgget(ipc_key,IPC_CREAT|0666);
    int len;
    char buffer[MAXBUFFER];

    // waiting for the server
    while (1)
    {
        memset(buffer, 0, MAXBUFFER);
        len=recv(*((int*)sockfd), buffer, MAXBUFFER, 0);
        if(len){
            cout<<"[Receive] recv successful! "<<len<<" bytes recv\n";
            cout<<buffer+1<<"\n";
        }
        MSG message;
        message.mtype = buffer[0];
        strcpy(message.mtext, buffer + 1);
        msgsnd(msqid, &message, MAXBUFFER, 0);
    }
    return nullptr;

}

void client::client_help() {
    cout<<"------OPTIONS------\n"
        <<"[c] connect to [IP]:[port]\n"
        <<"[q] close connection\n"
        <<"[t] get time of the server\n"
        <<"[n] get name of the server\n"
        <<"[l] get active clients list\n"
        <<"[s] send messages [IP]:[port]:[message]\n"
        <<"[e] exit\n"
        <<"[h] help list\n"
        <<"------------------\n"
        <<"Please input your options\n";
}

void client::client_connect(string instruction){
    if(socket_fd!=-1){
        cout<<"[Warning] server has already connected!\n";
        return;
    }

    int len;
    if(instruction.find(':')!=string::npos){
        len=instruction.find(':')-1;
    }else{
        cout<<"[Error] please input the right IP!\n";
    }

    string IP=instruction.substr(1,len);
    int port=stoi(instruction.substr(len+1,instruction.length()-1-instruction.find(':')));

    // request for a socket under TCP protocol
    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(socket_fd<0){
        cout<<"[Error] create socket error!\n";
        socket_fd=-1;
        return;
    }

    // initialize local information
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.s_addr = inet_addr(IP.c_str());

    // connect to server
    int res = connect(socket_fd,(sockaddr*)&serverAddr,sizeof(serverAddr));
    if(res<0) {
        cout << "[Error] connection failed!\n";
        return;
    }
    connection=1;
    cout<<"[Success] connection has established!\n";
    pthread_create(&interact_thread, nullptr, interact_handler, &socket_fd);
}

void client::get_time() {
    if(!connection){
        cout<<"[Error]client hasn't connected!\n";
        return;
    }
    char buffer=TIME;
    MSG timemsg;
    send(socket_fd,&buffer, sizeof(buffer), 0);
    msgrcv(msgqid, &timemsg, MAXBUFFER, (long)TIME, 0);
    time_t time = atol(timemsg.mtext);
    cout<<"[Receive] Server time: "<<ctime(&time);
    return;
}

void client::get_name() {
    if(!connection){
        cout<<"[Error]client hasn't connected!\n";
        return;
    }
    char buffer=NAME;
    MSG namemsg;
    send(socket_fd,&buffer,sizeof(buffer),0);
    msgrcv(msgqid,&namemsg,MAXBUFFER,(long)NAME,0);
    cout<<"[Recive] Server name: "<<namemsg.mtext<<"\n";
    return;
}

void client::get_cli() {
    if(!connection){
        cout<<"[Error]client hasn't connected!\n";
        return;
    }
    char buffer=CLILIST;
    MSG climsg;
    send(socket_fd,&buffer,sizeof(buffer),0);
    msgrcv(msgqid,&climsg,MAXBUFFER,(long)CLILIST,0);

    // deal with the format of client list
}

    void client::quit() {
    if(!connection){
        cout<<"[Error]client hasn't connected!\n";
        return;
    }
    char buffer = CLOSE;
    send(socket_fd,&buffer,sizeof(buffer),0);
    pthread_mutex_lock(&mtx);
    pthread_cancel(interact_thread);
    pthread_mutex_unlock(&mtx);
    close(socket_fd);
    socket_fd=-1;
    connection=0;
    cout<<"[Info] socket: "<<socket_fd<<" closed!\n";
    return;
}

void client::exitcli() {
    if(!connection){
        cout<<"[Info] detect no connection!\n";
        cout<<"[Info] exit the client... Welcome to use next time!\n";
        exit(0);
        return;
    }
    char buffer=CLOSE;
    send(socket_fd,&buffer,sizeof(buffer),0);
    close(socket_fd);
    cout<<"[Info] socket: "<<socket_fd<<" closed!\n";
    cout<<"[Info] exit the client... Welcome to use next time!\n";
    exit(0);
    return;
}

void client::send_message(int) {

}

int main(){
    client cli;
    cout<<"client is running...\n";
    cli.client_help();
    string instruction;
    while(true){
        // output cmd prompt
        cout<<">";
        // get the instruction in
        getline(cin,instruction);
        // deal with the input, delete whitespace
        while(instruction[0]==' '){
            instruction.erase(instruction.begin());
        }
        switch (instruction[0]){
            case 'c':
                cli.client_connect(instruction);
                break;
            case 'q':
                cli.quit();
                break;
            case 't':
                cli.get_time();
                break;
            case 'n':
                cli.get_name();
                break;
            case 'l':
                cli.get_cli();
                break;
            case 's':
                // cli.send_message(instruction);
                break;
            case 'e':
                cli.exitcli();
                break;
            case 'h':
                cli.client_help();
            case '\n':
                continue;
            default:
                cout<<"[error]please input correct instruction, use [h] to see the option list\n";
        }
    }
}