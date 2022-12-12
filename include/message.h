//
// Created by User on 2022/9/19.
//

#ifndef SOCKET_MESSAGE_H
#define SOCKET_MESSAGE_H
#include <cstdio>
#include <cstdlib>

#define MAXSIZE 0x100
#define TIME 'A'
#define NAME 'B'
#define CLILIST 'C'
#define SEND 'D'
#define CLOSE 'E'

typedef struct _msg
{
    long mtype; // type of the message
    char mtext[MAXSIZE]; // text of the message
}MSG;

#endif //SOCKET_MESSAGE_H
