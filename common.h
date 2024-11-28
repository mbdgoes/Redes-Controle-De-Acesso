#ifndef COMMON_H
#define COMMON_H

#include <arpa/inet.h>
#include <inttypes.h>
#include <netinet/in.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/select.h>

//================ CONSTANTES ====================
#define BUFSIZE 500

#define EXIT -1
#define REQ_CONNPEER 17
#define RES_CONNPEER 18
#define REQ_DISCPEER 19
#define REQ_CONN 20
#define RES_CONN 21
#define REQ_DISC 22
#define REQ_USRADD 33
#define REQ_USRACCESS 34
#define RES_USRACCESS 35
#define REQ_LOCREG 36
#define RES_LOCREG 37
#define REQ_USRLOC 38
#define RES_USRLOC 39
#define REQ_LOCLIST 40
#define RES_LOCLIST 41
#define REQ_USRAUTH 42
#define RES_USRAUTH 43
#define ERROR 255
#define OK 0
#define EXIT -1
#define REQ_LOCATION 44

//============ STRUCTS =============================
typedef struct Message {
    int type;          
    size_t size;
    char payload[BUFSIZE]; // Payload fixo (máximo de 500 bytes)
} Message;

typedef struct {
    char userDatabase[100][11];
    int specialPermissions[100];
    int userCount;
} UserServer;

typedef struct {
    char locationDatabase[100][11];
    int lastLocationSeen[100];
    int locationDataCount;
} LocationServer;

//============ FUNCOES DE REDE =====================
void DieWithUserMessage(const char *msg, const char *detail);
int initServerSockaddr(const char *proto, const char *portstr, struct sockaddr_storage *storage);
int addrParse(const char *addrstr, const char *portstr, struct sockaddr_storage *storage);

//============= FUNCOES DE DADOS ====================
void setMessage(Message *message, int type, char payload[BUFSIZE]);
void computeInput(Message *sentMessage, char command[BUFSIZE], int* error);
void computeCommand(Message *action, Message *receivedData);
void handleReceivedData(struct Message* receivedData, int sock);

#endif