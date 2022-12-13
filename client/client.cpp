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
    void send_message(string);
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
    // create buffer to store receive message from the server
    char buffer[MAXBUFFER];

    // waiting for the server
    while (1)
    {
        // clear the buffer
        memset(buffer, 0, MAXBUFFER);
        len=recv(*((int*)sockfd), buffer, MAXBUFFER, 0);
        //receive bytes from server
        if(len){
            cout<<"[Receive] recv successful! "<<len<<" bytes recv\n";
            if(buffer[0]==TRANS)
                cout<<"[Server] ";
            cout<<buffer+1<<"\n";
        }
        MSG message;
        message.mtype = buffer[0];
        strcpy(message.mtext, buffer + 1);
        if(message.mtype!=TRANS)
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
        <<"[s] send messages [IP]:[port]:[message]\n"//IP # port $ message
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
    if(instruction.find(' ')!=string::npos){
        len=instruction.find(':')-1;
    }else{
        cout<<"[Error] please input the right IP!\n";
        return;
    }

    string IP=instruction.substr(2,len-1);
    int port=stoi(instruction.substr(len+2,instruction.length()-1-instruction.find(' ')));

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
    // cout<<port<<endl;
    // cout<<IP.c_str()<<endl;
    serverAddr.sin_addr.s_addr = inet_addr(IP.c_str());

    // connect to server
    int res = connect(socket_fd,(sockaddr*)&serverAddr,sizeof(serverAddr));
    if(res<0) {
        cout << "[Error] connection failed!\n";
        socket_fd=-1;
        return;
    }
    connection=1;
    cout<<"[Success] connection has established!\n";
    // create a thread to interact with the server
    // pass the socket handler to the child thread
    pthread_create(&interact_thread, nullptr, interact_handler, &socket_fd);
}

void client::get_time() {
    if(!connection){
        cout<<"[Error]client hasn't connected!\n";
        return;
    }
    // create buffer to store message send to the server
    char buffer=TIME;
    MSG timemsg;
    //send message to server
    send(socket_fd,&buffer, sizeof(buffer), 0);
    //receive message from server
    msgrcv(msgqid, &timemsg, MAXBUFFER, (long)TIME, 0);
    time_t time = atol(timemsg.mtext);
    //print time
    cout<<"[Receive] Server time: "<<ctime(&time);
    return;
}

void client::get_name() {
    if(!connection){
        cout<<"[Error]client hasn't connected!\n";
        return;
    }
    // create buffer to store message send to the server
    char buffer=NAME;
    MSG namemsg;
    //send message to server
    send(socket_fd,&buffer,sizeof(buffer),0);
    //receive message from server
    msgrcv(msgqid,&namemsg,MAXBUFFER,(long)NAME,0);
    //print time
    cout<<"[Recive] Server name: "<<namemsg.mtext<<"\n";
    return;
}

void client::get_cli() {
    if(!connection){
        cout<<"[Error]client hasn't connected!\n";
        return;
    }
    // create buffer to store message send to the server
    char buffer=CLILIST;
    MSG climsg;
    //send message to server
    send(socket_fd,&buffer,sizeof(buffer),0);
    //receive message from server
    msgrcv(msgqid,&climsg,MAXBUFFER,(long)CLILIST,0);
    // create buffer to store message send to the server
    string list(climsg.mtext);
    cout<<"[Info] Client list: "<<"\n";
    string temp;
    //print list of client
    while(list.find('#')!=string::npos){
        temp=list.substr(0,list.find('#'));
        replace(temp.begin(),temp.end(),'|',':');
        cout<<temp<<"\n";
        list=list.substr(list.find('#')+1,list.length()-1-list.find('#'));
    }
}

void client::quit() {
    if(!connection){
        cout<<"[Error]client hasn't connected!\n";
        return;
    }
    // create buffer to store message send to the server
    char buffer = CLOSE;
    //send message to server
    send(socket_fd,&buffer,sizeof(buffer),0);
    //lock 
    pthread_mutex_lock(&mtx);
    pthread_cancel(interact_thread);
    //unlock
    pthread_mutex_unlock(&mtx);
    //clear socket_fd and connection
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
    // create buffer to store message send to the server
    char buffer=CLOSE;
    //send message to server
    send(socket_fd,&buffer,sizeof(buffer),0);
    close(socket_fd);
    cout<<"[Info] socket: "<<socket_fd<<" closed!\n";
    cout<<"[Info] exit the client... Welcome to use next time!\n";
    exit(0);
    return;
}

void client::send_message(string instruction) {
    if(socket_fd==-1){
        cout<<"[Error] server does not connected!\n";
        return;
    }
    //turn the instruction into sending message
    int len;
    if(instruction.find(' ')==string::npos){
        cout<<"[Error] please input the right instruction!\n";
        return;
    }
    instruction.replace(instruction.find(":"),1,"#");
    if(instruction.find(' ')==string::npos){
        cout<<"[Error] please input the right instruction!\n";
        return;
    }
    instruction.replace(instruction.find(":"),1,"$");
    string tmp=instruction.substr(2,instruction.length()-2);
    cout<<tmp;
    // create buffer to store message send to the server
    char buffer[MAXBUFFER] = {0};
    buffer[0] = SEND;
    sprintf(buffer + strlen(buffer), "%s", tmp.c_str());
    //send message to server
    send(socket_fd,&buffer,sizeof(buffer), 0);
    MSG statusmsg;
    //receive from server
    msgrcv(msgqid, &statusmsg, MAXBUFFER,(long)SEND, 0);

    cout<<statusmsg.mtext<<endl;
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
        // deal with different instructions
        switch (instruction[0]){
            case 'c':
                cli.client_connect(instruction);//connect to server
                break;
            case 'q':
                cli.quit();//quit from client
                break;
            case 't':
                cli.get_time();//get time from server
                break;
            case 'n':
                cli.get_name();//get name from server
                break;
            case 'l':
                cli.get_cli();//get connected list from server
                break;
            case 's':
                cli.send_message(instruction);//send message to server
                break;
            case 'e'://exit client
                cli.exitcli();
                break;
            case 'h'://print client help context
                cli.client_help();
            case '\n':
                continue;//do nothing
            default:
                cout<<"[error]please input correct instruction, use [h] to see the option list\n";
        }
    }
}
