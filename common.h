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
#define BUFSIZE              500
#define MAX_USERS            30
#define MAX_CLIENTS          10
#define USER_SERVER_PORT     50000
#define LOCATION_SERVER_PORT 60000

#define REQ_CONNPEER  17
#define RES_CONNPEER  18
#define REQ_DISCPEER  19
#define REQ_CONN      20
#define RES_CONN      21
#define REQ_DISC      22

#define REQ_USRADD    33
#define REQ_USRACCESS 34
#define RES_USRACCESS 35
#define REQ_LOCREG    36
#define RES_LOCREG    37
#define REQ_USRLOC    38
#define RES_USRLOC    39
#define REQ_LOCLIST   40
#define RES_LOCLIST   41
#define REQ_USRAUTH   42
#define RES_USRAUTH   43

#define ERROR         255
#define OK            0

//============ STRUCTS =============================

// Estrutura basica de mensagem
typedef struct Message {
    int type;          
    size_t size;
    char payload[BUFSIZE]; // Payload fixo (máximo de 500 bytes)
} Message;

// Mantem o estado do server de Usuario
typedef struct UserServer {
    char userDatabase[100][11];   // Database de Ids dos usuarios
    int specialPermissions[100];  // IS_SPECIAL de cada user
    int userCount;
    int clientCount;
    int clientIds[MAX_CLIENTS];
} UserServer;

// Mantem o estado do server de Localizacao
typedef struct LocationServer {
    char locationUserDatabase[100][11]; // Database de Ids dos usuarios
    int lastLocationSeen[100];          // Ultima localizacao de cada user
    int userCount;
    int clientCount;
    int clientIds[MAX_CLIENTS];
} LocationServer;

// Parametros do estado da conexao dos peers
typedef struct PeerConnection{
    int peerId;
    int socket;
    int isConnected;
    int port;
    int isInitiator;                // Flag para indicar server que inicia conexao
    int hasExchangedIds;            // Flag que indica se os servers ja trocaram os Ids
    int myId;
    int theirId;
    int otherPeerConnected;
    UserServer *userServer;         // Preenchido se o server é o server de usuários
    LocationServer *locationServer; // Preenchido se o server é o server de localizacao
} PeerConnection;

// Armazena as informacoes dos clientes para a thread de clientes 
typedef struct ClientThreadParams{
    int client_sock;
    PeerConnection *peerConn;
    UserServer *userServer;
    LocationServer *locationServer;
    int locationId;
} ClientThreadParams;

// Estado atual do cliente
typedef struct ClientState{
    int clientIds[2];
    int locationId;
    int isInitialized;
    int hasUserServerId;
    int hasLocServerId;
} ClientState;


//============ FUNCOES DE REDE =======================
int initServerSockaddr(const char *proto, const char *portstr, struct sockaddr_storage *storage);
int addrParse(const char *addrstr, const char *portstr, struct sockaddr_storage *storage);
void setMessage(Message *message, int type, char* payload);
char* returnErrorMessage(Message *message);
char* returnOkMessage(Message *message);

//============= FUNCOES DE CLIENTE ====================
void initializeClient(ClientState *state, int locationId);
void parseUserCommand(Message *sentMessage, char command[BUFSIZE], int* error, int* clientIds);
void handleConnectionResponse(Message *message, ClientState *state, int serverType);
void handleReceivedData(struct Message* receivedData, int sock, int serverType);
int validateLocationId(int locationId);

//============= FUNCOES DE SERVIDOR  ==================
void findUser(LocationServer *locationServer, Message *message, char* userId);
int establishPeerConnection(const char* serverAddress, int port, PeerConnection *peerConn);
int generateUniqueClientId(int* clientIds, int clientCount);
void *handlePeerConnection(void *arg);
void *handleServerStdin(void *arg);
void *handleClientMessages(void *arg);
void processServerMessage(UserServer *userServer, LocationServer *LocationServer, Message *action, Message *receivedData);

#endif