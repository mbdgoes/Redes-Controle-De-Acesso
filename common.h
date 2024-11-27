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

//================ CONSTANTES ====================
#define BUFSIZE 500

#define EXIT -1
#define REQ_USRADD 33

//Estrutura da mensagem
typedef struct Message {
    int type;          
    size_t size;
    char payload[BUFSIZE]; // Payload fixo (máximo de 500 bytes)
} Message;

//============ FUNCOES DE MENSAGEM =====================
void freeMessage(Message *msg);

//============ FUNCOES DE REDE =====================
void DieWithUserMessage(const char *msg, const char *detail);
int initServerSockaddr(const char *proto, const char *portstr, struct sockaddr_storage *storage);
int addrParse(const char *addrstr, const char *portstr, struct sockaddr_storage *storage);

//============= FUNCOES DE DADOS ====================
void computeInput(Message *sentMessage, char command[BUFSIZE], int* error);
void computeCommand(Message *action, Message *receivedData);
void handleReceivedData(struct Message* receivedData, int sock);

#endif